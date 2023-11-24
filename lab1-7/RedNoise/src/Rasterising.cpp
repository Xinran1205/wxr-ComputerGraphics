//
// Created by dell on 2023/11/6.
//

#include "Rasterising.h"

std::vector<std::vector<float>> initialiseDepthBuffer(int width, int height) {
    std::vector<std::vector<float>> depthBuffer;
    for (int y = 0; y < height; y++) {
        std::vector<float> row;
        for (int x = 0; x < width; x++) {
            row.push_back(0);
        }
        depthBuffer.push_back(row);
    }
    return depthBuffer;
}

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

//middle in Axis-Aligned Bounding Box (AABB)
//这个函数可以不需要，因为我模型本来就在000
glm::vec3 calculateModelCenter(const std::vector<ModelTriangle>& triangles) {
    glm::vec3 minCoords = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 maxCoords = glm::vec3(std::numeric_limits<float>::lowest());

    for (const auto& triangle : triangles) {
        for (const auto& vertex : triangle.vertices) {
            minCoords.x = std::min(minCoords.x, vertex.x);
            minCoords.y = std::min(minCoords.y, vertex.y);
            minCoords.z = std::min(minCoords.z, vertex.z);

            maxCoords.x = std::max(maxCoords.x, vertex.x);
            maxCoords.y = std::max(maxCoords.y, vertex.y);
            maxCoords.z = std::max(maxCoords.z, vertex.z);
        }
    }

    return (minCoords + maxCoords) / 2.0f;
}

glm::mat3 lookAt(glm::vec3 target) {
    glm::vec3 forward = glm::normalize(cameraPosition - target);
    glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), forward));
    glm::vec3 up = glm::cross(-forward, right);

    return glm::mat3(right, up, -forward);
}



void drawFilledTriangle (DrawingWindow &window, CanvasTriangle triangle, Colour colour) {
    std :: cout << "drawFilledTriangle is called" << std::endl;
    // print out the triangle
    std :: cout << triangle << std::endl;

    CanvasPoint bottom = triangle[0];
    CanvasPoint middle = triangle[1];
    CanvasPoint top = triangle[2];

    // sort the points by y value
    if (bottom.y > middle.y) {std::swap(bottom, middle);}
    if (bottom.y > top.y) {std::swap(bottom, top);}
    if (middle.y > top.y) {std::swap(middle, top);}
    // now bottom.y <= middle.y <= top.y

    std :: vector<CanvasPoint> pointsBottomToTop = interpolateCanvasPoint(bottom, top, int(top.y) - int(bottom.y) + 1);

    CanvasPoint extraPoint = pointsBottomToTop[int(middle.y) - int(bottom.y)];

    // call the function to draw the triangle, give it 3 new points
    drawPartTriangle(window, CanvasTriangle(middle, extraPoint, bottom), colour);
    drawPartTriangle(window, CanvasTriangle(middle, extraPoint, top), colour);
}

void drawPartTriangle (DrawingWindow &window, CanvasTriangle triangle, Colour colour){

    CanvasPoint MiddlePoint = triangle[0];
    CanvasPoint ExtraPoint = triangle[1];
    CanvasPoint TopOrBottom = triangle[2];

    uint32_t packedColour = (255 << 24) | (colour.red << 16) | (colour.green << 8) | colour.blue;

    int yStart = MiddlePoint.y;
    int yEnd = TopOrBottom.y;
    // check the Peak is the top or the bottom point
    // we always set the yStart to be the smaller one
    if (MiddlePoint.y > TopOrBottom.y){
       std :: swap (yStart, yEnd);
    }

    // Peak is the top or the bottom point
    std :: vector<CanvasPoint> pointsBetweenMiddleAndPeak;
    std :: vector<CanvasPoint> pointsBetweenExtraAndPeak;

    if (yStart == int(MiddlePoint.y)){
        // this means we need draw the bottom half triangle
        pointsBetweenMiddleAndPeak = interpolateCanvasPoint(MiddlePoint, TopOrBottom, yEnd - yStart + 1);
        pointsBetweenExtraAndPeak = interpolateCanvasPoint(ExtraPoint, TopOrBottom, yEnd - yStart + 1);
    } else {
        // this means we need draw the top half triangle,
        // so we need change the order of the points to make sure the "from" value is smaller than the "to" value
        pointsBetweenMiddleAndPeak = interpolateCanvasPoint(TopOrBottom, MiddlePoint, yEnd - yStart + 1);
        pointsBetweenExtraAndPeak = interpolateCanvasPoint(TopOrBottom, ExtraPoint, yEnd - yStart + 1);
    }

    //Always Draw the horizontal line from the bottomY to the middle
    // also from the Horizontal line from the points between middle and peak to the points between extra and peak
    for (int i = yStart; i < yEnd; i++) {
        int y = i;
        if (pointsBetweenMiddleAndPeak[i - yStart].x > pointsBetweenExtraAndPeak[i - yStart].x) {
            std :: swap(pointsBetweenMiddleAndPeak[i - yStart], pointsBetweenExtraAndPeak[i - yStart]);
        }

        std::vector<float> XlineDepth =interpolateSingleFloats(pointsBetweenMiddleAndPeak[i - yStart].depth,
                                                                pointsBetweenExtraAndPeak[i - yStart].depth,
                                                                pointsBetweenExtraAndPeak[i - yStart].x -
                                                                pointsBetweenMiddleAndPeak[i - yStart].x + 1);

        int x_start = pointsBetweenMiddleAndPeak[i - yStart].x;
        int x_end = pointsBetweenExtraAndPeak[i - yStart].x;
        // Draw horizontal line from x_start to x_end
        for (int x = x_start; x <= x_end; x++) {
            float CurrentPointDepth = 1.0f/XlineDepth[x - x_start];
            // Z buffer is closer to us if the value is smaller
            if (x >= 0 && (size_t)x < window.width &&
                (size_t)y >= 0 && (size_t)y < window.height && CurrentPointDepth > zBuffer[y][x]) {
                window.setPixelColour(x, y, packedColour);
                zBuffer[y][x] = CurrentPointDepth;
            }
        }
    }
}


