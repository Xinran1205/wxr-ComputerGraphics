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
#include "RotateCamera.h"
#include "HardShadowRendering.h"
#include "EnvironmentMapping.h"

#define WIDTH 320
#define HEIGHT 240


RayTriangleIntersection getClosestIntersection(const glm::vec3 &cameraPosition, const glm::vec3 &rayDirection, const std::vector<ModelTriangle> &triangles);
glm::vec3 computeRayDirection(int screenWidth, int screenHeight, int x, int y, float focalLength);
void renderRayTracedScene(DrawingWindow &window, const std::string& filename, float focalLength);


glm::vec3 calculateLightSourcePosition() {
    glm::vec3 v1 = glm::vec3(-2.779011, 2.749765, 2.802031);
    glm::vec3 v2 = glm::vec3(-2.779011, 2.7453132, -2.7899683);
    glm::vec3 v3 = glm::vec3(2.780989, 2.7453132, -2.7899683);
    glm::vec3 v4 = glm::vec3(2.780989, 2.749765, 2.802031);

    glm::vec3 lightSourcePosition = (v1 + v2 + v3 + v4) / 4.0f;
    lightSourcePosition *= 0.35f;
    return lightSourcePosition;
}

//对于我们的一个intersection，他与多个点光源有多个shadowIntersection
float FlatShadingSoft(RayTriangleIntersection intersection, const std::vector<RayTriangleIntersection> &shadowIntersections,
                      const std::vector<glm::vec3> &lightPoints, float ambientLight) {

    float totalBrightness = 0.0f;
    //遍历这个intersection与多个点光源的shadowIntersection，并且把所有的brightness加起来
    //最后求平均就是这个intersection的brightness
    for (size_t i = 0; i < lightPoints.size(); i++) {
        // Calculate brightness and specular intensity for each light point
        float brightness = calculateLighting(intersection.intersectionPoint,
                                             intersection.intersectedTriangle.normal, lightPoints[i]);
        float specularIntensity = calculateSpecularLighting(intersection.intersectionPoint, cameraPosition,
                                                            lightPoints[i], intersection.intersectedTriangle.normal, shininess);

        // Determine if the point is in shadow for the current light point
        if (shadowIntersections[i].distanceFromCamera < glm::length(lightPoints[i] - intersection.intersectionPoint) &&
            shadowIntersections[i].triangleIndex != intersection.triangleIndex) {
            // In shadow, only ambient light contributes
            brightness = ambientLight;
        } else {
            // Not in shadow, add ambient light
            brightness = glm::max(brightness + ambientLight, ambientLight);
        }

        // Accumulate total brightness and specular intensity
        totalBrightness += brightness + specularIntensity; // Add specular to each light's contribution
    }

    // Average the brightness from all light sources
    float averageBrightness = glm::clamp(totalBrightness / lightPoints.size(), 0.0f, 1.0f);
    return averageBrightness;
}

