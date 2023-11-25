#ifndef REDNOISE_DRAWTEXTURETRIANGLE_H
#define REDNOISE_DRAWTEXTURETRIANGLE_H

#include "CanvasTriangle.h"
#include "CanvasPoint.h"
#include <vector>
#include <glm/glm.hpp>
#include "DrawingWindow.h"
#include "ModelTriangle.h"
#include "TexturePoint.h"
#include "TextureMap.h"
#include "Interpolate.h"
#include "Globals.h"


//void drawTextureTriangle(DrawingWindow &window, CanvasTriangle triangle,TextureMap &textureMap);

std::vector<TexturePoint> interpolateTexturePoints(TexturePoint start, TexturePoint end, int numValues);

void drawTextureTriangle (DrawingWindow &window, CanvasTriangle triangle, Colour colour,TextureMap &textureMap);

void drawTexturePartTriangle (DrawingWindow &window, CanvasTriangle triangle, Colour colour,TextureMap &textureMap);


#endif //REDNOISE_DRAWTEXTURETRIANGLE_H
