#include "LoadFile.h"
#include "Globals.h"



// 这个变量是一对多的关系，一个顶点对应多个面法向量
std::map<glm::vec3, std::vector<glm::vec3>, Vec3Comparator> vertexPlaneNormals;

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
            std::array<glm::vec3, 3> triangleVertices = {
                    vertices[stoi(tokens[1]) - 1],
                    vertices[stoi(tokens[2]) - 1],
                    vertices[stoi(tokens[3]) - 1]
            };

            // Calculate the normal of the triangle (each triangle has a single normal).
            // 根据三角形两边的向量计算法向量
            glm::vec3 edge1 = triangleVertices[1] - triangleVertices[0];
            glm::vec3 edge2 = triangleVertices[2] - triangleVertices[0];
            glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

            ModelTriangle triangle(triangleVertices[0], triangleVertices[1], triangleVertices[2], currentColour);
            triangle.normal = normal;
            triangles.push_back(triangle);

            // the bottom is for gourod shading!!!

            // here calculate the normal for each vertex, put all the normals in a hashmap
            for (const ModelTriangle& triangle : triangles) {
                glm::vec3 normal = triangle.normal;
                for (const glm::vec3& vertex : triangle.vertices) {
                    // this vertexPlaneNormals is a hashmap from vertex to a list of normals and it is a global variable
                    vertexPlaneNormals[vertex].push_back(normal);
                }
            }

            for (const glm::vec3& vertex : vertices) {
                const std::vector<glm::vec3> &normals = vertexPlaneNormals[vertex];

                glm::vec3 sumNormals(0.0f, 0.0f, 0.0f);
                for (const glm::vec3 &n : normals) {
                    sumNormals += n;
                }
                glm::vec3 averageNormal = glm::normalize(sumNormals / static_cast<float>(normals.size()));
                //把这个平均法向量和顶点绑定起来
                vertexNormals[vertex] = averageNormal;
            }
        }
    }
    return triangles;
}