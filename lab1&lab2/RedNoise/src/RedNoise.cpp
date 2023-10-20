#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include "glm/glm.hpp"
#include "CanvasPoint.h"
#include <Colour.h>
#include <TextureMap.h>
#include <ModelTriangle.h>
#include "DrawTextureTriangle.h"
#include "Interpolate.h"
#include "LoadFile.h"


#define WIDTH 320
#define HEIGHT 240
std::vector<std::vector<float>> zBuffer;
glm::vec3 cameraPosition = glm::vec3(0, 0, 4);
glm::mat3 cameraOrientation = glm::mat3(1.0f);
float cameraSpeed = 5.0f;
float cameraRotationSpeed = 0.05f;

// Function to generate rotation matrix about the X axis
glm::mat3 rotateX(float angle) {
    glm::mat3 rotationMatrix = glm::mat3(
            1, 0, 0,
            0, cos(angle), -sin(angle),
            0, sin(angle), cos(angle)
    );
    return rotationMatrix;
}

// Function to generate rotation matrix about the Y axis
glm::mat3 rotateY(float angle) {
    glm::mat3 rotationMatrix = glm::mat3(
            cos(angle), 0, sin(angle),
            0, 1, 0,
            -sin(angle), 0, cos(angle)
    );
    return rotationMatrix;
}


void renderPointCloud(DrawingWindow &window, const std::string& filename, glm::vec3 cameraPosition, float focalLength);
CanvasPoint getCanvasIntersectionPoint(glm::vec3 cameraPosition, glm::vec3 vertexPosition, float focalLength);

// drawLine by using interpolation
void drawLineInterpolation(DrawingWindow &window, CanvasPoint from, CanvasPoint to, Colour colour){

    int dx = to.x - from.x;
    int dy = to.y - from.y;

    uint32_t packedColour = (255 << 24) | (colour.red << 16) | (colour.green << 8) | colour.blue;

    // Decide if we should step in x direction or y direction
    if (std::abs(dx) > std::abs(dy)) {
        float y = from.y;
        float slope = (float)dy / (float)dx;
        // this is very important to check the quadrant of the line
        if (dx<0){
            slope = -slope;
        }
        int stepX = (dx > 0) ? 1 : -1;
        for (int x = from.x; (stepX == 1) ? (x <= to.x) : (x >= to.x); x += stepX) {
            // check the x and y value is in the window
            if ((size_t)x >= 0 && (size_t)x < window.width &&
                round(y) >= 0 && round(y) < window.height) {
                window.setPixelColour(x, round(y), packedColour);
            }
            y += slope;
        }
    } else {
        float x = from.x;
        float slope = (float)dx / (float)dy;
        if (dy<0){
            slope = -slope;
        }
        int stepY = (dy > 0) ? 1 : -1;
        for (int y = from.y; (stepY == 1) ? (y <= to.y) : (y >= to.y) ; y += stepY) {
            if (round(x) >= 0 && round(x) < window.width &&
                    (size_t)y >= 0 && (size_t)y < window.height) {
                window.setPixelColour(round(x), y, packedColour);
            }
            x += slope;
        }
    }
}

void drawTriangle(DrawingWindow &window, CanvasTriangle triangle, Colour colour) {
    drawLineInterpolation(window, triangle[0], triangle[1], colour);
    drawLineInterpolation(window, triangle[1], triangle[2], colour);
    drawLineInterpolation(window, triangle[2], triangle[0], colour);
}


