#ifndef REDNOISE_INTERPOLATE_H
#define REDNOISE_INTERPOLATE_H
#include <vector>
#include <glm/glm.hpp>
#include "DrawingWindow.h"
#include "CanvasPoint.h"
#include "Colour.h"
#include "CanvasTriangle.h"

std::vector<float> interpolateSingleFloats(float from,float to, int numberOfValues);

std::vector<glm::vec3> interpolateTripleFloats(glm::vec3 from, glm::vec3 to, int numberOfValues);

void drawTheGreyScale(DrawingWindow &window);

void draw(DrawingWindow &window);

void drawLineBresenham(DrawingWindow &window, CanvasPoint from, CanvasPoint to, Colour colour);

void drawLineInterpolation(DrawingWindow &window, CanvasPoint from, CanvasPoint to, Colour colour);

void drawTriangle(DrawingWindow &window, CanvasTriangle triangle, Colour colour);

//void drawLine(DrawingWindow &window, CanvasPoint from, CanvasPoint to, Colour colour);


#endif //REDNOISE_INTERPOLATE_H
