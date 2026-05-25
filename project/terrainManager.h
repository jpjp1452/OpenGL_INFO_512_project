#pragma once

// =========================================================================
// INCLUDES
// =========================================================================
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
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"

// =========================================================================
// CONSTANTES / MACROS
// =========================================================================
#define TERRAIN_HEIGHT 10.0f
#define TERRAIN_RESOLUTION 64
#define TERRAIN_INTENSITY_CHANGE 0.04f
#define TERRAIN_FLAT_ZONE_SIZE (TERRAIN_RESOLUTION * 0.5f)
#define TERRAIN_TRANSITION_SIZE 1.40f
#define TERRAIN_FLAT_HEIGHT 127.5f
#define SOIL_TEXTURE "/Ground029_1K-PNG_Color.png"
#define SOIL_BUMP_TEXTURE "/Ground029_1K-PNG_Displacement.png"

#ifndef PATH_TO_TEXTURE
#define PATH_TO_TEXTURE "Textures"
#endif

#ifndef PATH_TO_SHADERS
#define PATH_TO_SHADERS "shaders"
#endif

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#endif

// =========================================================================
// DATA TYPES
// =========================================================================
struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    float life;
    float initialLife;
    float scale;
    glm::vec3 rotationAxis;
    float rotationAngle;
};

struct ImpactRing {
    glm::vec3 position;
    float life;
    float initialLife;
    float radius;
};

struct XY {
    int x;
    int y;
};

float simpleQuad[] = {
    // positions     // texture coords
    -1, 0, -1, 0, 0,
    -1, 0, 1, 0, 1,
    1, 0, 1, 1, 1,
    1, 0, -1, 1, 0,
};

// =========================================================================
// terrainManager - public interface
// =========================================================================
class terrainManager {
public:
    // Buffers / textures
    GLuint VBO = 0, VAO = 0;
    std::vector<GLuint> textureIDs;
    GLuint terrainTextureID = 0;
    GLuint terrainBumpTextureID = 0;
    unsigned char* currentTerrainData[3][3];
    GLuint textureID_terrain[3][3];

    // Transforms & shader
    float scale = 30.0f;
    glm::mat4 model = glm::mat4(1.0f);
    Shader terrainShader;

    // Chunks / sampling state
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
        {{-1, 0},  {0, 0},  {1, 0}},
        {{-1, 1},  {0, 1},  {1, 1}}
    };

    // Constructor : shader, buffers, textures & heightmaps initialisation
    terrainManager(std::string terrainShaderPrefix);

    // main interface
    void update(glm::vec3 cameraPos);
    void draw(glm::mat4 view, glm::mat4 projection, glm::vec3& cameraPos, glm::vec3 lightPos);
    void addImpact(const std::vector<ImpactRing>& rings, float now);
    void saveHeightmapToPNG(const std::string& filename);
    float terrainHeightAt(glm::vec3& worldPos);
    float height_under_camera(glm::vec3& cameraPos);
    void printChunkOffsets();

private:
    // --- Initialisation helpers ---
    void initShader(const std::string& prefix);
    void initBuffers();
    void initTerrainTextures();
    void loadSoilTextures();

    // --- Heightmap generation & upload helpers ---
    void generateAllHeightmaps();
    void generate_terrain_heightmap(int chunk_x, int chunk_y, unsigned char* data);
    void uploadHeightmapTexture(int i, int j);

    // --- Chunk movement helpers ---
    void moveChunkDown(bool moveDown);
    void moveChunkRight(bool moveRight);

    // --- Sampling ---
    float sampleHeightmap(int chunkX, int chunkY, int indexX, int indexY);
};

// =========================================================================
// IMPLEMENTATION
// =========================================================================

inline terrainManager::terrainManager(std::string terrainShaderPrefix) {
    // initialisation en �tapes claires
    initShader(terrainShaderPrefix);
    initBuffers();
    initTerrainTextures();
    loadSoilTextures();
    generateAllHeightmaps();
}

