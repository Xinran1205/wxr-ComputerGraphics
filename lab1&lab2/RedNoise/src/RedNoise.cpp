#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include "glm/glm.hpp"
#include "CanvasPoint.h"
#include <Colour.h>
#include <TextureMap.h>
#include <ModelTriangle.h>
#include <fstream>
#include<iostream>
#include <map>


#define WIDTH 320
#define HEIGHT 240

std::vector<std::vector<float>> zBuffer;

std::vector<float> interpolateSingleFloats(float from,float to, int numberOfValues);
std::vector<glm::vec3> interpolateTripleFloats(glm::vec3 from, glm::vec3 to, int numberOfValues);
void drawLineBresenham(DrawingWindow &window, CanvasPoint from, CanvasPoint to, Colour colour);
std::vector<TexturePoint> interpolateTexturePoints(TexturePoint start, TexturePoint end, int numValues);
void renderPointCloud(DrawingWindow &window, const std::string& filename, glm::vec3 cameraPosition, float focalLength);

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

// drawLine by using interpolation
void drawLineInterpolation(DrawingWindow &window, CanvasPoint from, CanvasPoint to, Colour colour){

    int dx = to.x - from.x;
    int dy = to.y - from.y;

    uint32_t packedColour = (255 << 24) | (colour.red << 16) | (colour.green << 8) | colour.blue;

    // Decide if we should step in x direction or y direction
    if (std::abs(dx) > std::abs(dy)) {
        float y = from.y;
        float slope = (float)dy / (float)dx;
        // this is very important to check the quadrant of the line
        if (dx<0){
            slope = -slope;
        }
        int stepX = (dx > 0) ? 1 : -1;
        for (int x = from.x; (stepX == 1) ? (x <= to.x) : (x >= to.x); x += stepX) {
            // check the x and y value is in the window
            if ((size_t)x >= 0 && (size_t)x < window.width &&
                round(y) >= 0 && round(y) < window.height) {
                window.setPixelColour(x, round(y), packedColour);
            }
            y += slope;
        }
    } else {
        float x = from.x;
        float slope = (float)dx / (float)dy;
        if (dy<0){
            slope = -slope;
        }
        int stepY = (dy > 0) ? 1 : -1;
        for (int y = from.y; (stepY == 1) ? (y <= to.y) : (y >= to.y) ; y += stepY) {
            if (round(x) >= 0 && round(x) < window.width &&
                    (size_t)y >= 0 && (size_t)y < window.height) {
                window.setPixelColour(round(x), y, packedColour);
            }
            x += slope;
        }
    }
}

void drawTriangle(DrawingWindow &window, CanvasTriangle triangle, Colour colour) {
    drawLineInterpolation(window, triangle[0], triangle[1], colour);
    drawLineInterpolation(window, triangle[1], triangle[2], colour);
    drawLineInterpolation(window, triangle[2], triangle[0], colour);
}



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

