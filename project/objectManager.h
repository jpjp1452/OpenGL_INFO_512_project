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

#ifndef PATH_TO_OBJECTS
#define PATH_TO_OBJECTS "Model"
#endif


#ifndef PATH_TO_TEXTURE
#define PATH_TO_TEXTURE "Textures"
#endif

#define MAX_SIZE_T std::numeric_limits<size_t>::max()

#include "shader.h"

//get texture name from mtl file
std::string textureNameInMtl(const std::string &mtlPath)
{
    std::ifstream infile(mtlPath);
    if (!infile.is_open())
    {
        throw std::runtime_error(std::string("Failed to open MTL file: ") + mtlPath);
    }
    std::string line;
    bool foundTexture = false;
    while (std::getline(infile, line))
    {
        std::istringstream iss(line);
        std::string indice;
        iss >> indice;
        if (indice == "map_Kd")
        {
            std::string textureName;
            iss >> textureName;
            infile.close();
            return textureName;
        }
    }
    infile.close();
    throw std::runtime_error("No texture found in MTL file: " + mtlPath);
}


// Split a string by a delimiter and convert the parts to unsigned int, returning them as a tuple
std::tuple<unsigned int, unsigned int, unsigned int> split(const std::string &s, char delimiter)
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
    if (tokens.size() < 3)
    {
        tokens.push_back(MAX_SIZE_T);
    }
    if (tokens.size() != 3)
    {   
        std::cout << "tokens size : " << tokens.size() << std::endl;    
        throw std::runtime_error("Invalid face data: " + s);
    }
    return {tokens[0], tokens[1], tokens[2]};
}


struct Vertex
{
    glm::vec3 Position;
    glm::vec2 Texture;
    glm::vec3 Normal;
};

struct FaceData
{
    std::tuple<unsigned int, unsigned int, unsigned int> f1;
    std::tuple<unsigned int, unsigned int, unsigned int> f2;
    std::tuple<unsigned int, unsigned int, unsigned int> f3;
};


struct IndicesKeyHash
{
    std::size_t operator()(const std::tuple<unsigned int, unsigned int, unsigned int> &key) const
    {
        const std::size_t h1 = std::hash<unsigned int>{}(std::get<0>(key));
        const std::size_t h2 = std::hash<unsigned int>{}(std::get<1>(key));
        const std::size_t h3 = std::hash<unsigned int>{}(std::get<2>(key));
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};


struct VertexNormalCounter
{
    glm::vec3 Normal;
    int count = 0;
};

struct uniformSetters
{
    std::vector<std::pair<std::string, float>> setFloats;
    std::vector<std::pair<std::string, GLint>> setIntegers;
    std::vector<std::pair<std::string, glm::vec3>> setVec3;
    std::vector<std::pair<std::string, glm::mat4>> setMat4;
};

class Object
{

public:
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> textures;
    std::vector<glm::vec3> normals;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    bool hasTexture = false;
    std::string texturePath;
    int numVertices;
    GLuint VBO, VAO, EBO;
    GLuint textureID = 0;


    Object() = default;

    Object(const char *path)
    {   
        //print path
        std::cout << "Loading model from path: " << path << std::endl;


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
                hasTexture = true;
                iss >> mtlPath;
                mtlPath = "/" + mtlPath;
                mtlPath = PATH_TO_OBJECTS + mtlPath;
                //adding png extension
                texturePath = textureNameInMtl(mtlPath);
                texturePath = "/" + texturePath;
                texturePath = PATH_TO_TEXTURE + texturePath;
            }
        }
        std::cout << "OBJ file parsed successfully." << std::endl;
        std::cout << facesIndices.size() << " faces found." << std::endl;
        std::cout << positions.size() << " vertices found." << std::endl;
        std::cout << normals.size() << " normals found." << std::endl;
        std::cout << textures.size() << " texture coordinates found." << std::endl;

        if (normals.empty())
        {
            // calculate normals if not present in the OBJ file
            std::cout << "Calculating normals..." << std::endl;
            for (const auto &face : facesIndices)
            {
                size_t p1 = std::get<0>(face.f1);
                size_t p2 = std::get<0>(face.f2);
                size_t p3 = std::get<0>(face.f3);

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
                    vertexIndexToNormalMap[p1] = normals.size();
                }
                if (vertexIndexToNormalMap.find(p2) == vertexIndexToNormalMap.end())
                {
                    normals.push_back(glm::vec3(0.0f));
                    vertexIndexToNormalMap[p2] = normals.size();
                }
                if (vertexIndexToNormalMap.find(p3) == vertexIndexToNormalMap.end())
                {
                    normals.push_back(glm::vec3(0.0f));
                    vertexIndexToNormalMap[p3] = normals.size();
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
                size_t p1 = std::get<0>(face.f1);
                size_t p2 = std::get<0>(face.f2);
                size_t p3 = std::get<0>(face.f3);

                face.f1 = std::make_tuple(std::get<0>(face.f1), std::get<1>(face.f1), vertexIndexToNormalMap[p1]);
                face.f2 = std::make_tuple(std::get<0>(face.f2), std::get<1>(face.f2), vertexIndexToNormalMap[p2]);
                face.f3 = std::make_tuple(std::get<0>(face.f3), std::get<1>(face.f3), vertexIndexToNormalMap[p3]);
                
            }
            std::cout << "Normals calculated successfully." << std::endl;
        }

        infile.close();
        if (textures.empty() && hasTexture)
        {
            throw std::runtime_error("No texture coordinates found in OBJ file");
        }

