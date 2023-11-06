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

void renderPointCloud(DrawingWindow &window, const std::string& filename, float focalLength);
CanvasPoint getCanvasIntersectionPoint(glm::vec3 cameraPosition, glm::vec3 vertexPosition, float focalLength);
std::vector<std::vector<float>> initialiseDepthBuffer(int width, int height);

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

//this is the function to rotate the camera around the y axis， this is the movement of the camera, only change x and z
glm::vec3 orbitCameraAroundY(glm::vec3 cameraPos, float angle, glm::vec3 ModelCenter) {
    glm::vec3 translatedPosition = cameraPos - ModelCenter; // Translate to origin

    // Apply rotation
    float newX = translatedPosition.x * cos(angle) - translatedPosition.z * sin(angle);
    float newZ = translatedPosition.x * sin(angle) + translatedPosition.z * cos(angle);

    // Translate back to the original position
    return glm::vec3(newX, cameraPos.y, newZ) ;
}

//middle in Axis-Aligned Bounding Box (AABB)
glm::vec3 calculateModelCenter(const std::vector<ModelTriangle>& triangles) {
    glm::vec3 minCoords = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 maxCoords = glm::vec3(std::numeric_limits<float>::lowest());

    for (const auto& triangle : triangles) {
        for (const auto& vertex : triangle.vertices) {
            minCoords.x = std::min(minCoords.x, vertex.x);
            minCoords.y = std::min(minCoords.y, vertex.y);
            minCoords.z = std::min(minCoords.z, vertex.z);

            maxCoords.x = std::max(maxCoords.x, vertex.x);
            maxCoords.y = std::max(maxCoords.y, vertex.y);
            maxCoords.z = std::max(maxCoords.z, vertex.z);
        }
    }

    return (minCoords + maxCoords) / 2.0f;
}

glm::mat3 lookAt(glm::vec3 target) {
    glm::vec3 forward = glm::normalize(cameraPosition - target);
    glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), forward));
    glm::vec3 up = glm::cross(-forward, right);

    return glm::mat3(right, up, -forward);
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
                float CurrentPointDepth = 1.0f/XlineDepth[x - x_start];
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
            float CurrentPointDepth = 1.0f/XlineDepth[x - x_start];
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
            cameraOrientation = rotateX(cameraRotationSpeed) * cameraOrientation;
        } else if (event.key.keysym.sym == SDLK_k) { // Pitch down
            cameraOrientation = rotateX(-cameraRotationSpeed) * cameraOrientation;
        } else if (event.key.keysym.sym == SDLK_j) { // Yaw left
            cameraOrientation = rotateY(cameraRotationSpeed) * cameraOrientation;
        } else if (event.key.keysym.sym == SDLK_l) { // Yaw right
            cameraOrientation = rotateY(-cameraRotationSpeed) * cameraOrientation;
        }else if (event.key.keysym.sym == SDLK_1) {
//            window.clearPixels();  // Clear the window
//            zBuffer = initialiseDepthBuffer(window.width, window.height);
            std::cout << "1 is pressed, I dont know how to set this to u" << std::endl;
            CanvasPoint p1(rand() % (window.width-1), rand() % (window.height-1));
            CanvasPoint p2(rand() % (window.width-1), rand() % (window.height-1));
            CanvasPoint p3(rand() % (window.width-1), rand() % (window.height-1));
            CanvasTriangle randomTriangle(p1, p2, p3);
            drawTriangle(window, randomTriangle, Colour(rand()%255, rand()%255, rand()%255));
        } else if (event.key.keysym.sym == SDLK_2){
//            window.clearPixels();  // Clear the window
            zBuffer = initialiseDepthBuffer(window.width, window.height);
            std::cout << "2 is pressed, I dont know how to set this to f" << std::endl;
            //blue triangle
            CanvasPoint p1(161.719, 105.046, 3.94212);
            CanvasPoint p2(176.539, 102.588, 3.38689);
            CanvasPoint p3(176.5, 204.901, 3.38658);
//            CanvasPoint p3(rand() % (window.width-1), rand() % (window.height-1), rand() % 100);
//            CanvasPoint p3(rand() % (window.width-1), rand() % (window.height-1), rand() % 100);
//            CanvasPoint p3(rand() % (window.width-1), rand() % (window.height-1), rand() % 100);
            CanvasTriangle randomTriangle(p1, p2, p3);
            drawFilledTriangle(window, randomTriangle, Colour(0, 0, 255));

            CanvasPoint q1(126.676, 233.644, 2.52898);
            CanvasPoint q2(271.02, 199.378, 3.62419);
            CanvasPoint q3(272.676, 39.9797, 3.60606);
            CanvasTriangle randomTriangle2(q1, q2, q3);
            drawFilledTriangle(window, randomTriangle2, Colour(128, 0, 128));
        } else if (event.key.keysym.sym == SDLK_3) {
            window.clearPixels();  // Clear the window
            zBuffer = initialiseDepthBuffer(window.width, window.height);
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
            window.clearPixels();  // Clear the window
            zBuffer = initialiseDepthBuffer(window.width, window.height);
            std::cout << "4 is pressed" << std::endl;
            renderPointCloud(window, "../cornell-box.obj", 2);
        }
	} else if (event.type == SDL_MOUSEBUTTONDOWN) {
		window.savePPM("output.ppm");
		window.saveBMP("output.bmp");
	}
}


void renderPointCloud(DrawingWindow &window, const std::string& filename, float focalLength) {
    // Load the triangles from the OBJ file.
    std::vector<ModelTriangle> triangles = loadOBJ(filename, 0.35);

    glm::vec3 ModelCenter = calculateModelCenter(triangles);
    float degree = 1.0f;
    float orbitRotationSpeed = degree * (M_PI / 180.0f);
    //translate, this is just move the camera
    cameraPosition = orbitCameraAroundY(cameraPosition, orbitRotationSpeed, ModelCenter);
    //rotate, this will rotate the camera and let it look at the center of the model
    cameraOrientation = lookAt(ModelCenter);

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

    CanvasPoint canvasPoint;
    glm::vec3 relativePosition = vertexPosition - cameraPosition;

    glm::vec3 vertexPositionNew = relativePosition*cameraOrientation;

    // Compute the projection using the formulas
    float canvasX = focalLength * (vertexPositionNew[0] / vertexPositionNew[2]);
    canvasX *= 150;
    // move the origin to the center of the screen
    canvasX = canvasX + WIDTH / 2.0f;
    float canvasY = focalLength * (vertexPositionNew[1] / vertexPositionNew[2]);
    canvasY *= 150;
    canvasY = canvasY + HEIGHT / 2.0f;

    canvasPoint.x = canvasX;
    canvasPoint.y = canvasY;
    canvasPoint.depth = vertexPositionNew[2];
    return canvasPoint;
}

std::vector<std::vector<float>> initialiseDepthBuffer(int width, int height) {
    std::vector<std::vector<float>> depthBuffer;
    for (int y = 0; y < height; y++) {
        std::vector<float> row;
        for (int x = 0; x < width; x++) {
            row.push_back(0);
        }
        depthBuffer.push_back(row);
    }
    return depthBuffer;
}

int main(int argc, char *argv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;

//    zBuffer = initialiseDepthBuffer(window.width, window.height);

	while (true) {

		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window);
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
	return 0;
}