// using interpolation to draw filled triangle
// BUG should be fixed(segmentation fault),Very huge bug 15/10/2023
void drawFilledTriangle (DrawingWindow &window, CanvasTriangle triangle, Colour colour) {
    CanvasPoint bottom = triangle[0];
    CanvasPoint middle = triangle[1];
    CanvasPoint top = triangle[2];

    uint32_t packedColour = (255 << 24) | (colour.red << 16) | (colour.green << 8) | colour.blue;

    // sort the points by y value
    if (bottom.y > middle.y) {
        std::swap(bottom, middle);
    }
    if (bottom.y > top.y) {
        std::swap(bottom, top);
    }
    if (middle.y > top.y) {
        std::swap(middle, top);
    }
    // now bottom.y <= middle.y <= top.y

    // interpolate the depth values
    std::vector<float> depthValuesBottomToTop = interpolateSingleFloats(bottom.depth, top.depth, top.y - bottom.y + 1);
    std::vector<float> depthValuesBottomToMiddle = interpolateSingleFloats(bottom.depth, middle.depth, middle.y - bottom.y + 1);
    std::vector<float> depthValuesMiddleToTop = interpolateSingleFloats(middle.depth, top.depth, top.y - middle.y + 1);

    // Calculate lines
    // This value is the list of the x value of the start/end line between bottom and top
    std::vector<float> xValuesBottomToTop = interpolateSingleFloats(bottom.x, top.x, top.y - bottom.y + 1);
    std::vector<float> xValuesBottomToMiddle = interpolateSingleFloats(bottom.x, middle.x, middle.y - bottom.y + 1);
    std::vector<float> xValuesMiddleToTop = interpolateSingleFloats(middle.x, top.x, top.y - middle.y + 1);

    // Rasterize top half triangle (from middle to top)
    for (int i = 0; i < top.y - middle.y + 1; i++) {
        int y = middle.y + i;
        //here， maybe some error to transfer float to int
        int x_start = std::round(xValuesMiddleToTop[i]);
        int x_end = std::round(xValuesBottomToTop[middle.y - bottom.y + i]);
        if (x_start > x_end) std::swap(x_start, x_end); // ensure x_start <= x_end

        float depthStart = depthValuesMiddleToTop[i];
        float depthEnd = depthValuesBottomToTop[middle.y - bottom.y + i];
        std::vector<float> XlineDepth = interpolateSingleFloats(depthStart, depthEnd, x_end - x_start + 1);

        // Draw horizontal line from x_start to x_end
        for (int x = x_start; x <= x_end; x++) {
            float CurrentPointDepth = XlineDepth[x - x_start];
            // Z buffer is closer to us if the value is smaller
            if (x >= 0 && (size_t)x < window.width &&
                (size_t)y >= 0 && (size_t)y < window.height && CurrentPointDepth < zBuffer[y][x]) {
                window.setPixelColour(x, y, packedColour);
                zBuffer[y][x] = CurrentPointDepth;
            }
        }
    }

    // Rasterize bottom half triangle (from bottom to middle)
    for (int i = 0; i < middle.y - bottom.y + 1; i++) {
        int y = bottom.y + i;
        int x_start = std::round(xValuesBottomToMiddle[i]);
        int x_end = std::round(xValuesBottomToTop[i]);
        if (x_start > x_end) std::swap(x_start, x_end); // ensure x_start <= x_end

        float depthStart = depthValuesBottomToMiddle[i];
        float depthEnd = depthValuesBottomToTop[i];
        std::vector<float> XlineDepth = interpolateSingleFloats(depthStart, depthEnd, x_end - x_start + 1);

        // Draw horizontal line from x_start to x_end
        for (int x = x_start; x <= x_end; x++) {
            float CurrentPointDepth = XlineDepth[x - x_start];
            if (x >= 0 && (size_t)x < window.width &&
                (size_t)y >= 0 && (size_t)y < window.height && CurrentPointDepth < zBuffer[y][x]) {
                window.setPixelColour(x, y, packedColour);
                zBuffer[y][x] = CurrentPointDepth;
            }
        }
    }
}

void drawTextureTriangle(DrawingWindow &window, CanvasTriangle triangle,TextureMap &textureMap) {
    CanvasPoint bottom = triangle[0];
    CanvasPoint middle = triangle[1];
    CanvasPoint top = triangle[2];

    // sort the points by y value
    if (bottom.y > middle.y) {
        std::swap(bottom, middle);
    }
    if (bottom.y > top.y) {
        std::swap(bottom, top);
    }
    if (middle.y > top.y) {
        std::swap(middle, top);
    }

    std::vector<float> xValuesBottomToTop = interpolateSingleFloats(bottom.x, top.x, top.y - bottom.y + 1);
    std::vector<float> xValuesBottomToMiddle = interpolateSingleFloats(bottom.x, middle.x, middle.y - bottom.y + 1);
    std::vector<float> xValuesMiddleToTop = interpolateSingleFloats(middle.x, top.x, top.y - middle.y + 1);

    std::vector<TexturePoint> texCoordsBottomToTop = interpolateTexturePoints(bottom.texturePoint, top.texturePoint, top.y - bottom.y + 1);
    std::vector<TexturePoint> texCoordsBottomToMiddle = interpolateTexturePoints(bottom.texturePoint, middle.texturePoint, middle.y - bottom.y + 1);
    std::vector<TexturePoint> texCoordsMiddleToTop = interpolateTexturePoints(middle.texturePoint, top.texturePoint, top.y - middle.y + 1);

    // Rasterize top half triangle (from middle to top)
    for (int i = 0; i < top.y - middle.y + 1; i++) {
        int y = middle.y + i;
        int x_start = xValuesMiddleToTop[i];
        int x_end = xValuesBottomToTop[middle.y - bottom.y + i];
        TexturePoint texStart = texCoordsMiddleToTop[i];
        TexturePoint texEnd = texCoordsBottomToTop[middle.y - bottom.y + i];
        std::vector<TexturePoint> texRow = interpolateTexturePoints(texStart, texEnd, x_end - x_start + 1);

        int texIdx = 0;
        for (int x = x_start; x <= x_end; x++) {
            if (x >= 0 && (size_t)x < window.width && (size_t)y >= 0 && (size_t)y < window.height) {
                uint32_t colour = textureMap.pixels[(size_t)texRow[texIdx].y * textureMap.width + (size_t)texRow[texIdx].x];
                window.setPixelColour(x, y, colour);
            }
            texIdx++;
        }
    }

    // Rasterize bottom half triangle (from bottom to middle)
    for (int i = 0; i < middle.y - bottom.y + 1; i++) {
        int y = bottom.y + i;
        int x_start = xValuesBottomToMiddle[i];
        int x_end = xValuesBottomToTop[i];
        TexturePoint texStart = texCoordsBottomToMiddle[i];
        TexturePoint texEnd = texCoordsBottomToTop[i];
        std::vector<TexturePoint> texRow = interpolateTexturePoints(texStart, texEnd, x_end - x_start + 1);

        int texIdx = 0;
        for (int x = x_start; x <= x_end; x++) {
            if (x >= 0 && (size_t)x < window.width && (size_t)y >= 0 && (size_t)y < window.height) {
                uint32_t colour = textureMap.pixels[(size_t)texRow[texIdx].y * textureMap.width + (size_t)texRow[texIdx].x];
                window.setPixelColour(x, y, colour);
            }
            texIdx++;
        }
    }
}

