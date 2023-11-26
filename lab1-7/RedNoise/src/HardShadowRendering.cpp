//
// Created by dell on 2023/11/25.
//
#include "HardShadowRendering.h"

// calculate diffuse lighting
float calculateLighting(const glm::vec3 &point, const glm::vec3 &normal, const glm::vec3 &lightSource) {
    glm::vec3 toLight = lightSource - point;
    float distance = glm::length(toLight);
    float proximityBrightness = 3.0f / (5 * M_PI * distance * distance);


    glm::vec3 lightDirection = glm::normalize(toLight);
    glm::vec3 normalDirection = glm::normalize(normal);
    // cos(theta) = dot product of the normal and the light direction
    float dotProduct = glm::dot(normalDirection, lightDirection);
    float incidenceBrightness = std::max(dotProduct, 0.0f);
//    float incidenceBrightness = std::abs(dotProduct);
    // combine the two brightness factors
    float combinedBrightness = proximityBrightness * incidenceBrightness;
    // clamp the combined brightness between 0 and 1
    return glm::clamp(combinedBrightness, 0.0f, 1.0f);
}

glm::vec3 calculateBarycentricCoordinates(const glm::vec3 &P, const std::array<glm::vec3, 3> &triangleVertices) {
    glm::vec3 A = triangleVertices[0];
    glm::vec3 B = triangleVertices[1];
    glm::vec3 C = triangleVertices[2];

    glm::vec3 AB = B - A;
    glm::vec3 AC = C - A;
    glm::vec3 BC = C - B;
    glm::vec3 AP = P - A;
    glm::vec3 BP = P - B;

    // 计算三角形ABC的面积的两倍（使用叉乘）
    float areaABC = glm::length(glm::cross(AB, AC));
    // 计算P相对于三角形顶点的子三角形的面积的两倍
    float areaPBC = glm::length(glm::cross(BP, BC));
    float areaPCA = glm::length(glm::cross(AP, AC));

    // 重心坐标是子三角形面积除以整个三角形面积
    float u = areaPBC / areaABC;
    float v = areaPCA / areaABC;
    float w = 1.0f - u - v;

    return glm::vec3(u, v, w);
}

RayTriangleIntersection getClosestIntersection(const glm::vec3 &cameraPosition,
                                               const glm::vec3 &rayDirection, const std::vector<ModelTriangle> &triangles) {
    RayTriangleIntersection closestIntersection;
    closestIntersection.distanceFromCamera = std::numeric_limits<float>::infinity(); // 初始化为最大值
    float closestDistance = std::numeric_limits<float>::infinity(); // 初始化为最大值

    // go through all the triangles and find the closest intersection
    for (size_t i = 0; i < triangles.size(); i++) {
        const ModelTriangle &triangle = triangles[i];

        glm::vec3 e0 = triangle.vertices[1] - triangle.vertices[0];
        glm::vec3 e1 = triangle.vertices[2] - triangle.vertices[0];
        glm::vec3 SPVector = cameraPosition - triangle.vertices[0];
        glm::mat3 DEMatrix(-rayDirection, e0, e1);
        glm::vec3 possibleSolution = glm::inverse(DEMatrix) * SPVector;

        float t = possibleSolution.x, u = possibleSolution.y, v = possibleSolution.z;

        // check if the intersection is in front of the camera, and if it is the closest intersection so far
        if (t > 0 && u >= 0 && u <= 1 && v >= 0 && v <= 1 && u + v <= 1) {
            if (t < closestDistance) {
                closestDistance = t;
                glm::vec3 intersectionPoint = cameraPosition + t * rayDirection;
                closestIntersection = RayTriangleIntersection(intersectionPoint, t, triangle, i);
            }
        }
    }

    // if no intersection was found, closestIntersection.distanceFromCamera will remain infinity
    return closestIntersection;
}

