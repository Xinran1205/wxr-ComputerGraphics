#include "LoadFile.h"

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
            float r, g, b;
            r = std::stof(tokens[1]);
            g = std::stof(tokens[2]);
            b = std::stof(tokens[3]);
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