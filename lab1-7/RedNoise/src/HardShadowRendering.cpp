#include "HardShadowRendering.h"

// calculate diffuse lighting
float calculateLighting(const glm::vec3 &point, const glm::vec3 &normal, const glm::vec3 &lightSource) {
    glm::vec3 toLight = lightSource - point;
    float distance = glm::length(toLight);
    float proximityBrightness = 7.0f / (5 * M_PI * distance * distance);
    glm::vec3 lightDirection = glm::normalize(toLight);
    glm::vec3 normalDirection = glm::normalize(normal);
    // cos(theta) = dot product of the normal and the light direction
    float dotProduct = glm::dot(normalDirection, lightDirection);
    float incidenceBrightness = std::max(dotProduct, 0.0f);
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

    // according to the formula: area = 1/2 * |AB x AC x sin(theta)|
    float areaABC = glm::length(glm::cross(AB, AC));
    float areaPBC = glm::length(glm::cross(BP, BC));
    float areaPCA = glm::length(glm::cross(AP, AC));
    float areaPAB = glm::length(glm::cross(AP, AB));

    float u = areaPBC / areaABC;
    float v = areaPCA / areaABC;
    float w = areaPAB / areaABC;

    return glm::vec3(u, v, w);
}

RayTriangleIntersection getClosestIntersection(const glm::vec3 &cameraPosition,
                                               const glm::vec3 &rayDirection, const std::vector<ModelTriangle> &triangles) {
    RayTriangleIntersection closestIntersection;
    closestIntersection.distanceFromCamera = std::numeric_limits<float>::infinity(); // initialize to infinity
    float closestDistance = std::numeric_limits<float>::infinity(); // initialize to infinity

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
    // the camera initially at (0, 0, 4), and the image plane is at z = 2

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

    if (shadowIntersection.distanceFromCamera < glm::length(sourceLight - intersection.intersectionPoint)&&
    shadowIntersection.triangleIndex != intersection.triangleIndex) {
        // if the intersection is in shadow, only use the ambient light
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
    // calculate the brightness for each vertex
    for (int i = 0; i < 3; i++) {
        glm::vec3 vertex = intersection.intersectedTriangle.vertices[i];
        // if we already calculated the brightness for this vertex, skip it
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
    // get the barycentric coordinates of the intersection point
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
    // I have already cached the vertex normals in the loadOBJ function
    glm::vec3 normal0  = vertexNormals[intersection.intersectedTriangle.vertices[0]];
    glm::vec3 normal1  = vertexNormals[intersection.intersectedTriangle.vertices[1]];
    glm::vec3 normal2  = vertexNormals[intersection.intersectedTriangle.vertices[2]];
    // get the barycentric coordinates of the intersection point
    glm::vec3 barycentricCoords = calculateBarycentricCoordinates(intersection.intersectionPoint, intersection.intersectedTriangle.vertices);

    // interpolate the normal
    glm::vec3 interpolatedNormal =
            barycentricCoords.x * normal0 +
            barycentricCoords.y * normal1 +
            barycentricCoords.z * normal2;

    interpolatedNormal = glm::normalize(interpolatedNormal);

    // phong shading, calculate the diffuse lighting and specular lighting by passing in the interpolated normal
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

// This function is to return the color of the reflected ray
Colour traceReflectiveRay(const glm::vec3 &rayOrigin, const glm::vec3 &rayDirection, const std::vector<ModelTriangle> &triangles,
                          int depth, const glm::vec3 &sourceLight, float ambientLight) {
    // if the ray has been reflected more than 3 times, return black
    if (depth >= 3) {
        return Colour(0, 0, 0);
    }

    RayTriangleIntersection intersection = getClosestIntersection(rayOrigin, rayDirection, triangles);
    if (intersection.intersectedTriangle.isMirror) {
        // actually this condition is hard to be satisfied, unless I have multiple mirrors
        // if the ray reflected by the mirror hits another mirror, it will recursively call the traceReflectiveRay function
        glm::vec3 reflectDir = glm::reflect(rayDirection, intersection.intersectedTriangle.normal);
        glm::vec3 reflectOrigin = intersection.intersectionPoint + reflectDir * 0.001f;
        return traceReflectiveRay(reflectOrigin, reflectDir, triangles, depth + 1, sourceLight, ambientLight);
    } else if(intersection.intersectedTriangle.isGlass){
        // this situation is very complex
        // if the reflection ray hits the glass, then it will be refracted
        // it will call the traceRefractiveRay function
        float indexOfRefraction = 1.6;
        glm::vec3 normal{};
        if (glm::dot(rayDirection, intersection.intersectedTriangle.normal)<0) {
            normal = -intersection.intersectedTriangle.normal;
        }else{
            normal = intersection.intersectedTriangle.normal;
        }
        glm::vec3 refractDir = calculate_refracted_ray(rayDirection, normal, indexOfRefraction);
        glm::vec3 refractOrigin = intersection.intersectionPoint + normal * 0.001f; // avoid self-intersection

        Colour refractColour = traceRefractiveRay(refractOrigin, refractDir, triangles, 1,sourceLight,ambientLight);
        return refractColour;
    }else{
        // this is the most common situation
        // if the ray hits a non-mirror surface, calculate the brightness
        if (intersection.distanceFromCamera == std::numeric_limits<float>::infinity()) {
            return Colour(0, 0, 0);
        }

        glm::vec3 shadowRay = glm::normalize(sourceLight - intersection.intersectionPoint);
        RayTriangleIntersection shadowIntersection = getClosestIntersection(intersection.intersectionPoint + shadowRay * 0.001f,
                                                                            shadowRay, triangles);

        // there are three different shading methods, you can choose any shading method
        // no difference for cornell box, default is flat shading
//                float combinedBrightness = phongShading(intersection,shadowIntersection, sourceLight, ambientLight);
//                float combinedBrightness = GouraudShading(intersection,shadowIntersection, sourceLight, ambientLight);
        float combinedBrightness = FlatShading(intersection,shadowIntersection, sourceLight, ambientLight);

        Colour colour = intersection.intersectedTriangle.colour;
        colour.red *= combinedBrightness;
        colour.green *= combinedBrightness;
        colour.blue *= combinedBrightness;

        return colour;
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


// This function is to return the color of the refracted ray
Colour traceRefractiveRay(const glm::vec3& refractOrigin,
                          const glm::vec3& refractDir, const std::vector<ModelTriangle>& triangles,
                          int depth, const glm::vec3 &sourceLight, float ambientLight) {
    // if the ray has been refracted more than 240 times, return black
    if (depth > 240) {
        return Colour(0, 0, 0);
    }

    // get the closest intersection
    RayTriangleIntersection closestIntersection = getClosestIntersection(refractOrigin, refractDir, triangles);

    // if the closest intersection is infinity, return the background color
    if (closestIntersection.distanceFromCamera == std::numeric_limits<float>::infinity()) {
        return Colour(0, 0, 0);
    }

    // Here is very tricky, I calculate the next intersection point in advance to determine whether the ray is inside the glass
    glm::vec3 NextRefractOrigin = closestIntersection.intersectionPoint + closestIntersection.intersectedTriangle.normal * 0.001f;
    RayTriangleIntersection NextClosestIntersection = getClosestIntersection(NextRefractOrigin,
                                                                             refractDir, triangles);
    bool isInside = NextClosestIntersection.intersectedTriangle.isGlass;
    // if the current intersection is the glass and the next intersection is not in the glass
    // then the ray is leaving the glass
    if (closestIntersection.intersectedTriangle.isGlass && !isInside) {
        // because the ray is leaving the glass, we need to calculate the new refractive index
        // which is the inverse of the current refractive index
        float newIndexOfRefraction = 1/1.3;
        glm::vec3 normal{};
        // here, we must ensure that the cos(theta) between the normal and the refract ray is positive
        if (glm::dot(refractDir, closestIntersection.intersectedTriangle.normal)<0) {
            normal = -closestIntersection.intersectedTriangle.normal;
        }else{
            normal = closestIntersection.intersectedTriangle.normal;
        }

        // use the glm::refract function to calculate the new refract ray
        glm::vec3 newRefractDir =calculate_refracted_ray(refractDir, normal, newIndexOfRefraction);
        glm::vec3 newRefractOrigin = closestIntersection.intersectionPoint + normal * 0.001f;

        // find the final intersection point
        RayTriangleIntersection FinalClosestIntersection = getClosestIntersection(newRefractOrigin,
                                                                                 newRefractDir, triangles);
        // if the final intersection is infinity, return the background color
        if (FinalClosestIntersection.distanceFromCamera == std::numeric_limits<float>::infinity()) {
            return Colour(0, 0, 0);
        }
        // if this final intersection is a mirror, then we need to call the traceReflectiveRay function
        // this is very tricky here.
        if (FinalClosestIntersection.intersectedTriangle.isMirror){
            glm::vec3 reflectDir = glm::reflect(newRefractDir, FinalClosestIntersection.intersectedTriangle.normal);
            glm::vec3 reflectOrigin = FinalClosestIntersection.intersectionPoint + reflectDir * 0.001f;
            Colour reflectColour = traceReflectiveRay(reflectOrigin, reflectDir, triangles, 1,sourceLight,ambientLight);
            return reflectColour;
        }
        // if the code reaches here, it means that the final intersection is just a normal surface
        glm::vec3 shadowRay = glm::normalize(sourceLight - FinalClosestIntersection.intersectionPoint);
        RayTriangleIntersection shadowIntersection = getClosestIntersection(FinalClosestIntersection.intersectionPoint + shadowRay * 0.001f,
                                                                            shadowRay, triangles);
        // there are three different shading methods, you can choose any shading method
        // no difference for cornell box, default is flat shading
//                float combinedBrightness = phongShading(intersection,shadowIntersection, sourceLight, ambientLight);
//                float combinedBrightness = GouraudShading(intersection,shadowIntersection, sourceLight, ambientLight);
        float combinedBrightness = FlatShading(FinalClosestIntersection,shadowIntersection, sourceLight, ambientLight);

        Colour colour = FinalClosestIntersection.intersectedTriangle.colour;
        colour.red *= combinedBrightness;
        colour.green *= combinedBrightness;
        colour.blue *= combinedBrightness;

        return colour;
    }else{
        // in this case, because the next intersection is still in the glass
        glm::vec3 newRefractOrigin = closestIntersection.intersectionPoint + closestIntersection.intersectedTriangle.normal * 0.001f;

        // so we need to recursively call the traceRefractiveRay function
        return traceRefractiveRay(newRefractOrigin, refractDir, triangles, depth + 1,sourceLight,ambientLight);
    }
}


void renderRayTracedScene(DrawingWindow &window, const std::string& filename, float focalLength,const std::string& materialFilename,
                          const int signalForShading) {
    // Load the triangles from the OBJ file.
    std::vector<ModelTriangle> triangles = loadOBJ(filename, 0.35,materialFilename);

    glm::vec3 ModelCenter = calculateModelCenter(triangles);
    float degree = 1.0f;
    float orbitRotationSpeed = degree * (M_PI / 180.0f);
    // Translate the camera
    cameraPosition = orbitCameraAroundY(cameraPosition, orbitRotationSpeed, ModelCenter);
    // Rotate the camera to look at the model center
    cameraOrientation = lookAt(ModelCenter);

    std::cout << "Loaded " << triangles.size() << " triangles for ray tracing" << std::endl;

// if the file is the sphere, we should put the light source in front of the sphere
    glm::vec3 sourceLight;
    if (filename=="../sphere.obj"){
        sourceLight = glm::vec3(0.4, 0.4, 1.5);
    }else{
        // this is the default light source position for the cornell box
        sourceLight = glm::vec3(0, 0.89, 0.1);
    }
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
                // if the intersection is a mirror, then we need to calculate the reflected ray
                if (intersection.intersectedTriangle.isMirror){
                    glm::vec3 reflectDir = glm::reflect(rayDirection, intersection.intersectedTriangle.normal);
                    glm::vec3 reflectOrigin = intersection.intersectionPoint + reflectDir * 0.001f;
                    // this is the recursive call
                    Colour reflectColour = traceReflectiveRay(reflectOrigin, reflectDir, triangles, 1,sourceLight,ambientLight);
                    uint32_t rgbColour = (255 << 24) |
                                         (int(reflectColour.red) << 16) |
                                         (int(reflectColour.green) << 8) |
                                         int(reflectColour.blue);
                    window.setPixelColour(x, y, rgbColour);

                }else if(intersection.intersectedTriangle.isGlass){
                    // if the intersection is a glass, then we need to calculate the refracted ray
                    float indexOfRefraction = 1.3; // the refractive index from air to glass
                    glm::vec3 normal{};
                    // here is very tricky, we must ensure that the cos(theta) between the normal and the refract ray is positive
                    if (glm::dot(rayDirection, intersection.intersectedTriangle.normal)<0) {
                        normal = -intersection.intersectedTriangle.normal;
                    }else{
                        normal = intersection.intersectedTriangle.normal;
                    }
                    glm::vec3 refractDir = calculate_refracted_ray(rayDirection, normal, indexOfRefraction);
                    glm::vec3 refractOrigin = intersection.intersectionPoint + normal * 0.001f;

                    Colour refractColour = traceRefractiveRay(refractOrigin, refractDir, triangles, 1,sourceLight,ambientLight);
                    uint32_t rgbColour = (255 << 24) |
                                         (int(refractColour.red) << 16) |
                                         (int(refractColour.green) << 8) |
                                         int(refractColour.blue);
                    window.setPixelColour(x, y, rgbColour);
                }else{
                    // if the intersection is not a mirror or a glass
                    // it means the intersection is just a normal surface

                    // here is very crucial, this condition is to say that if we look from outside the wall,
                    // then draw the color directly with the ambientLight, otherwise there will be some shadows
                    if (glm::dot(rayDirection, intersection.intersectedTriangle.normal)>0) {
                        Colour colour = intersection.intersectedTriangle.colour;
                        float brightness = ambientLight;
                        uint32_t rgbColour = (255 << 24) |
                                             (int(brightness*colour.red) << 16) |
                                             (int(brightness*colour.green) << 8) |
                                             int(brightness*colour.blue);
                        window.setPixelColour(x, y, rgbColour);
                    }else{
                        glm::vec3 shadowRay = glm::normalize(sourceLight - intersection.intersectionPoint);
                        RayTriangleIntersection shadowIntersection = getClosestIntersection(intersection.intersectionPoint + shadowRay * 0.001f,
                                                                                            shadowRay, triangles);

                        //there are three different shading methods, you can choose any shading method
                        float combinedBrightness;
                        if(signalForShading==1){
                            combinedBrightness = FlatShading(intersection,shadowIntersection, sourceLight, ambientLight);
                        }else if(signalForShading==2) {
                            combinedBrightness = GouraudShading(intersection, shadowIntersection, sourceLight,ambientLight);
                        }else if(signalForShading==3){
                            combinedBrightness = phongShading(intersection, shadowIntersection, sourceLight,ambientLight);
                        }else{
                            std::cout << "Please enter the correct signal for shading" << std::endl;
                            exit(1);
                        }
                        Colour colour = intersection.intersectedTriangle.colour;
                        uint32_t rgbColour = (255 << 24) |
                                             (int(combinedBrightness * colour.red) << 16) |
                                             (int(combinedBrightness * colour.green) << 8) |
                                             int(combinedBrightness * colour.blue);
                        window.setPixelColour(x, y, rgbColour);

                    }
                }
            } else {
                // No intersection found, set the pixel to the background color,
                window.setPixelColour(x, y, 0);
            }
        }
    }
}