glm::vec3 computeRayDirection(int screenWidth, int screenHeight, int x, int y, float focalLength, glm::mat3 cameraOrientation) {
    // the camera is at (0, 0, 4), and the image plane is at z = 2

    float scale = 150.0f; // Adjust this factor to zoom in or out

    float canvasX = (x - screenWidth / 2) / scale;
    float canvasY = -(screenHeight / 2 - y) / scale;

    glm::vec3 imagePlanePoint = glm::vec3(canvasX, canvasY, focalLength);

    // rotate the camera and let it look at the model center
    glm::vec3 rayDirection = cameraOrientation * imagePlanePoint;
    // normalize the ray direction
    rayDirection = glm::normalize(rayDirection);

    return rayDirection;
}

float calculateSpecularLighting(const glm::vec3 &point,const glm::vec3 &cameraPosition,
                                const glm::vec3 &lightSource, const glm::vec3 &normal, int shininess) {
    glm::vec3 viewDirection = glm::normalize(cameraPosition - point);
    glm::vec3 lightDirection = glm::normalize(lightSource - point);
    // calculate the reflection direction
    glm::vec3 reflectDir = glm::reflect(-lightDirection, normal);
    // compare the reflection direction with the view direction
    float spec = glm::pow(glm::max(glm::dot(viewDirection, reflectDir), 0.0f), shininess);
    return spec;
}

float FlatShading(RayTriangleIntersection intersection, RayTriangleIntersection shadowIntersection,
                  const glm::vec3 &sourceLight, float ambientLight) {
    float brightness = calculateLighting(intersection.intersectionPoint,
                                         intersection.intersectedTriangle.normal, sourceLight);
    float specularIntensity = calculateSpecularLighting(intersection.intersectionPoint, cameraPosition,
                                                        sourceLight, intersection.intersectedTriangle.normal, shininess);


    if (shadowIntersection.distanceFromCamera < glm::length(sourceLight - intersection.intersectionPoint)&& shadowIntersection.triangleIndex != intersection.triangleIndex) {
        brightness = ambientLight;
    } else {
        // if the intersection is not in shadow, combine the brightness with the ambient light
        brightness = glm::max(brightness + ambientLight, ambientLight);
    }

    // combine the brightness with the specular intensity
    float combinedBrightness = glm::clamp(brightness + specularIntensity, 0.0f, 1.0f);
    return combinedBrightness;
}

float GouraudShading(RayTriangleIntersection intersection, RayTriangleIntersection shadowIntersection,
                     const glm::vec3 &sourceLight, float ambientLight){
    for (int i = 0; i < 3; i++) {
        glm::vec3 vertex = intersection.intersectedTriangle.vertices[i];
        // 如果这个顶点已经计算过了，就不用再计算了
        if (vertexBrightnessGlobal.find(vertex) != vertexBrightnessGlobal.end()){
            continue;
        }

        glm::vec3 normal = vertexNormals[vertex];
        // calculate the diffuse lighting and specular lighting
        float brightness = calculateLighting(vertex, normal, sourceLight);
        float specularIntensity = calculateSpecularLighting(vertex, cameraPosition, sourceLight, normal, shininess);

        if (shadowIntersection.distanceFromCamera < glm::length(sourceLight - vertex)&& shadowIntersection.triangleIndex != intersection.triangleIndex) {
            brightness = ambientLight;
        } else {
            // if the intersection is not in shadow, combine the brightness with the ambient light
            brightness = glm::max(brightness + ambientLight, ambientLight);
        }

        // combine the brightness with the specular intensity
        float combinedBrightness = glm::clamp(brightness + specularIntensity, 0.0f, 1.0f);

        vertexBrightnessGlobal[vertex] = combinedBrightness;
    }
    // 根据重心坐标插值出交点的brigtness
    glm::vec3 barycentricCoords = calculateBarycentricCoordinates(intersection.intersectionPoint, intersection.intersectedTriangle.vertices);

    // interpolate the normal
    float ResultVertexBrightness =
            barycentricCoords.x * vertexBrightnessGlobal[intersection.intersectedTriangle.vertices[0]] +
            barycentricCoords.y * vertexBrightnessGlobal[intersection.intersectedTriangle.vertices[1]] +
            barycentricCoords.z * vertexBrightnessGlobal[intersection.intersectedTriangle.vertices[2]];
    return ResultVertexBrightness;
}

