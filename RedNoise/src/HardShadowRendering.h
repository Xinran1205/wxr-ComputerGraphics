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

void renderRayTracedScene(DrawingWindow &window, const std::string& filename, float focalLength,const std::string& materialFilename,const int signalForShading);

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

Colour traceRefractiveRay(const glm::vec3& refractOrigin,
                          const glm::vec3& refractDir, const std::vector<ModelTriangle>& triangles,
                          int depth,const glm::vec3 &sourceLight, float ambientLight);

glm::vec3 calculate_refracted_ray(const glm::vec3 &incident, const glm::vec3 &normal, float ior);


#endif //REDNOISE_HARDSHADOWRENDERING_H
