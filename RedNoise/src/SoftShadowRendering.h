//
// Created by dell on 2023/11/27.
//

#ifndef REDNOISE_SOFTSHADOWRENDERING_H
#define REDNOISE_SOFTSHADOWRENDERING_H


#include "HardShadowRendering.h"
float FlatShadingSoft(RayTriangleIntersection intersection, const std::vector<RayTriangleIntersection> &shadowIntersections,
                      const std::vector<glm::vec3> &lightPoints, float ambientLight);
float GouraudShadingSoft(RayTriangleIntersection intersection, const std::vector<RayTriangleIntersection> &shadowIntersections,
                         const std::vector<glm::vec3> &lightPoints, float ambientLight);
float phongShadingSoft(RayTriangleIntersection intersection, const std::vector<RayTriangleIntersection> &shadowIntersections,
                       const std::vector<glm::vec3> &lightPoints, float ambientLight);
void renderRayTracedSceneSoftShadow(DrawingWindow &window, const std::string& filename, float focalLength,
                                    const std::string& materialFilename,const int signalForShading);


#endif //REDNOISE_SOFTSHADOWRENDERING_H