inline void terrainManager::initShader(const std::string& prefix) {
    // Create terrain shader with tessellation
    ShaderFilePaths terrainShaderPaths;
    terrainShaderPaths.addVertexShader(std::string(PATH_TO_SHADERS) + "/" + prefix + ".vert");
    terrainShaderPaths.addTessellationControlShader(std::string(PATH_TO_SHADERS) + "/" + prefix + ".tcs");
    terrainShaderPaths.addTessellationEvaluationShader(std::string(PATH_TO_SHADERS) + "/" + prefix + ".tes");
    terrainShaderPaths.addFragmentShader(std::string(PATH_TO_SHADERS) + "/" + prefix + ".frag");
    this->terrainShader = Shader(terrainShaderPaths);

    terrainShader.use();
    terrainShader.setInteger("heightMap", 0);
    terrainShader.setInteger("textureBrickColor", 1);
    terrainShader.setInteger("textureBrickBump", 2);
    terrainShader.setFloat("scale", scale);
    terrainShader.setFloat("HEIGHT_SCALE", TERRAIN_HEIGHT);
}

inline void terrainManager::initBuffers() {
    model = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f));
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(simpleQuad), simpleQuad, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

inline void terrainManager::initTerrainTextures() {
    // Create textures for all chunks and allocate CPU buffers
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            currentTerrainData[i][j] = new unsigned char[TERRAIN_RESOLUTION * TERRAIN_RESOLUTION];
            glGenTextures(1, &textureID_terrain[i][j]);
            glBindTexture(GL_TEXTURE_2D, textureID_terrain[i][j]);
            // Use clamp_to_edge and nearest filtering for heightmap to avoid seams
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
    }
}

inline void terrainManager::generateAllHeightmaps() {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            // chunk coords: j-1, i-1 to center at 0,0
            generate_terrain_heightmap(j - 1, i - 1, currentTerrainData[i][j]);
            // upload initial data
            uploadHeightmapTexture(i, j);
        }
    }
}

inline void terrainManager::uploadHeightmapTexture(int i, int j) {
    glBindTexture(GL_TEXTURE_2D, textureID_terrain[i][j]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, TERRAIN_RESOLUTION, TERRAIN_RESOLUTION, 0, GL_RED, GL_UNSIGNED_BYTE, currentTerrainData[i][j]);
    glGenerateMipmap(GL_TEXTURE_2D);
}

