#include "EnvironmentMapping.h"


uint32_t sampleColourFromTextureMap(const TextureMap &textureMap, float u, float v) {
    // 将UV坐标映射到像素坐标
    int x = static_cast<int>(u * (textureMap.width - 1));
    int y = static_cast<int>(v * (textureMap.height - 1));

    // 确保坐标在有效范围内
    x = glm::clamp(x, 0, static_cast<int>(textureMap.width) - 1);
    y = glm::clamp(y, 0, static_cast<int>(textureMap.height) - 1);

    // 从TextureMap中采样颜色
    return textureMap.pixels[y * textureMap.width + x];
}

uint32_t getColourFromEnvironmentMap(const glm::vec3 &reflectionVector, const std::array<TextureMap, 6>& textures) {
    float absX = fabs(reflectionVector.x);
    float absY = fabs(reflectionVector.y);
    float absZ = fabs(reflectionVector.z);

    int index;
    float u, v;  // UV coordinates for texture sampling

    // Determine which texture to use and calculate UV coordinates
    if (absX > absY && absX > absZ) {
        // The reflection vector is pointing in the 'right' or 'left' direction
        index = reflectionVector.x > 0 ? 0 : 1; // 'right' or 'left' texture index
        u = reflectionVector.x > 0 ? -reflectionVector.z : reflectionVector.z;
        v = reflectionVector.y;
        u = 0.5f * (1.0f + u / absX);
        v = 0.5f * (1.0f - v / absX);
    } else if (absY > absX && absY > absZ) {
        // The reflection vector is pointing in the 'up' or 'down' direction
        index = reflectionVector.y > 0 ? 2 : 3; // 'up' or 'down' texture index
        u = reflectionVector.x;
        v = reflectionVector.y > 0 ? -reflectionVector.z : reflectionVector.z;
        u = 0.5f * (1.0f + u / absY);
        v = 0.5f * (1.0f - v / absY);
    } else {
        // The reflection vector is pointing in the 'front' or 'back' direction
        index = reflectionVector.z > 0 ? 4 : 5; // 'front' or 'back' texture index
        u = reflectionVector.z > 0 ? reflectionVector.x : -reflectionVector.x;
        v = reflectionVector.y;
        u = 0.5f * (1.0f + u / absZ);
        v = 0.5f * (1.0f - v / absZ);
    }

    // Flip the V coordinate because texture coordinates start from the top left corner
//    if (index ==4){
//        v = 1.0f - v;
//    }

    // Sample the color from the texture
    uint32_t packedColour = sampleColourFromTextureMap(textures[index], u, v);

    return packedColour;
}

void renderRayTracedSceneForEnv(DrawingWindow &window, const std::string& filename,
                                float focalLength,const std::array<TextureMap, 6>& textures) {
    // Load the triangles from the OBJ file.
    std::vector<ModelTriangle> triangles = loadOBJ(filename, 0.5);

    std::cout << "Loaded " << triangles.size() << " triangles for ray tracing" << std::endl;

    glm::vec3 ModelCenter = calculateModelCenter(triangles);
    cameraOrientation = lookAt(ModelCenter);

    // Loop over each pixel on the image plane
    for (int y = 0; y < int(window.height); y++) {
        for (int x = 0; x < int(window.width); x++) {
            // Compute the ray direction for this pixel
            glm::vec3 rayDirection = computeRayDirection(window.width, window.height, x, y, focalLength,cameraOrientation);

            // Find the closest intersection of this ray with the scene
            RayTriangleIntersection intersection = getClosestIntersection(cameraPosition, rayDirection, triangles);

            // If an intersection was found, color the pixel accordingly
            if (intersection.distanceFromCamera != std::numeric_limits<float>::infinity()) {
                //如果交点是镜面的

                //根据反射光的方向从环境贴图中采样颜色
                glm::vec3 reflectDir = glm::reflect(rayDirection, intersection.intersectedTriangle.normal);
                uint32_t rgbColour = getColourFromEnvironmentMap(reflectDir, textures);
                window.setPixelColour(x, y, rgbColour);

            } else {
                // No intersection found, set the pixel to the background color,
                window.setPixelColour(x, y, 0);
            }
        }
    }
}