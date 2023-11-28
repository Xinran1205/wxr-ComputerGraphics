#include "SoftShadowRendering.h"


// for each intersection, it has multiple shadowIntersections with multiple light points
float FlatShadingSoft(RayTriangleIntersection intersection, const std::vector<RayTriangleIntersection> &shadowIntersections,
                      const std::vector<glm::vec3> &lightPoints, float ambientLight) {

    float totalBrightness = 0.0f;
    // go through all the shadowIntersections and add up all the brightness
    // at the end, we will get the average brightness, this is the brightness of the intersection
    for (size_t i = 0; i < lightPoints.size(); i++) {
        // Calculate brightness and specular intensity for each light point
        float brightness = calculateLighting(intersection.intersectionPoint,
                                             intersection.intersectedTriangle.normal, lightPoints[i]);
        float specularIntensity = calculateSpecularLighting(intersection.intersectionPoint, cameraPosition,
                                                            lightPoints[i], intersection.intersectedTriangle.normal, shininess);

        // Determine if the point is in shadow for the current light point
        if (shadowIntersections[i].distanceFromCamera < glm::length(lightPoints[i] - intersection.intersectionPoint) &&
            shadowIntersections[i].triangleIndex != intersection.triangleIndex) {
            // In shadow, only ambient light contributes
            brightness = ambientLight;
        } else {
            // Not in shadow, add ambient light
            brightness = glm::max(brightness + ambientLight, ambientLight);
        }

        // Accumulate total brightness and specular intensity
        totalBrightness += brightness + specularIntensity; // Add specular to each light's contribution
    }

    // Average the brightness from all light sources
    float averageBrightness = glm::clamp(totalBrightness / lightPoints.size(), 0.0f, 1.0f);
    return averageBrightness;
}

float GouraudShadingSoft(RayTriangleIntersection intersection, const std::vector<RayTriangleIntersection> &shadowIntersections,
                         const std::vector<glm::vec3> &lightPoints, float ambientLight){
    for (int i = 0; i < 3; i++) {
        glm::vec3 vertex = intersection.intersectedTriangle.vertices[i];
        // if this vertex has been calculated before, skip it
        if (vertexBrightnessGlobal.find(vertex) != vertexBrightnessGlobal.end()){
            continue;
        }

        glm::vec3 normal = vertexNormals[vertex];
        // calculate the diffuse lighting and specular lighting
        float totalBrightness = 0.0f;

        // for every light point, calculate the diffuse lighting and specular lighting
        for (size_t j = 0; j < lightPoints.size(); j++) {
            float brightness = calculateLighting(vertex, normal, lightPoints[j]);
            float specularIntensity = calculateSpecularLighting(vertex, cameraPosition, lightPoints[j], normal, shininess);

            if (shadowIntersections[j].distanceFromCamera < glm::length(lightPoints[j] - vertex) &&
                shadowIntersections[j].triangleIndex != intersection.triangleIndex) {
                brightness = ambientLight;
            } else {
                brightness += ambientLight;
            }

            totalBrightness += brightness + specularIntensity;
        }

        // combine the brightness from all light sources
        float averageBrightness = glm::clamp(totalBrightness / lightPoints.size(), 0.0f, 1.0f);

        vertexBrightnessGlobal[vertex] = averageBrightness;
    }

    glm::vec3 barycentricCoords = calculateBarycentricCoordinates(intersection.intersectionPoint,
                                                                  intersection.intersectedTriangle.vertices);

    // interpolate the brightness
    float ResultVertexBrightness =
            barycentricCoords.x * vertexBrightnessGlobal[intersection.intersectedTriangle.vertices[0]] +
            barycentricCoords.y * vertexBrightnessGlobal[intersection.intersectedTriangle.vertices[1]] +
            barycentricCoords.z * vertexBrightnessGlobal[intersection.intersectedTriangle.vertices[2]];
    return ResultVertexBrightness;
}