std::vector<TexturePoint> interpolateTexturePoints(TexturePoint start, TexturePoint end, int numValues) {
    std::vector<TexturePoint> result;
    float dx = (end.x - start.x) /(numValues - 1);
    float dy = (end.y - start.y) /(numValues - 1);

    for (int i = 0; i < numValues; i++) {
        result.push_back(TexturePoint(start.x + i * dx, start.y + i * dy));
    }
    return result;
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

//Task 3: Single Dimenson Greyscale Interpolation
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
            std::cout << "1 is pressed, I dont know how to set this to u" << std::endl;
            CanvasPoint p1(rand() % window.width-1, rand() % window.height-1);
            CanvasPoint p2(rand() % window.width-1, rand() % window.height-1);
            CanvasPoint p3(rand() % window.width-1, rand() % window.height-1);
            CanvasTriangle randomTriangle(p1, p2, p3);
            drawTriangle(window, randomTriangle, Colour(rand()%255, rand()%255, rand()%255));
        } else if (event.key.keysym.sym == SDLK_2){
            std::cout << "2 is pressed, I dont know how to set this to f" << std::endl;
            CanvasPoint p1(rand() % window.width-1, rand() % window.height-1);
            CanvasPoint p2(rand() % window.width-1, rand() % window.height-1);
            CanvasPoint p3(rand() % window.width-1, rand() % window.height-1);

            CanvasTriangle randomTriangle(p1, p2, p3);
            drawFilledTriangle(window, randomTriangle, Colour(rand()%255, rand()%255, rand()%255));
        } else if (event.key.keysym.sym == SDLK_3) {
            //texture map
            std::cout << "3 is pressed, I dont know how to set this to t" << std::endl;
            CanvasPoint p1(160, 10);
            p1.texturePoint = TexturePoint(195, 5);
            CanvasPoint p2(300, 230);
            p2.texturePoint = TexturePoint(395, 380);
            CanvasPoint p3(10, 150);
            p3.texturePoint = TexturePoint(65, 330);

            CanvasTriangle triangle(p1, p2, p3);
            TextureMap textureMap("../texture.ppm");
            drawTextureTriangle(window, triangle, textureMap);
        }else if (event.key.keysym.sym == SDLK_4){
            std::cout << "4 is pressed, I dont know how to set this to a" << std::endl;
            renderPointCloud(window, "../cornell-box.obj", glm::vec3(0, 0, 4), 2);
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

// return a hashmap from material name to colour
std::map<std::string, Colour> loadMaterials(const std::string& filename) {
    // Map from material name to colour.
    std::map<std::string, Colour> materials;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Failed to open the .mtl file!" << std::endl;
        return materials;
    }

    std::string line;
    std::string currentMaterialName;
    while (std::getline(file, line)) {
        auto tokens = split(line, ' ');
        if (tokens[0] == "newmtl") {
           currentMaterialName = tokens[1];
        } else if (tokens[0] == "Kd") {
            int r, g, b;
            r = std::stoi(tokens[1]);
            g = std::stoi(tokens[2]);
            b = std::stoi(tokens[3]);
            // Assuming the values in the mtl file are between 0 and 1.
            materials[currentMaterialName] = Colour(currentMaterialName, r * 255, g * 255, b * 255);
        }
    }
    return materials;
}

