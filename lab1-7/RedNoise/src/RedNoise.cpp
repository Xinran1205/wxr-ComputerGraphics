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

int shininess = 500;
RayTriangleIntersection getClosestIntersection(const glm::vec3 &cameraPosition, const glm::vec3 &rayDirection, const std::vector<ModelTriangle> &triangles);
glm::vec3 computeRayDirection(int screenWidth, int screenHeight, int x, int y, float focalLength);
void renderRayTracedScene(DrawingWindow &window, const std::string& filename, float focalLength);

// calculate diffuse lighting
float calculateLighting(const glm::vec3 &point, const glm::vec3 &normal, const glm::vec3 &lightSource) {
    glm::vec3 toLight = lightSource - point;
    float distance = glm::length(toLight);
    float proximityBrightness = 3.0f / (5 * M_PI * distance * distance);


    glm::vec3 lightDirection = glm::normalize(toLight);
    glm::vec3 normalDirection = glm::normalize(normal);
    // cos(theta) = dot product of the normal and the light direction
    float dotProduct = glm::dot(normalDirection, lightDirection);
    float incidenceBrightness = std::max(dotProduct, 0.0f);

    // combine the two brightness factors
    float combinedBrightness = proximityBrightness * incidenceBrightness;
    // clamp the combined brightness between 0 and 1
    return glm::clamp(combinedBrightness, 0.0f, 1.0f);
}

glm::vec3 calculateBarycentricCoordinates(const glm::vec3 &P, const std::array<glm::vec3, 3> &triangleVertices) {
    glm::vec3 A = triangleVertices[0];
    glm::vec3 B = triangleVertices[1];
    glm::vec3 C = triangleVertices[2];

    glm::vec3 AB = B - A;
    glm::vec3 AC = C - A;
    glm::vec3 BC = C - B;
    glm::vec3 AP = P - A;
    glm::vec3 BP = P - B;

    // 计算三角形ABC的面积的两倍（使用叉乘）
    float areaABC = glm::length(glm::cross(AB, AC));
    // 计算P相对于三角形顶点的子三角形的面积的两倍
    float areaPBC = glm::length(glm::cross(BP, BC));
    float areaPCA = glm::length(glm::cross(AP, AC));

    // 重心坐标是子三角形面积除以整个三角形面积
    float u = areaPBC / areaABC;
    float v = areaPCA / areaABC;
    float w = 1.0f - u - v;

    return glm::vec3(u, v, w);
}

float calculateSpecularLighting(const glm::vec3 &point,const glm::vec3 &cameraPosition, const glm::vec3 &lightSource, const glm::vec3 &normal, int shininess) {
    glm::vec3 viewDirection = glm::normalize(cameraPosition - point);
    glm::vec3 lightDirection = glm::normalize(lightSource - point);
    // calculate the reflection direction
    glm::vec3 reflectDir = glm::reflect(-lightDirection, normal);
    // compare the reflection direction with the view direction
    float spec = glm::pow(glm::max(glm::dot(viewDirection, reflectDir), 0.0f), shininess);
    return spec;
}

glm::vec3 calculateLightSourcePosition() {
    glm::vec3 v1 = glm::vec3(-2.779011, 2.749765, 2.802031);
    glm::vec3 v2 = glm::vec3(-2.779011, 2.7453132, -2.7899683);
    glm::vec3 v3 = glm::vec3(2.780989, 2.7453132, -2.7899683);
    glm::vec3 v4 = glm::vec3(2.780989, 2.749765, 2.802031);

    glm::vec3 lightSourcePosition = (v1 + v2 + v3 + v4) / 4.0f;
    lightSourcePosition *= 0.35f;
    return lightSourcePosition;
}

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

glm::vec3 computeRayDirection(int screenWidth, int screenHeight, int x, int y, float focalLength, glm::mat3 cameraOrientation) {
    // the camera is at (0, 0, 4), and the image plane is at z = 2

    float scale = 150.0f; // Adjust this factor to zoom in or out

    float canvasX = (x - screenWidth / 2) / scale;
    float canvasY = -(screenHeight / 2 - y) / scale;

    glm::vec3 imagePlanePoint = glm::vec3(canvasX, canvasY, focalLength);

    // rotate the camera and let it look at the model center
    glm::vec3 rayDirection = cameraOrientation * imagePlanePoint;
    // normalize the ray direction
    rayDirection = glm::normalize(rayDirection);

    return rayDirection;
}

