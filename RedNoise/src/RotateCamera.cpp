//
// Created by dell on 2023/11/24.
//

#include "RotateCamera.h"



// Function to generate rotation matrix about the X axis
glm::mat3 rotateX(float angle) {
    glm::mat3 rotationMatrix = glm::mat3(
            1, 0, 0,
            0, cos(angle), -sin(angle),
            0, sin(angle), cos(angle)
    );
    return rotationMatrix;
}

// Function to generate rotation matrix about the Y axis
glm::mat3 rotateY(float angle) {
    glm::mat3 rotationMatrix = glm::mat3(
            cos(angle), 0, sin(angle),
            0, 1, 0,
            -sin(angle), 0, cos(angle)
    );
    return rotationMatrix;
}

//this is the function to rotate the camera around the y axis， this is the movement of the camera, only change x and z
glm::vec3 orbitCameraAroundY(glm::vec3 cameraPos, float angle, glm::vec3 ModelCenter) {
    glm::vec3 translatedPosition = cameraPos - ModelCenter; // Translate to origin

    // Apply rotation
    float newX = translatedPosition.x * cos(angle) - translatedPosition.z * sin(angle);
    float newZ = translatedPosition.x * sin(angle) + translatedPosition.z * cos(angle);

    // Translate back to the original position
    return glm::vec3(newX, cameraPos.y, newZ) ;
}

glm::vec3 orbitCameraAroundYInverse(glm::vec3 cameraPos, float angle, glm::vec3 ModelCenter) {
    glm::vec3 translatedPosition = cameraPos - ModelCenter; // Translate to origin

    // Apply rotation in the opposite direction
    float newX = translatedPosition.x * cos(-angle) - translatedPosition.z * sin(-angle);
    float newZ = translatedPosition.x * sin(-angle) + translatedPosition.z * cos(-angle);

    // Translate back to the original position
    return glm::vec3(newX, cameraPos.y, newZ);
}

glm::vec3 orbitCameraAroundX(glm::vec3 cameraPos, float angle, glm::vec3 ModelCenter) {
    glm::vec3 translatedPosition = cameraPos - ModelCenter; // Translate to origin

    // Apply rotation
    float newY = translatedPosition.y * cos(angle) + translatedPosition.z * sin(angle);
    float newZ = -translatedPosition.y * sin(angle) + translatedPosition.z * cos(angle);

    // Translate back to the original position
    return glm::vec3(cameraPos.x, newY, newZ);
}

glm::vec3 orbitCameraAroundXInverse(glm::vec3 cameraPos, float angle, glm::vec3 ModelCenter) {
    glm::vec3 translatedPosition = cameraPos - ModelCenter; // Translate to origin

    // Apply rotation in the opposite direction
    float newY = translatedPosition.y * cos(-angle) + translatedPosition.z * sin(-angle);
    float newZ = -translatedPosition.y * sin(-angle) + translatedPosition.z * cos(-angle);

    // Translate back to the original position
    return glm::vec3(cameraPos.x, newY, newZ);
}