float phongShading(RayTriangleIntersection intersection, RayTriangleIntersection shadowIntersection,
                   const glm::vec3 &sourceLight, float ambientLight) {
    glm::vec3 normal0  = vertexNormals[intersection.intersectedTriangle.vertices[0]];
    glm::vec3 normal1  = vertexNormals[intersection.intersectedTriangle.vertices[1]];
    glm::vec3 normal2  = vertexNormals[intersection.intersectedTriangle.vertices[2]];
    // calculateBarycentricCoordinates will return u, v, w
    glm::vec3 barycentricCoords = calculateBarycentricCoordinates(intersection.intersectionPoint, intersection.intersectedTriangle.vertices);

    // interpolate the normal
    glm::vec3 interpolatedNormal =
            barycentricCoords.x * normal0 +
            barycentricCoords.y * normal1 +
            barycentricCoords.z * normal2;

    interpolatedNormal = glm::normalize(interpolatedNormal);

    // phong shading
    float brightness = calculateLighting(intersection.intersectionPoint, interpolatedNormal, sourceLight);
    float specularIntensity = calculateSpecularLighting(intersection.intersectionPoint, cameraPosition, sourceLight,
                                                        interpolatedNormal, shininess);

    if (shadowIntersection.distanceFromCamera < glm::length(sourceLight - intersection.intersectionPoint)&& shadowIntersection.triangleIndex != intersection.triangleIndex) {
        brightness = ambientLight;
    } else {
        // if the intersection is not in shadow, combine the brightness with the ambient light
        brightness = glm::max(brightness + ambientLight, ambientLight);
    }

    // combine the brightness with the specular intensity
    float combinedBrightness = glm::clamp(brightness + specularIntensity, 0.0f, 1.0f);
    return combinedBrightness;
}

// 添加一个新的函数来处理反射光线的递归追踪
Colour traceReflectiveRay(const glm::vec3 &rayOrigin, const glm::vec3 &rayDirection, const std::vector<ModelTriangle> &triangles,
                          int depth, const glm::vec3 &sourceLight, float ambientLight) {
    // 递归深度大于3就停止
    if (depth >= 3) {
        return Colour(0, 0, 0);
    }

    RayTriangleIntersection intersection = getClosestIntersection(rayOrigin, rayDirection, triangles);
    //其实这个条件基本上走不进去，除非我有多个镜面
    if (intersection.intersectedTriangle.isMirror) {
        glm::vec3 reflectDir = glm::reflect(rayDirection, intersection.intersectedTriangle.normal);
        glm::vec3 reflectOrigin = intersection.intersectionPoint + reflectDir * 0.002f; // 避免自我交叉
        return traceReflectiveRay(reflectOrigin, reflectDir, triangles, depth + 1, sourceLight, ambientLight);
    } else if(intersection.intersectedTriangle.isGlass){
        //如果反射撞到玻璃上面，这个intersection就是和玻璃的交点

        float indexOfRefraction = 1.6;
        glm::vec3 normal{};
        if (glm::dot(rayDirection, intersection.intersectedTriangle.normal)<0) {
            normal = -intersection.intersectedTriangle.normal;
        }else{
            normal = intersection.intersectedTriangle.normal;
        }
        glm::vec3 refractDir = calculate_refracted_ray(rayDirection, normal, indexOfRefraction);
        glm::vec3 refractOrigin = intersection.intersectionPoint + normal * 0.002f; // 避免自我交叉

        Colour refractColour = traceRefractiveRay(refractOrigin, refractDir, triangles, 1,sourceLight,ambientLight);
        return refractColour;
    }else{
        if (intersection.distanceFromCamera == std::numeric_limits<float>::infinity()) {
            return Colour(0, 0, 0);
        }

        glm::vec3 shadowRay = glm::normalize(sourceLight - intersection.intersectionPoint);
        RayTriangleIntersection shadowIntersection = getClosestIntersection(intersection.intersectionPoint + shadowRay * 0.002f,
                                                                            shadowRay, triangles);

        //这里是三个不同的shading方法，可以自己选择
//                float combinedBrightness = phongShading(intersection,shadowIntersection, sourceLight, ambientLight);
//                float combinedBrightness = GouraudShading(intersection,shadowIntersection, sourceLight, ambientLight);
        float combinedBrightness = FlatShading(intersection,shadowIntersection, sourceLight, ambientLight);

        Colour colour = intersection.intersectedTriangle.colour;
        colour.red *= combinedBrightness;
        colour.green *= combinedBrightness;
        colour.blue *= combinedBrightness;

        return colour; // 返回交点的颜色
    }
}