void renderRayTracedScene(DrawingWindow &window, const std::string& filename, float focalLength) {
    // Load the triangles from the OBJ file.
    std::vector<ModelTriangle> triangles = loadOBJ(filename, 0.35);

    glm::vec3 ModelCenter = calculateModelCenter(triangles);
    float degree = 1.0f;
    float orbitRotationSpeed = degree * (M_PI / 180.0f);
    // Translate the camera
    cameraPosition = orbitCameraAroundY(cameraPosition, orbitRotationSpeed, ModelCenter);
    // Rotate the camera to look at the model center
    cameraOrientation = lookAt(ModelCenter);

    std::cout << "Loaded " << triangles.size() << " triangles for ray tracing" << std::endl;

//    glm::vec3 sourceLight = calculateLightSourcePosition();
    glm::vec3 sourceLight = glm::vec3(0, 0.8, 0);
    float ambientLight = 0.3f;  // ambient light intensity

    // Loop over each pixel on the image plane
    for (int y = 0; y < int(window.height); y++) {
        for (int x = 0; x < int(window.width); x++) {
            // Compute the ray direction for this pixel
            glm::vec3 rayDirection = computeRayDirection(window.width, window.height, x, y, focalLength,cameraOrientation);

            // Find the closest intersection of this ray with the scene
            RayTriangleIntersection intersection = getClosestIntersection(cameraPosition, rayDirection, triangles);

            // If an intersection was found, color the pixel accordingly
            if (intersection.distanceFromCamera != std::numeric_limits<float>::infinity()) {
                glm::vec3 shadowRay = glm::normalize(sourceLight - intersection.intersectionPoint);
                RayTriangleIntersection shadowIntersection = getClosestIntersection(intersection.intersectionPoint + shadowRay * 0.002f, shadowRay, triangles);


                // these are the normals of the vertices of the intersected triangle
                glm::vec3 normal0  = vertexToNormals[intersection.intersectedTriangle.vertices[0]].front();
                glm::vec3 normal1  = vertexToNormals[intersection.intersectedTriangle.vertices[1]].front();
                glm::vec3 normal2  = vertexToNormals[intersection.intersectedTriangle.vertices[2]].front();
                // calculateBarycentricCoordinates will return u, v, w
                glm::vec3 barycentricCoords = calculateBarycentricCoordinates(intersection.intersectionPoint, intersection.intersectedTriangle.vertices);

                // interpolate the normal
                glm::vec3 interpolatedNormal =
                        barycentricCoords.x * normal0 +
                        barycentricCoords.y * normal1 +
                        barycentricCoords.z * normal2;

                interpolatedNormal = glm::normalize(interpolatedNormal);



                // calculate the diffuse lighting and specular lighting
//                float brightness = calculateLighting(intersection.intersectionPoint, intersection.intersectedTriangle.normal, sourceLight);
                float brightness = calculateLighting(intersection.intersectionPoint, interpolatedNormal, sourceLight);
//                float specularIntensity = calculateSpecularLighting(intersection.intersectionPoint, cameraPosition, sourceLight, intersection.intersectedTriangle.normal, shininess);
                float specularIntensity = calculateSpecularLighting(intersection.intersectionPoint, cameraPosition, sourceLight, interpolatedNormal, shininess);

                if (shadowIntersection.distanceFromCamera < glm::length(sourceLight - intersection.intersectionPoint)&& shadowIntersection.triangleIndex != intersection.triangleIndex) {
                    brightness = ambientLight;
                } else {
                    // if the intersection is not in shadow, combine the brightness with the ambient light
                    brightness = glm::max(brightness + ambientLight, ambientLight);
                }

                // combine the brightness with the specular intensity
                float combinedBrightness = glm::clamp(brightness + specularIntensity, 0.0f, 1.0f);

                Colour colour = intersection.intersectedTriangle.colour;
                uint32_t rgbColour = (255 << 24) |
                                     (int(combinedBrightness * colour.red) << 16) |
                                     (int(combinedBrightness * colour.green) << 8) |
                                     int(combinedBrightness * colour.blue);
                window.setPixelColour(x, y, rgbColour);
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
            renderRayTracedScene(window, "../cornell-box.obj", 2);
        } else if(event.key.keysym.sym == SDLK_6) {
            renderRayTracedScene(window, "../sphere.obj", 2);
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
