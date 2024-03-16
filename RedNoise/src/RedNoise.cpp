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
#include "normalMap.h"
#include "SoftShadowRendering.h"
#include <iomanip>
#include <sstream>

#define WIDTH 320
#define HEIGHT 240

int counter = 0;
bool isDefaultMode = true;

void handleEvent(SDL_Event event, DrawingWindow &window) {
    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_1) {
            std::cout << "random triangle" << std::endl;
            CanvasPoint p1(rand() % (window.width - 1), rand() % (window.height - 1));
            CanvasPoint p2(rand() % (window.width - 1), rand() % (window.height - 1));
            CanvasPoint p3(rand() % (window.width - 1), rand() % (window.height - 1));
            CanvasTriangle randomTriangle(p1, p2, p3);
            drawTriangle(window, randomTriangle, Colour(rand() % 255, rand() % 255, rand() % 255));
        } else if (event.key.keysym.sym == SDLK_2) {
            std::cout << "draw filled triangle " << std::endl;
            zBuffer = initialiseDepthBuffer(window.width, window.height);
            CanvasPoint p1(rand() % (window.width - 1), rand() % (window.height - 1), rand() % 100);
            CanvasPoint p2(rand() % (window.width - 1), rand() % (window.height - 1), rand() % 100);
            CanvasPoint p3(rand() % (window.width - 1), rand() % (window.height - 1), rand() % 100);
            // must initialize the texturePoint
            p1.texturePoint = TexturePoint(0, 0);
            p2.texturePoint = TexturePoint(0, 0);
            p3.texturePoint = TexturePoint(0, 0);
            CanvasTriangle randomTriangle(p1, p2, p3);
            TextureMap textureMap("../texture.ppm");
            drawTextureTriangle(window, randomTriangle, Colour(rand() % 255, rand() % 255, rand() % 255), textureMap);
        } else if (event.key.keysym.sym == SDLK_3) {
            std::cout << "draw texture triangle" << std::endl;
            window.clearPixels();
            zBuffer = initialiseDepthBuffer(window.width, window.height);
            CanvasPoint p1(160, 10);
            p1.texturePoint = TexturePoint(195, 5);
            CanvasPoint p2(300, 230);
            p2.texturePoint = TexturePoint(395, 380);
            CanvasPoint p3(10, 150);
            p3.texturePoint = TexturePoint(65, 330);
            CanvasTriangle triangle(p1, p2, p3);
            TextureMap textureMap("../texture.ppm");
            drawTextureTriangle(window, triangle,Colour(255, 255, 255), textureMap);
        } else if(event.key.keysym.sym == SDLK_4){
            std::cout << "Wireframe 3D scene rendering" << std::endl;
            window.clearPixels();  // Clear the window
            DrawWireframe(window, "../cornell-box.obj", 2,"../material/cornell-box.mtl");
        } else if(event.key.keysym.sym == SDLK_5) {
            std::cout << "Rasterising" << std::endl;
            window.clearPixels();  // Clear the window
            zBuffer = initialiseDepthBuffer(window.width, window.height);
            TextureMap textureMap("../texture.ppm");
            renderPointCloud(window, "../textured-cornell-box.obj", 2, textureMap,"../material/cornell-box.mtl");
        } else if (event.key.keysym.sym == SDLK_6) {
            std::cout << "Ray Tracing, only reflection" << std::endl;
            // this code contains reflection and refraction, but we choose not to load refraction material
            window.clearPixels();
            renderRayTracedScene(window, "../cornell-box.obj", 2,"../material/onlyReflection.mtl",1);
        } else if(event.key.keysym.sym == SDLK_7) {
            std::cout << "Ray Tracing, only Refraction" << std::endl;
            // this code contains reflection and refraction, but we choose not to load reflection material
            window.clearPixels();
            renderRayTracedScene(window, "../cornell-box.obj", 2,"../material/onlyRefraction.mtl",1);
        }else if(event.key.keysym.sym == SDLK_8) {
            std::cout << "Ray Tracing, combined reflection and refraction!" << std::endl;
            // test reflection and refraction together!!
            window.clearPixels();
            renderRayTracedScene(window, "../cornell-box.obj", 2,"../material/cornell-box.mtl",1);
        }else if (event.key.keysym.sym == SDLK_9) {
            std::cout << "Ray Tracing, rendering sphere by using flat shading, gouraud shading or phong shading !" << std::endl;
            cameraPosition = glm::vec3(0, 0.9, 1.9);
            window.clearPixels();
            // signalForShading = 1,2,3 represent flat shading, gouraud shading, phong shading
//            renderRayTracedScene(window, "../sphere.obj", 1,"../material/sphere.mtl",1);
//            renderRayTracedScene(window, "../sphere.obj", 1,"../material/sphere.mtl",2);
            renderRayTracedScene(window, "../sphere.obj", 1,"../material/sphere.mtl",3);
        }else if(event.key.keysym.sym == SDLK_z){
            std::cout << "Soft shadow!" << std::endl;
            window.clearPixels();
            renderRayTracedSceneSoftShadow(window, "../cornell-box.obj", 2,
                                           "../material/cornell-box.mtl",1);
        }else if(event.key.keysym.sym == SDLK_x) {
            // environment mapping
            std::cout << "Environment mapping!" << std::endl;
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
            cameraPosition = glm::vec3(0, 0, 0.5);
            // we do not need to set this model to be a mirror
            // because this function assume the model is a mirror
            renderRayTracedSceneForEnv(window, "../envsphere.obj", 0.4,textures,"../material/cornell-box.mtl");
        }else if(event.key.keysym.sym == SDLK_c){
            std::cout << "Normal mapping!" << std::endl;
            window.clearPixels();
            TextureMap textureMap("../NormalMap/tex.ppm");
            renderRayTracedSceneNormal(window, "../NormalMap/NormalMap.obj", 2,
                                       textureMap,"../material/cornell-box.mtl");


            // below this line all key events are for camera control
        }else if (event.key.keysym.sym == SDLK_i) { // Pitch up
            std::cout << "Pitch up" << std::endl;
            float degree = 1.0f;
            float orbitRotationSpeed = degree * (M_PI / 180.0f);
            // Orbit camera around X-axis at a defined speed.
            cameraPosition = orbitCameraAroundX(cameraPosition, orbitRotationSpeed, glm::vec3(0, 0, 0));
            cameraOrientation = lookAt(glm::vec3(0, 0, 0));
        } else if (event.key.keysym.sym == SDLK_k) { // Pitch down
            std::cout << "Pitch down" << std::endl;
            float degree = 1.0f;
            float orbitRotationSpeed = degree * (M_PI / 180.0f);
            // Orbit camera in the reverse direction around X-axis at a defined speed.
            cameraPosition = orbitCameraAroundXInverse(cameraPosition, orbitRotationSpeed, glm::vec3(0, 0, 0));
            cameraOrientation = lookAt(glm::vec3(0, 0, 0));
        } else if (event.key.keysym.sym == SDLK_j) { // Yaw left
            std::cout << "Yaw left" << std::endl;
            float degree = 1.0f;
            float orbitRotationSpeed = degree * (M_PI / 180.0f);
            // Orbit camera around Y-axis at a defined speed.
            cameraPosition = orbitCameraAroundY(cameraPosition, orbitRotationSpeed, glm::vec3(0, 0, 0));
            cameraOrientation = lookAt(glm::vec3(0, 0, 0));
        } else if (event.key.keysym.sym == SDLK_l) { // Yaw right
            std::cout << "Yaw right" << std::endl;
            float degree = 1.0f;
            float orbitRotationSpeed = degree * (M_PI / 180.0f);
            // Orbit camera in the reverse direction around Y-axis at a defined speed.
            cameraPosition = orbitCameraAroundYInverse(cameraPosition, orbitRotationSpeed, glm::vec3(0, 0, 0));
            cameraOrientation = lookAt(glm::vec3(0, 0, 0));
        } else if (event.key.keysym.sym == SDLK_w){
            std::cout << "move camera towards" << std::endl;
            cameraPosition = cameraPosition + glm::vec3(0, 0, -0.1);
            cameraOrientation = lookAt(glm::vec3(0, 0, 0));
        }else if (event.key.keysym.sym == SDLK_s){
            std::cout << "move camera backwards" << std::endl;
            cameraPosition = cameraPosition + glm::vec3(0, 0, 0.1);
            cameraOrientation = lookAt(glm::vec3(0, 0, 0));
        }else if (event.key.keysym.sym == SDLK_a) {
            std::cout << "move camera left" << std::endl;
            cameraPosition = cameraPosition + glm::vec3(-0.1, 0, 0);
            cameraOrientation = lookAt(glm::vec3(0, 0, 0));
        }else if (event.key.keysym.sym == SDLK_d) {
            std::cout << "move camera right" << std::endl;
            cameraPosition = cameraPosition + glm::vec3(0.1, 0, 0);
            cameraOrientation = lookAt(glm::vec3(0, 0, 0));
        }else if (event.key.keysym.sym == SDLK_q) {
            std::cout << "move camera up" << std::endl;
            cameraPosition = cameraPosition + glm::vec3(0, 0.1, 0);
            cameraOrientation = lookAt(glm::vec3(0, 0, 0));
        }else if (event.key.keysym.sym == SDLK_e) {
            std::cout << "move camera down" << std::endl;
            cameraPosition = cameraPosition + glm::vec3(0, -0.1, 0);
            cameraOrientation = lookAt(glm::vec3(0, 0, 0));
        }else if (event.key.keysym.sym == SDLK_g) {
            std::cout << "mouse button down, save image!" << std::endl;

            std::ostringstream filenameStream;
            filenameStream << "../Frames/" << std::setfill('0') << std::setw(5) << counter;
            std::string filename = filenameStream.str();

            window.savePPM(filename + ".ppm");
            window.saveBMP(filename + ".bmp");
            counter++;
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
        // this is default mode, we can change it by pressing key 1-9 and z,x,c to choose other mode
        if (isDefaultMode){
            std::cout << "Ray Tracing, combined reflection and refraction!" << std::endl;
            window.clearPixels();
            renderRayTracedScene(window, "../cornell-box.obj", 2,"../material/cornell-box.mtl",1);
            isDefaultMode = false;
        }
		window.renderFrame();
	}
	return 0;
}
