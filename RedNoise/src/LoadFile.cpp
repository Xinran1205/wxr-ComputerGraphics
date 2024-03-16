#include "LoadFile.h"
#include "Globals.h"




// this is a hashmap from vertex to a list of normals
// one vertex can have multiple facet normals
std::map<glm::vec3, std::vector<glm::vec3>, Vec3Comparator> vertexPlaneNormals;


// return a hashmap from material name to colour
// MaterialProperties is a struct that contains colour, isMirror and isGlass
std::map<std::string, MaterialProperties> loadMaterials(const std::string& filename) {
    // Map from material name to the material properties.
    std::map<std::string, MaterialProperties> materials;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Failed to open the .mtl file!" << std::endl;
        return materials;
    }

    std::string line;
    std::string currentMaterialName;
    Colour currentColour;
    bool isMirror = false;
    bool isGlass = false;
    while (std::getline(file, line)) {
        auto tokens = split(line, ' ');
        if (tokens[0] == "newmtl") {
            currentMaterialName = tokens[1];
            // here is very crucial!
            isMirror = false;  // Reset for each new material
            isGlass = false;
        } else if (tokens[0] == "mirror") {
            isMirror = tokens[1] == "1";  // if mirror is 1, then it is a mirror
            // in the mtl file we must make sure that the mirror is before Kd
        }else if(tokens[0] == "glass"){
            isGlass = tokens[1] == "1";
        }else if (tokens[0] == "Kd"){
            float r, g, b;
            r = std::stof(tokens[1]);
            g = std::stof(tokens[2]);
            b = std::stof(tokens[3]);
            currentColour = Colour(currentMaterialName, r * 255, g * 255, b * 255);
            materials[currentMaterialName] = MaterialProperties{currentColour, isMirror, isGlass};
        }
    }
    return materials;
}

std::vector<ModelTriangle> loadOBJ(const std::string& filename, float scalingFactor, const std::string& materialName) {
    std::vector<ModelTriangle> triangles;
    std::vector<glm::vec3> vertices;
    std::vector<TexturePoint> texturePoints;

    // Open the file.
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open the file!" << std::endl;
        return triangles;
    }
    // Load the materials from the .mtl file.
    std::map<std::string, MaterialProperties> materialsProperties = loadMaterials(materialName);
    MaterialProperties currentMaterialProps;
//    Colour currentColour;
    std::string line;
    while (std::getline(file, line)) {
        // Tokenize the line for easier parsing.
        auto tokens = split(line, ' ');

        if (tokens[0] == "usemtl") {
            currentMaterialProps = materialsProperties[tokens[1]];
        } else if (tokens[0] == "v") {
            // Check if line starts with 'v' (vertex).
            // stof converts a string to a float.
            glm::vec3 vertex(stof(tokens[1]), stof(tokens[2]), stof(tokens[3]));
            vertex *= scalingFactor; // Apply scaling
            vertices.push_back(vertex);

            // Check if line starts with 'f' (face).
        } else if (tokens[0] == "vt") {
            TexturePoint texturePoint{stof(tokens[1]), stof(tokens[2])};
            texturePoints.push_back(texturePoint);
        } else if (tokens[0] == "f") {
            std::array<glm::vec3, 3> triangleVertices;          // store the vertices of the triangle
            std::array<TexturePoint, 3> triangleTexturePoints;  // store the texture points of three vertices of the triangle

            // construct each triangle
            for (int i = 0; i < 3; i++) {
                std::vector<std::string> vertexTexturePair = split(tokens[i + 1], '/');
                int vertexIndex = stoi(vertexTexturePair[0]) - 1;  // OBJ index starts from 1
                triangleVertices[i] = vertices[vertexIndex];

                // if this vertex has a texture coordinate
                if (vertexTexturePair.size() > 1 && !vertexTexturePair[1].empty()) {
                    int textureIndex = stoi(vertexTexturePair[1]) - 1; // OBJ index starts from 1
                    triangleTexturePoints[i] = texturePoints[textureIndex];
                }
            }

            // calculate the normal of this triangle
            glm::vec3 edge1 = triangleVertices[1] - triangleVertices[0];
            glm::vec3 edge2 = triangleVertices[2] - triangleVertices[0];
            glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

            // create a triangle and set its properties
            ModelTriangle triangle(triangleVertices[0], triangleVertices[1], triangleVertices[2], currentMaterialProps.colour);
            triangle.texturePoints = triangleTexturePoints;
            triangle.normal = normal;
            triangle.isMirror = currentMaterialProps.isMirror;
            triangle.isGlass = currentMaterialProps.isGlass;
            triangles.push_back(triangle);
        }
    }
    // the bottom is for gouraud shading and phong shading!!!

    // here calculate the normal for each vertex, put all the normals in a hashmap
    for (const ModelTriangle &triangle: triangles) {
        glm::vec3 normal = triangle.normal;
        for (const glm::vec3 &vertex: triangle.vertices) {
            // this vertexPlaneNormals is a hashmap from vertex to a list of normals and it is a global variable
            vertexPlaneNormals[vertex].push_back(normal);
        }
    }

    for (const glm::vec3 &vertex: vertices) {
        const std::vector<glm::vec3> &normals = vertexPlaneNormals[vertex];

        glm::vec3 sumNormals(0.0f, 0.0f, 0.0f);
        for (const glm::vec3 &n: normals) {
            sumNormals += n;
        }
        glm::vec3 averageNormal = glm::normalize(sumNormals / static_cast<float>(normals.size()));
        // bind the average normal to the vertex
        vertexNormals[vertex] = averageNormal;
    }
    return triangles;
}