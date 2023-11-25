//
// Created by dell on 2023/11/24.
//

#ifndef REDNOISE_ROTATECAMERA_H
#define REDNOISE_ROTATECAMERA_H

#include "glm/glm.hpp"

glm::mat3 rotateX(float angle);
glm::mat3 rotateY(float angle);
glm::vec3 orbitCameraAroundY(glm::vec3 cameraPos, float angle, glm::vec3 ModelCenter);
glm::vec3 orbitCameraAroundYInverse(glm::vec3 cameraPos, float angle, glm::vec3 ModelCenter);
glm::vec3 orbitCameraAroundX(glm::vec3 cameraPos, float angle, glm::vec3 ModelCenter);
glm::vec3 orbitCameraAroundXInverse(glm::vec3 cameraPos, float angle, glm::vec3 ModelCenter);

#endif //REDNOISE_ROTATECAMERA_H
