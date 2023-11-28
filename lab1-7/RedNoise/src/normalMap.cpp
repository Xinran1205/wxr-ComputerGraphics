#include "normalMap.h"

// There is a little bug in this class, cannot get the correct texture color from the texture map.

// But I think my processes are mostly correct!


// this function using the normal vector got from the normal texture map to calculate the lighting
float FlatShadingNormal(RayTriangleIntersection intersection, RayTriangleIntersection shadowIntersection,
                  const glm::vec3 &sourceLight, float ambientLight,glm::vec3 normalMap ) {
    float brightness = calculateLighting(intersection.intersectionPoint,
                                         normalMap, sourceLight);
    float specularIntensity = calculateSpecularLighting(intersection.intersectionPoint, cameraPosition,
                                                        sourceLight, normalMap, shininess);


    if (shadowIntersection.distanceFromCamera < glm::length(sourceLight - intersection.intersectionPoint)&&
        shadowIntersection.triangleIndex != intersection.triangleIndex) {
        brightness = ambientLight;
    } else {
        // if the intersection is not in shadow, combine the brightness with the ambient light
        brightness = glm::max(brightness + ambientLight, ambientLight);
    }

    // combine the brightness with the specular intensity
    float combinedBrightness = glm::clamp(brightness + specularIntensity, 0.0f, 1.0f);
    return combinedBrightness;
}


void renderRayTracedSceneNormal(DrawingWindow &window, const std::string& filename, float focalLength,
                                TextureMap &textureMap,const std::string& materialFilename) {
    // Load the triangles from the OBJ file.
    std::vector<ModelTriangle> triangles = loadOBJ(filename, 0.35,materialFilename);
    //先手动设置三角形顶点的纹理坐标,一共就两个三角形
    triangles[0].texturePoints = {TexturePoint(80, 210), TexturePoint(200, 210), TexturePoint(200, 50)};
    triangles[1].texturePoints = {TexturePoint(80, 210), TexturePoint(200, 50), TexturePoint(80, 50)};

    TextureMap normalMap = TextureMap("../NormalMap/normalTex.ppm");

    glm::vec3 ModelCenter = calculateModelCenter(triangles);
    float degree = 1.0f;
    float orbitRotationSpeed = degree * (M_PI / 180.0f);
    // Translate the camera
    cameraPosition = orbitCameraAroundY(cameraPosition, orbitRotationSpeed, ModelCenter);
    // Rotate the camera to look at the model center
    cameraOrientation = lookAt(ModelCenter);
    std::cout << "Loaded " << triangles.size() << " triangles for ray tracing" << std::endl;

    glm::vec3 sourceLight = glm::vec3(0.5, 0.5, 1);
    float ambientLight = 0.9f;  // ambient light intensity
    // Loop over each pixel on the image plane
    for (int y = 0; y < int(window.height); y++) {
        for (int x = 0; x < int(window.width); x++) {
            // Compute the ray direction for this pixel
            glm::vec3 rayDirection = computeRayDirection(window.width, window.height, x, y, focalLength,cameraOrientation);

            // Find the closest intersection of this ray with the scene
            RayTriangleIntersection intersection = getClosestIntersection(cameraPosition, rayDirection, triangles);

            // If an intersection was found, color the pixel accordingly
            if (intersection.distanceFromCamera != std::numeric_limits<float>::infinity()) {
                //这里这个颜色应该为这个点的纹理的颜色
                //用重心坐标根据这个三角形的三个点的纹理坐标，插值出这个点的纹理坐标
                glm::vec3 barycentricCoords = calculateBarycentricCoordinates(intersection.intersectionPoint,
                                                                              intersection.intersectedTriangle.vertices);
                TexturePoint intersectTexturePoints ={barycentricCoords.x * intersection.intersectedTriangle.texturePoints[0].x +
                                                      barycentricCoords.y * intersection.intersectedTriangle.texturePoints[1].x +
                                                      barycentricCoords.z * intersection.intersectedTriangle.texturePoints[2].x,
                                                      barycentricCoords.x * intersection.intersectedTriangle.texturePoints[0].y +
                                                      barycentricCoords.y * intersection.intersectedTriangle.texturePoints[1].y +
                                                        barycentricCoords.z * intersection.intersectedTriangle.texturePoints[2].y};
                uint32_t packedColour = textureMap.pixels[int(intersectTexturePoints.y * textureMap.width + intersectTexturePoints.x)];

                // this is the normal value get from the normal texture map
                uint32_t normalVal = normalMap.pixels[int(intersectTexturePoints.y * normalMap.width + intersectTexturePoints.x)];

                // extract the RGB value from the normal value
                float red = (normalVal >> 16) & 0xFF;
                float green = (normalVal >> 8) & 0xFF;
                float blue = normalVal & 0xFF;

                // map the RGB value to the range [-1,1]
                glm::vec3 normal = glm::vec3((red / 127.5f) - 1.0f,
                                             (green / 127.5f) - 1.0f,
                                             (blue / 127.5f) - 1.0f);
                normal = glm::normalize(normal);

                glm::vec3 shadowRay = glm::normalize(sourceLight - intersection.intersectionPoint);
                RayTriangleIntersection shadowIntersection = getClosestIntersection(intersection.intersectionPoint + shadowRay * 0.002f,
                                                                                    shadowRay, triangles);

                float combinedBrightness = FlatShadingNormal(intersection,shadowIntersection, sourceLight, ambientLight,normal);

                // 解包
                glm::vec3 colour = glm::vec3((packedColour >> 16) & 0xFF, (packedColour >> 8) & 0xFF, packedColour & 0xFF);
                // 应用光照强度到颜色
                glm::vec3 litColour = colour * combinedBrightness;
                // 重新打包颜色值
                uint32_t rgbColour = (255 << 24) + (int(litColour.r) << 16) + (int(litColour.g) << 8) + int(litColour.b);
                window.setPixelColour(x, y,rgbColour );

            } else {
                // No intersection found, set the pixel to the background color,
                window.setPixelColour(x, y, 0);
            }
        }
    }
}