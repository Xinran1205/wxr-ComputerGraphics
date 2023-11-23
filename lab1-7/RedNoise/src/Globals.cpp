//
// Created by dell on 2023/11/6.
//

#include "Globals.h"
#include "glm/glm.hpp"
#include <vector>

std::vector<std::vector<float>> zBuffer;
glm::vec3 cameraPosition = glm::vec3(0, 0, 2);
glm::mat3 cameraOrientation = glm::mat3(1.0f);
float cameraSpeed = 5.0f;
float cameraRotationSpeed = 0.05f;
std::map<glm::vec3, glm::vec3, Vec3Comparator> vertexNormals;
std::map<glm::vec3, float,Vec3Comparator> vertexBrightnessGlobal;