glm::vec3 calculate_refracted_ray(const glm::vec3 &incident, const glm::vec3 &normal, float ior) {
    float cosi = glm::clamp(glm::dot(-incident, normal), -1.0f, 1.0f);
    float etai = 1, etat = ior;
    glm::vec3 n = normal;
    if (cosi < 0) {
        cosi = -cosi;
        std::swap(etai, etat);
        n = -n;
    }
    float eta = etai / etat;
    float k = 1 - eta * eta * (1 - cosi * cosi);
    if (k < 0) {
        return glm::vec3(0); // Total internal reflection
    } else {
        return eta * incident + (eta * cosi - static_cast<float>(sqrt(k))) * n;
    }
}

Colour traceRefractiveRay(const glm::vec3& refractOrigin,
                          const glm::vec3& refractDir, const std::vector<ModelTriangle>& triangles,
                          int depth, const glm::vec3 &sourceLight, float ambientLight) {
    // 递归的基本情况：如果超过了最大递归深度，返回默认颜色（例如，背景颜色）
//    if (depth > 5) {
//        return Colour(0, 0, 0); // 黑色或任何其他背景颜色
//    }

    // 在场景中找到折射光线与最近的交点
    RayTriangleIntersection closestIntersection = getClosestIntersection(refractOrigin, refractDir, triangles);

//     如果没有找到交点，返回背景颜色
    if (closestIntersection.distanceFromCamera == std::numeric_limits<float>::infinity()) {
        return Colour(0, 0, 0); // 根据场景背景定义这个颜色
    }

    // 假设我此时从交点往相同方向继续射，如果下面碰到的物体还是玻璃，说明我们在玻璃内部
    glm::vec3 NextRefractOrigin = closestIntersection.intersectionPoint + closestIntersection.intersectedTriangle.normal * 0.001f;
    RayTriangleIntersection NextClosestIntersection = getClosestIntersection(NextRefractOrigin,
                                                                             refractDir, triangles);
    bool isInside = NextClosestIntersection.intersectedTriangle.isGlass;
    //这个条件模拟的出玻璃时的情况
    if (closestIntersection.intersectedTriangle.isGlass && !isInside) {
        // 计算新的折射率
        float newIndexOfRefraction = 1/1.3;
        glm::vec3 normal{};
        if (glm::dot(refractDir, closestIntersection.intersectedTriangle.normal)<0) {
            normal = -closestIntersection.intersectedTriangle.normal;
        }else{
            normal = closestIntersection.intersectedTriangle.normal;
        }

        // 使用glm::refract计算折射方向
        glm::vec3 newRefractDir =calculate_refracted_ray(refractDir, normal, newIndexOfRefraction);

        // 计算新的折射原点，稍微偏移以避免自我交叉
        glm::vec3 newRefractOrigin = closestIntersection.intersectionPoint + normal * 0.001f;

        RayTriangleIntersection FinalClosestIntersection = getClosestIntersection(newRefractOrigin,
                                                                                 newRefractDir, triangles);
        //如果这个新的交点是无穷远，说明没有新的交点，返回背景颜色
        if (FinalClosestIntersection.distanceFromCamera == std::numeric_limits<float>::infinity()) {
            // 如果没有新的交点，返回背景颜色
            return Colour(0, 0, 0);
        }
        // 如果这个出玻璃后的交点是镜面的，发生反射
        if (FinalClosestIntersection.intersectedTriangle.isMirror){
            glm::vec3 reflectDir = glm::reflect(newRefractDir, FinalClosestIntersection.intersectedTriangle.normal);
            glm::vec3 reflectOrigin = FinalClosestIntersection.intersectionPoint + reflectDir * 0.002f; // 避免自我交叉
            Colour reflectColour = traceReflectiveRay(reflectOrigin, reflectDir, triangles, 1,sourceLight,ambientLight);
            return reflectColour;
        }
        // 走到这说明最后打到一个物体上面
        glm::vec3 shadowRay = glm::normalize(sourceLight - FinalClosestIntersection.intersectionPoint);
        RayTriangleIntersection shadowIntersection = getClosestIntersection(FinalClosestIntersection.intersectionPoint + shadowRay * 0.002f,
                                                                            shadowRay, triangles);
        //这里是三个不同的shading方法，可以自己选择
//                float combinedBrightness = phongShading(intersection,shadowIntersection, sourceLight, ambientLight);
//                float combinedBrightness = GouraudShading(intersection,shadowIntersection, sourceLight, ambientLight);
        float combinedBrightness = FlatShading(FinalClosestIntersection,shadowIntersection, sourceLight, ambientLight);

        Colour colour = FinalClosestIntersection.intersectedTriangle.colour;
        colour.red *= combinedBrightness;
        colour.green *= combinedBrightness;
        colour.blue *= combinedBrightness;

        return colour; // 返回交点的颜色

//        // 递归追踪新的折射光线
//        return traceRefractiveRay(newRefractOrigin, newRefractDir, triangles, depth + 1);
    }else{
        // 仍然在玻璃内
        glm::vec3 newRefractOrigin = closestIntersection.intersectionPoint + closestIntersection.intersectedTriangle.normal * 0.001f;

        // 递归追踪新的折射光线
        return traceRefractiveRay(newRefractOrigin, refractDir, triangles, depth + 1,sourceLight,ambientLight);
    }
}