std::vector<ModelTriangle> loadOBJ(const std::string& filename, float scalingFactor) {
    std::vector<ModelTriangle> triangles;
    std::vector<glm::vec3> vertices;

    // Open the file.
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open the file!" << std::endl;
        return triangles;
    }
    // Load the materials from the .mtl file.
    std::map<std::string, Colour> palette = loadMaterials("../cornell-box.mtl");
    Colour currentColour;
    std::string line;
    while (std::getline(file, line)) {
        // Tokenize the line for easier parsing.
        auto tokens = split(line, ' ');

        if (tokens[0]== "usemtl"){
            currentColour = palette[tokens[1]];
        }else if (tokens[0] == "v") {
            // Check if line starts with 'v' (vertex).
            // stof converts a string to a float.
            glm::vec3 vertex(stof(tokens[1]), stof(tokens[2]), stof(tokens[3]));
            vertex *= scalingFactor; // Apply scaling
            vertices.push_back(vertex);

            // Check if line starts with 'f' (face).
        } else if (tokens[0] == "f") {
            // Convert from 1-based index to 0-based index for vertices.
            ModelTriangle triangle(vertices[stoi(tokens[1]) - 1],
                                   vertices[stoi(tokens[2]) - 1],
                                   vertices[stoi(tokens[3]) - 1],
                                   currentColour);
            triangles.push_back(triangle);
        }
    }
    return triangles;
}

CanvasPoint getCanvasIntersectionPoint(glm::vec3 cameraPosition, glm::vec3 vertexPosition, float focalLength) {
    // Convert from model to camera coordinates
    float x_rel = vertexPosition.x - cameraPosition.x;
    float y_rel = vertexPosition.y - cameraPosition.y;
    float z_rel = vertexPosition.z - cameraPosition.z;

    // Compute the projection using the formulas
    float canvasX = focalLength * (x_rel / z_rel);
    // move the origin to the center of the screen
    canvasX = canvasX + WIDTH / 2.0f;
    float canvasY = focalLength * (y_rel / z_rel);
    canvasY = canvasY + HEIGHT / 2.0f;

    // z越大离得越远
    return {canvasX, canvasY,vertexPosition.z};
}


void renderPointCloud(DrawingWindow &window, const std::string& filename, glm::vec3 cameraPosition, float focalLength) {
    // Load the triangles from the OBJ file.
    std::vector<ModelTriangle> triangles = loadOBJ(filename, 0.35);

    //上面这一步没问题
    // the point position is wrong
    for (const auto& triangle : triangles) {
        CanvasPoint projectedPoints[3];
        for (int i = 0; i < 3; i++) {
            projectedPoints[i] = getCanvasIntersectionPoint(cameraPosition, triangle.vertices[i], focalLength);
            // Fix potential upside-down issue
            projectedPoints[i].y = HEIGHT - projectedPoints[i].y;
        }
        drawFilledTriangle(window, CanvasTriangle(projectedPoints[0], projectedPoints[1], projectedPoints[2]),
                     Colour(triangle.colour.red, triangle.colour.green, triangle.colour.blue));
    }
}

std::vector<std::vector<float>> initialiseDepthBuffer(int width, int height) {
    std::vector<std::vector<float>> depthBuffer;
    for (int y = 0; y < height; y++) {
        std::vector<float> row;
        for (int x = 0; x < width; x++) {
            row.push_back(std::numeric_limits<float>::infinity());
        }
        depthBuffer.push_back(row);
    }
    return depthBuffer;
}

int main(int argc, char *argv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;

    // test loadOBJ
//    std::vector<ModelTriangle> triangles = loadOBJ("../cornell-box.obj", 0.35);
//    for (size_t i = 0; i < triangles.size(); i++) {
//        std ::cout <<triangles[i].colour << std::endl;
//        std::cout << triangles[i] << std::endl;
//
//    }
//    std::cout << "Loaded " << triangles.size() << " triangles" << std::endl;

    zBuffer = initialiseDepthBuffer(window.width, window.height);

	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window);
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
	return 0;
}
