#include "EnvironmentMapping.h"

void renderRayTracedSceneForEnv(DrawingWindow &window, const std::string& filename,
                                float focalLength,const std::array<TextureMap, 6>& textures,const std::string& materialFilename) {
    // Load the triangles from the OBJ file.
    std::vector<ModelTriangle> triangles = loadOBJ(filename, 0.5,materialFilename);

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
                //我默认整个模型都是镜面的
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