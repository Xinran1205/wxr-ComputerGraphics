#include "Interpolate.h"

// Single Element Numerical Interpolation
// return a list of floats that are interpolated between the two floats
std::vector<float> interpolateSingleFloats(float from,float to, int numberOfValues){
    int gap = numberOfValues - 1;
    float step = (to - from) / gap;
    std::vector<float> result;
    for (int i = 0; i < numberOfValues; i++){
        result.push_back(from + (step * i));
    }
    return result;
}

// Three Element Numerical Interpolation
//return a list of 3D vectors that are interpolated between the two 3D vectors
std::vector<glm::vec3> interpolateTripleFloats(glm::vec3 from, glm::vec3 to, int numberOfValues){
    int gap = numberOfValues - 1;
    float stepX = (to.x - from.x) / gap;
    float stepY = (to.y - from.y) / gap;
    float stepZ = (to.z - from.z) / gap;
    std::vector<glm::vec3> result;
    for (int i = 0; i < numberOfValues; i++){
        result.push_back(glm::vec3(from.x + (stepX * i), from.y + (stepY * i), from.z + (stepZ * i)));
    }
    return result;
}


// lab1 practice Single Dimenson Greyscale Interpolation
void drawTheGreyScale(DrawingWindow &window) {
    window.clearPixels();
    for (size_t y = 0; y < window.height; y++) {
        // Interpolate the grey values for this row.
        std::vector<float> greyValues = interpolateSingleFloats(255.0, 0.0, window.width);
        for (size_t x = 0; x < window.width; x++) {
            // Use the interpolated grey value for R, G, and B channels.
            float grey = greyValues[x];
            // use the Alpha channel to make the image transparent.
            uint32_t colour = (255 << 24) + (int(grey) << 16) + (int(grey) << 8) + int(grey);
            window.setPixelColour(x, y, colour);
        }
    }
}

//lab2 practice Two Dimensional Colour Interpolation
void draw(DrawingWindow &window) {
    window.clearPixels();
    glm::vec3 topLeft(255, 0, 0);        // red
    glm::vec3 topRight(0, 0, 255);       // blue
    glm::vec3 bottomRight(0, 255, 0);    // green
    glm::vec3 bottomLeft(255, 255, 0);   // yellow
    // leftSide is a list of 3D vectors that are interpolated between topLeft and bottomLeft
    // rightSide is a list of 3D vectors that are interpolated between topRight and bottomRight
    // They are 2 columns of colours that are interpolated from top to bottom
    std::vector<glm::vec3> leftSide = interpolateTripleFloats(topLeft, bottomLeft, window.height);
    std::vector<glm::vec3> rightSide = interpolateTripleFloats(topRight, bottomRight, window.height);
    for (size_t y = 0; y < window.height; y++) {
        // for each row of the image, interpolate the colours between the left and right side.
        std::vector<glm::vec3> row = interpolateTripleFloats(leftSide[y], rightSide[y], window.width);
        for (size_t x = 0; x < window.width; x++) {
            glm::vec3 colour = row[x];
            uint32_t colourInt = (255 << 24) + (int(colour.x) << 16) + (int(colour.y) << 8) + int(colour.z);
            window.setPixelColour(x, y, colourInt);
        }
    }
}

// drawLine by using Bresenham's line algorithm
void drawLineBresenham(DrawingWindow &window, CanvasPoint from, CanvasPoint to, Colour colour){
    // we use absolute value to make sure the slope is always positive
    int x = from.x;
    int y = from.y;
    int dx = abs(to.x - from.x);
    int dy = abs(to.y - from.y);

    // determine the step direction for x and y
    int DirectionX = 1;
    if (to.x < from.x) {
        DirectionX = -1;
    }

    int DirectionY = 1;
    if (to.y < from.y) {
        DirectionY = -1;
    }

    uint32_t pixelColour = (colour.red << 24) | (colour.green << 16) | (colour.blue << 8) | 0xFF; // Assuming colour contains red, green, and blue components

    // if slope is less than 1, we step in x direction, otherwise we step in y direction
    if (dx >= dy) {
        int accumulatorError = 0;
        // very important, (size_t)x<window.width && (size_t)y<window.height, avoid x or y out of bound
        for (int i = 0; i <= dx && (size_t)x<window.width && (size_t)y<window.height; i++) {
            window.setPixelColour(x, y, pixelColour);
            x += DirectionX;
            // error += dy/dx  , we need avoid using float, so we all multiply dx
            accumulatorError += dy;
            // if error > 1/2, y += 1
            if (accumulatorError > dx/2) {
                y += DirectionY;
                // error -= 1
                accumulatorError -= dx;
            }
        }
    } else {
        int accumulatorError = 0;
        for (int i = 0; i <= dy && (size_t)x<window.width && (size_t)y<window.height; i++) {
            window.setPixelColour(x, y, pixelColour);
            y += DirectionY;
            accumulatorError += dx;
            if (accumulatorError > dy / 2) {
                x += DirectionX;
                accumulatorError -= dy;
            }
        }
    }
}

////task2 draw line
//void drawLine(DrawingWindow &window){
//
//    drawLineBresenham(window, CanvasPoint(0, 0), CanvasPoint(window.width/2, window.height/2), Colour(255, 255, 255));
//    drawLineBresenham(window, CanvasPoint(window.width-1, 0), CanvasPoint(window.width/2, window.height/2), Colour(255, 255, 255));
//    drawLineBresenham(window, CanvasPoint(window.width/2, 0), CanvasPoint(window.width/2, window.height-1), Colour(255, 255, 255));
//    drawLineBresenham(window, CanvasPoint(window.width/3, window.height/2), CanvasPoint(2*(window.width/3), window.height/2), Colour(255, 255, 255));
//}