// using interpolation to draw filled triangle
// BUG should be fixed(segmentation fault), caused by triangle shape and float int conversion
// Very huge bug 15/10/2023, fixed for integer 16/10/2023,but I am not sure is that finished for float
// Zbuffer is not working
void drawFilledTriangle (DrawingWindow &window, CanvasTriangle triangle, Colour colour) {
    std :: cout << "drawFilledTriangle is called" << std::endl;
    // print out the triangle
    std :: cout << triangle << std::endl;

    CanvasPoint bottom = triangle[0];
    CanvasPoint middle = triangle[1];
    CanvasPoint top = triangle[2];

    uint32_t packedColour = (255 << 24) | (colour.red << 16) | (colour.green << 8) | colour.blue;

    // sort the points by y value
    if (bottom.y > middle.y) {
        std::swap(bottom, middle);
    }
    if (bottom.y > top.y) {
        std::swap(bottom, top);
    }
    if (middle.y > top.y) {
        std::swap(middle, top);
    }
    // now bottom.y <= middle.y <= top.y

    // interpolate the depth values
    std::vector<float> depthValuesBottomToTop = interpolateSingleFloats(bottom.depth, top.depth, int(top.y) - int(bottom.y) + 1);
    std::vector<float> depthValuesBottomToMiddle = interpolateSingleFloats(bottom.depth, middle.depth, int(middle.y) - int(bottom.y) + 1);
    std::vector<float> depthValuesMiddleToTop = interpolateSingleFloats(middle.depth, top.depth, int(top.y) - int(middle.y) + 1);

    // Calculate lines
    // This value is the list of the x value of the start/end line between bottom and top
    std::vector<float> xValuesBottomToTop = interpolateSingleFloats(bottom.x, top.x, int(top.y) - int(bottom.y) + 1);
    std::vector<float> xValuesBottomToMiddle = interpolateSingleFloats(bottom.x, middle.x, int(middle.y) - int(bottom.y) + 1);
    std::vector<float> xValuesMiddleToTop = interpolateSingleFloats(middle.x, top.x, int(top.y) - int(middle.y) + 1);

    if (int(middle.y) != int(top.y)) {
        // Rasterize top half triangle (from middle to top)
        for (int i = 0; i < int(top.y - middle.y+1); i++) {
            int y = middle.y + i;
            int x_start = std::round(xValuesMiddleToTop[i]);
            int x_end = std::round(xValuesBottomToTop[middle.y - bottom.y + i]);
            if (x_start > x_end) std::swap(x_start, x_end); // ensure x_start <= x_end

            float depthStart = depthValuesMiddleToTop[i];
            float depthEnd = depthValuesBottomToTop[middle.y - bottom.y + i];
            std::vector<float> XlineDepth = interpolateSingleFloats(depthStart, depthEnd, x_end - x_start + 1);

            // Draw horizontal line from x_start to x_end
            for (int x = x_start; x <= x_end; x++) {
                float CurrentPointDepth = XlineDepth[x - x_start];
                // Z buffer is closer to us if the value is smaller
                if (x >= 0 && (size_t)x < window.width &&
                    (size_t)y >= 0 && (size_t)y < window.height && CurrentPointDepth > zBuffer[y][x]) {
                    window.setPixelColour(x, y, packedColour);
                    zBuffer[y][x] = CurrentPointDepth;
                }
            }
        }
    }

    if (int(middle.y) == int(bottom.y)) {
        return;
    }
    // Rasterize bottom half triangle (from bottom to middle)
    for (int i = 0; i < int(middle.y - bottom.y + 1); i++) {
        int y = bottom.y + i;
        int x_start = std::round(xValuesBottomToMiddle[i]);
        int x_end = std::round(xValuesBottomToTop[i]);
        if (x_start > x_end) std::swap(x_start, x_end); // ensure x_start <= x_end

        float depthStart = depthValuesBottomToMiddle[i];
        float depthEnd = depthValuesBottomToTop[i];
        std::vector<float> XlineDepth = interpolateSingleFloats(depthStart, depthEnd, x_end - x_start + 1);

        // Draw horizontal line from x_start to x_end
        for (int x = x_start; x <= x_end; x++) {
            float CurrentPointDepth = XlineDepth[x - x_start];
            if (x >= 0 && (size_t)x < window.width &&
                (size_t)y >= 0 && (size_t)y < window.height && CurrentPointDepth > zBuffer[y][x]) {
                window.setPixelColour(x, y, packedColour);
                zBuffer[y][x] = CurrentPointDepth;
            }
        }
    }
}

