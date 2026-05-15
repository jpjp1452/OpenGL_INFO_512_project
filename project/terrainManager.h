#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <functional>
#include <tuple>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <limits>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glm/gtc/noise.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "shader.h"

#define HEIGHT 10.0f
#define WIDTH 256


#ifndef PATH_TO_TEXTURE
#define PATH_TO_TEXTURE "Textures"
#endif

float simpleQuad[] = {
    // positions     // texture coords
    -1,  0, -1,  0, 0,
    -1,  0,  1,  0, 1,
     1,  0,  1,  1, 1,
     1,  0, -1,  1, 0,
};

class terrainManager
{
public:
    GLuint VBO, VAO;
    std::vector<GLuint> textureIDs;
    float scale = 1.0f;
    std::vector<glm::vec2> planeCoords;
    std::vector<glm::mat4> modelMatrices;

    GLuint brickTextureID,brickBumpTextureID;

    terrainManager(Shader &shader)
    {
        std::cout << "Initializing terrain..." << std::endl;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(simpleQuad), simpleQuad, GL_STATIC_DRAW);

        // terrain.vert uses explicit locations 0 (position) and 1 (tex_coord)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, false, 5 * sizeof(float), (void *)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, false, 5 * sizeof(float), (void *)(3 * sizeof(float)));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);


        unsigned char *dataC = new unsigned char[WIDTH * WIDTH];
        for (int y = 0; y < WIDTH; y++)
        {
            for (int x = 0; x < WIDTH; x++)
            {
                float value = glm::perlin(glm::vec2(x,y) * 0.01f);
                unsigned char colorValue = static_cast<unsigned char>((value + 1.0f) * 127.5f);
                dataC[y * WIDTH + x] = colorValue;
            }
        }
        
        textureIDs.push_back((GLuint)0);
        glGenTextures(1, &textureIDs[0]);
        glBindTexture(GL_TEXTURE_2D, textureIDs[0]);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, WIDTH, WIDTH, 0, GL_RED, GL_UNSIGNED_BYTE, dataC);
        glGenerateMipmap(GL_TEXTURE_2D);

        delete[] dataC;

        std::cout << "going to load brick"<<std::endl; 
        //color Bricks097_1K-PNG_Color.png
        glGenTextures(1, &brickTextureID);
        glBindTexture(GL_TEXTURE_2D, brickTextureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        int width, height, nrChannels;
        std::cout << "Loading texture: " << PATH_TO_TEXTURE << "/Bricks097_1K-PNG_Color.png" << std::endl;
        stbi_set_flip_vertically_on_load(true);
        // Force RGBA so input buffer format always matches glTexImage2D format.
        unsigned char *data = stbi_load((std::string(PATH_TO_TEXTURE) + "/Bricks097_1K-PNG_Color.png").c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);
        if (data)        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            stbi_image_free(data);
            std::cout << "Texture loaded: " << PATH_TO_TEXTURE << "/Bricks097_1K-PNG_Color.png" << " (" << width << "x" << height << ")" << std::endl;
        }
        else        {
            std::cerr << "Failed to load texture: " << PATH_TO_TEXTURE << "/Bricks097_1K-PNG_Color.png"
                      << " reason: " << stbi_failure_reason() << std::endl;
        }  
        std::cout << "going to load brick bump"<<std::endl;
        //bump Bricks097_1K-PNG_Displacement.png
        glGenTextures(1, &brickBumpTextureID);
        glBindTexture(GL_TEXTURE_2D, brickBumpTextureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Force a single channel for displacement/height data.
        data = stbi_load((std::string(PATH_TO_TEXTURE) + "/Bricks097_1K-PNG_Displacement.png").c_str(), &width, &height, &nrChannels, STBI_grey);
        if (data)        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            stbi_image_free(data);
            std::cout << "Texture loaded: " << PATH_TO_TEXTURE << "/Bricks097_1K-PNG_Displacement.png" << " (" << width << "x" << height << ")" << std::endl;
        }
        else        {
            std::cerr << "Failed to load texture: " << PATH_TO_TEXTURE << "/Bricks097_1K-PNG_Displacement.png"
                      << " reason: " << stbi_failure_reason() << std::endl;
        }   


        

        std::cout << "Texture loaded: procedural_perlin_noise (" << WIDTH << "x" << WIDTH << ")" << std::endl;
    }


    void draw()
    {
        glBindVertexArray(this->VAO);
        glActiveTexture(GL_TEXTURE0); //perlin noise texture
        glBindTexture(GL_TEXTURE_2D, textureIDs[0]);

        glActiveTexture(GL_TEXTURE1); //brick color texture
        glBindTexture(GL_TEXTURE_2D, brickTextureID);
        glActiveTexture(GL_TEXTURE2); //brick bump texture
        glBindTexture(GL_TEXTURE_2D, brickBumpTextureID);

        glPatchParameteri(GL_PATCH_VERTICES, 4);
        glDrawArrays(GL_PATCHES, 0, 4);
    }

};