float phongShadingSoft(RayTriangleIntersection intersection, const std::vector<RayTriangleIntersection> &shadowIntersections,
                       const std::vector<glm::vec3> &lightPoints, float ambientLight) {
    glm::vec3 normal0  = vertexNormals[intersection.intersectedTriangle.vertices[0]];
    glm::vec3 normal1  = vertexNormals[intersection.intersectedTriangle.vertices[1]];
    glm::vec3 normal2  = vertexNormals[intersection.intersectedTriangle.vertices[2]];
    glm::vec3 barycentricCoords = calculateBarycentricCoordinates(intersection.intersectionPoint, intersection.intersectedTriangle.vertices);

    // interpolate the normal
    glm::vec3 interpolatedNormal =
            barycentricCoords.x * normal0 +
            barycentricCoords.y * normal1 +
            barycentricCoords.z * normal2;

    interpolatedNormal = glm::normalize(interpolatedNormal);

    float totalBrightness = 0.0f;
    for (size_t i = 0; i < lightPoints.size(); i++) {
        float brightness = calculateLighting(intersection.intersectionPoint, interpolatedNormal, lightPoints[i]);
        float specularIntensity = calculateSpecularLighting(intersection.intersectionPoint, cameraPosition, lightPoints[i],
                                                            interpolatedNormal, shininess);

        if (shadowIntersections[i].distanceFromCamera < glm::length(lightPoints[i] - intersection.intersectionPoint) &&
            shadowIntersections[i].triangleIndex != intersection.triangleIndex) {
            brightness = ambientLight; // In shadow, only ambient light contributes
        } else {
            brightness = glm::max(brightness + ambientLight, ambientLight); // Not in shadow, add ambient light
        }

        totalBrightness += brightness + specularIntensity; // Add specular to each light's contribution
    }

    // Average the brightness from all light sources
    float averageBrightness = glm::clamp(totalBrightness / lightPoints.size(), 0.0f, 1.0f);
    return averageBrightness;
}

void renderRayTracedSceneSoftShadow(DrawingWindow &window, const std::string& filename, float focalLength,
                                    const std::string& materialFilename,const int signalForShading) {
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

    // Define multi-point light source
    std::vector<glm::vec3> lightPoints = {{0.01, 0.89, -0.2},{0.02,0.89,0},{0.03,0.89,0.1},
                                          {0.04,0.89,0.2},{0.05,0.89,0.3},{0.06,0.89,-0.1}};
    float ambientLight = 0.3f;  // ambient light intensity

    // Loop over each pixel on the image plane
    for (int y = 0; y < int(window.height); y++) {
        for (int x = 0; x < int(window.width); x++) {
            // Compute the ray direction for this pixel
            glm::vec3 rayDirection = computeRayDirection(window.width, window.height, x, y, focalLength, cameraOrientation);

            // Find the closest intersection of this ray with the scene
            RayTriangleIntersection intersection = getClosestIntersection(cameraPosition, rayDirection, triangles);

            // If an intersection was found, color the pixel accordingly
            if (intersection.distanceFromCamera != std::numeric_limits<float>::infinity()) {
                // here is the key point, this judgement is to say that if we look from outside the wall,
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
                    // Initialize combined brightness
                    std::vector<RayTriangleIntersection> AllshadowIntersection;
                    // Iterate over each point light to calculate soft shadows
                    // combined all shadowIntersections in a list
                    for (const auto& lightPoint : lightPoints) {
                        glm::vec3 shadowRay = glm::normalize(lightPoint - intersection.intersectionPoint);
                        RayTriangleIntersection shadowIntersection = getClosestIntersection(intersection.intersectionPoint + shadowRay * 0.002f, shadowRay, triangles);
                        AllshadowIntersection.push_back(shadowIntersection);
                    }

                    float combinedBrightness;
                    if (signalForShading == 1){
                        combinedBrightness = FlatShadingSoft(intersection, AllshadowIntersection, lightPoints, ambientLight);
                    } else if (signalForShading == 2){
                        combinedBrightness = GouraudShadingSoft(intersection, AllshadowIntersection, lightPoints, ambientLight);
                    } else if (signalForShading == 3){
                        combinedBrightness = phongShadingSoft(intersection, AllshadowIntersection, lightPoints, ambientLight);
                    } else {
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
            } else {
                // No intersection found, set the pixel to the background color
                window.setPixelColour(x, y, 0);
            }
        }
    }
}