void handleEvent(SDL_Event event, DrawingWindow &window) {
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_LEFT) {
            std::cout << "LEFT" << std::endl;
        }else if (event.key.keysym.sym == SDLK_RIGHT) {
            std::cout << "RIGHT" << std::endl;
        }else if (event.key.keysym.sym == SDLK_UP) {
            std::cout << "UP" << std::endl;
        }else if(event.key.keysym.sym == SDLK_DOWN) {
            std::cout << "DOWN" << std::endl;
        } else if (event.key.keysym.sym == SDLK_w) {
            cameraPosition.z -= cameraSpeed;
        } else if (event.key.keysym.sym == SDLK_s) {
            cameraPosition.z += cameraSpeed;
        } else if (event.key.keysym.sym == SDLK_a) {
            cameraPosition.x -= cameraSpeed;
        } else if (event.key.keysym.sym == SDLK_d) {
            cameraPosition.x += cameraSpeed;
        } else if (event.key.keysym.sym == SDLK_q) {
            cameraPosition.y -= cameraSpeed;
        } else if (event.key.keysym.sym == SDLK_e) {
            cameraPosition.y += cameraSpeed;
        }else if (event.key.keysym.sym == SDLK_i) { // Pitch up
            //当角度为正时，这个旋转矩阵会使点围绕X轴进行逆时针旋转。这意味着如果您直接将这个旋转应用于场景中的一个物体，那么这个物体会像向下倾斜30度一样旋转
            //因为这个应用在模型，模型向下等于抬头看
            cameraOrientation = rotateX(cameraRotationSpeed) * cameraOrientation;
        } else if (event.key.keysym.sym == SDLK_k) { // Pitch down
            cameraOrientation = rotateX(-cameraRotationSpeed) * cameraOrientation;
        } else if (event.key.keysym.sym == SDLK_j) { // Yaw left
            cameraOrientation = rotateY(cameraRotationSpeed) * cameraOrientation;
        } else if (event.key.keysym.sym == SDLK_l) { // Yaw right
            cameraOrientation = rotateY(-cameraRotationSpeed) * cameraOrientation;
        }else if (event.key.keysym.sym == SDLK_1) {
            std::cout << "1 is pressed, I dont know how to set this to u" << std::endl;
            CanvasPoint p1(rand() % (window.width-1), rand() % (window.height-1));
            CanvasPoint p2(rand() % (window.width-1), rand() % (window.height-1));
            CanvasPoint p3(rand() % (window.width-1), rand() % (window.height-1));
            CanvasTriangle randomTriangle(p1, p2, p3);
            drawTriangle(window, randomTriangle, Colour(rand()%255, rand()%255, rand()%255));
        } else if (event.key.keysym.sym == SDLK_2){
            std::cout << "2 is pressed, I dont know how to set this to f" << std::endl;
            CanvasPoint p1(rand() % (window.width-1), rand() % (window.height-1));
            CanvasPoint p2(rand() % (window.width-1), rand() % (window.height-1));
            CanvasPoint p3(rand() % (window.width-1), rand() % (window.height-1));
            CanvasTriangle randomTriangle(p1, p2, p3);
            drawFilledTriangle(window, randomTriangle, Colour(rand()%255, rand()%255, rand()%255));
        } else if (event.key.keysym.sym == SDLK_3) {
            //texture map
            std::cout << "3 is pressed, I dont know how to set this to t" << std::endl;
            CanvasPoint p1(160, 10);
            p1.texturePoint = TexturePoint(195, 5);
            CanvasPoint p2(300, 230);
            p2.texturePoint = TexturePoint(395, 380);
            CanvasPoint p3(10, 150);
            p3.texturePoint = TexturePoint(65, 330);

            CanvasTriangle triangle(p1, p2, p3);
            TextureMap textureMap("../texture.ppm");
            drawTextureTriangle(window, triangle, textureMap);
        }else if (event.key.keysym.sym == SDLK_4){
            std::cout << "4 is pressed, I dont know how to set this to a" << std::endl;
            renderPointCloud(window, "../cornell-box.obj", cameraPosition, 2);
        }
	} else if (event.type == SDL_MOUSEBUTTONDOWN) {
		window.savePPM("output.ppm");
		window.saveBMP("output.bmp");
	}
}


void renderPointCloud(DrawingWindow &window, const std::string& filename, glm::vec3 cameraPosition, float focalLength) {
    // Load the triangles from the OBJ file.
    std::vector<ModelTriangle> triangles = loadOBJ(filename, 0.35);

    std::cout << "Loaded " << triangles.size() << " triangles" << std::endl;

    for (const auto& triangle : triangles) {
        CanvasPoint projectedPoints[3];
        for (int i = 0; i < 3; i++) {
            projectedPoints[i] = getCanvasIntersectionPoint(cameraPosition, triangle.vertices[i], focalLength);
//          window.setPixelColour(projectedPoints[i].x, projectedPoints[i].y, (255 << 24) | (255 << 16) | (255 << 8) | 255);
        }
//        drawTriangle(window, CanvasTriangle(projectedPoints[0], projectedPoints[1], projectedPoints[2]),
//                     Colour(triangle.colour.red, triangle.colour.green, triangle.colour.blue));

        drawFilledTriangle(window, CanvasTriangle(projectedPoints[0], projectedPoints[1], projectedPoints[2]),
                     Colour(triangle.colour.red, triangle.colour.green, triangle.colour.blue));
    }
}

CanvasPoint getCanvasIntersectionPoint(glm::vec3 cameraPosition, glm::vec3 vertexPosition, float focalLength) {
    // Rotate the translated vertex
    glm::vec3 vertexPositionNew = cameraOrientation * vertexPosition;

    // Convert from model to camera coordinates
    glm::vec3 relativePosition = vertexPositionNew - cameraPosition;

    // Compute the projection using the formulas
    float canvasX = focalLength * -(relativePosition[0] / relativePosition[2]);
    canvasX *= 150;
    // move the origin to the center of the screen
    canvasX = canvasX + WIDTH / 2.0f;
    float canvasY = focalLength * (relativePosition[1] / relativePosition[2]);
    canvasY *= 150;
    canvasY = canvasY + HEIGHT / 2.0f;

    return {canvasX, canvasY, vertexPosition.z};
}

std::vector<std::vector<float>> initialiseDepthBuffer(int width, int height) {
    std::vector<std::vector<float>> depthBuffer;
    for (int y = 0; y < height; y++) {
        std::vector<float> row;
        for (int x = 0; x < width; x++) {
            row.push_back(std::numeric_limits<float>::lowest());
        }
        depthBuffer.push_back(row);
    }
    return depthBuffer;
}

int main(int argc, char *argv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;

    zBuffer = initialiseDepthBuffer(window.width, window.height);

	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window);
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
	return 0;
}
