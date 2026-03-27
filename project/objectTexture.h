#ifndef OBJECT_H
#define OBJECT_H

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <limits>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#define MAX_SIZE_T std::numeric_limits<size_t>::max()

#include "shader.h"

/*Principe :
 * On donne le path du fichier -> on lit le fichier
 * 2 �tape
 * 1)load le model -> lit le fichier ligne par ligne
 * liste de position de normal de texture
 * suivant la premi�re lettre : lit les valeur suivant et les met dans un vec puis push dans la bonne liste
 * en gros sotck les data dans une frome de tableau
 */

std::vector<size_t> split(const std::string &s, char delimiter)
{
    size_t start = 0;
    size_t index = 0;
    std::vector<size_t> tokens;

    while (index < s.size())
    {
        if (s[index] == delimiter)
        {
            if (start != index)
            {
                tokens.push_back(std::stoul(s.substr(start, index - start)));
            }
            else
            {
                tokens.push_back(MAX_SIZE_T);
            }
            start = index + 1;
        }
        index++;
    }
    if (start != index)
    {
        tokens.push_back(std::stoul(s.substr(start, index - start)));
    }
    else
    {
        tokens.push_back(MAX_SIZE_T);
    }
    if (tokens.size() <= 3)
    {
        tokens.push_back(MAX_SIZE_T);
    }
    if (tokens.size() != 3)
    {
        throw std::runtime_error("Invalid face data: " + s);
    }
    return tokens;
}

struct Vertex
{
    glm::vec3 Position;
    glm::vec2 Texture;
    glm::vec3 Normal;
};

struct FaceData
{
    std::vector<size_t> f1;
    std::vector<size_t> f2;
    std::vector<size_t> f3;
};

struct Face
{
    glm::vec3 v1, v2, v3;
    glm::vec3 Normal;
};
struct VertexNormalCounter
{
    glm::vec3 Normal;
    int count = 0;
};

class Object
{

private:
public:
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> textures;
    std::vector<glm::vec3> normals;
    std::vector<Vertex> vertices;
    bool hasTexture = false;
    std::string texturePath;
    int numVertices;
    GLuint VBO, VAO;
    GLuint textureID = 0;
    glm::mat4 model = glm::mat4(1.0);

    Object(const char *path)
    {
        if (path == nullptr || path[0] == '\0')
        {
            throw std::invalid_argument("Object: invalid OBJ path (null or empty)");
        }

        std::ifstream infile(path);
        if (!infile.is_open())
        {
            throw std::runtime_error(std::string("Object: failed to open OBJ file: ") + path);
        }
        std::unordered_map<size_t, VertexNormalCounter> vertexNormalMap;
        std::unordered_map<size_t, size_t> vertexIndexToNormalMap;
        std::vector<FaceData> facesIndices;
        std::string mtlPath;

        std::string line;
        while (std::getline(infile, line))
        {
            std::istringstream iss(line);
            std::string indice;
            iss >> indice;
            // std::cout << "indice : " << indice << std::endl;
            if (indice == "v")
            {
                float x, y, z;
                iss >> x >> y >> z;
                positions.push_back(glm::vec3(x, y, z));
            }
            else if (indice == "vn")
            {
                float x, y, z;
                iss >> x >> y >> z;
                normals.push_back(glm::vec3(x, y, z));
            }
            else if (indice == "vt")
            {
                float u, v;
                iss >> u >> v;
                textures.push_back(glm::vec2(u, v));
            }
            else if (indice == "f")
            {
                std::string f1, f2, f3;
                iss >> f1 >> f2 >> f3;
                facesIndices.push_back({split(f1, '/'), split(f2, '/'), split(f3, '/')});
            }
            else if (indice == "mtllib")
            {
                iss >> mtlPath;
            }
            else if (indice == "usemtl")
            {
                hasTexture = true;
                iss >> texturePath;
            }
        }
        std::cout << "OBJ file parsed successfully." << std::endl;
        std::cout << facesIndices.size() << " faces found." << std::endl;
        std::cout << positions.size() << " vertices found." << std::endl;
        std::cout << normals.size() << " normals found." << std::endl;
        std::cout << textures.size() << " texture coordinates found." << std::endl;

        if (normals.empty())
        {
            // calculate normals
            std::cout << "Calculating normals..." << std::endl;
            for (const auto &face : facesIndices)
            {
                size_t p1 = face.f1[0];
                size_t p2 = face.f2[0];
                size_t p3 = face.f3[0];

                glm::vec3 v1 = positions.at(p1 - 1);
                glm::vec3 v2 = positions.at(p2 - 1);
                glm::vec3 v3 = positions.at(p3 - 1);
                glm::vec3 normal = glm::normalize(glm::cross(v2 - v1, v3 - v1));
                vertexNormalMap[p1].Normal += normal;
                vertexNormalMap[p1].count++;
                vertexNormalMap[p2].Normal += normal;
                vertexNormalMap[p2].count++;
                vertexNormalMap[p3].Normal += normal;
                vertexNormalMap[p3].count++;

                // check if p1, p2, p3 already exist in vertexIndexToNormalMap
                if (vertexIndexToNormalMap.find(p1) == vertexIndexToNormalMap.end())
                {
                    normals.push_back(glm::vec3(0.0f));
                    vertexIndexToNormalMap[p1] = vertexIndexToNormalMap.size();
                }
                if (vertexIndexToNormalMap.find(p2) == vertexIndexToNormalMap.end())
                {
                    normals.push_back(glm::vec3(0.0f));
                    vertexIndexToNormalMap[p2] = vertexIndexToNormalMap.size();
                }
                if (vertexIndexToNormalMap.find(p3) == vertexIndexToNormalMap.end())
                {
                    normals.push_back(glm::vec3(0.0f));
                    vertexIndexToNormalMap[p3] = vertexIndexToNormalMap.size();
                }
            }
            for (auto &entry : vertexNormalMap)
            {
                entry.second.Normal /= entry.second.count;
                size_t index = vertexIndexToNormalMap[entry.first] - 1;
                normals[index] = entry.second.Normal;
            }

            for (auto &face : facesIndices)
            {
                size_t p1 = face.f1[0];
                size_t p2 = face.f2[0];
                size_t p3 = face.f3[0];

                face.f1[2] = vertexIndexToNormalMap[p1];
                face.f2[2] = vertexIndexToNormalMap[p2];
                face.f3[2] = vertexIndexToNormalMap[p3];
            }
        }

        infile.close();
        if (textures.empty())
        {
            throw std::runtime_error("Texture specified in OBJ file but no texture coordinates found.");
        }

        for (const auto &face : facesIndices)
        {
            size_t p1 = face.f1[0];
            size_t t1 = face.f1[1];
            size_t n1 = face.f1[2];
            size_t p2 = face.f2[0];
            size_t t2 = face.f2[1];
            size_t n2 = face.f2[2];
            size_t p3 = face.f3[0];
            size_t t3 = face.f3[1];
            size_t n3 = face.f3[2];

            Vertex v1, v2, v3;
            v1.Position = positions.at(p1 - 1);
            v1.Texture = textures.at(t1 - 1);
            v1.Normal = normals.at(n1 - 1);

            v2.Position = positions.at(p2 - 1);
            v2.Texture = textures.at(t2 - 1);
            v2.Normal = normals.at(n2 - 1);

            v3.Position = positions.at(p3 - 1);
            v3.Texture = textures.at(t3 - 1);
            v3.Normal = normals.at(n3 - 1);

            vertices.push_back(v1);
            vertices.push_back(v2);
            vertices.push_back(v3);
        }

        numVertices = vertices.size();
        printf("Model loaded with %d vertices\n", numVertices);
    }

