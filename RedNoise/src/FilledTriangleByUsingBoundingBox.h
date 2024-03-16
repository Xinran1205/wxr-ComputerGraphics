#ifndef REDNOISE_FILLEDTRIANGLEBYUSINGBOUNDINGBOX_H
#define REDNOISE_FILLEDTRIANGLEBYUSINGBOUNDINGBOX_H

#include "CanvasTriangle.h"
#include "CanvasPoint.h"
#include <vector>
#include <glm/glm.hpp>
#include "DrawingWindow.h"
#include "ModelTriangle.h"
//this class is not used, it is another way to draw a filled triangle by using bounding box

std::vector<glm::vec2> findBoundingBox(CanvasTriangle triangle);

glm::vec2 calculateTheLine(CanvasPoint p1, CanvasPoint p2);

bool isInTheTriangle(CanvasTriangle triangle, CanvasPoint p);

void drawFilledTriangleUsingBoundingBox(DrawingWindow &window, CanvasTriangle triangle, Colour colour);


#endif //REDNOISE_FILLEDTRIANGLEBYUSINGBOUNDINGBOX_H