void renderRayTracedScene(DrawingWindow &window, const std::string& filename, float focalLength) {
    // Load the triangles from the OBJ file.
    std::vector<ModelTriangle> triangles = loadOBJ(filename, 0.35);

    glm::vec3 ModelCenter = calculateModelCenter(triangles);
    float degree = 1.0f;
    float orbitRotationSpeed = degree * (M_PI / 180.0f);
    // Translate the camera
    cameraPosition = orbitCameraAroundY(cameraPosition, orbitRotationSpeed, ModelCenter);
    // Rotate the camera to look at the model center
    cameraOrientation = lookAt(ModelCenter);

    std::cout << "Loaded " << triangles.size() << " triangles for ray tracing" << std::endl;

//    glm::vec3 sourceLight = calculateLightSourcePosition();
    glm::vec3 sourceLight = glm::vec3(0, 0.8, 0);
    float ambientLight = 0.3f;  // ambient light intensity

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
                if (intersection.intersectedTriangle.isMirror){
                    glm::vec3 reflectDir = glm::reflect(rayDirection, intersection.intersectedTriangle.normal);
                    glm::vec3 reflectOrigin = intersection.intersectionPoint + reflectDir * 0.002f; // 避免自我交叉
                    Colour reflectColour = traceReflectiveRay(reflectOrigin, reflectDir, triangles, 1,sourceLight,ambientLight);
                    uint32_t rgbColour = (255 << 24) |
                                         (int(reflectColour.red) << 16) |
                                         (int(reflectColour.green) << 8) |
                                         int(reflectColour.blue);
                    window.setPixelColour(x, y, rgbColour);

                }else if(intersection.intersectedTriangle.isGlass){
                    // 如果交点是玻璃的，发生折射
                    float indexOfRefraction = 1.3;
                    glm::vec3 normal{};
                    if (glm::dot(rayDirection, intersection.intersectedTriangle.normal)<0) {
                        normal = -intersection.intersectedTriangle.normal;
                    }else{
                        normal = intersection.intersectedTriangle.normal;
                    }
                    glm::vec3 refractDir = calculate_refracted_ray(rayDirection, normal, indexOfRefraction);
                    glm::vec3 refractOrigin = intersection.intersectionPoint + normal * 0.001f; // 避免自我交叉

                    Colour refractColour = traceRefractiveRay(refractOrigin, refractDir, triangles, 1,sourceLight,ambientLight);
                    uint32_t rgbColour = (255 << 24) |
                                         (int(refractColour.red) << 16) |
                                         (int(refractColour.green) << 8) |
                                         int(refractColour.blue);
                    window.setPixelColour(x, y, rgbColour);
                }else{//交点不是镜面的
                    glm::vec3 shadowRay = glm::normalize(sourceLight - intersection.intersectionPoint);
                    RayTriangleIntersection shadowIntersection = getClosestIntersection(intersection.intersectionPoint + shadowRay * 0.002f,
                                                                                        shadowRay, triangles);

                    //这里是三个不同的shading方法，可以自己选择
//                float combinedBrightness = phongShading(intersection,shadowIntersection, sourceLight, ambientLight);
//                float combinedBrightness = GouraudShading(intersection,shadowIntersection, sourceLight, ambientLight);
                    float combinedBrightness = FlatShading(intersection,shadowIntersection, sourceLight, ambientLight);

                    Colour colour = intersection.intersectedTriangle.colour;
                    uint32_t rgbColour = (255 << 24) |
                                         (int(combinedBrightness * colour.red) << 16) |
                                         (int(combinedBrightness * colour.green) << 8) |
                                         int(combinedBrightness * colour.blue);
                    window.setPixelColour(x, y, rgbColour);
                }
            } else {
                // No intersection found, set the pixel to the background color,
                window.setPixelColour(x, y, 0);
            }
        }
    }
}


