#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <cstdlib>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define MAX_SPEED 5.5f
#define NEIGHBOR_DISTANCE 50.0f
#define SEPARATION_DISTANCE 20.0f
#define MAX_DISTANCE_FROM_CENTER 30.0f
#define MIN_DISTANCE_FROM_CENTER 5.0f
#define CAMERA_ATTRACTION_FORCE 0.005f
#define EPSILON 1e-6f

inline glm::vec3 safeNormalize(const glm::vec3 &v)
{
    float len = glm::length(v);
    if (len < EPSILON)
    {
        return glm::vec3(0.0f);
    }
    return v / len;
}

struct Boid
{
    glm::vec3 position;
    glm::vec3 velocity;
};

class BoidManager
{

public:
    std::vector<Boid> boids;
    std::vector<Boid> oldboids;
    glm::vec3 center;

    BoidManager(size_t numBoids)
        : center(0.0f)
    {
        for (size_t i = 0; i < numBoids; i++)
        {
            Boid boid;
            boid.position = glm::vec3(rand() % 100 - 50, rand() % 100 - 50, rand() % 100 - 50);
            boid.velocity = glm::vec3((rand() / (float)RAND_MAX) * 2 - 1, (rand() / (float)RAND_MAX) * 2 - 1, (rand() / (float)RAND_MAX) * 2 - 1);
            boid.velocity = safeNormalize(boid.velocity) * MAX_SPEED;
            boids.push_back(boid);
        }
    }

    void update(float deltaTime)
    {
        oldboids = boids;

        for (size_t i = 0; i < boids.size(); i++)
        {

            glm::vec3 alignment(0.0f);
            glm::vec3 cohesion(0.0f);
            glm::vec3 separation(0.0f);
            int neighborCount = 0;

            for (size_t j = 0; j < boids.size(); j++)
            {
                if (i == j)
                    continue;

                float distance = glm::distance(oldboids[i].position, oldboids[j].position);
                if (distance < NEIGHBOR_DISTANCE)
                {
                    if (distance < EPSILON)
                    {
                        distance = EPSILON; // Avoid division by zero
                    }
                    alignment += oldboids[j].velocity / distance; // Closer boids have more influence
                    cohesion += oldboids[j].position;
                    if (distance < SEPARATION_DISTANCE)
                    {

                        separation -= (oldboids[j].position - oldboids[i].position) / distance;
                    }
                    neighborCount++;
                }
            }

            // add perturbation to avoid perfect alignment
            float perturbationStrength = 0.02f;
            glm::vec3 perturbation = glm::vec3((rand() / (float)RAND_MAX) * 2 - 1, (rand() / (float)RAND_MAX) * 2 - 1, (rand() / (float)RAND_MAX) * 2 - 1) * perturbationStrength;
            cohesion += perturbation;

            if (neighborCount > 0)
            {
                alignment /= neighborCount;
                alignment = safeNormalize(alignment) * MAX_SPEED;

                cohesion /= neighborCount;
                cohesion = safeNormalize(cohesion - oldboids[i].position) * MAX_SPEED;

                separation = safeNormalize(separation) * MAX_SPEED;
            }

            float distanceFromCenter = glm::distance(oldboids[i].position, center);
            glm::vec3 toCenter = safeNormalize(center - oldboids[i].position);

            // Gentle attraction keeps the flock loosely centered around the camera.
            cohesion += toCenter * (MAX_SPEED * CAMERA_ATTRACTION_FORCE);

            glm::vec3 boundaryForce(0.0f);
            if (distanceFromCenter > MAX_DISTANCE_FROM_CENTER)
            {
                boundaryForce = toCenter * MAX_SPEED;
            }
            else if (distanceFromCenter < MIN_DISTANCE_FROM_CENTER)
            {
                boundaryForce = -toCenter * MAX_SPEED * ((MIN_DISTANCE_FROM_CENTER - distanceFromCenter) / MIN_DISTANCE_FROM_CENTER);
            }

            boids[i].velocity += alignment + cohesion + separation + boundaryForce;
            if (glm::length(boids[i].velocity) > MAX_SPEED)
            {
                boids[i].velocity = safeNormalize(boids[i].velocity) * MAX_SPEED;
            }
            boids[i].position += boids[i].velocity * deltaTime;
        }
    }
};