void renderPointCloud(DrawingWindow &window, const std::string& filename, float focalLength) {
    // Load the triangles from the OBJ file.
    std::vector<ModelTriangle> triangles = loadOBJ(filename, 0.35);
    glm::vec3 ModelCenter = calculateModelCenter(triangles);
    float degree = 1.0f;
    float orbitRotationSpeed = degree * (M_PI / 180.0f);
    //translate, this is just move the camera
    cameraPosition = orbitCameraAroundY(cameraPosition, orbitRotationSpeed, ModelCenter);
    //rotate, this will rotate the camera and let it look at the center of the model
    cameraOrientation = lookAt(ModelCenter);

    std::cout << "Loaded " << triangles.size() << " triangles" << std::endl;

    for (const auto& triangle : triangles) {
        CanvasPoint projectedPoints[3];
        for (int i = 0; i < 3; i++) {
            projectedPoints[i] = getCanvasIntersectionPoint(cameraPosition, triangle.vertices[i], focalLength);
//          window.setPixelColour(projectedPoints[i].x, projectedPoints[i].y, (255 << 24) | (255 << 16) | (255 << 8) | 255);
        }
//        drawTriangle(window, CanvasTriangle(projectedPoints[0], projectedPoints[1], projectedPoints[2]),
//                     Colour(triangle.colour.red, triangle.colour.green, triangle.colour.blue));
        drawFilledTriangle(window, CanvasTriangle(projectedPoints[0], projectedPoints[1], projectedPoints[2]),
                           Colour(triangle.colour.red, triangle.colour.green, triangle.colour.blue));
    }
}

void renderTexPointCloud(DrawingWindow &window, const std::string& filename, float focalLength) {
    TextureMap textureMap("../textureCoenell/texture.ppm");
    std::vector<ModelTriangle> triangles = loadTexOBJ(filename, 0.35);
    glm::vec3 ModelCenter = calculateModelCenter(triangles);
    float degree = 1.0f;
    float orbitRotationSpeed = degree * (M_PI / 180.0f);
    //translate, this is just move the camera
    cameraPosition = orbitCameraAroundY(cameraPosition, orbitRotationSpeed, ModelCenter);
    //rotate, this will rotate the camera and let it look at the center of the model
    cameraOrientation = lookAt(ModelCenter);

    std::cout << "Loaded " << triangles.size() << " triangles" << std::endl;

    for (const auto& triangle : triangles) {
        CanvasPoint projectedPoints[3];
        for (int i = 0; i < 3; i++) {
            projectedPoints[i] = getCanvasIntersectionPoint(cameraPosition, triangle.vertices[i], focalLength);
            //这里可以直接这么赋值
            //需要校验，如果不存纹理坐标，就还是0，如果有纹理坐标需要扩大大小
            if(triangle.texturePoints[i].x != 0 && triangle.texturePoints[i].y != 0){
                projectedPoints[i].texturePoint.x = triangle.texturePoints[i].x*150+WIDTH / 2.0f;
                projectedPoints[i].texturePoint.y = triangle.texturePoints[i].y*150+HEIGHT / 2.0f;
            }else{
                projectedPoints[i].texturePoint.x = 0;
                projectedPoints[i].texturePoint.y = 0;
            }
        }
        drawTextureTriangle(window, CanvasTriangle(projectedPoints[0], projectedPoints[1], projectedPoints[2]),
                            Colour(triangle.colour.red, triangle.colour.green, triangle.colour.blue), textureMap);
    }
}


CanvasPoint getCanvasIntersectionPoint(glm::vec3 cameraPosition, glm::vec3 vertexPosition, float focalLength) {

    CanvasPoint canvasPoint;
    glm::vec3 relativePosition = vertexPosition - cameraPosition;

    glm::vec3 vertexPositionNew = relativePosition*cameraOrientation;

    // Compute the projection using the formulas
    float canvasX = focalLength * (vertexPositionNew[0] / vertexPositionNew[2]);
    canvasX *= 150;
    // move the origin to the center of the screen
    canvasX = canvasX + WIDTH / 2.0f;
    float canvasY = focalLength * (vertexPositionNew[1] / vertexPositionNew[2]);
    canvasY *= 150;
    canvasY = canvasY + HEIGHT / 2.0f;

    canvasPoint.x = canvasX;
    canvasPoint.y = canvasY;
    canvasPoint.depth = vertexPositionNew[2];
    return canvasPoint;
}




