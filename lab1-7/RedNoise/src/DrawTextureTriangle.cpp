#include "DrawTextureTriangle.h"

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



void drawTextureTriangle (DrawingWindow &window, CanvasTriangle triangle,Colour colour,TextureMap &textureMap) {
    std :: cout << "drawFilledTriangle is called" << std::endl;
    // print out the triangle
    std :: cout << triangle << std::endl;
    //这个canvaspoint里面有texturePoint
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
    drawTexturePartTriangle(window, CanvasTriangle(middle, extraPoint, bottom), colour,textureMap);
    drawTexturePartTriangle(window, CanvasTriangle(middle, extraPoint, top), colour,textureMap);
}

void drawTexturePartTriangle (DrawingWindow &window, CanvasTriangle triangle, Colour colour,TextureMap &textureMap) {

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
        std::vector<TexturePoint> XlineTexturePoint = interpolateTexturePoints(pointsBetweenMiddleAndPeak[i - yStart].texturePoint,
                                                                               pointsBetweenExtraAndPeak[i - yStart].texturePoint,
                                                                               pointsBetweenExtraAndPeak[i - yStart].x -
                                                                               pointsBetweenMiddleAndPeak[i - yStart].x + 1);

        int x_start = pointsBetweenMiddleAndPeak[i - yStart].x;
        int x_end = pointsBetweenExtraAndPeak[i - yStart].x;
        // Draw horizontal line from x_start to x_end
        int texIdx = 0;
        for (int x = x_start; x <= x_end; x++) {
            float CurrentPointDepth = 1.0f/XlineDepth[x - x_start];
            // Z buffer is closer to us if the value is smaller
            if (x >= 0 && (size_t)x < window.width &&
                (size_t)y >= 0 && (size_t)y < window.height && CurrentPointDepth > zBuffer[y][x]) {
                if(triangle.vertices[0].texturePoint.x != 0 && triangle.vertices[0].texturePoint.y != 0
                &&triangle.vertices[1].texturePoint.x != 0 && triangle.vertices[1].texturePoint.y != 0
                &&triangle.vertices[2].texturePoint.x != 0 && triangle.vertices[2].texturePoint.y != 0){
                    packedColour= textureMap.pixels[(size_t)XlineTexturePoint[texIdx].y * textureMap.width + (size_t)XlineTexturePoint[texIdx].x];
                }
                window.setPixelColour(x, y, packedColour);
                zBuffer[y][x] = CurrentPointDepth;
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