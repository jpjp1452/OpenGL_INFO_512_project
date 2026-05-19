#pragma once
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

#define TERRAIN_HEIGHT 10.0f
#define TERRAIN_RESOLUTION 64
#define TERRAIN_INTENSITY_CHANGE 0.04f
#define TERRAIN_FLAT_ZONE_SIZE (TERRAIN_RESOLUTION * 0.5f)
#define TERRAIN_TRANSITION_SIZE 1.15f
#define TERRAIN_FLAT_HEIGHT 127.5f
#define SOIL_TEXTURE "/Ground029_1K-PNG_Color.png"
#define SOIL_BUMP_TEXTURE "/Ground029_1K-PNG_Displacement.png"

#ifndef PATH_TO_TEXTURE
#define PATH_TO_TEXTURE "Textures"
#endif

#ifndef PATH_TO_SHADERS
#define PATH_TO_SHADERS "shaders"
#endif

float simpleQuad[] = {
    // positions     // texture coords
    -1,
    0,
    -1,
    0,
    0,
    -1,
    0,
    1,
    0,
    1,
    1,
    0,
    1,
    1,
    1,
    1,
    0,
    -1,
    1,
    0,
};

struct XY
{
    int x;
    int y;
};

class terrainManager
{
public:
    GLuint VBO, VAO;
    std::vector<GLuint> textureIDs;
    float scale = 30.0f;
    std::vector<glm::vec2> planeCoords;
    std::vector<glm::mat4> modelMatrices;
    GLuint terrainTextureID, terrainBumpTextureID;
    unsigned char *currentTerrainData[3][3];
    GLuint textureID_terrain[3][3];
    glm::mat4 model = glm::mat4(1.0f);
    Shader terrainShader;
    int currentChunkX = 0;
    int currentChunkY = 0;
    float floatCurrentChunkX = 0.0f;
    float floatCurrentChunkY = 0.0f;
    int previousChunkX = 0;
    int previousChunkY = 0;
    float heightUnderCamera = 0.0f;
    float previousHeightUnderCamera = 0.0f;

    XY chunkOffsets[3][3] = {
        {{-1, -1}, {0, -1}, {1, -1}},
        {{-1, 0}, {0, 0}, {1, 0}},
        {{-1, 1}, {0, 1}, {1, 1}}};

    void printChunkOffsets()
    {
        std::cout << "Chunk Offsets:" << std::endl;
        for (int y = 0; y < 3; y++)
        {
            for (int x = 0; x < 3; x++)
            {
                std::cout << "(" << chunkOffsets[y][x].y << ", " << chunkOffsets[y][x].x << ") ";
            }
            std::cout << std::endl;
        }
    }

    void saveHeightmapToPNG(const std::string &filename)
    {
        const int bigWidth = TERRAIN_RESOLUTION * 3;
        const int bigHeight = TERRAIN_RESOLUTION * 3;

        // RGB
        std::vector<unsigned char> grayData(bigWidth * bigHeight * 3, 0);

        for (int cy = 0; cy < 3; cy++)
        {
            for (int cx = 0; cx < 3; cx++)
            {
                int offsetX = cx * TERRAIN_RESOLUTION;
                int offsetY = cy * TERRAIN_RESOLUTION;

                for (int y = 0; y < TERRAIN_RESOLUTION; y++)
                {
                    for (int x = 0; x < TERRAIN_RESOLUTION; x++)
                    {
                        int bigX = offsetX + x;
                        int bigY = offsetY + y;

                        int bigIndex =
                            (bigY * bigWidth + bigX) * 3;

                        int smallIndex =
                            y * TERRAIN_RESOLUTION + x;

                        unsigned char value =
                            currentTerrainData[cy][cx][smallIndex];

                        // RGB grayscale
                        grayData[bigIndex + 0] = value;
                        grayData[bigIndex + 1] = value;
                        grayData[bigIndex + 2] = value;
                    }
                }
            }
        }

        stbi_write_png(
            filename.c_str(),
            bigWidth,
            bigHeight,
            3,
            grayData.data(),
            bigWidth * 3);
    }
    float home_made_perlin(float x, float y)
    {

        float value = std::sin(x * 0.1f) * 10 + std::cos(y * 0.1f) * 40;
        value = (value * value) * std::tanh(value * 0.1f);

        return std::sin(value + x + y);
    }