float GouraudShadingSoft(RayTriangleIntersection intersection, const std::vector<RayTriangleIntersection> &shadowIntersections,
                         const std::vector<glm::vec3> &lightPoints, float ambientLight){
    for (int i = 0; i < 3; i++) {
        glm::vec3 vertex = intersection.intersectedTriangle.vertices[i];
        // 如果这个顶点已经计算过了，就不用再计算了
        if (vertexBrightnessGlobal.find(vertex) != vertexBrightnessGlobal.end()){
            continue;
        }

        glm::vec3 normal = vertexNormals[vertex];
        // calculate the diffuse lighting and specular lighting
        float totalBrightness = 0.0f;

        // 对每个光源点计算漫反射和镜面反射
        for (size_t j = 0; j < lightPoints.size(); j++) {
            float brightness = calculateLighting(vertex, normal, lightPoints[j]);
            float specularIntensity = calculateSpecularLighting(vertex, cameraPosition, lightPoints[j], normal, shininess);

            // 检查该顶点是否对于每个光源点都在阴影中
            if (shadowIntersections[j].distanceFromCamera < glm::length(lightPoints[j] - vertex) &&
                shadowIntersections[j].triangleIndex != intersection.triangleIndex) {
                brightness = ambientLight;  // 在阴影中，只考虑环境光
            } else {
                brightness += ambientLight;  // 不在阴影中，添加环境光
            }

            totalBrightness += brightness + specularIntensity;
        }

        // 结合平均漫反射和平均镜面反射得到最终顶点亮度
        float averageBrightness = glm::clamp(totalBrightness / lightPoints.size(), 0.0f, 1.0f);

        vertexBrightnessGlobal[vertex] = averageBrightness;
    }
    // 根据重心坐标插值出交点的brigtness
    glm::vec3 barycentricCoords = calculateBarycentricCoordinates(intersection.intersectionPoint,
                                                                  intersection.intersectedTriangle.vertices);

    // interpolate the normal
    float ResultVertexBrightness =
            barycentricCoords.x * vertexBrightnessGlobal[intersection.intersectedTriangle.vertices[0]] +
            barycentricCoords.y * vertexBrightnessGlobal[intersection.intersectedTriangle.vertices[1]] +
            barycentricCoords.z * vertexBrightnessGlobal[intersection.intersectedTriangle.vertices[2]];
    return ResultVertexBrightness;
}

float phongShadingSoft(RayTriangleIntersection intersection, const std::vector<RayTriangleIntersection> &shadowIntersections,
                       const std::vector<glm::vec3> &lightPoints, float ambientLight) {
    glm::vec3 normal0  = vertexNormals[intersection.intersectedTriangle.vertices[0]];
    glm::vec3 normal1  = vertexNormals[intersection.intersectedTriangle.vertices[1]];
    glm::vec3 normal2  = vertexNormals[intersection.intersectedTriangle.vertices[2]];
    // calculateBarycentricCoordinates will return u, v, w
    glm::vec3 barycentricCoords = calculateBarycentricCoordinates(intersection.intersectionPoint, intersection.intersectedTriangle.vertices);

    // interpolate the normal
    glm::vec3 interpolatedNormal =
            barycentricCoords.x * normal0 +
            barycentricCoords.y * normal1 +
            barycentricCoords.z * normal2;

    interpolatedNormal = glm::normalize(interpolatedNormal);

    float totalBrightness = 0.0f;
    for (size_t i = 0; i < lightPoints.size(); i++) {
        float brightness = calculateLighting(intersection.intersectionPoint, interpolatedNormal, lightPoints[i]);
        float specularIntensity = calculateSpecularLighting(intersection.intersectionPoint, cameraPosition, lightPoints[i],
                                                            interpolatedNormal, shininess);

        if (shadowIntersections[i].distanceFromCamera < glm::length(lightPoints[i] - intersection.intersectionPoint) &&
            shadowIntersections[i].triangleIndex != intersection.triangleIndex) {
            brightness = ambientLight; // In shadow, only ambient light contributes
        } else {
            brightness = glm::max(brightness + ambientLight, ambientLight); // Not in shadow, add ambient light
        }

        totalBrightness += brightness + specularIntensity; // Add specular to each light's contribution
    }

    // Average the brightness from all light sources
    float averageBrightness = glm::clamp(totalBrightness / lightPoints.size(), 0.0f, 1.0f);
    return averageBrightness;
}

