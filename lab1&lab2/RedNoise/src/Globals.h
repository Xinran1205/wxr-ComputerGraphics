//
// Created by dell on 2023/11/6.
//

#ifndef GLOBALS_H
#define GLOBALS_H

#include "glm/glm.hpp"
#include <vector>


extern std::vector<std::vector<float>> zBuffer;
extern glm::vec3 cameraPosition;
extern glm::mat3 cameraOrientation;
extern float cameraSpeed;
extern float cameraRotationSpeed;

#endif //GLOBALS_H
