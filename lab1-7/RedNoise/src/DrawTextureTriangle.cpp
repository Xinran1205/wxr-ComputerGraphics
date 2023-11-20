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

std::vector<TexturePoint> interpolateTexturePoints(TexturePoint start, TexturePoint end, int numValues) {
    std::vector<TexturePoint> result;
    float dx = (end.x - start.x) /(numValues - 1);
    float dy = (end.y - start.y) /(numValues - 1);

    for (int i = 0; i < numValues; i++) {
        result.push_back(TexturePoint(start.x + i * dx, start.y + i * dy));
    }
    return result;
}