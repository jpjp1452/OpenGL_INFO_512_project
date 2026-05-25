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
#define SPEED 5.0f
#define DIFFICULTY_FACTOR 1.00f
#define DIFFICULTY_INCREMENT 0.0125f
#define DAMAGE_FROM_PROJECTILE 60.1f
#define MAX_ADDITIONAL_DISTANCE 300.0f

#ifndef PATH_TO_SHADERS
#define PATH_TO_SHADERS "shaders"
#endif



struct AlienInfo
{
    glm::vec3 alienPosition;
    glm::vec3 alienScale;
    float health;
    float maxHealth;
    float speed;
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
    float maxHead;
    float minFeet;
    float minDistanceFromCenter = 0.0f;
    float difficultyLevel = 1.0f;

    GLuint healthBarVAO, healthBarVBO;

    AlienInfo newAlien()
    {
        AlienInfo info;
        float x = ((rand() / (float)RAND_MAX) - 0.5f) * MAX_ADDITIONAL_DISTANCE;
        float y = ((rand() / (float)RAND_MAX) - 0.5f) * MAX_ADDITIONAL_DISTANCE;
        float z = ((rand() / (float)RAND_MAX) - 0.5f) * MAX_ADDITIONAL_DISTANCE;
        x += (x > 0) ? minDistanceFromCenter : -minDistanceFromCenter;
        y += (y > 0) ? minDistanceFromCenter : -minDistanceFromCenter;
        z += (z > 0) ? minDistanceFromCenter : -minDistanceFromCenter;

        info.alienPosition = glm::vec3(x, y, z);
        float scale = ((rand() / (float)RAND_MAX)) * ADDITIONAL_SCALE + MIN_SCALE;
        info.alienScale = glm::vec3(scale);
        info.health = BASE_HEALTH * scale  * DIFFICULTY_FACTOR;
        info.maxHealth = info.health;
        info.speed = SPEED * difficultyLevel * DIFFICULTY_FACTOR;
        return info;
    }

