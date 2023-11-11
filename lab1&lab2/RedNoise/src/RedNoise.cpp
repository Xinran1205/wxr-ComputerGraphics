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
#include "RayTriangleIntersection.h"
#include "Rasterising.h"
#include "Globals.h"

#define WIDTH 320
#define HEIGHT 240


RayTriangleIntersection getClosestIntersection(const glm::vec3 &cameraPosition, const glm::vec3 &rayDirection, const std::vector<ModelTriangle> &triangles);
glm::vec3 computeRayDirection(int screenWidth, int screenHeight, int x, int y, float focalLength);
void renderRayTracedScene(DrawingWindow &window, const std::string& filename, float focalLength);

RayTriangleIntersection getClosestIntersection(const glm::vec3 &cameraPosition, const glm::vec3 &rayDirection, const std::vector<ModelTriangle> &triangles) {
    RayTriangleIntersection closestIntersection;
    closestIntersection.distanceFromCamera = std::numeric_limits<float>::infinity(); // 初始化为最大值
    float closestDistance = std::numeric_limits<float>::infinity(); // 初始化为最大值

    // go through all the triangles and find the closest intersection
    for (size_t i = 0; i < triangles.size(); i++) {
        const ModelTriangle &triangle = triangles[i];

        glm::vec3 e0 = triangle.vertices[1] - triangle.vertices[0];
        glm::vec3 e1 = triangle.vertices[2] - triangle.vertices[0];
        glm::vec3 SPVector = cameraPosition - triangle.vertices[0];
        glm::mat3 DEMatrix(-rayDirection, e0, e1);
        glm::vec3 possibleSolution = glm::inverse(DEMatrix) * SPVector;

        float t = possibleSolution.x, u = possibleSolution.y, v = possibleSolution.z;

        // check if the intersection is in front of the camera, and if it is the closest intersection so far
        if (t > 0 && u >= 0 && u <= 1 && v >= 0 && v <= 1 && u + v <= 1) {
            if (t < closestDistance) {
                closestDistance = t;
                glm::vec3 intersectionPoint = cameraPosition + t * rayDirection;
                closestIntersection = RayTriangleIntersection(intersectionPoint, t, triangle, i);
            }
        }
    }

    // if no intersection was found, closestIntersection.distanceFromCamera will remain infinity
    return closestIntersection;
}

glm::vec3 computeRayDirection(int screenWidth, int screenHeight, int x, int y, float focalLength) {
    // the camera is at (0, 0, 4), and the image plane is at z = 2

    glm::vec3 imagePlanePoint(x - screenWidth / 2,screenHeight / 2 - y,-focalLength);
//    glm::vec3 imagePlanePoint(x - screenWidth / 2,screenHeight / 2 - y,focalLength);

    // calculate the ray direction
    glm::vec3 rayDirection = imagePlanePoint - cameraPosition;

    // normalize the ray direction
    rayDirection = glm::normalize(rayDirection);

    return rayDirection;
}

void renderRayTracedScene(DrawingWindow &window, const std::string& filename, float focalLength) {
    // Load the triangles from the OBJ file.
    std::vector<ModelTriangle> triangles = loadOBJ(filename, 0.35);

//    glm::vec3 ModelCenter = calculateModelCenter(triangles);
//    float degree = 1.0f;
//    float orbitRotationSpeed = degree * (M_PI / 180.0f);
//    // Translate the camera
//    cameraPosition = orbitCameraAroundY(cameraPosition, orbitRotationSpeed, ModelCenter);
//    // Rotate the camera to look at the model center
//    cameraOrientation = lookAt(ModelCenter);
//
    std::cout << "Loaded " << triangles.size() << " triangles for ray tracing" << std::endl;

    // Loop over each pixel on the image plane
    for (int y = 0; y < int(window.height); y++) {
        for (int x = 0; x < int(window.width); x++) {
            // Compute the ray direction for this pixel
            glm::vec3 rayDirection = computeRayDirection(window.width, window.height, x, y, focalLength);

            // Find the closest intersection of this ray with the scene
            RayTriangleIntersection intersection = getClosestIntersection(cameraPosition, rayDirection, triangles);

            // If an intersection was found, color the pixel accordingly
            if (intersection.distanceFromCamera != std::numeric_limits<float>::infinity()) {
                uint32_t colour = (255 << 24) | (intersection.intersectedTriangle.colour.red << 16) |
                                  (intersection.intersectedTriangle.colour.green << 8) | intersection.intersectedTriangle.colour.blue;
                window.setPixelColour(x, y, colour);
            } else {
                // No intersection found, set the pixel to the background color,
                window.setPixelColour(x, y, 0);
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
//            CanvasPoint p1(101.115, 133.422, 5.04429);
//            CanvasPoint p2(253.54, 98.6787, 3.17545);
//            CanvasPoint p3(68.762, 97.6856, 3.14208);


            CanvasPoint p1(rand() % (window.width-1), rand() % (window.height-1), rand() % 100);
            CanvasPoint p2(rand() % (window.width-1), rand() % (window.height-1), rand() % 100);
            CanvasPoint p3(rand() % (window.width-1), rand() % (window.height-1), rand() % 100);
            CanvasTriangle randomTriangle(p1, p2, p3);
            drawFilledTriangle(window, randomTriangle, Colour(rand()%255, rand()%255, rand()%255));
//
//            CanvasPoint q1(101.115, 133.422, 5.04429);
//            CanvasPoint q2(216.459, 133.808, 5.07766);
//            CanvasPoint q3(253.54, 98.6787, 3.17545);
//            CanvasTriangle randomTriangle2(q1, q2, q3);
//            drawFilledTriangle(window, randomTriangle2, Colour(128, 0, 128));
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
        } else if(event.key.keysym.sym == SDLK_5){
            renderRayTracedScene(window, "../cornell-box.obj", 200);
        }
    } else if (event.type == SDL_MOUSEBUTTONDOWN) {
        window.savePPM("output.ppm");
        window.saveBMP("output.bmp");
    }
}

int main(int argc, char *argv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;
	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window);
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
	return 0;
}

//glm::vec3 computeRayDirection(int screenWidth, int screenHeight, int x, int y, float focalLength) {
//    // Convert screen pixel to normalized device coordinates (-1 to 1)
//    float ndcX = (2.0f * x) / screenWidth - 1.0f;
//    float ndcY = 1.0f - (2.0f * y) / screenHeight; // Invert Y to match image coordinates
//
//    // Convert NDC to world coordinates (Assuming the camera is looking towards -Z in world space)
//    glm::vec3 rayWorld(ndcX * screenWidth / screenHeight, ndcY, -focalLength);
//
//    // Rotate the ray direction according to the camera orientation
//    glm::vec3 rayDirection = rayWorld;
//
//    // Normalize the ray direction
//    rayDirection = glm::normalize(rayDirection);
//
//    return rayDirection;
//}