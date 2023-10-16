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


void drawTextureTriangle(DrawingWindow &window, CanvasTriangle triangle,TextureMap &textureMap);

std::vector<TexturePoint> interpolateTexturePoints(TexturePoint start, TexturePoint end, int numValues);


#endif //REDNOISE_DRAWTEXTURETRIANGLE_H
