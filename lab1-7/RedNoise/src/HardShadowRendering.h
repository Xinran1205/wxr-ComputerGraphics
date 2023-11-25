//
// Created by dell on 2023/11/25.
//

#ifndef REDNOISE_HARDSHADOWRENDERING_H
#define REDNOISE_HARDSHADOWRENDERING_H

#include <DrawingWindow.h>
#include <Utils.h>
#include "glm/glm.hpp"
#include "LoadFile.h"
#include "Rasterising.h"
#include "Globals.h"
#include "RayTriangleIntersection.h"

glm::vec3 computeRayDirection(int screenWidth, int screenHeight, int x, int y, float focalLength, glm::mat3 cameraOrientation);
float calculateSpecularLighting(const glm::vec3 &point,const glm::vec3 &cameraPosition,
                                const glm::vec3 &lightSource, const glm::vec3 &normal, int shininess);
RayTriangleIntersection getClosestIntersection(const glm::vec3 &cameraPosition,
                                               const glm::vec3 &rayDirection, const std::vector<ModelTriangle> &triangles);
glm::vec3 calculateBarycentricCoordinates(const glm::vec3 &P, const std::array<glm::vec3, 3> &triangleVertices);
float calculateLighting(const glm::vec3 &point, const glm::vec3 &normal, const glm::vec3 &lightSource);
float FlatShading(RayTriangleIntersection intersection, RayTriangleIntersection shadowIntersection,
                  const glm::vec3 &sourceLight, float ambientLight);

float GouraudShading(RayTriangleIntersection intersection, RayTriangleIntersection shadowIntersection,
                     const glm::vec3 &sourceLight, float ambientLight);

float phongShading(RayTriangleIntersection intersection, RayTriangleIntersection shadowIntersection,
                   const glm::vec3 &sourceLight, float ambientLight);

#endif //REDNOISE_HARDSHADOWRENDERING_H
