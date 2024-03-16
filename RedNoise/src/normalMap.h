//
// Created by dell on 2023/11/27.
//

#ifndef REDNOISE_NORMALMAP_H
#define REDNOISE_NORMALMAP_H

#include <DrawingWindow.h>
#include <Utils.h>
#include "glm/glm.hpp"
#include "LoadFile.h"
#include "Rasterising.h"
#include "Globals.h"
#include "RayTriangleIntersection.h"
#include "HardShadowRendering.h"

float FlatShadingNormal(RayTriangleIntersection intersection, RayTriangleIntersection shadowIntersection,
                        const glm::vec3 &sourceLight, float ambientLight,glm::vec3 normalMap);
void renderRayTracedSceneNormal(DrawingWindow &window, const std::string& filename, float focalLength,
                                TextureMap &textureMap,const std::string& materialFilename);


#endif //REDNOISE_NORMALMAP_H
