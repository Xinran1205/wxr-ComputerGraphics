#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include "glm/glm.hpp"
#include "CanvasPoint.h"
#include <Colour.h>
#include <CanvasTriangle.h>

#define WIDTH 320
#define HEIGHT 240

std::vector<float> interpolateSingleFloats(float from,float to, int numberOfValues);
std::vector<glm::vec3> interpolateTripleFloats(glm::vec3 from, glm::vec3 to, int numberOfValues);
void drawLineBresenham(DrawingWindow &window, CanvasPoint from, CanvasPoint to, Colour colour);

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
        for (int i = 0; i <= dx; i++) {
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
        for (int i = 0; i <= dy; i++) {
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

// drawLine by using interpolation
void drawLineInterpolation(DrawingWindow &window, CanvasPoint from, CanvasPoint to, Colour colour){

    if (from.x > to.x || (from.x == to.x && from.y > to.y)) {
        std::swap(from, to);
    }

    int dx = to.x - from.x;
    int dy = to.y - from.y;

    uint32_t packedColour = (255 << 24) | (colour.red << 16) | (colour.green << 8) | colour.blue;

    // Decide if we should step in x direction or y direction
    if (std::abs(dx) > std::abs(dy)) {
        float y = from.y;
        // this slope is just interpolation, for example, we have dx = 100, dy = 50, separate dy into 100 parts,
        // slope is actually the step
        // for loop is 100 times, and each time y += 0.5
        float slope = (float)dy / (float)dx;
        int stepX = (dx > 0) ? 1 : -1;
        for (int x = from.x; x != to.x; x += stepX) {
            window.setPixelColour(x, round(y), packedColour);
            y += slope;
        }
    } else {
        float x = from.x;
        float slope = (float)dx / (float)dy;
        int stepY = (dy > 0) ? 1 : -1;
        for (int y = from.y; y != to.y; y += stepY) {
            window.setPixelColour(round(x), y, packedColour);
            x += slope;
        }
    }
}

void drawTriangle(DrawingWindow &window, CanvasTriangle triangle, Colour colour) {
    drawLineBresenham(window, triangle[0], triangle[1], colour);
    drawLineBresenham(window, triangle[1], triangle[2], colour);
    drawLineBresenham(window, triangle[2], triangle[0], colour);
}





// Task 2: Single Element Numerical Interpolation
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

//Task 3: Single Dimension Greyscale Interpolation
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

// Task 4: Three Element Numerical Interpolation
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

//Task 5: Two Dimensional Colour Interpolation
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

void handleEvent(SDL_Event event, DrawingWindow &window) {
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_LEFT) {
            std::cout << "LEFT" << std::endl;
        }else if (event.key.keysym.sym == SDLK_RIGHT) {
            std::cout << "RIGHT" << std::endl;
        }else if (event.key.keysym.sym == SDLK_UP) {
            std::cout << "UP" << std::endl;
        }else if(event.key.keysym.sym == SDLK_DOWN) {
            std::cout << "DOWN" << std::endl;
        } else if (event.key.keysym.sym == SDLK_1) {
            std::cout << "1 pressed, I dont know how to set this to u" << std::endl;
            CanvasPoint p1(rand() % window.width, rand() % window.height);
            CanvasPoint p2(rand() % window.width, rand() % window.height);
            CanvasPoint p3(rand() % window.width, rand() % window.height);
            CanvasTriangle randomTriangle(p1, p2, p3);
            drawTriangle(window, randomTriangle, Colour(rand()%255, rand()%255, rand()%255));
        }
	} else if (event.type == SDL_MOUSEBUTTONDOWN) {
		window.savePPM("output.ppm");
		window.saveBMP("output.bmp");
	}
}

//task2 draw line
void drawLine(DrawingWindow &window){

    drawLineBresenham(window, CanvasPoint(0, 0), CanvasPoint(window.width/2, window.height/2), Colour(255, 255, 255));
    drawLineBresenham(window, CanvasPoint(window.width-1, 0), CanvasPoint(window.width/2, window.height/2), Colour(255, 255, 255));
    drawLineBresenham(window, CanvasPoint(window.width/2, 0), CanvasPoint(window.width/2, window.height-1), Colour(255, 255, 255));
    drawLineBresenham(window, CanvasPoint(window.width/3, window.height/2), CanvasPoint(2*(window.width/3), window.height/2), Colour(255, 255, 255));
}

int main(int argc, char *argv[]) {
//    // test interpolateSingleFloats
//    std::vector<float> result;
//    result = interpolateSingleFloats(2.2, 8.5, 7);
//    for(size_t i=0; i<result.size(); i++) std::cout << result[i] << " ";
//    std::cout << std::endl;

    // test interpolateTripleFloats
//    std::vector<glm::vec3> result2;
//    result2 = interpolateTripleFloats(glm::vec3(1.0, 4.0, 9.2), glm::vec3(4.0, 1.0, 9.8), 4);
//    for(size_t i=0; i<result2.size(); i++) std::cout << result2[i].x << " " << result2[i].y << " " << result2[i].z << std::endl;
//    std::cout << std::endl;

	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;
	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window);
        drawLine(window);
//		draw(window);
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}
