#include "EnvironmentMapping.h"


// according to the reflection vector, sample the colour from the environment map
// 3D -> 2D
uint32_t getColourFromEnvironmentMap(const glm::vec3 &reflectionVector, const std::array<TextureMap, 6>& textures) {
    float absX = fabs(reflectionVector.x);
    float absY = fabs(reflectionVector.y);
    float absZ = fabs(reflectionVector.z);

    int index;
    float u, v;  // UV coordinates for texture sampling

    // Determine which texture to use and calculate UV coordinates
    // if the x value is the largest then we will consider to use the 'right' or 'left' texture
    // else if the y value is the largest then we will consider to use the 'up' or 'down' texture
    // else if the z value is the largest then we will consider to use the 'front' or 'back' texture
    if (absX > absY && absX > absZ) {
        // The reflection vector is pointing in the 'right' or 'left' direction
        if (reflectionVector.x > 0){
            index = 0; // right texture index
        }else{
            index = 1; // left texture index
        }
        if (reflectionVector.x > 0){
            u = -reflectionVector.z;
        }else{
            u = reflectionVector.z;
        }
        v = reflectionVector.y;
        // restrict the range of u and v to [0,1], do not allow them to be negative
        // normalize
        u = 0.5f * (1.0f + u / absX);
        v = 0.5f * (1.0f - v / absX);
    } else if (absY > absX && absY > absZ) {
        // The reflection vector is pointing in the 'up' or 'down' direction
        if (reflectionVector.y > 0){
            index = 2; // up texture index
        }else{
            index = 3; // down texture index
        }
        u = reflectionVector.x;
        if (reflectionVector.y > 0){
            v = -reflectionVector.z;
        }else{
            v = reflectionVector.z;
        }
        u = 0.5f * (1.0f + u / absY);
        v = 0.5f * (1.0f - v / absY);
    } else {
        // The reflection vector is pointing in the 'front' or 'back' direction
        if (reflectionVector.z > 0){
            index = 4; // front texture index
        }else{
            index = 5; // back texture index
        }
        if (reflectionVector.z > 0){
            u = reflectionVector.x;
        }else{
            u = -reflectionVector.x;
        }
        v = reflectionVector.y;
        u = 0.5f * (1.0f + u / absZ);
        v = 0.5f * (1.0f - v / absZ);
    }
    // calculate the texture point coordinate by remapping the range of u and v to [0, width] and [0, height]
    int x = int(u * (textures[index].width - 1));
    int y = int(v * (textures[index].height - 1));

    // get the colour from the texture map
    uint32_t packedColour = textures[index].pixels[y * textures[index].width + x];
    return packedColour;
}

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