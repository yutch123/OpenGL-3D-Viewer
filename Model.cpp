#include "Model.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <iostream>
#include <cstdlib>
#include <cfloat>
#include <algorithm>
#include <stb_image.h>
#include <fstream>
#include <sstream>

namespace
{
    int FindDiffuseTextureIndex(const std::vector<Texture>& textures)
    {
        for (size_t i = 0; i < textures.size(); ++i)
        {
            if (textures[i].type == "texture_diffuse" && textures[i].id != 0)
                return static_cast<int>(i);
        }
        return -1;
    }
}

void Model::Draw(Shader& shader)
{
    shader.use();

    glm::mat4 modelMat = getModelMatrix();
    shader.setMat4("model", modelMat);

    for (size_t i = 0; i < meshes.size(); ++i)
    {
        if (!meshVisible[i])
            continue;

        if (i >= meshColors.size())
            meshColors.push_back(glm::vec3(1.0f));

        const auto& textures = meshes[i].textures;

        int diffuseIndex = FindDiffuseTextureIndex(textures);

        if (diffuseIndex >= 0)
        {
            shader.setBool("useTexture", true);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textures[diffuseIndex].id);
            shader.setInt("texture_diffuse1", 0);
        }
        else
        {
            shader.setBool("useTexture", false);
            shader.setVec3("objectColor", meshColors[i]);

        }

        meshes[i].Draw(shader);
    }
}

void Model::loadModel(const std::string& path)
{
    std::cout << "[loadModel] Loading model: " << path << std::endl;

    bool isGLTF =
        path.find(".glb") != std::string::npos ||
        path.find(".gltf") != std::string::npos;

    unsigned int flags =
        aiProcess_Triangulate |
        aiProcess_CalcTangentSpace;

    if (!isGLTF)
        flags |= aiProcess_FlipUVs;

    scene = importer.ReadFile(path, flags);

    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode)
    {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return;
    }

    directory = path.substr(0, path.find_last_of("/\\"));
    std::cout << "[loadModel] Directory: " << directory << std::endl;

    meshes.clear();
    meshColors.clear();
    textures_loaded.clear();
    meshVisible.clear();
    meshNotes.clear();

    processNode(scene->mRootNode, scene);

    meshVisible.resize(meshes.size(), true);
    meshNotes.resize(meshes.size(), ""); // создаем пустую заметку для каждого меша

    loadNotes(getNotesFilePath());

    for (size_t i = 0; i < meshes.size(); ++i)
    {
        meshes[i].setInfo(u8"Слой " + std::to_string(i + 1) + u8" в моем произвольном эпителии");
    }

    std::cout << "[loadModel] Total meshes loaded: " << meshes.size() << std::endl;
}

std::string Model::getNotesFilePath() const
{
    return directory + "/mesh_notes.txt";
}

void Model::saveNotes(const std::string& path) const
{
    std::ofstream file(path, std::ios::out | std::ios::trunc);
    if (!file.is_open())
        return;

    for (size_t i = 0; i < meshNotes.size(); ++i)
    {
        file << i << '\n';
        file << meshNotes[i] << '\n';
        file << "---NOTE_END---" << '\n';
    }
}

void Model::loadNotes(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
        return;

    std::vector<std::string> loadedNotes(meshNotes.size(), "");

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty())
            continue;

        int index = -1;
        try
        {
            index = std::stoi(line);
        }
        catch (...)
        {
            continue;
        }

        if (index < 0 || index >= static_cast<int>(loadedNotes.size()))
            continue;

        std::ostringstream noteStream;
        while (std::getline(file, line))
        {
            if (line == "---NOTE_END---")
                break;

            if (!noteStream.str().empty())
                noteStream << '\n';

            noteStream << line;
        }

        loadedNotes[index] = noteStream.str();
    }

    meshNotes = loadedNotes;
}

size_t Model::getMeshCount() const
{
    return meshes.size();
}

std::string& Model::getMeshNote(int index)
{
    return meshNotes[index];
}

