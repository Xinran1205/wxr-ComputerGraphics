#include "FilledTriangleByUsingBoundingBox.h"

// return a list of 2D vectors that are the bounding box of the triangle
std::vector<glm::vec2> findBoundingBox(CanvasTriangle triangle) {
    std::vector<glm::vec2> result;
    // just randomly initialize the value
    int minX = triangle[0].x;
    int maxX = triangle[0].x;
    int minY = triangle[0].y;
    int maxY = triangle[0].y;
    for (int i = 1; i < 3; i++) {
        if (triangle[i].x < minX) {
            minX = triangle[i].x;
        }
        if (triangle[i].x > maxX) {
            maxX = triangle[i].x;
        }
        if (triangle[i].y < minY) {
            minY = triangle[i].y;
        }
        if (triangle[i].y > maxY) {
            maxY = triangle[i].y;
        }
    }
    result.push_back(glm::vec2(minX, minY));
    result.push_back(glm::vec2(maxX, maxY));
    return result;
}

glm::vec2 calculateTheLine(CanvasPoint p1, CanvasPoint p2) {
    glm::vec2 result;
    result.x = p2.x - p1.x;
    result.y = p2.y - p1.y;
    return result;
}

// return true if the point is inside the triangle
bool isInTheTriangle(CanvasTriangle triangle, CanvasPoint p) {
    //line V01
    glm::vec2 line1 = calculateTheLine(triangle[0], triangle[1]);
    //line V12
    glm::vec2 line2 = calculateTheLine(triangle[1], triangle[2]);
    //line V20
    glm::vec2 line3 = calculateTheLine(triangle[2], triangle[0]);

    // use cross product to determine if the point is inside the triangle

    // Line Vp0
    glm::vec2 vp0 = calculateTheLine(triangle[0], p);
    // Line Vp1
    glm::vec2 vp1 = calculateTheLine(triangle[1], p);
    // Line Vp2
    glm::vec2 vp2 = calculateTheLine(triangle[2], p);

    float c0 = line1.x * vp0.y - line1.y * vp0.x;
    float c1 = line2.x * vp1.y - line2.y * vp1.x;
    float c2 = line3.x * vp2.y - line3.y * vp2.x;

    if ((c0 >= 0 && c1 >= 0 && c2 >= 0) || (c0 <= 0 && c1 <= 0 && c2 <= 0)) {
        return true;
    }

    return false;
}

void drawFilledTriangleUsingBoundingBox (DrawingWindow &window, CanvasTriangle triangle, Colour colour) {
    std::vector<glm::vec2> boundingBox = findBoundingBox(triangle);
    int minX = boundingBox[0].x;
    int maxX = boundingBox[1].x;
    int minY = boundingBox[0].y;
    int maxY = boundingBox[1].y;

    uint32_t pixelColour = (colour.red << 24) | (colour.green << 16) | (colour.blue << 8) | 0xFF;
    // for each pixel in the bounding box, check if it is inside the triangle
    for (int x = minX; x <= maxX; x++) {
        for (int y = minY; y <= maxY; y++) {
            CanvasPoint p(x, y);
            if (isInTheTriangle(triangle, p)) {
                window.setPixelColour(x, y, pixelColour);
            }
        }
    }
}