inline void terrainManager::loadSoilTextures() {
    glGenTextures(1, &terrainTextureID);
    glBindTexture(GL_TEXTURE_2D, terrainTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width = 0, height = 0, nrChannels = 0;
    stbi_set_flip_vertically_on_load(true);

    unsigned char* data = stbi_load((std::string(PATH_TO_TEXTURE) + SOIL_TEXTURE).c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
        std::cout << "Texture loaded: " << PATH_TO_TEXTURE << SOIL_TEXTURE << " (" << width << "x" << height << ")" << std::endl;
    }
    else {
        std::cerr << "Failed to load texture: " << PATH_TO_TEXTURE << SOIL_TEXTURE
            << " reason: " << (stbi_failure_reason() ? stbi_failure_reason() : "unknown") << std::endl;
    }

    // heightmap 
    glGenTextures(1, &terrainBumpTextureID);
    glBindTexture(GL_TEXTURE_2D, terrainBumpTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    data = stbi_load((std::string(PATH_TO_TEXTURE) + SOIL_BUMP_TEXTURE).c_str(), &width, &height, &nrChannels, STBI_grey);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
        std::cout << "Texture loaded: " << PATH_TO_TEXTURE << SOIL_BUMP_TEXTURE << " (" << width << "x" << height << ")" << std::endl;
    }
    else {
        std::cerr << "Failed to load texture: " << PATH_TO_TEXTURE << SOIL_BUMP_TEXTURE
            << " reason: " << (stbi_failure_reason() ? stbi_failure_reason() : "unknown") << std::endl;
    }
}

inline void terrainManager::generate_terrain_heightmap(int chunk_x, int chunk_y, unsigned char* data) {
    const float maxDistanceSquarred = (TERRAIN_FLAT_ZONE_SIZE * TERRAIN_FLAT_ZONE_SIZE);
    const float transitionDistanceSquarred = maxDistanceSquarred * TERRAIN_TRANSITION_SIZE;
    for (int y = 0; y < TERRAIN_RESOLUTION; y++) {
        int y_local = y;
        int chunk_y_local = chunk_y;
        if (y == TERRAIN_RESOLUTION - 1) {
            y_local = 0;
            chunk_y_local = chunk_y + 1;
        }

        for (int x = 0; x < TERRAIN_RESOLUTION; x++) {
            int x_local = x;
            int chunk_x_local = chunk_x;
            if (x == TERRAIN_RESOLUTION - 1) {
                x_local = 0;
                chunk_x_local = chunk_x + 1;
            }

            float worldX = chunk_x_local * (TERRAIN_RESOLUTION)+x_local;
            float worldY = chunk_y_local * (TERRAIN_RESOLUTION)+y_local;

            float noise = glm::perlin(glm::vec2(worldX, worldY) * TERRAIN_INTENSITY_CHANGE);
            float noiseMapped = (noise + 1.0f) * 127.5f;
            float finalValue = noiseMapped;

            float dx = worldX - TERRAIN_RESOLUTION / 2.0f;
            float dy = worldY - TERRAIN_RESOLUTION / 2.0f;
            float distanceFromCenterSquarred = (dx * dx) + (dy * dy);

            if (distanceFromCenterSquarred < maxDistanceSquarred) {
                finalValue = TERRAIN_FLAT_HEIGHT; // Flat zone
            }
            else if (distanceFromCenterSquarred < transitionDistanceSquarred) {
                float t = (distanceFromCenterSquarred - maxDistanceSquarred) / transitionDistanceSquarred;
                finalValue = glm::mix(TERRAIN_FLAT_HEIGHT, noiseMapped, t);
            }
            finalValue = glm::clamp(finalValue, 0.0f, 255.0f);
            data[y * TERRAIN_RESOLUTION + x] = static_cast<unsigned char>(finalValue);
        }
    }
}

inline void terrainManager::moveChunkDown(bool moveDown) {
    unsigned char* tmpTerrain = nullptr;
    GLuint tmpTextureID = 0;
    if (moveDown) {
        for (int j = 0; j < 3; j++) {
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

            generate_terrain_heightmap(newChunkX, newChunkY, tmpTerrain);
            chunkOffsets[0][j] = { newChunkX, newChunkY };
            currentTerrainData[0][j] = tmpTerrain;
            textureID_terrain[0][j] = tmpTextureID;
            glBindTexture(GL_TEXTURE_2D, textureID_terrain[0][j]);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TERRAIN_RESOLUTION, TERRAIN_RESOLUTION, GL_RED, GL_UNSIGNED_BYTE, currentTerrainData[0][j]);
        }
    }
    else {
        for (int j = 0; j < 3; j++) {
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

            generate_terrain_heightmap(newChunkX, newChunkY, tmpTerrain);

            chunkOffsets[2][j] = { newChunkX, newChunkY };
            currentTerrainData[2][j] = tmpTerrain;
            textureID_terrain[2][j] = tmpTextureID;
            glBindTexture(GL_TEXTURE_2D, textureID_terrain[2][j]);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TERRAIN_RESOLUTION, TERRAIN_RESOLUTION, GL_RED, GL_UNSIGNED_BYTE, currentTerrainData[2][j]);
        }
    }
}

