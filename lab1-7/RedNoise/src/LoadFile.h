#ifndef REDNOISE_LOADFILE_H
#define REDNOISE_LOADFILE_H

#include <vector>
#include <glm/glm.hpp>
#include "DrawingWindow.h"
#include "CanvasPoint.h"
#include "Colour.h"
#include "ModelTriangle.h"
#include "map"
#include <fstream>
#include<iostream>
#include <Utils.h>

std::map<std::string, Colour> loadMaterials(const std::string& filename);

std::vector<ModelTriangle> loadOBJ(const std::string& filename, float scalingFactor);

std::vector<ModelTriangle> loadTexOBJ(const std::string& filename, float scalingFactor);
#endif //REDNOISE_LOADFILE_H