void renderRayTracedSceneSoftShadow(DrawingWindow &window, const std::string& filename, float focalLength) {
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

    // Define your multi-point light source here
    std::vector<glm::vec3> lightPoints = {{0, 0.7, 0},{0,0.8,0},{0,0.9,0},
                                          {0,0.75,0},{0,0.85,0},{0,0.95,0}};
    float ambientLight = 0.3f;  // ambient light intensity

    // Loop over each pixel on the image plane
    for (int y = 0; y < int(window.height); y++) {
        for (int x = 0; x < int(window.width); x++) {
            // Compute the ray direction for this pixel
            glm::vec3 rayDirection = computeRayDirection(window.width, window.height, x, y, focalLength, cameraOrientation);

            // Find the closest intersection of this ray with the scene
            RayTriangleIntersection intersection = getClosestIntersection(cameraPosition, rayDirection, triangles);

            // If an intersection was found, color the pixel accordingly
            if (intersection.distanceFromCamera != std::numeric_limits<float>::infinity()) {
                // Initialize combined brightness
                std::vector<RayTriangleIntersection> AllshadowIntersection;
                // Iterate over each point light to calculate soft shadows
                for (const auto& lightPoint : lightPoints) {
                    glm::vec3 shadowRay = glm::normalize(lightPoint - intersection.intersectionPoint);
                    RayTriangleIntersection shadowIntersection = getClosestIntersection(intersection.intersectionPoint + shadowRay * 0.002f, shadowRay, triangles);
                    AllshadowIntersection.push_back(shadowIntersection);
                }

                // Calculate average brightness from all light points
//                float combinedBrightness = phongShadingSoft(intersection, AllshadowIntersection, lightPoints, ambientLight);
                float combinedBrightness = FlatShadingSoft(intersection, AllshadowIntersection, lightPoints, ambientLight);
//                float combinedBrightness = GouraudShadingSoft(intersection, AllshadowIntersection, lightPoints, ambientLight);
                Colour colour = intersection.intersectedTriangle.colour;
                uint32_t rgbColour = (255 << 24) |
                                     (int(combinedBrightness * colour.red) << 16) |
                                     (int(combinedBrightness * colour.green) << 8) |
                                     int(combinedBrightness * colour.blue);
                window.setPixelColour(x, y, rgbColour);
            } else {
                // No intersection found, set the pixel to the background color
                window.setPixelColour(x, y, 0);
            }
        }
    }
}

