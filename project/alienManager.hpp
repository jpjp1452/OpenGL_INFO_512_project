#pragma once
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/noise.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "objectManager.h"
#include "terrainManager.h"
#include "objectManager.h"
#include "shader.h"
#include <limits>

#define BASE_HEALTH 100.0f
#define MIN_SCALE 3.0f
#define ADDITIONAL_SCALE 5.0f
#define MIN_DISTANCE_FROM_SPHERE 20.0f
#define ADDITIONAL_DISTANCE 10.0f
#define SPEED 2.5f


#ifndef PATH_TO_SHADERS
#define PATH_TO_SHADERS "shaders"
#endif

glm::mat4 rotateAtoB(glm::vec3 a, glm::vec3 b, glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f))
{
    a = glm::normalize(a);
    b = glm::normalize(b);
    float cosTheta = glm::dot(a, b);
    glm::vec3 rotationAxis;
    if (cosTheta < -0.9999f)
    {
        // If vectors are opposite, find an orthogonal vector for rotation axis
        rotationAxis = glm::cross(glm::vec3(0.0f, 0.0f, 1.0f), a);
        if (glm::length(rotationAxis) < 0.0001f) // If collinear with Z, use X axis
            rotationAxis = glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), a);
        rotationAxis = glm::normalize(rotationAxis);
        return glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), rotationAxis);
    }
    else if (cosTheta > 0.9999f)
    {
        // If vectors are the same, no rotation needed
        return glm::mat4(1.0f);
    }
    else
    {
        rotationAxis = glm::cross(a, b);
        float angle = acos(cosTheta);
        return glm::rotate(glm::mat4(1.0f), angle, rotationAxis);
    }
}

struct AlienInfo
{
    glm::vec3 alienPosition;
    glm::vec3 alienScale;
    float health;
    float maxHealth;
};

class AlienManager
{
public:
    std::vector<AlienInfo> alienInfos;
    ObjectsData &alienObjectData;
    terrainManager &terrain;
    Shader shaderHealthBar;
    float radiusReflective;
    int countTouchingSphere = 0;

    float offsetY = 0.0f;
    float radiusAlien = 0.0f;
    float heightAlien = 0.0f;
    float minDistanceFromCenter = 0.0f;
    GLuint healthBarVAO, healthBarVBO;

