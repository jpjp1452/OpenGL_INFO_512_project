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

class AsteroidManager
{
public:
    std::vector<glm::vec3> asteroidPositions;
    std::vector<glm::vec3> asteroidScales;
    terrainManager &terrain;
    AsteroidManager(size_t how_many_asteroids, terrainManager &terrainManager) : terrain(terrainManager)
    {
        float scaling = 4.0f;
        asteroidPositions.resize(how_many_asteroids);
        asteroidScales.resize(how_many_asteroids);

        for (size_t i = 0; i < how_many_asteroids; i++)
        {
            float x = ((rand() / (float)RAND_MAX) - 0.5f) * 300.0f;
            float y = ((rand() / (float)RAND_MAX) - 0.5f) * 300.0f;
            float z = ((rand() / (float)RAND_MAX) - 0.5f) * 300.0f;

            asteroidPositions[i] = glm::vec3(x, y, z);

            float scale = ((rand() / (float)RAND_MAX)) * scaling + scaling * 0.5f;
            asteroidScales[i] = glm::vec3(scale);
        }
    }

    void update(ObjectsData &asteroidData)
    {   
        float minx = 99999999.0f;
        float maxx = -99999999.0f;
        float minz = 99999999.0f;
        float maxz = -99999999.0f;
        for (size_t i = 0; i < asteroidData.object.positions.size(); i++)
        {
            glm::vec3 pos = asteroidData.object.positions[i];
            if (pos.x < minx) minx = pos.x;
            if (pos.x > maxx) maxx = pos.x;
            if (pos.z < minz) minz = pos.z;
            if (pos.z > maxz) maxz = pos.z;
        }



        float avgDistx = (abs(minx) + abs(maxx)) * 0.5f;
        float avgDistZ = (abs(minz) + abs(maxz)) * 0.5f;
    

        float distanceFromCenterY = (avgDistx + avgDistZ) * 0.5f;

        for (size_t i = 0; i < asteroidPositions.size(); i++)
        {
            // move asteroid toward 0 0 0
            glm::vec3 direction = -asteroidPositions[i];
            float distance = glm::length(direction);
            if (distance > 0.1f)
            {
                direction = glm::normalize(direction);
                asteroidPositions[i] += direction * 0.05f;
                asteroidPositions[i].y = terrain.terrainHeightAt(asteroidPositions[i]) + 0.5f * asteroidScales[i].x; // keep asteroid above terrain

                // direction vers le centre
                glm::vec3 toCenter = glm::normalize(-asteroidPositions[i]);

                // rotation Y seulement + correction pi/2
                float angleY = atan2(toCenter.x, toCenter.z) - glm::radians(90.0f);

                // transform
                glm::mat4 model(1.0f);

                // translation EN DERNIER
                model = glm::translate(model, asteroidPositions[i]);

                // rotation
                model *= glm::rotate(
                    glm::mat4(1.0f),
                    angleY,
                    glm::vec3(0.0f, 1.0f, 0.0f));



                



                // scale local
                model *= glm::scale(
                    glm::mat4(1.0f),
                    glm::vec3(asteroidScales[i]));

                asteroidData.modelMatrices[i] = model;
            }
        }
    }
};