void handleEvent(SDL_Event event, DrawingWindow &window) {
    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_LEFT) {
            std::cout << "LEFT" << std::endl;
        } else if (event.key.keysym.sym == SDLK_RIGHT) {
            std::cout << "RIGHT" << std::endl;
        } else if (event.key.keysym.sym == SDLK_UP) {
            std::cout << "UP" << std::endl;
        } else if (event.key.keysym.sym == SDLK_DOWN) {
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
        } else if (event.key.keysym.sym == SDLK_i) { // Pitch up
            float degree = 1.0f;
            float orbitRotationSpeed = degree * (M_PI / 180.0f);
            //translate, this is just move the camera
            cameraPosition = orbitCameraAroundX(cameraPosition, orbitRotationSpeed, glm::vec3(0, 0, 0));
            cameraOrientation = lookAt(glm::vec3(0, 0, 0));
        } else if (event.key.keysym.sym == SDLK_k) { // Pitch down
            float degree = 1.0f;
            float orbitRotationSpeed = degree * (M_PI / 180.0f);
            //translate, this is just move the camera
            cameraPosition = orbitCameraAroundXInverse(cameraPosition, orbitRotationSpeed, glm::vec3(0, 0, 0));
            cameraOrientation = lookAt(glm::vec3(0, 0, 0));
        } else if (event.key.keysym.sym == SDLK_j) { // Yaw left
            float degree = 1.0f;
            float orbitRotationSpeed = degree * (M_PI / 180.0f);
            //translate, this is just move the camera
            cameraPosition = orbitCameraAroundY(cameraPosition, orbitRotationSpeed, glm::vec3(0, 0, 0));
            cameraOrientation = lookAt(glm::vec3(0, 0, 0));
        } else if (event.key.keysym.sym == SDLK_l) { // Yaw right
            float degree = 1.0f;
            float orbitRotationSpeed = degree * (M_PI / 180.0f);
            //translate, this is just move the camera
            cameraPosition = orbitCameraAroundYInverse(cameraPosition, orbitRotationSpeed, glm::vec3(0, 0, 0));
            cameraOrientation = lookAt(glm::vec3(0, 0, 0));
        } else if (event.key.keysym.sym == SDLK_1) {
//            window.clearPixels();  // Clear the window
//            zBuffer = initialiseDepthBuffer(window.width, window.height);
            std::cout << "1 is pressed, I dont know how to set this to u" << std::endl;
            CanvasPoint p1(rand() % (window.width - 1), rand() % (window.height - 1));
            CanvasPoint p2(rand() % (window.width - 1), rand() % (window.height - 1));
            CanvasPoint p3(rand() % (window.width - 1), rand() % (window.height - 1));
            CanvasTriangle randomTriangle(p1, p2, p3);
            drawTriangle(window, randomTriangle, Colour(rand() % 255, rand() % 255, rand() % 255));
        } else if (event.key.keysym.sym == SDLK_2) {
//            window.clearPixels();  // Clear the window
            zBuffer = initialiseDepthBuffer(window.width, window.height);
            std::cout << "2 is pressed, I dont know how to set this to f" << std::endl;
            CanvasPoint p1(rand() % (window.width - 1), rand() % (window.height - 1), rand() % 100);
            CanvasPoint p2(rand() % (window.width - 1), rand() % (window.height - 1), rand() % 100);
            CanvasPoint p3(rand() % (window.width - 1), rand() % (window.height - 1), rand() % 100);
            p1.texturePoint = TexturePoint(0, 0);
            p2.texturePoint = TexturePoint(0, 0);
            p3.texturePoint = TexturePoint(0, 0);
            CanvasTriangle randomTriangle(p1, p2, p3);
            TextureMap textureMap("../texture.ppm");
            drawTextureTriangle(window, randomTriangle, Colour(rand() % 255, rand() % 255, rand() % 255), textureMap);
//            drawFilledTriangle(window, randomTriangle, Colour(rand() % 255, rand() % 255, rand() % 255));
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
            drawTextureTriangle(window, triangle,Colour(255, 255, 255), textureMap);
        } else if (event.key.keysym.sym == SDLK_4) {
            window.clearPixels();  // Clear the window
            zBuffer = initialiseDepthBuffer(window.width, window.height);
            std::cout << "4 is pressed" << std::endl;
            TextureMap textureMap("../texture.ppm");
            renderPointCloud(window, "../cornell-box.obj", 2, textureMap);
//            renderPointCloud(window, "../cornell-box.obj", 2);
        } else if (event.key.keysym.sym == SDLK_5) {
            window.clearPixels();
            renderRayTracedScene(window, "../cornell-box.obj", 2);
        } else if (event.key.keysym.sym == SDLK_6) {
            //要测试这个，首先光和相机的位置都要调一下！
            window.clearPixels();
            renderRayTracedScene(window, "../sphere.obj", 2);
        } else if(event.key.keysym.sym == SDLK_7){
            window.clearPixels();  // Clear the window
            zBuffer = initialiseDepthBuffer(window.width, window.height);
            std::cout << "7 is pressed" << std::endl;
            TextureMap textureMap("../texture.ppm");
//            cameraOrientation = lookAt(glm::vec3(0, 0, 0));
            renderPointCloud(window, "../textured-cornell-box.obj", 2, textureMap);
        } else if(event.key.keysym.sym == SDLK_8){
            window.clearPixels();
            renderRayTracedSceneSoftShadow(window, "../cornell-box.obj", 2);
        }else if(event.key.keysym.sym == SDLK_9) {
            // environment mapping, 环境映射
            window.clearPixels();
            TextureMap frontTexture("../skybox/front.ppm");
            TextureMap backTexture("../skybox/back.ppm");
            TextureMap leftTexture("../skybox/left.ppm");
            TextureMap rightTexture("../skybox/right.ppm");
            TextureMap topTexture("../skybox/top.ppm");
            TextureMap bottomTexture("../skybox/bottom.ppm");
            std::array<TextureMap, 6> textures = {
                    rightTexture, leftTexture, topTexture, bottomTexture, frontTexture, backTexture
            };
            //这个球体不需要设置成mirror，因为我这个函数默认就模型全部是mirror
            renderRayTracedSceneForEnv(window, "../envsphere.obj", 2,textures);
        }
        else if (event.type == SDL_MOUSEBUTTONDOWN) {
            window.savePPM("output.ppm");
            window.saveBMP("output.bmp");
        }
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