    void generate_terrain_heightmap(int chunk_x, int chunk_y, unsigned char *data)
    {

        const float maxDistanceSquarred = (TERRAIN_FLAT_ZONE_SIZE * TERRAIN_FLAT_ZONE_SIZE);
        const float transitionDistanceSquarred = maxDistanceSquarred * TERRAIN_TRANSITION_SIZE;

        std::cout << "Generating heightmap for chunk (" << chunk_x << ", " << chunk_y << ")" << std::endl;
        for (int y = 0; y < TERRAIN_RESOLUTION; y++)
        {
            int y_local = y;
            int chunk_y_local = chunk_y;
            if (y == TERRAIN_RESOLUTION - 1)
            {
                y_local = 0;
                chunk_y_local = chunk_y + 1;
            }

            for (int x = 0; x < TERRAIN_RESOLUTION; x++)
            {
                int x_local = x;
                int chunk_x_local = chunk_x;
                if (x == TERRAIN_RESOLUTION - 1)
                {
                    x_local = 0;
                    chunk_x_local = chunk_x + 1;
                }

                float worldX = chunk_x_local * (TERRAIN_RESOLUTION) + x_local;
                float worldY = chunk_y_local * (TERRAIN_RESOLUTION) + y_local;

                float noise = glm::perlin(glm::vec2(worldX, worldY) * TERRAIN_INTENSITY_CHANGE);
                float noiseMapped = (noise + 1.0f) * 127.5f;
                float finalValue = noiseMapped;

                float dx = worldX - TERRAIN_RESOLUTION / 2.0f;
                float dy = worldY - TERRAIN_RESOLUTION / 2.0f;
                float distanceFromCenterSquarred = (dx * dx) + (dy * dy);

                if (distanceFromCenterSquarred < maxDistanceSquarred)
                {
                    finalValue = TERRAIN_FLAT_HEIGHT; // Flat zone
                }
                else if (distanceFromCenterSquarred < transitionDistanceSquarred)
                {
                    float t = (distanceFromCenterSquarred - maxDistanceSquarred) / transitionDistanceSquarred;
                    finalValue = glm::mix(TERRAIN_FLAT_HEIGHT, noiseMapped, t);
                }
                finalValue = glm::clamp(finalValue, 0.0f, 255.0f);
                data[y * TERRAIN_RESOLUTION + x] = static_cast<unsigned char>(finalValue);
            }
        }
    }

