//
// Created by dell on 2023/11/27.
//

#ifndef REDNOISE_ENVIRONMENTMAPPING_H
#define REDNOISE_ENVIRONMENTMAPPING_H

#include <DrawingWindow.h>
#include <Utils.h>
#include "glm/glm.hpp"
#include "LoadFile.h"
#include "Rasterising.h"
#include "Globals.h"
#include "RayTriangleIntersection.h"
#include "HardShadowRendering.h"

uint32_t sampleColourFromTextureMap(const TextureMap &textureMap, float u, float v);
uint32_t getColourFromEnvironmentMap(const glm::vec3 &reflectionVector, const std::array<TextureMap, 6>& textures);
void renderRayTracedSceneForEnv(DrawingWindow &window, const std::string& filename, float focalLength
                                ,const std::array<TextureMap, 6>& textures,const std::string& materialFilename);


#endif //REDNOISE_ENVIRONMENTMAPPING_H
