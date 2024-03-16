#ifndef GLOBALS_H
#define GLOBALS_H

#include "glm/glm.hpp"
#include <vector>
#include <map>
extern std::vector<std::vector<float>> zBuffer;
extern glm::vec3 cameraPosition;
extern glm::mat3 cameraOrientation;
extern float cameraSpeed;
extern float cameraRotationSpeed;
extern int shininess;

struct Vec3Comparator {
    bool operator() (const glm::vec3& a, const glm::vec3& b) const {
        return std::tie(a.x, a.y, a.z) < std::tie(b.x, b.y, b.z);
    }
};
extern std::map<glm::vec3, glm::vec3, Vec3Comparator> vertexNormals;
extern std::map<glm::vec3, float,Vec3Comparator> vertexBrightnessGlobal;


#endif //GLOBALS_H