    AlienManager(size_t how_many_aliens, terrainManager &terrainManager, ObjectsData &alienData, float reflectiveSphereRadius) : terrain(terrainManager), alienObjectData(alienData)
    {   

        




        float minx = std::numeric_limits<float>::max();
        float maxx = std::numeric_limits<float>::lowest();
        float minz = std::numeric_limits<float>::max();
        float maxz = std::numeric_limits<float>::lowest();
        float miny = std::numeric_limits<float>::max();
        float maxy = std::numeric_limits<float>::lowest();

        for (const auto &vertex : alienObjectData.object.vertices)
        {
            if (vertex.Position.x < minx)
                minx = vertex.Position.x;
            if (vertex.Position.x > maxx)
                maxx = vertex.Position.x;
            if (vertex.Position.z < minz)
                minz = vertex.Position.z;
            if (vertex.Position.z > maxz)
                maxz = vertex.Position.z;
            if (vertex.Position.y < miny)
                miny = vertex.Position.y;
            if (vertex.Position.y > maxy)
                maxy = vertex.Position.y;
        }
        heightAlien = maxy - miny;
        offsetY = -miny;
        radiusAlien = (maxx - minx + maxz - minz) * 0.5f;
        radiusReflective = reflectiveSphereRadius;
        alienInfos.resize(how_many_aliens);

        minDistanceFromCenter = radiusReflective + MIN_DISTANCE_FROM_SPHERE;
        for (size_t i = 0; i < how_many_aliens; i++)
        {
            float x = ((rand() / (float)RAND_MAX) - 0.5f) * 300.0f;
            float y = ((rand() / (float)RAND_MAX) - 0.5f) * 300.0f;
            float z = ((rand() / (float)RAND_MAX) - 0.5f) * 300.0f;
            x += (x > 0) ? minDistanceFromCenter : -minDistanceFromCenter;
            y += (y > 0) ? minDistanceFromCenter : -minDistanceFromCenter;
            z += (z > 0) ? minDistanceFromCenter : -minDistanceFromCenter;

            alienInfos[i].alienPosition = glm::vec3(x, y, z);
            float scale = ((rand() / (float)RAND_MAX)) * ADDITIONAL_SCALE + MIN_SCALE;
            alienInfos[i].alienScale = glm::vec3(scale);
            alienInfos[i].health = BASE_HEALTH * scale;
            alienInfos[i].maxHealth = BASE_HEALTH * scale;
        }
        updatePosition(0.0f); 






        shaderHealthBar = Shader(PATH_TO_SHADERS "/healthBar.vert", PATH_TO_SHADERS "/healthBar.frag");


        float quadVertices[] = {
            // positions 
            -1.0f, -1.0f, 0.0f, // bottom left
            -1.0f,  1.0f, 0.0f, // top left
             1.0f,  1.0f, 0.0f,  // top right
             -1.0f, -1.0f, 0.0f, // bottom left
             1.0f,  1.0f, 0.0f,  // top right
             1.0f, -1.0f, 0.0f   // bottom right
        };

        glGenVertexArrays(1, &healthBarVAO);
        glGenBuffers(1, &healthBarVBO);

        glBindVertexArray(healthBarVAO);
        glBindBuffer(GL_ARRAY_BUFFER, healthBarVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
    }

    void updatePosition(float deltaTime)
    {
        for (size_t i = 0; i < alienInfos.size(); i++)
        {
            glm::vec3 direction = -alienInfos[i].alienPosition;
            float distance = glm::length(direction);
            glm::mat4 model(1.0f);

            if (distance > radiusReflective)
            {
                direction = glm::normalize(direction);
                alienInfos[i].alienPosition += direction * SPEED * deltaTime;
                alienInfos[i].alienPosition.y = terrain.terrainHeightAt(alienInfos[i].alienPosition) + offsetY * alienInfos[i].alienScale.y; // keep alien above terrain
                glm::vec3 toCenter = glm::normalize(-alienInfos[i].alienPosition);
                float angleY = atan2(toCenter.x, toCenter.z) - glm::radians(90.0f);
                model = glm::translate(model, alienInfos[i].alienPosition);
                model *= glm::rotate(glm::mat4(1.0f),angleY,glm::vec3(0.0f, 1.0f, 0.0f));
                model *= glm::scale(glm::mat4(1.0f),alienInfos[i].alienScale);
                alienObjectData.modelMatrices[i] = model;
            }
            else
            {
                countTouchingSphere++;
            }
        }
    }

    void update(float deltaTime)
    {
        updatePosition(deltaTime);
    }

    void drawHealthBar(glm::mat4 view, glm::mat4 projection, glm::vec3 cameraUp, glm::vec3 cameraRight)
    {
        shaderHealthBar.use();
        glBindVertexArray(healthBarVAO);

        for (size_t i = 0; i < alienInfos.size(); i++)
        {
            if (alienInfos[i].health <= 0.0f)
                continue;

            glm::vec3 center = alienInfos[i].alienPosition + glm::vec3(0.0f, (heightAlien-offsetY) * alienInfos[i].alienScale.y + 0.5f, 0.0f);
            float healthPercent = alienInfos[i].health / alienInfos[i].maxHealth;
            float barWidth = 1.0f;
            float barHeight = 0.2f;

            shaderHealthBar.use();
            shaderHealthBar.setMatrix4("V", view);
            shaderHealthBar.setMatrix4("P", projection);
            shaderHealthBar.setVector3f("center", center);
            shaderHealthBar.setVector3f("cameraRight", cameraRight);
            shaderHealthBar.setVector3f("cameraUp", cameraUp);
            shaderHealthBar.setFloat("healthPercent", 0.11f);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }
};