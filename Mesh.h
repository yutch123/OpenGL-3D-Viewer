#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <string>

#define MAX_BONE_INFLUENCE 4

class Shader;

struct Vertex
{
    glm::vec3 Position = glm::vec3(0.0f);
    glm::vec3 Normal = glm::vec3(0.0f);
    glm::vec2 TexCoords = glm::vec2(0.0f);
    glm::vec3 Tangent = glm::vec3(0.0f);
    glm::vec3 Bitangent = glm::vec3(0.0f);

    int   m_BoneIDs[MAX_BONE_INFLUENCE] = { 0, 0, 0, 0 };
    float m_Weights[MAX_BONE_INFLUENCE] = { 0.0f, 0.0f, 0.0f, 0.0f };
};

struct Texture
{
    unsigned int id = 0;
    std::string type;
    std::string path;
};

class Mesh
{
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    void setInfo(const std::string& text) { info = text; }
    std::string getInfo() const { return info; }

    Mesh(
        std::vector<Vertex> vertices,
        std::vector<unsigned int> indices,
        std::vector<Texture> textures
    );

    void Draw(Shader& shader);
    void DrawForPicking(Shader& shader, const glm::vec3& color);

    int pickingID = 0;

private:
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;

    std::string info;

    void setupMesh();
};