        // Create vertices and indices array for indexed drawing
        unsigned uniqueVerticesCount = 0;
        using indicesKey = std::tuple<unsigned int, unsigned int, unsigned int>;
        std::unordered_map<indicesKey, unsigned int, IndicesKeyHash> uniqueVerticesMap;
        for (const auto &face : facesIndices)
        {
            std::tuple<unsigned int, unsigned int, unsigned int> verticesIndeces[3] = {face.f1, face.f2, face.f3};
            for (const auto &indicesKey : verticesIndeces)
            {
                if (uniqueVerticesMap.find(indicesKey) == uniqueVerticesMap.end())
                {
                    Vertex vertex;
                    vertex.Position = positions.at(std::get<0>(indicesKey) - 1);
                    if(hasTexture){
                        vertex.Texture = textures.at(std::get<1>(indicesKey) - 1);
                    } else {
                        vertex.Texture = glm::vec2(0.0f, 0.0f);
                    }
                    vertex.Normal = normals.at(std::get<2>(indicesKey) - 1);
                    vertices.push_back(vertex);
                    uniqueVerticesMap[indicesKey] = uniqueVerticesCount++;
                }
                indices.push_back(uniqueVerticesMap[indicesKey]);
            }
        }
        numVertices = vertices.size();
        printf("Model loaded with %d vertices\n", numVertices);
    }

    void makeObject(Shader shader)
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
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        //VBO
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 8 * numVertices, data, GL_STATIC_DRAW);

        //EBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), indices.data(), GL_STATIC_DRAW);

        auto att_pos = glGetAttribLocation(shader.ID, "position");
        glEnableVertexAttribArray(att_pos);
        glVertexAttribPointer(att_pos, 3, GL_FLOAT, false, 8 * sizeof(float), (void *)0);
        if (hasTexture)
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

        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    }

    // Charger une texture PNG
    bool loadTexture(const char *texturePath)
    {
        int width, height, nrChannels;
        stbi_set_flip_vertically_on_load(true); 
        unsigned char *data = stbi_load(texturePath, &width, &height, &nrChannels, 0);

        if (!data)
        {
            std::cerr << "Failed to load texture: " << texturePath << std::endl;
            hasTexture = false;
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

struct ObjectsData
{
    Object object;
    Shader shader;
    std::vector<glm::mat4> modelMatrices;
};

class ObjectManager
{
public:
    std::unordered_map<std::string, ObjectsData> objects;

    ObjectManager() = default;

    void addObject(const std::string &name, const char *objPath, Shader shader)
    {   
        auto it = objects.find(name);
        if (it != objects.end())
        {
            it->second.modelMatrices.push_back(glm::mat4(1.0f));
            return;
        }
        Object obj(objPath);
        obj.makeObject(shader);
        if (obj.hasTexture)
        {
            if (!obj.loadTexture(obj.texturePath.c_str()))
            {
                std::cout << "Failed to load texture for object " << name << ", the object will be rendered without texture" << std::endl;
            }
        }
        objects.emplace(name, ObjectsData{obj, shader, {glm::mat4(1.0f)}});
    }
    //remove one model matrix of the object
    void removeOneObject(const std::string &name)
    {
        auto it = objects.find(name);
        if (it == objects.end())
        {
            std::cout << "Object with name " << name << " doesn't exist" << std::endl;
            return;
        }
        if (it->second.modelMatrices.size() >= 1)
        {
            it->second.modelMatrices.pop_back();
        }
        else
        {
            std::cout << "No model matrix to remove for object " << name << std::endl;
        }
    }





    void drawObject(const std::string &name, uniformSetters &setters)
    {
        auto it = objects.find(name);
        if (it == objects.end())
        {
            std::cout << "Object with name " << name << " doesn't exist" << std::endl;
            return;
        }

        ObjectsData &data = it->second;
        data.shader.use();

        // Set default uniforms for texture shader

        if (data.object.hasTexture){
            data.shader.setInteger("useTexture", 1);
            data.shader.setInteger("texture1", 0);
        }



        // Apply custom uniforms from setters
        for (const auto &set : setters.setIntegers)
            data.shader.setInteger(set.first.c_str(), set.second);
        for (const auto &set : setters.setFloats)
            data.shader.setFloat(set.first.c_str(), set.second);
        for (const auto &set : setters.setVec3)
            data.shader.setVector3f(set.first.c_str(), set.second);
        for (const auto &set : setters.setMat4)
            data.shader.setMatrix4(set.first.c_str(), set.second);

        if (data.modelMatrices.empty())
            return;

        glBindVertexArray(data.object.VAO);
        if (data.object.textureID != 0)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, data.object.textureID);
        }

        // Draw all model instances
        for (const auto &model : data.modelMatrices)
        {
            data.shader.setMatrix4("M", model);
            data.shader.setMatrix4("itM", glm::transpose(glm::inverse(model)));
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(data.object.indices.size()), GL_UNSIGNED_INT, 0);
        }

        glBindVertexArray(0);
    }


    //destroy all gpu data
    ~ObjectManager()
    {
        for (auto &pair : objects)        {
            Object &obj = pair.second.object;
            glDeleteVertexArrays(1, &obj.VAO);
            glDeleteBuffers(1, &obj.VBO);
            glDeleteBuffers(1, &obj.EBO);
            if (obj.textureID != 0)            {
                glDeleteTextures(1, &obj.textureID);
            }
        }
    }


};

#endif