////如果交点是玻璃的，发生折射
////                    // 计算Fresnel效应
////                    float kr; // 反射系数
//float indexOfRefraction = 1.6;
////                    computeFresnel(rayDirection, intersection.intersectedTriangle.normal, indexOfRefraction, kr);
////
////                    Colour reflectionColour = {0, 0, 0};
//Colour refractionColour = {0, 0, 0};
////
////                    // 如果有反射
////                    if (kr < 1) {
////                        glm::vec3 reflectDir = glm::reflect(rayDirection, intersection.intersectedTriangle.normal);
////                        glm::vec3 reflectOrigin = intersection.intersectionPoint + reflectDir * 0.002f;
////                        reflectionColour = traceReflectiveRay(reflectOrigin, reflectDir, triangles, 1);
////                    }
////
////                    // 计算折射方向（如果有必要）
////                    if (kr > 0) {
//glm::vec3 refractDir = glm::refract(rayDirection, intersection.intersectedTriangle.normal, indexOfRefraction);
////这里可能要改成延法线相反方向平移，不确定
//glm::vec3 refractOrigin = intersection.intersectionPoint + refractDir * 0.002f;
//refractionColour = traceRefractiveRay(intersection.intersectedTriangle,refractOrigin,
//                                      refractDir, triangles, 1, indexOfRefraction);
////                    }
////
////                    int mixedRed = kr * reflectionColour.red + (1 - kr) * refractionColour.red;
////                    int mixedGreen = kr * reflectionColour.green + (1 - kr) * refractionColour.green;
////                    int mixedBlue = kr * reflectionColour.blue + (1 - kr) * refractionColour.blue;
//
////                    Colour pixelColour = Colour(mixedRed, mixedGreen, mixedBlue);
//uint32_t rgbColour = (255 << 24) |
//                     (refractionColour.red << 16) |
//                     (refractionColour.green << 8) |
//                     refractionColour.blue;
//window.setPixelColour(x, y, rgbColour);