// The previous version of drawFilledTriangle

//void drawFilledTriangle (DrawingWindow &window, CanvasTriangle triangle, Colour colour) {
//    std :: cout << "drawFilledTriangle is called" << std::endl;
//    // print out the triangle
//    std :: cout << triangle << std::endl;
//
//    CanvasPoint bottom = triangle[0];
//    CanvasPoint middle = triangle[1];
//    CanvasPoint top = triangle[2];
//
//    uint32_t packedColour = (255 << 24) | (colour.red << 16) | (colour.green << 8) | colour.blue;
//
//    // sort the points by y value
//    if (bottom.y > middle.y) {
//        std::swap(bottom, middle);
//    }
//    if (bottom.y > top.y) {
//        std::swap(bottom, top);
//    }
//    if (middle.y > top.y) {
//        std::swap(middle, top);
//    }
//    // now bottom.y <= middle.y <= top.y
//
//    // interpolate the depth values
//    std::vector<float> depthValuesBottomToTop = interpolateSingleFloats(bottom.depth, top.depth, int(top.y) - int(bottom.y) + 1);
//    std::vector<float> depthValuesBottomToMiddle = interpolateSingleFloats(bottom.depth, middle.depth, int(middle.y) - int(bottom.y) + 1);
//    std::vector<float> depthValuesMiddleToTop = interpolateSingleFloats(middle.depth, top.depth, int(top.y) - int(middle.y) + 1);
//
//    // Calculate lines
//    // This value is the list of the x value of the start/end line between bottom and top
//    std::vector<float> xValuesBottomToTop = interpolateSingleFloats(bottom.x, top.x, int(top.y) - int(bottom.y) + 1);
//    std::vector<float> xValuesBottomToMiddle = interpolateSingleFloats(bottom.x, middle.x, int(middle.y) - int(bottom.y) + 1);
//    std::vector<float> xValuesMiddleToTop = interpolateSingleFloats(middle.x, top.x, int(top.y) - int(middle.y) + 1);
//
//    if (int(middle.y) != int(top.y)) {
//        // Rasterize top half triangle (from middle to top)
//        for (int i = 0; i < int(top.y - middle.y+1); i++) {
//            int y = middle.y + i;
//            int x_start = std::round(xValuesMiddleToTop[i]);
//            int x_end = std::round(xValuesBottomToTop[middle.y - bottom.y + i]);
//            if (x_start > x_end) std::swap(x_start, x_end); // ensure x_start <= x_end
//
//            float depthStart = depthValuesMiddleToTop[i];
//            float depthEnd = depthValuesBottomToTop[middle.y - bottom.y + i];
//            std::vector<float> XlineDepth = interpolateSingleFloats(depthStart, depthEnd, x_end - x_start + 1);
//
//            // Draw horizontal line from x_start to x_end
//            for (int x = x_start; x <= x_end; x++) {
//                float CurrentPointDepth = 1.0f/XlineDepth[x - x_start];
//                // Z buffer is closer to us if the value is smaller
//                if (x >= 0 && (size_t)x < window.width &&
//                    (size_t)y >= 0 && (size_t)y < window.height && CurrentPointDepth > zBuffer[y][x]) {
//                    window.setPixelColour(x, y, packedColour);
//                    zBuffer[y][x] = CurrentPointDepth;
//                }
//            }
//        }
//    }
//
//    if (int(middle.y) == int(bottom.y)) {
//        return;
//    }
//    // Rasterize bottom half triangle (from bottom to middle)
//    for (int i = 0; i < int(middle.y - bottom.y + 1); i++) {
//        int y = bottom.y + i;
//        int x_start = std::round(xValuesBottomToMiddle[i]);
//        int x_end = std::round(xValuesBottomToTop[i]);
//        if (x_start > x_end) std::swap(x_start, x_end); // ensure x_start <= x_end
//
//        float depthStart = depthValuesBottomToMiddle[i];
//        float depthEnd = depthValuesBottomToTop[i];
//        std::vector<float> XlineDepth = interpolateSingleFloats(depthStart, depthEnd, x_end - x_start + 1);
//
//        // Draw horizontal line from x_start to x_end
//        for (int x = x_start; x <= x_end; x++) {
//            float CurrentPointDepth = 1.0f/XlineDepth[x - x_start];
//            if (x >= 0 && (size_t)x < window.width &&
//                (size_t)y >= 0 && (size_t)y < window.height && CurrentPointDepth > zBuffer[y][x]) {
//                window.setPixelColour(x, y, packedColour);
//                zBuffer[y][x] = CurrentPointDepth;
//            }
//        }
//    }
//}
