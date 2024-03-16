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

//middle in Axis-Aligned Bounding Box (AABB)
//this function is calculate the middle point of the model
//mostly model center is in 000 but sometimes it is not
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

void renderPointCloud(DrawingWindow &window, const std::string& filename, float focalLength, TextureMap &textureMap,const std::string& materialFilename) {
    std::vector<ModelTriangle> triangles = loadOBJ(filename, 0.35,materialFilename);
    glm::vec3 ModelCenter = calculateModelCenter(triangles);
    float degree = 1.0f;
    float orbitRotationSpeed = degree * (M_PI / 180.0f);
//    translate, this is just move the camera
    cameraPosition = orbitCameraAroundY(cameraPosition, orbitRotationSpeed, ModelCenter);
//    rotate, this will rotate the camera and let it look at the center of the model
    cameraOrientation = lookAt(ModelCenter);

    std::cout << "Loaded " << triangles.size() << " triangles" << std::endl;

    for (const auto& triangle : triangles) {
        CanvasPoint projectedPoints[3];
        for (int i = 0; i < 3; i++) {
            projectedPoints[i] = getCanvasIntersectionPoint(cameraPosition, triangle.vertices[i], focalLength);

            // here is to calculate the texture point coordinate by remapping the range of u and v to [0, width] and [0, height]
            // if the texture point is not 0,0, then we need to expand the texture point coordinate
            if(triangle.texturePoints[i].x != 0 && triangle.texturePoints[i].y != 0){
                projectedPoints[i].texturePoint.x = triangle.texturePoints[i].x*150+WIDTH / 2.0f;
                projectedPoints[i].texturePoint.y = triangle.texturePoints[i].y*150+HEIGHT / 2.0f;
            }else{
                // if the point do not have texture point, then we set the texture point to be 0,0
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

void DrawWireframe(DrawingWindow &window, const std::string& filename, float focalLength,const std::string& materialFilename) {
    std::vector<ModelTriangle> triangles = loadOBJ(filename, 0.35,materialFilename);
    glm::vec3 ModelCenter = calculateModelCenter(triangles);
    float degree = 1.0f;
    float orbitRotationSpeed = degree * (M_PI / 180.0f);
    //translate, this is just move the camera
    cameraPosition = orbitCameraAroundY(cameraPosition, orbitRotationSpeed, ModelCenter);
//    rotate, this will rotate the camera and let it look at the center of the model
    cameraOrientation = lookAt(ModelCenter);

    std::cout << "Loaded " << triangles.size() << " triangles" << std::endl;

    for (const auto& triangle : triangles) {
        CanvasPoint projectedPoints[3];
        for (int i = 0; i < 3; i++) {
            projectedPoints[i] = getCanvasIntersectionPoint(cameraPosition, triangle.vertices[i], focalLength);
        }
        drawTriangle(window, CanvasTriangle(projectedPoints[0], projectedPoints[1], projectedPoints[2]),
                     Colour(triangle.colour.red, triangle.colour.green, triangle.colour.blue));
    }
}


// The functions below are deprecated


//This function is Deprecated, substitute by drawTextureTriangle
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

// This function is Deprecated, substitute by drawTexturePartTriangle
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
