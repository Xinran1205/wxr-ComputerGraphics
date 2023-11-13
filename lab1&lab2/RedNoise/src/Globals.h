//
// Created by dell on 2023/11/6.
//

#ifndef GLOBALS_H
#define GLOBALS_H

#include "glm/glm.hpp"
#include <vector>
#include <unordered_map>

struct Vec3Hash {
    std::size_t operator()(const glm::vec3& vec) const {
        // 使用 std::hash 来为每个浮点数生成哈希值，并以某种方式组合它们
        std::size_t hx = std::hash<float>()(vec.x);
        std::size_t hy = std::hash<float>()(vec.y);
        std::size_t hz = std::hash<float>()(vec.z);
        // 将三个哈希值组合成一个哈希值
        return hx ^ (hy << 1) ^ (hz << 2); // 这只是一种组合方式
    }
};

extern std::vector<std::vector<float>> zBuffer;
extern glm::vec3 cameraPosition;
extern glm::mat3 cameraOrientation;
extern float cameraSpeed;
extern float cameraRotationSpeed;
extern std::unordered_map<glm::vec3, std::vector<glm::vec3>, Vec3Hash> vertexToNormals;


#endif //GLOBALS_H
