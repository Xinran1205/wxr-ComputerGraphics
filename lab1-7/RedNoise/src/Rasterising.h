//
// Created by dell on 2023/11/6.
//

#ifndef REDNOISE_RASTERISING_H
#define REDNOISE_RASTERISING_H

#endif //REDNOISE_RASTERISING_H

#include "CanvasTriangle.h"
#include "DrawingWindow.h"
#include "ModelTriangle.h"
#include "TextureMap.h"
#include "Utils.h"
#include "LoadFile.h"
#include "Interpolate.h"
#include "Globals.h"
#include "DrawTextureTriangle.h"
#include "RotateCamera.h"


#define WIDTH 320
#define HEIGHT 240

std::vector<std::vector<float>> initialiseDepthBuffer(int width, int height);
glm::vec3 calculateModelCenter(const std::vector<ModelTriangle>& triangles);
glm::mat3 lookAt(glm::vec3 target);
void drawFilledTriangle (DrawingWindow &window, CanvasTriangle triangle, Colour colour);
CanvasPoint getCanvasIntersectionPoint(glm::vec3 cameraPosition, glm::vec3 vertexPosition, float focalLength);
void drawPartTriangle (DrawingWindow &window, CanvasTriangle triangle, Colour colour);
void renderPointCloud(DrawingWindow &window, const std::string& filename, float focalLength, TextureMap &textureMap);