    void makeObject(Shader shader, bool texture = true)
    {
        /* This is a working but not perfect solution, you can improve it if you need/want
         * What happens if you call this function twice on an Model ?
         * What happens when a shader doesn't have a position, tex_coord or normal attribute ?
         */

        float *data = new float[8 * numVertices];
        for (int i = 0; i < numVertices; i++)
        {
            Vertex v = vertices.at(i);
            data[i * 8] = v.Position.x;
            data[i * 8 + 1] = v.Position.y;
            data[i * 8 + 2] = v.Position.z;

            data[i * 8 + 3] = v.Texture.x;
            data[i * 8 + 4] = v.Texture.y;

            data[i * 8 + 5] = v.Normal.x;
            data[i * 8 + 6] = v.Normal.y;
            data[i * 8 + 7] = v.Normal.z;
        }

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        // define VBO and VAO as active buffer and active vertex array
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * numVertices, data, GL_STATIC_DRAW);

        auto att_pos = glGetAttribLocation(shader.ID, "position");
        glEnableVertexAttribArray(att_pos);
        glVertexAttribPointer(att_pos, 3, GL_FLOAT, false, 8 * sizeof(float), (void *)0);

        if (texture)
        {
            auto att_tex = glGetAttribLocation(shader.ID, "tex_coord");
            glEnableVertexAttribArray(att_tex);
            glVertexAttribPointer(att_tex, 2, GL_FLOAT, false, 8 * sizeof(float), (void *)(3 * sizeof(float)));
        }

        auto att_col = glGetAttribLocation(shader.ID, "normal");
        glEnableVertexAttribArray(att_col);
        glVertexAttribPointer(att_col, 3, GL_FLOAT, false, 8 * sizeof(float), (void *)(5 * sizeof(float)));

        // desactive the buffer
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        delete[] data;
    }

    void draw()
    {

        glBindVertexArray(this->VAO);

        // Binder la texture si elle existe
        if (textureID != 0)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureID);
        }

        glDrawArrays(GL_TRIANGLES, 0, numVertices);
    }

    // Charger une texture PNG
    bool loadTexture(const char *texturePath)
    {
        int width, height, nrChannels;
        unsigned char *data = stbi_load(texturePath, &width, &height, &nrChannels, 0);

        if (!data)
        {
            std::cerr << "Failed to load texture: " << texturePath << std::endl;
            return false;
        }

        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        // Configuration des paramètres de texture
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Identifier le format de l'image
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;

        // Charger les données
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);

        std::cout << "Texture loaded: " << texturePath << " (" << width << "x" << height << ")" << std::endl;
        return true;
    }
};
#endif