inline void terrainManager::moveChunkRight(bool moveRight) {
    unsigned char* tmpTerrain = nullptr;
    GLuint tmpTextureID = 0;

    if (moveRight) {
        for (int i = 0; i < 3; i++) {
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

            generate_terrain_heightmap(newChunkX, newChunkY, tmpTerrain);
            chunkOffsets[i][0] = { newChunkX, newChunkY };
            currentTerrainData[i][0] = tmpTerrain;
            textureID_terrain[i][0] = tmpTextureID;
            glBindTexture(GL_TEXTURE_2D, textureID_terrain[i][0]);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TERRAIN_RESOLUTION, TERRAIN_RESOLUTION, GL_RED, GL_UNSIGNED_BYTE, currentTerrainData[i][0]);
        }
    }
    else {
        for (int i = 0; i < 3; i++) {
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

            generate_terrain_heightmap(newChunkX, newChunkY, tmpTerrain);
            chunkOffsets[i][2] = { newChunkX, newChunkY };
            currentTerrainData[i][2] = tmpTerrain;
            textureID_terrain[i][2] = tmpTextureID;
            glBindTexture(GL_TEXTURE_2D, textureID_terrain[i][2]);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TERRAIN_RESOLUTION, TERRAIN_RESOLUTION, GL_RED, GL_UNSIGNED_BYTE, currentTerrainData[i][2]);
        }
    }
}

inline float terrainManager::sampleHeightmap(int chunkX, int chunkY, int indexX, int indexY) {
    if (chunkX < 0 || chunkX > 2 || chunkY < 0 || chunkY > 2) {
        return (127.5f / 255.0f - 0.5f) * TERRAIN_HEIGHT;
    }
    return (currentTerrainData[chunkY][chunkX][indexY * TERRAIN_RESOLUTION + indexX] / 255.0f - 0.5f) * TERRAIN_HEIGHT;
}

inline float terrainManager::terrainHeightAt(glm::vec3& worldPos) {
    float worldX = worldPos.x;
    float worldY = worldPos.z;
    const float chunkSize = 2.0f * scale;
    float floatChunkX = (worldX + scale) / chunkSize;
    float floatChunkY = (worldY + scale) / chunkSize;
    int chunkX = static_cast<int>(floor(floatChunkX));
    int chunkY = static_cast<int>(floor(floatChunkY));
    float decimalChunkX = floatChunkX - static_cast<float>(chunkX);
    float decimalChunkY = floatChunkY - static_cast<float>(chunkY);

    float preciseIndexX = decimalChunkX * static_cast<float>(TERRAIN_RESOLUTION - 1);
    float preciseIndexY = decimalChunkY * static_cast<float>(TERRAIN_RESOLUTION - 1);

    int x0 = static_cast<int>(floor(preciseIndexX));
    int y0 = static_cast<int>(floor(preciseIndexY));
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    int x0ChunkOffset = 0;
    int x1ChunkOffset = 0;
    int y0ChunkOffset = 0;
    int y1ChunkOffset = 0;

    if (x1 >= TERRAIN_RESOLUTION) {
        x1 = 0;
        x1ChunkOffset = 1;
    }
    if (y1 >= TERRAIN_RESOLUTION) {
        y1 = 0;
        y1ChunkOffset = 1;
    }

    int offsetXchunk_center = chunkX - currentChunkX;
    int offsetYchunk_center = chunkY - currentChunkY;
    if (abs(offsetXchunk_center) > 1 || abs(offsetYchunk_center) > 1) {
        return (127.5f / 255.0f - 0.5f) * TERRAIN_HEIGHT;
    }

    int baseChunkSampleX = offsetXchunk_center + 1;
    int baseChunkSampleY = offsetYchunk_center + 1;

    float h00 = sampleHeightmap(baseChunkSampleX + x0ChunkOffset, baseChunkSampleY + y0ChunkOffset, x0, y0);
    float h10 = sampleHeightmap(baseChunkSampleX + x1ChunkOffset, baseChunkSampleY + y0ChunkOffset, x1, y0);
    float h01 = sampleHeightmap(baseChunkSampleX + x0ChunkOffset, baseChunkSampleY + y1ChunkOffset, x0, y1);
    float h11 = sampleHeightmap(baseChunkSampleX + x1ChunkOffset, baseChunkSampleY + y1ChunkOffset, x1, y1);

    float fracX = preciseIndexX - static_cast<float>(x0);
    float fracY = preciseIndexY - static_cast<float>(y0);
    float height0 = glm::mix(h00, h10, fracX);
    float height1 = glm::mix(h01, h11, fracX);

    float bilinearHeight = glm::mix(height0, height1, fracY);
    return bilinearHeight;
}