void Model::setMeshNote(int index, const std::string& note)
{
    if (index < 0 || index >= static_cast<int>(meshNotes.size()))
        return;

    meshNotes[index] = note;
}

void Model::drawForPicking(Shader& shader)
{
    if (!pickingEnabled)
        return;

    shader.use();

    for (size_t i = 0; i < meshes.size(); ++i)
    {
        if (!meshVisible[i])
            continue;

        unsigned int id = static_cast<unsigned int>(i + 1);
        glm::vec3 pickColor(
            (id & 0xFF) / 255.0f,
            ((id >> 8) & 0xFF) / 255.0f,
            ((id >> 16) & 0xFF) / 255.0f
        );

        shader.setVec3("pickingColor", pickColor);
        meshes[i].DrawForPicking(shader, pickColor);
    }
}

unsigned int TextureFromMemory(const aiTexture* tex)
{
    unsigned int textureID = 0;
    glGenTextures(1, &textureID);

    int width = 0, height = 0, nrComponents = 0;
    unsigned char* data = nullptr;

    if (tex->mHeight == 0)
    {
        std::cout << "[TextureFromMemory] Compressed embedded texture" << std::endl;

        data = stbi_load_from_memory(
            reinterpret_cast<unsigned char*>(tex->pcData),
            tex->mWidth,
            &width, &height, &nrComponents, 0
        );
    }
    else
    {
        std::cout << "[TextureFromMemory] Raw embedded texture: "
            << tex->mWidth << "x" << tex->mHeight << std::endl;

        width = static_cast<int>(tex->mWidth);
        height = static_cast<int>(tex->mHeight);
        nrComponents = 4;
        data = reinterpret_cast<unsigned char*>(tex->pcData);
    }

    if (!data)
    {
        std::cerr << "[TextureFromMemory] Embedded texture failed to load" << std::endl;
        return 0;
    }

    GLenum format = GL_RED;
    if (nrComponents == 4)
        format = GL_RGBA;
    else if (nrComponents == 3)
        format = GL_RGB;
    else if (nrComponents == 1)
        format = GL_RED;

    std::cout << "[TextureFromMemory] Loaded "
        << width << "x" << height
        << " | channels = " << nrComponents
        << " | textureID = " << textureID
        << std::endl;

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (tex->mHeight == 0)
        stbi_image_free(data);

    return textureID;
}

glm::mat4 Model::getModelMatrix() const
{
    glm::vec3 center = getCenter();

    glm::mat4 modelMat = glm::mat4(1.0f);
    modelMat = glm::translate(modelMat, position);
    modelMat = modelMat * rotationMatrix;
    modelMat = glm::scale(modelMat, glm::vec3(scale));
    modelMat = glm::translate(modelMat, -center);

    return modelMat;
}

unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma)
{
    std::string filename = directory + '/' + std::string(path);

    unsigned int textureID = 0;
    glGenTextures(1, &textureID);

    std::cout << "[TextureFromFile] Loading: " << filename << std::endl;

    int width = 0, height = 0, nrComponents = 0;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);

    if (!data)
    {
        std::cerr << "[TextureFromFile] Failed to load texture: " << filename << std::endl;
        return 0;
    }

    GLenum format = GL_RED;
    if (nrComponents == 1)
        format = GL_RED;
    else if (nrComponents == 3)
        format = GL_RGB;
    else if (nrComponents == 4)
        format = GL_RGBA;
    else
    {
        std::cerr << "[TextureFromFile] Unknown number of channels: " << nrComponents << std::endl;
        stbi_image_free(data);
        return 0;
    }

    std::cout << "[TextureFromFile] Success: "
        << width << "x" << height
        << " | channels = " << nrComponents
        << " | textureID = " << textureID
        << std::endl;

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return textureID;
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
    std::cout << "[processNode] Node meshes: " << node->mNumMeshes
        << " | children: " << node->mNumChildren
        << std::endl;

    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* ai_mesh = scene->mMeshes[node->mMeshes[i]];

        Mesh newMesh = processMesh(ai_mesh, scene);
        meshes.push_back(newMesh);

        glm::vec3 meshColor(1.0f);
        if (ai_mesh->mMaterialIndex >= 0)
        {
            aiMaterial* material = scene->mMaterials[ai_mesh->mMaterialIndex];
            aiColor3D color(1.0f, 1.0f, 1.0f);

            if (material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
                meshColor = glm::vec3(color.r, color.g, color.b);
        }

        meshColors.push_back(meshColor);
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    std::cout << "\n[processMesh] -----------------------------" << std::endl;
    std::cout << "[processMesh] Vertices: " << mesh->mNumVertices
        << " | Faces: " << mesh->mNumFaces
        << " | Material index: " << mesh->mMaterialIndex
        << std::endl;

    unsigned int uvChannel = 0;

    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        aiString texPath;

        if (material->GetTexture(aiTextureType_BASE_COLOR, 0, &texPath, nullptr, &uvChannel) == AI_SUCCESS)
        {
            std::cout << "[processMesh] BASE_COLOR uses UV channel: " << uvChannel << std::endl;
        }
        else if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath, nullptr, &uvChannel) == AI_SUCCESS)
        {
            std::cout << "[processMesh] DIFFUSE uses UV channel: " << uvChannel << std::endl;
        }
        else
        {
            uvChannel = 0;
        }
    }

    std::cout << "[processMesh] UV channels available: " << mesh->GetNumUVChannels() << std::endl;

    bool hasUV =
        (uvChannel < AI_MAX_NUMBER_OF_TEXTURECOORDS) &&
        (mesh->mTextureCoords[uvChannel] != nullptr);

    std::cout << "[processMesh] Using UV channel " << uvChannel
        << " | available: " << (hasUV ? "YES" : "NO") << std::endl;

    float minU = FLT_MAX, minV = FLT_MAX;
    float maxU = -FLT_MAX, maxV = -FLT_MAX;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex{};

        vertex.Position = {
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        };

        if (mesh->HasNormals())
        {
            vertex.Normal = {
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            };
        }

        if (hasUV)
        {
            vertex.TexCoords = {
                mesh->mTextureCoords[uvChannel][i].x,
                mesh->mTextureCoords[uvChannel][i].y
            };

            minU = std::min(minU, vertex.TexCoords.x);
            minV = std::min(minV, vertex.TexCoords.y);
            maxU = std::max(maxU, vertex.TexCoords.x);
            maxV = std::max(maxV, vertex.TexCoords.y);

            if (i < 5)
            {
                std::cout << "[processMesh] UV[" << i << "] = ("
                    << vertex.TexCoords.x << ", "
                    << vertex.TexCoords.y << ")"
                    << std::endl;
            }
        }
        else
        {
            vertex.TexCoords = { 0.0f, 0.0f };
        }

        if (mesh->HasTangentsAndBitangents())
        {
            vertex.Tangent = {
                mesh->mTangents[i].x,
                mesh->mTangents[i].y,
                mesh->mTangents[i].z
            };
            vertex.Bitangent = {
                mesh->mBitangents[i].x,
                mesh->mBitangents[i].y,
                mesh->mBitangents[i].z
            };
        }

        for (int j = 0; j < MAX_BONE_INFLUENCE; ++j)
        {
            vertex.m_BoneIDs[j] = 0;
            vertex.m_Weights[j] = 0.0f;
        }

        vertices.push_back(vertex);
    }

    if (hasUV)
    {
        std::cout << "[processMesh] UV range: "
            << "U(" << minU << " .. " << maxU << "), "
            << "V(" << minV << " .. " << maxV << ")"
            << std::endl;
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        std::cout << "[processMesh] Diffuse count  = "
            << material->GetTextureCount(aiTextureType_DIFFUSE) << std::endl;
        std::cout << "[processMesh] Specular count = "
            << material->GetTextureCount(aiTextureType_SPECULAR) << std::endl;
        std::cout << "[processMesh] BaseColor count = "
            << material->GetTextureCount(aiTextureType_BASE_COLOR) << std::endl;

        std::vector<Texture> baseColorMaps =
            loadMaterialTextures(material, aiTextureType_BASE_COLOR, "texture_diffuse");

        if (baseColorMaps.empty())
        {
            baseColorMaps =
                loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        }

        textures.insert(textures.end(), baseColorMaps.begin(), baseColorMaps.end());

        std::vector<Texture> specularMaps =
            loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    }

    std::cout << "[processMesh] Final texture count in mesh = "
        << textures.size() << std::endl;

    return Mesh(vertices, indices, textures);
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName)
{
    std::vector<Texture> textures;

    unsigned int count = mat->GetTextureCount(type);
    std::cout << "[loadMaterialTextures] type = " << typeName
        << " | count = " << count
        << std::endl;

    for (unsigned int i = 0; i < count; i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);

        std::cout << "[loadMaterialTextures] Requested "
            << typeName << " | path = " << str.C_Str()
            << std::endl;

        if (str.length == 0)
        {
            std::cout << "[loadMaterialTextures] Skip empty texture path" << std::endl;
            continue;
        }

        bool skip = false;
        for (unsigned int j = 0; j < textures_loaded.size(); j++)
        {
            if (textures_loaded[j].path == str.C_Str() &&
                textures_loaded[j].type == typeName)
            {
                std::cout << "[loadMaterialTextures] Reusing already loaded texture: "
                    << str.C_Str() << std::endl;

                textures.push_back(textures_loaded[j]);
                skip = true;
                break;
            }
        }

        if (skip)
            continue;

        Texture texture;
        texture.type = typeName;
        texture.path = str.C_Str();
        texture.id = 0;

        if (str.C_Str()[0] == '*')
        {
            int texIndex = std::atoi(str.C_Str() + 1);

            std::cout << "[loadMaterialTextures] Embedded texture index = "
                << texIndex << std::endl;

            if (scene && texIndex < static_cast<int>(scene->mNumTextures))
                texture.id = TextureFromMemory(scene->mTextures[texIndex]);
            else
                std::cerr << "[loadMaterialTextures] Invalid embedded texture index: "
                << texIndex << std::endl;
        }
        else
        {
            texture.id = TextureFromFile(str.C_Str(), directory, false);
        }

        if (texture.id == 0)
        {
            std::cout << "[loadMaterialTextures] Skip invalid texture"
                << " | type = " << texture.type
                << " | path = " << texture.path
                << std::endl;
            continue;
        }

        std::cout << "[loadMaterialTextures] Loaded result"
            << " | id = " << texture.id
            << " | type = " << texture.type
            << " | path = " << texture.path
            << std::endl;

        textures.push_back(texture);
        textures_loaded.push_back(texture);
    }

    return textures;
}

void Model::setRotationMatrix(const glm::mat4& rot)
{
    rotationMatrix = rot;
}

void Model::selectMesh(int index)
{
    if (index < 0 || index >= static_cast<int>(meshes.size()))
    {
        selectedMeshIndex = -1;
        return;
    }

    selectedMeshIndex = index;
}

void Model::showAllMeshes()
{
    for (size_t i = 0; i < meshVisible.size(); ++i)
        meshVisible[i] = true;
}

void Model::isolateSelectedMesh()
{
    if (selectedMeshIndex < 0 || selectedMeshIndex >= static_cast<int>(meshVisible.size()))
    {
        showAllMeshes();
        return;
    }

    for (size_t i = 0; i < meshVisible.size(); ++i)
        meshVisible[i] = (static_cast<int>(i) == selectedMeshIndex);
}

int Model::getSelectedMesh() const
{
    return selectedMeshIndex;
}

Mesh& Model::getMesh(int index)
{
    return meshes[index];
}

float Model::getBoundingRadius() const
{
    return glm::length(getSize()) * 0.5f;
}