    terrainManager(std::string terrainShaderPrefix)
    {

        // Create terrain shader with tessellation
        ShaderFilePaths terrainShaderPaths;
        terrainShaderPaths.addVertexShader(PATH_TO_SHADERS "/" + terrainShaderPrefix + ".vert");
        terrainShaderPaths.addTessellationControlShader(PATH_TO_SHADERS "/" + terrainShaderPrefix + ".tcs");
        terrainShaderPaths.addTessellationEvaluationShader(PATH_TO_SHADERS "/" + terrainShaderPrefix + ".tes");
        terrainShaderPaths.addFragmentShader(PATH_TO_SHADERS "/" + terrainShaderPrefix + ".frag");
        this->terrainShader = Shader(terrainShaderPaths);

        terrainShader.use();
        terrainShader.setInteger("heightMap", 0);
        terrainShader.setInteger("textureBrickColor", 1);
        terrainShader.setInteger("textureBrickBump", 2);
        terrainShader.setFloat("scale", scale);
        terrainShader.setFloat("HEIGHT_SCALE", TERRAIN_HEIGHT);

        model = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f));
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(simpleQuad), simpleQuad, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, false, 5 * sizeof(float), (void *)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, false, 5 * sizeof(float), (void *)(3 * sizeof(float)));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // Create textures for all chunks
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
            {

                currentTerrainData[i][j] = new unsigned char[TERRAIN_RESOLUTION * TERRAIN_RESOLUTION];
                generate_terrain_heightmap(j - 1, i - 1, currentTerrainData[i][j]);
                glGenTextures(1, &textureID_terrain[i][j]);
                glBindTexture(GL_TEXTURE_2D, textureID_terrain[i][j]);
                // Use clamp_to_edge and nearest filtering for heightmap to avoid seams
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, TERRAIN_RESOLUTION, TERRAIN_RESOLUTION, 0, GL_RED, GL_UNSIGNED_BYTE, currentTerrainData[i][j]);
                glGenerateMipmap(GL_TEXTURE_2D);
            }
        }

        std::cout << "going to load brick" << std::endl;
        // color Bricks097_1K-PNG_Color.png
        glGenTextures(1, &terrainTextureID);
        glBindTexture(GL_TEXTURE_2D, terrainTextureID);
        // For color texture keep repeat and linear filtering
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        int width, height, nrChannels;
        std::cout << "Loading texture: " << PATH_TO_TEXTURE << SOIL_TEXTURE << std::endl;
        stbi_set_flip_vertically_on_load(true);
        // Force RGBA so input buffer format always matches glTexImage2D format.
        unsigned char *data = stbi_load((std::string(PATH_TO_TEXTURE) + SOIL_TEXTURE).c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            stbi_image_free(data);
            std::cout << "Texture loaded: " << PATH_TO_TEXTURE << SOIL_TEXTURE << " (" << width << "x" << height << ")" << std::endl;
        }
        else
        {
            std::cerr << "Failed to load texture: " << PATH_TO_TEXTURE << SOIL_TEXTURE
                      << " reason: " << stbi_failure_reason() << std::endl;
        }
        std::cout << "going to load terrain bump" << std::endl;
        // bump Bricks097_1K-PNG_Displacement.png
        glGenTextures(1, &terrainBumpTextureID);
        glBindTexture(GL_TEXTURE_2D, terrainBumpTextureID);
        // Use clamp_to_edge and nearest filtering for bump/displacement map to avoid seams
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        // Force a single channel for displacement/height data.
        data = stbi_load((std::string(PATH_TO_TEXTURE) + SOIL_BUMP_TEXTURE).c_str(), &width, &height, &nrChannels, STBI_grey);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            stbi_image_free(data);
            std::cout << "Texture loaded: " << PATH_TO_TEXTURE << SOIL_BUMP_TEXTURE << " (" << width << "x" << height << ")" << std::endl;
        }
        else
        {
            std::cerr << "Failed to load texture: " << PATH_TO_TEXTURE << SOIL_BUMP_TEXTURE
                      << " reason: " << stbi_failure_reason() << std::endl;
        }

        std::cout << "Texture loaded: procedural_perlin_noise (" << TERRAIN_RESOLUTION << "x" << TERRAIN_RESOLUTION << ")" << std::endl;
    }

    void moveChunkDown(bool moveDown)
    {
        unsigned char *tmpTerrain;
        GLuint tmpTextureID;
        std::cout << "move Down: " << moveDown << std::endl;
        if (moveDown)
        {

            for (int j = 0; j < 3; j++)
            {

                tmpTerrain = currentTerrainData[2][j];
                tmpTextureID = textureID_terrain[2][j];

                chunkOffsets[2][j] = chunkOffsets[1][j];
                chunkOffsets[1][j] = chunkOffsets[0][j];
                currentTerrainData[2][j] = currentTerrainData[1][j];
                currentTerrainData[1][j] = currentTerrainData[0][j];
                textureID_terrain[2][j] = textureID_terrain[1][j];
                textureID_terrain[1][j] = textureID_terrain[0][j];
                int newChunkY = currentChunkY - 1;
                int newChunkX = currentChunkX - 1 + j;

                std::cout << "new chunk: (" << newChunkY << ", " << newChunkX << ")" << std::endl;
                generate_terrain_heightmap(newChunkX, newChunkY, tmpTerrain);
                chunkOffsets[0][j] = {newChunkX, newChunkY};
                currentTerrainData[0][j] = tmpTerrain;
                textureID_terrain[0][j] = tmpTextureID;
                glBindTexture(GL_TEXTURE_2D, textureID_terrain[0][j]);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TERRAIN_RESOLUTION, TERRAIN_RESOLUTION, GL_RED, GL_UNSIGNED_BYTE, currentTerrainData[0][j]);
            }
        }
        else
        {
            for (int j = 0; j < 3; j++)
            {

                tmpTerrain = currentTerrainData[0][j];
                tmpTextureID = textureID_terrain[0][j];

                chunkOffsets[0][j] = chunkOffsets[1][j];
                chunkOffsets[1][j] = chunkOffsets[2][j];
                currentTerrainData[0][j] = currentTerrainData[1][j];
                currentTerrainData[1][j] = currentTerrainData[2][j];
                textureID_terrain[0][j] = textureID_terrain[1][j];
                textureID_terrain[1][j] = textureID_terrain[2][j];
                int newChunkX = currentChunkX - 1 + j;
                int newChunkY = currentChunkY + 1;

                std::cout << "new chunk: (" << newChunkY << ", " << newChunkX << ")" << std::endl;
                generate_terrain_heightmap(newChunkX, newChunkY, tmpTerrain);

                chunkOffsets[2][j] = {newChunkX, newChunkY};
                currentTerrainData[2][j] = tmpTerrain;
                textureID_terrain[2][j] = tmpTextureID;
                glBindTexture(GL_TEXTURE_2D, textureID_terrain[2][j]);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TERRAIN_RESOLUTION, TERRAIN_RESOLUTION, GL_RED, GL_UNSIGNED_BYTE, currentTerrainData[2][j]);
            }
        }
    }
    void moveChunkRight(bool moveRight)
    {
        unsigned char *tmpTerrain;
        GLuint tmpTextureID;
        std::cout << "move Right: " << moveRight << std::endl;

        if (moveRight)
        {
            for (int i = 0; i < 3; i++)
            {
                tmpTerrain = currentTerrainData[i][2];
                tmpTextureID = textureID_terrain[i][2];

                currentTerrainData[i][2] = currentTerrainData[i][1];
                currentTerrainData[i][1] = currentTerrainData[i][0];
                textureID_terrain[i][2] = textureID_terrain[i][1];
                textureID_terrain[i][1] = textureID_terrain[i][0];
                chunkOffsets[i][2] = chunkOffsets[i][1];
                chunkOffsets[i][1] = chunkOffsets[i][0];
                int newChunkX = currentChunkX - 1;
                int newChunkY = currentChunkY - 1 + i;

                std::cout << "new chunk: (" << newChunkY << ", " << newChunkX << ")" << std::endl;
                generate_terrain_heightmap(newChunkX, newChunkY, tmpTerrain);
                chunkOffsets[i][0] = {newChunkX, newChunkY};
                currentTerrainData[i][0] = tmpTerrain;
                textureID_terrain[i][0] = tmpTextureID;
                glBindTexture(GL_TEXTURE_2D, textureID_terrain[i][0]);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TERRAIN_RESOLUTION, TERRAIN_RESOLUTION, GL_RED, GL_UNSIGNED_BYTE, currentTerrainData[i][0]);
            }
        }
        else
        {
            for (int i = 0; i < 3; i++)
            {

                tmpTerrain = currentTerrainData[i][0];
                tmpTextureID = textureID_terrain[i][0];

                chunkOffsets[i][0] = chunkOffsets[i][1];
                chunkOffsets[i][1] = chunkOffsets[i][2];
                currentTerrainData[i][0] = currentTerrainData[i][1];
                currentTerrainData[i][1] = currentTerrainData[i][2];
                textureID_terrain[i][0] = textureID_terrain[i][1];
                textureID_terrain[i][1] = textureID_terrain[i][2];
                textureID_terrain[i][2] = tmpTextureID;
                int newChunkX = currentChunkX + 1;
                int newChunkY = currentChunkY - 1 + i;

                std::cout << "new chunk: (" << newChunkY << ", " << newChunkX << ")" << std::endl;
                generate_terrain_heightmap(newChunkX, newChunkY, tmpTerrain);
                chunkOffsets[i][2] = {newChunkX, newChunkY};
                currentTerrainData[i][2] = tmpTerrain;
                textureID_terrain[i][2] = tmpTextureID;
                glBindTexture(GL_TEXTURE_2D, textureID_terrain[i][2]);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TERRAIN_RESOLUTION, TERRAIN_RESOLUTION, GL_RED, GL_UNSIGNED_BYTE, currentTerrainData[i][2]);
            }
        }
    }

    float sampleHeightmap(int chunkX, int chunkY, int indexX, int indexY)
    {

        if (chunkX < 0 || chunkX > 2 || chunkY < 0 || chunkY > 2)
        {
            return (127.5 / 255.0f - 0.5f) * TERRAIN_HEIGHT;
        }
        return (currentTerrainData[chunkY][chunkX][indexY * TERRAIN_RESOLUTION + indexX] / 255.0f - 0.5f) * TERRAIN_HEIGHT;
    }

    float terrainHeightAt(glm::vec3 &worldPos)
    {
        float worldX = worldPos.x;
        float worldY = worldPos.z;
        float signX = (worldX >= 0) ? 1.0f : -1.0f;
        float signY = (worldY >= 0) ? 1.0f : -1.0f;
        float floatChunkX = (worldX + scale * signX) / (2 * scale);
        float floatChunkY = (worldY + scale * signY) / (2 * scale);
        int chunkX = static_cast<int>(floatChunkX);
        int chunkY = static_cast<int>(floatChunkY);
        float decimalChunkX = floatChunkX - chunkX;
        float decimalChunkY = floatChunkY - chunkY;

        int indexX;
        int indexY;
        int indexHeightX[2];
        int offsetXchunk[2];
        int indexHeightY[2];
        if (decimalChunkX > 0.0f)
        {
            indexX = static_cast<int>(decimalChunkX * TERRAIN_RESOLUTION);
            indexHeightX[0] = static_cast<int>(floor(decimalChunkX * TERRAIN_RESOLUTION));
            indexHeightX[1] = static_cast<int>(ceil(decimalChunkX * TERRAIN_RESOLUTION));
        }
        else
        {
            indexX = static_cast<int>((1.0f + decimalChunkX) * TERRAIN_RESOLUTION);
            indexHeightX[0] = static_cast<int>(ceil((1.0f + decimalChunkX) * TERRAIN_RESOLUTION));
            indexHeightX[1] = static_cast<int>(floor((1.0f + decimalChunkX) * TERRAIN_RESOLUTION));
            decimalChunkX += 1.0f; // for bilinear interpolation
        }
        if (decimalChunkY > 0.0f)
        {
            indexY = static_cast<int>(decimalChunkY * TERRAIN_RESOLUTION);
            indexHeightY[0] = static_cast<int>(floor(decimalChunkY * TERRAIN_RESOLUTION));
            indexHeightY[1] = static_cast<int>(ceil(decimalChunkY * TERRAIN_RESOLUTION));
        }
        else
        {
            indexY = static_cast<int>((1.0f + decimalChunkY) * TERRAIN_RESOLUTION);
            indexHeightY[0] = static_cast<int>(ceil((1.0f + decimalChunkY) * TERRAIN_RESOLUTION));
            indexHeightY[1] = static_cast<int>(floor((1.0f + decimalChunkY) * TERRAIN_RESOLUTION));
            decimalChunkY += 1.0f; // for bilinear interpolation
        }

        // std::cout << "chunkX: " << chunkX << ", chunkY: " << chunkY << std::endl;
        // std::cout << "decimalChunkX: " << decimalChunkX << ", decimalChunkY: " << decimalChunkY << std::endl;
        // std::cout << "indexX: " << indexX << ", indexY: " << indexY << std::endl;
        // std::cout << "currentChunkX: " << currentChunkX << ", currentChunkY: " << currentChunkY << std::endl;

        float avgHeight = 0.0f;
        int count = 0;
        const int sampleRadius = 2;

        int offsetXchunk_center = chunkX - currentChunkX;
        int offsetYchunk_center = chunkY - currentChunkY;
        // std::cout << "offsetXchunk_center: " << offsetXchunk_center << ", offsetYchunk_center: " << offsetYchunk_center << std::endl;
        if (abs(offsetXchunk_center) > 1 || abs(offsetYchunk_center) > 1)
        {
            return (127.5 / 255.0f - 0.5f) * TERRAIN_HEIGHT;
        }

        // bilinear interpolation for more accurate height under camera
        float heighMatrix[2][2];
        for (int i = 0; i < 2; i++)
        {
            for (int j = 0; j < 2; j++)
            {
                int sampleX = indexHeightX[j];
                int sampleY = indexHeightY[i];
                int chunkOffsetX = 0;
                int chunkOffsetY = 0;
                if (sampleX < 0)
                {
                    chunkOffsetX = -1;
                    sampleX += TERRAIN_RESOLUTION;
                }
                else if (sampleX >= TERRAIN_RESOLUTION)
                {
                    chunkOffsetX = 1;
                    sampleX -= TERRAIN_RESOLUTION;
                }
                if (sampleY < 0)
                {
                    chunkOffsetY = -1;
                    sampleY += TERRAIN_RESOLUTION;
                }
                else if (sampleY >= TERRAIN_RESOLUTION)
                {
                    chunkOffsetY = 1;
                    sampleY -= TERRAIN_RESOLUTION;
                }
                chunkOffsetX += offsetXchunk_center;
                chunkOffsetY += offsetYchunk_center;
                int samplechunkX = chunkOffsetX + 1;
                int samplechunkY = chunkOffsetY + 1;
                heighMatrix[i][j] = sampleHeightmap(samplechunkX, samplechunkY, sampleX, sampleY);
                // std::cout << "heatMatrix[" << i << "][" << j << "] = " << heighMatrix[i][j] << std::endl;
            }
        }

        float fracX = decimalChunkX * TERRAIN_RESOLUTION - indexHeightX[0];
        float fracY = decimalChunkY * TERRAIN_RESOLUTION - indexHeightY[0];
        float height0 = glm::mix(heighMatrix[0][0], heighMatrix[0][1], fracX);
        float height1 = glm::mix(heighMatrix[1][0], heighMatrix[1][1], fracX);

        float bilinearHeight = glm::mix(height0, height1, abs(fracY));
        // std::cout << "height00: " << height00 << ", height10: " << height10 << ", height01: " << height01 << ", height11: " << height11 << std::endl;
        // std::cout << "fracX: " << fracX << ", fracY: " << fracY << std::endl;
        // std::cout << "bilinearHeight: " << bilinearHeight << std::endl;
        // std::cout << "heightUnderCamera before bilinear: " << heightUnderCamera << std::endl;
        return bilinearHeight;
    }

    float height_under_camera(glm::vec3 &cameraPos)
    {
        return heightUnderCamera;
    }

    void update(glm::vec3 cameraPos)
    {

        previousHeightUnderCamera = heightUnderCamera;
        previousChunkX = currentChunkX;
        previousChunkY = currentChunkY;

        float signX = (cameraPos.x >= 0) ? 1.0f : -1.0f;
        float signY = (cameraPos.z >= 0) ? 1.0f : -1.0f;

        floatCurrentChunkX = (cameraPos.x + scale * signX) / (2 * scale);
        floatCurrentChunkY = (cameraPos.z + scale * signY) / (2 * scale);
        currentChunkX = static_cast<int>(floatCurrentChunkX);
        currentChunkY = static_cast<int>(floatCurrentChunkY);
        int deltaX = currentChunkX - previousChunkX;
        int deltaY = currentChunkY - previousChunkY;
        // std::cout << "" << std::endl;
        // std::cout << "Current chunk: y x: (" << currentChunkY << ", " << currentChunkX << ")" << std::endl;
        // std::cout << "cameraPos: " << cameraPos.x << ", " << cameraPos.y << ", " << cameraPos.z << std::endl;

        // update chunks if camera moved to another chunk
        if (deltaX != 0)
        {
            moveChunkRight(deltaX < 0);
        }
        if (deltaY != 0)
        {
            moveChunkDown(deltaY < 0);
        }

        // count is alteast 1 so no division by zero
        float avgHeight = terrainHeightAt(cameraPos);
        // smoothing with previous height to avoid sudden jumps
        const float smoothFactor = 0.40f;
        avgHeight = glm::mix(previousHeightUnderCamera, avgHeight, smoothFactor);
        heightUnderCamera = avgHeight;
    }

    void draw(glm::mat4 view, glm::mat4 projection, glm::vec3 &cameraPos, glm::vec3 lightPos)
    {

        terrainShader.use();
        terrainShader.setMatrix4("V", view);
        terrainShader.setMatrix4("P", projection);
        terrainShader.setVector3f("u_view_pos", cameraPos);
        terrainShader.setVector3f("lightPos", lightPos);

        glBindVertexArray(VAO);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, terrainTextureID);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, terrainBumpTextureID);

        glPatchParameteri(GL_PATCH_VERTICES, 4);

        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
            {

                float chunkExtent = scale * 2.0f;
                XY offset = chunkOffsets[i][j];
                glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3((offset.x) * chunkExtent, 0.0f, (offset.y) * chunkExtent));
                model = glm::scale(model, glm::vec3(scale, scale, scale));
                terrainShader.setMatrix4("M", model);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, textureID_terrain[i][j]);
                glDrawArrays(GL_PATCHES, 0, 4);
            }
        }
    }
};