inline float terrainManager::height_under_camera(glm::vec3& cameraPos) {
    (void)cameraPos;
    return heightUnderCamera;
}

inline void terrainManager::update(glm::vec3 cameraPos) {
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

    // update chunks if camera moved to another chunk
    if (deltaX != 0) {
        moveChunkRight(deltaX < 0);
    }
    if (deltaY != 0) {
        moveChunkDown(deltaY < 0);
    }

    // compute smoothed height under camera
    float avgHeight = terrainHeightAt(cameraPos);
    const float smoothFactor = 0.40f;
    avgHeight = glm::mix(previousHeightUnderCamera, avgHeight, smoothFactor);
    heightUnderCamera = avgHeight;
}

inline void terrainManager::draw(glm::mat4 view, glm::mat4 projection, glm::vec3& cameraPos, glm::vec3 lightPos) {
    terrainShader.use();
    terrainShader.setMatrix4("V", view);
    terrainShader.setMatrix4("P", projection);
    terrainShader.setVector3f("u_view_pos", cameraPos);
    terrainShader.setVector3f("lightPos", lightPos);
    terrainShader.setFloat("time", glfwGetTime());

    glBindVertexArray(VAO);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, terrainTextureID);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, terrainBumpTextureID);

    glPatchParameteri(GL_PATCH_VERTICES, 4);

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
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

inline void terrainManager::addImpact(const std::vector<ImpactRing>& rings, float now) {
    terrainShader.use();
    int count = std::min((int)rings.size(), 16);
    terrainShader.setInteger("impactCount", count);

    for (int i = 0; i < count; i++) {
        std::string posName = "impactPos[" + std::to_string(i) + "]";
        std::string timeName = "impactTime[" + std::to_string(i) + "]";

        terrainShader.setVector3f(posName.c_str(), rings[i].position);
        float t = now - (rings[i].initialLife - rings[i].life);
        terrainShader.setFloat(timeName.c_str(), t);
    }
}

inline void terrainManager::saveHeightmapToPNG(const std::string& filename) {
    const int bigWidth = TERRAIN_RESOLUTION * 3;
    const int bigHeight = TERRAIN_RESOLUTION * 3;

    // RGB
    std::vector<unsigned char> grayData(bigWidth * bigHeight * 3, 0);

    for (int cy = 0; cy < 3; cy++) {
        for (int cx = 0; cx < 3; cx++) {
            int offsetX = cx * TERRAIN_RESOLUTION;
            int offsetY = cy * TERRAIN_RESOLUTION;

            for (int y = 0; y < TERRAIN_RESOLUTION; y++) {
                for (int x = 0; x < TERRAIN_RESOLUTION; x++) {
                    int bigX = offsetX + x;
                    int bigY = offsetY + y;

                    int bigIndex = (bigY * bigWidth + bigX) * 3;
                    int smallIndex = y * TERRAIN_RESOLUTION + x;

                    unsigned char value = currentTerrainData[cy][cx][smallIndex];

                    // RGB grayscale
                    grayData[bigIndex + 0] = value;
                    grayData[bigIndex + 1] = value;
                    grayData[bigIndex + 2] = value;
                }
            }
        }
    }
    stbi_write_png(filename.c_str(), bigWidth, bigHeight, 3, grayData.data(), bigWidth * 3);
}


inline void terrainManager::printChunkOffsets() {
    std::cout << "Chunk Offsets:" << std::endl;
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            std::cout << "(" << chunkOffsets[y][x].y << ", " << chunkOffsets[y][x].x << ") ";
        }
        std::cout << std::endl;
    }
}