    AlienManager(size_t how_many_aliens, terrainManager &terrainManager, ObjectsData &alienData, float reflectiveSphereRadius) : terrain(terrainManager), alienObjectData(alienData)
    {

        difficultyLevel = 1.0f;
        //finding bounding box of the model , to know where the head and feet are compared to the center of the model
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
        maxHead = maxy;
        minFeet = miny;
        offsetY = -miny;
        radiusAlien = (maxx - minx + maxz - minz) * 0.25f;
        radiusReflective = reflectiveSphereRadius;
        alienInfos.resize(how_many_aliens);
        minDistanceFromCenter = radiusReflective + MIN_DISTANCE_FROM_SPHERE;
        for (size_t i = 0; i < how_many_aliens; i++)
        {
            alienInfos[i] = newAlien();
        }
        updatePosition(0.0f);

        shaderHealthBar = Shader(PATH_TO_SHADERS "/healthBar.vert", PATH_TO_SHADERS "/healthBar.frag");
        //init health bar buffers
        float quadVertices[] = {
            // positions
            -1.0f, -1.0f, 0.0f, // bottom left
            -1.0f, 1.0f, 0.0f,  // top left
            1.0f, 1.0f, 0.0f,   // top right
            -1.0f, -1.0f, 0.0f, // bottom left
            1.0f, 1.0f, 0.0f,   // top right
            1.0f, -1.0f, 0.0f   // bottom right
        };

        glGenVertexArrays(1, &healthBarVAO);
        glGenBuffers(1, &healthBarVBO);

        glBindVertexArray(healthBarVAO);
        glBindBuffer(GL_ARRAY_BUFFER, healthBarVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void updatePosition(float deltaTime)
    {   
        countTouchingSphere = 0;
        for (size_t i = 0; i < alienInfos.size(); i++)
        {
            glm::vec3 direction = -alienInfos[i].alienPosition;
            float distance = glm::length(direction) - (alienInfos[i].alienScale.y * radiusAlien);
            glm::mat4 model(1.0f);

            if (distance > radiusReflective)
            {
                direction = glm::normalize(direction);
                alienInfos[i].alienPosition += direction * alienInfos[i].speed * deltaTime;
                alienInfos[i].alienPosition.y = terrain.terrainHeightAt(alienInfos[i].alienPosition) + offsetY * alienInfos[i].alienScale.y; // keep alien above terrain
                //rotate alien to face the center
                glm::vec3 toCenter = glm::normalize(-alienInfos[i].alienPosition);
                float angleY = atan2(toCenter.x, toCenter.z) - glm::radians(90.0f);
                model = glm::translate(model, alienInfos[i].alienPosition);
                model *= glm::rotate(glm::mat4(1.0f), angleY, glm::vec3(0.0f, 1.0f, 0.0f));
                model *= glm::scale(glm::mat4(1.0f), alienInfos[i].alienScale);
                alienObjectData.modelMatrices[i] = model;
            }
            else
            {
                countTouchingSphere++;
            }
        }
    }

    void lasersHitCheck(std::vector<glm::vec3> projectileStarts, std::vector<glm::vec3> projectileEnds)
    {
        for (size_t i = 0; i < alienInfos.size(); i++)
        {

            for (size_t j = 0; j < projectileEnds.size(); j++)
            {
                glm::vec3 start = projectileStarts[j];
                glm::vec3 end = projectileEnds[j];
                glm::vec3 dir = end - start;
                float segLen2 = glm::dot(dir, dir);
                glm::vec3 dirN = glm::normalize(dir);
                glm::vec3 toAlien = alienInfos[i].alienPosition - start;
                //project the point on to the segment, then check if it is inside the alien bounding box
                float projectionLength = glm::dot(toAlien, dirN);
                projectionLength = glm::clamp(projectionLength, 0.0f, glm::length(dir));
                glm::vec3 projectedPoint = start + projectionLength * dirN;
                float distanceToAlienX = projectedPoint.x - alienInfos[i].alienPosition.x;
                float distanceToAlienY = projectedPoint.y - alienInfos[i].alienPosition.y;
                float distanceToAlienZ = projectedPoint.z - alienInfos[i].alienPosition.z;

                float lowerBoundDifY = minFeet * alienInfos[i].alienScale.y;
                float upperBoundDifY = maxHead * alienInfos[i].alienScale.y;
                if (lowerBoundDifY < distanceToAlienY && distanceToAlienY < upperBoundDifY)
                {

                    float horizontalDistance = sqrt(distanceToAlienX * distanceToAlienX + distanceToAlienZ * distanceToAlienZ);
                    if (horizontalDistance < radiusAlien * alienInfos[i].alienScale.x)
                    {
                        alienInfos[i].health -= DAMAGE_FROM_PROJECTILE;
                        if (alienInfos[i].health < 0.0f)
                        {
                            difficultyLevel += DIFFICULTY_INCREMENT;
                            alienInfos[i] = newAlien();
                        }
                    }
                }
            }
        }
    }

    int update(float deltaTime,std::vector<glm::vec3> projectileStarts, std::vector<glm::vec3> projectileEnds)
    {   
        lasersHitCheck(projectileStarts, projectileEnds);
        updatePosition(deltaTime);
        return countTouchingSphere;
    }

    void drawHealthBar(glm::mat4 view, glm::mat4 projection, glm::vec3 cameraUp, glm::vec3 cameraRight)
    {
        shaderHealthBar.use();
        glBindVertexArray(healthBarVAO);

        for (size_t i = 0; i < alienInfos.size(); i++)
        {
            if (alienInfos[i].health <= 0.0f)
                continue;

            glm::vec3 center = alienInfos[i].alienPosition + glm::vec3(0.0f, (heightAlien - offsetY) * alienInfos[i].alienScale.y + 0.5f, 0.0f);
            float healthPercent = alienInfos[i].health / alienInfos[i].maxHealth;
            float barWidth = 1.0f;
            float barHeight = 0.2f;

            shaderHealthBar.use();
            shaderHealthBar.setMatrix4("V", view);
            shaderHealthBar.setMatrix4("P", projection);
            shaderHealthBar.setVector3f("center", center);
            shaderHealthBar.setVector3f("cameraRight", cameraRight);
            shaderHealthBar.setVector3f("cameraUp", cameraUp);
            shaderHealthBar.setFloat("healthPercent", alienInfos[i].health / alienInfos[i].maxHealth);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }

    ~AlienManager()
    {
        glDeleteVertexArrays(1, &healthBarVAO);
        glDeleteBuffers(1, &healthBarVBO);
    }
};