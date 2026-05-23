// =========================================================================
// INCLUDES
// =========================================================================

// --- Standard Library ---
#include <iostream>
#include <vector>
#include <map>
#include <string>

// --- OpenGL & Dependencies (glad before GLFW) ---
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>  
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/random.hpp>

// --- Local Headers ---
#include "camera.h"
#include "shader.h"
#include "objectManager.h"
#include "boids.h"
#include "terrainManager.h"
#include "asteriodManager.hpp"


// =========================================================================
// MACROS & CONSTANTS
// =========================================================================

#ifndef PATH_TO_OBJECTS
#define PATH_TO_OBJECTS "Model"
#endif

#ifndef PATH_TO_TEXTURE
#define PATH_TO_TEXTURE "Textures"
#endif

#ifndef PATH_TO_SHADERS
#define PATH_TO_SHADERS "shaders"
#endif

// --- Application Settings ---
const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;

// --- Gameplay & Physics Factors ---
#define SPEED_FACTOR 1.0f
#define MouvementMultiplier 0.6f * SPEED_FACTOR
#define RotationMultiplier 2.9f * SPEED_FACTOR
#define JUMP_VELOCITY 0.2f * SPEED_FACTOR


// =========================================================================
// GLOBAL VARIABLES
// =========================================================================

// --- Camera & Physics ---
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float gravity = 0.1f;
float verticalVelocity = 0.0f;
bool freeFalling = false;
glm::vec3 fallingPosition = camera.Position;

// --- Input & Shooting State ---
bool shooting = false;
float delayBetweenShots = 0.5f;
float timeSinceLastShot = delayBetweenShots + 1.0f; 
double mouseX;
double mouseY;
double lastMouseX;
double lastMouseY;

// --- Particles & Effects Data ---
std::vector<Particle> particles;
std::vector<ImpactRing> impactRings;
std::vector<glm::vec3> projectileStarts;
std::vector<glm::vec3> projectileEnds;
std::vector<float> projectileBirthTimes;


// =========================================================================
// HELPER FUNCTIONS & REFACTORED PROCEDURES
// =========================================================================

void processInput(GLFWwindow *window)
{   
    lastMouseX = mouseX;
    lastMouseY = mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    float deltaX = (float)(mouseX - lastMouseX);
    float deltaY = (float)(mouseY - lastMouseY);

    // Handle shooting 
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && timeSinceLastShot >= delayBetweenShots){
        shooting = true;
    }
    else {
        shooting = false;
    }

    // Handle jump
    if ((glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) && !freeFalling)
    {
        verticalVelocity = JUMP_VELOCITY;
        freeFalling = true;
        fallingPosition = camera.Position;
    }

    // Handle escape
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Handle movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboardMovement(FORWARD, 0.1f * MouvementMultiplier);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboardMovement(BACKWARD, 0.1f * MouvementMultiplier);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboardMovement(LEFT, 0.1f * MouvementMultiplier);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboardMovement(RIGHT, 0.1f * MouvementMultiplier);
        
    // Handle rotation (keyboard & mouse)
    if ((glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) || deltaY < 0)
        camera.ProcessKeyboardRotation(0.0f, 1.0f, 0.1f * RotationMultiplier*abs(deltaY));
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS || deltaY > 0)
        camera.ProcessKeyboardRotation(0.0f, -1.0f, 0.1f * RotationMultiplier*abs(deltaY));
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS || deltaX < 0)
        camera.ProcessKeyboardRotation(-1.0f, 0.0f, 0.1f * RotationMultiplier*abs(deltaX));
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS || deltaX > 0)
        camera.ProcessKeyboardRotation(1.0f, 0.0f, 0.1f * RotationMultiplier*abs(deltaX));
}

void loadCubemapFace(const char* path, const GLenum& targetFace)
{
    int imWidth, imHeight, imNrChannels;
    unsigned char* data = stbi_load(path, &imWidth, &imHeight, &imNrChannels, 0);
    if (data)
    {
        GLenum format = GL_RGB;
        if (imNrChannels == 1)
            format = GL_RED;
        else if (imNrChannels == 3)
            format = GL_RGB;
        else if (imNrChannels == 4)
            format = GL_RGBA;

        glTexImage2D(targetFace, 0, format, imWidth, imHeight, 0, format, GL_UNSIGNED_BYTE, data);
    }
    else {
        std::cout << "Failed to Load texture: " << path << std::endl;
        const char* reason = stbi_failure_reason();
        std::cout << (reason == NULL ? "Unknown reason" : reason) << std::endl;
    }
    stbi_image_free(data);
}

GLFWwindow* initOpenGL(int width, int height) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return nullptr;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    GLFWwindow *window = glfwCreateWindow(width, height, "OpenGL Project", nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return nullptr;
    }

    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, width, height);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    return window;
}

void updatePhysics(float deltaTime, terrainManager& terrain) {
    terrain.update(camera.Position);
    float heightUnderCamera = terrain.height_under_camera(camera.Position);
    float minHeight = heightUnderCamera + 1.5f;

    if (freeFalling) {
        verticalVelocity -= gravity * deltaTime;
        fallingPosition.y += verticalVelocity;
        camera.Position.y = fallingPosition.y;
        if (camera.Position.y < minHeight) {
            camera.Position.y = minHeight;
            verticalVelocity = 0.0f;
            freeFalling = false;
        }
    }
    else {
        camera.Position.y = minHeight;
    }
}

void processShooting(double now, float deltaTime, terrainManager& terrain, int& weaponAnimFrame) {
    if (shooting) {
        timeSinceLastShot = 0.0f;
        weaponAnimFrame = 1;
        shooting = false;

        glm::vec3 projectileDirection = camera.Front;
        glm::vec3 muzzleOffsetLocal = glm::vec3(0.3f, -0.25f, 0.0f);
        glm::vec3 weaponOrigin =
            camera.Position +
            (camera.Right * muzzleOffsetLocal.x) +
            (camera.Up * muzzleOffsetLocal.y) +
            (camera.Front * -muzzleOffsetLocal.z);

        const float maxDist = 50.0f;
        const float step = 0.25f;

        // 1) Raycast: camera to target point
        glm::vec3 camImpact = camera.Position;
        glm::vec3 camDir = glm::normalize(camera.Front);
        float traveled = 0.0f;
        glm::vec3 p = camera.Position;
        while (traveled < maxDist) {
            p += camDir * step;
            traveled += step;
            if (p.y < terrain.terrainHeightAt(p)) {
                p.y = terrain.terrainHeightAt(p);
                camImpact = p;
                break;
            }
        }
        if (traveled >= maxDist) camImpact = camera.Position + camDir * maxDist;

        // 2) Shot Direction (Weapon to Camera's impact point)
        glm::vec3 shotDir = glm::normalize(camImpact - weaponOrigin);

        // 3) Final Raycast for physical impact
        glm::vec3 impactPoint = weaponOrigin;
        traveled = 0.0f;
        p = weaponOrigin;
        while (traveled < maxDist) {
            p += shotDir * step;
            traveled += step;
            if (p.y < terrain.terrainHeightAt(p)) {
                p.y = terrain.terrainHeightAt(p);
                impactPoint = p;
                break;
            }
        }
        if (traveled >= maxDist) impactPoint = weaponOrigin + shotDir * maxDist;

        // Spawn particles
        for (int i = 0; i < 25; i++) {
            Particle part;
            part.position = impactPoint;

            float theta = ((rand() % 100) / 100.0f) * 6.28f;
            float phi = ((rand() % 100) / 100.0f) * 3.14f;

            glm::vec3 dir = glm::vec3(cos(theta) * sin(phi), cos(phi), sin(theta) * sin(phi));
            glm::vec3 finalDir = glm::normalize(dir * 1.5f + (-projectileDirection) * 2.0f);

            part.velocity = finalDir * (5.0f + (rand() % 300) / 100.0f);
            part.life = 0.2f + (rand() % 100) / 400.0f;
            part.initialLife = part.life;
            part.scale = 0.25f + (rand() % 100) / 500.0f;
            part.rotationAxis = glm::normalize(glm::vec3(dir.x, dir.y, dir.z + 0.1f));
            part.rotationAngle = rand() % 360;

            particles.push_back(part);
        }

        projectileStarts.push_back(weaponOrigin);
        projectileEnds.push_back(impactPoint);
        projectileBirthTimes.push_back((float)now);

        ImpactRing ring;
        ring.position = impactPoint;
        ring.life = 0.6f;
        ring.initialLife = ring.life;
        ring.radius = 0.1f;
        impactRings.push_back(ring);
    } else {
        timeSinceLastShot += deltaTime;
        if (timeSinceLastShot >= 0.1f) {
            weaponAnimFrame = 2;
            if (timeSinceLastShot >= 0.2f) weaponAnimFrame = 3;
            if (timeSinceLastShot >= 0.3f) weaponAnimFrame = 0;
        }
    }
}

void updateProjectiles(double now, ObjectsData& projectileData) {
    // Projectiles Garbage Collection
    for (size_t i = 0; i < projectileEnds.size(); ) {
        if (now - projectileBirthTimes[i] > 0.15f) {
            projectileStarts.erase(projectileStarts.begin() + i);
            projectileEnds.erase(projectileEnds.begin() + i);
            projectileBirthTimes.erase(projectileBirthTimes.begin() + i);
        } else {
            ++i;
        }
    }

    projectileData.modelMatrices.clear();
    for (size_t i = 0; i < projectileEnds.size(); ++i) {
        glm::vec3 start = projectileStarts[i];
        glm::vec3 end = projectileEnds[i];
        glm::vec3 delta = end - start;
        float distance = glm::length(delta);
        glm::vec3 forwardDir = (distance > 0.0001f) ? glm::normalize(delta) : camera.Front;
        distance = glm::max(distance, 0.15f);

        glm::vec3 midPoint = (start + end) * 0.5f;
        glm::mat4 trans = glm::translate(glm::mat4(1.0f), midPoint);
        
        // Ensure rotateAtoB is accessible or defined before invoking it
        glm::mat4 rot = rotateAtoB(glm::vec3(0.0f, 0.0f, 1.0f), forwardDir); 
        glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(0.001f, 0.001f, distance * 0.5f));

        projectileData.modelMatrices.push_back(trans * rot * scaleMat);
    }
}

void renderParticlesAndRings(Shader& particleShader, Shader& ringImpactShader, GLuint particleVAO, GLuint ringVAO, const glm::mat4& view, const glm::mat4& projection, float deltaTime) {
    glDepthMask(GL_FALSE);                    
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);        
    glDisable(GL_CULL_FACE);

    // Particles
    particleShader.use();
    particleShader.setMatrix4("V", view);
    particleShader.setMatrix4("P", projection);

    glBindVertexArray(particleVAO);
    for (size_t i = 0; i < particles.size(); ) {
        Particle& p = particles[i];
        p.life -= deltaTime;
        if (p.life <= 0.0f) {
            particles.erase(particles.begin() + i);
            continue;
        }

        glm::vec3 noise = glm::sphericalRand(1.0f) * 0.2f;
        p.velocity += noise * deltaTime;
        p.velocity *= 0.98f;
        p.position += p.velocity * deltaTime;

        float t = p.life / p.initialLife;
        glm::mat4 modelp = glm::translate(glm::mat4(1.0f), p.position);

        particleShader.setMatrix4("M", modelp);
        particleShader.setFloat("size", p.scale * (0.3f + t));

        glDrawArrays(GL_TRIANGLES, 0, 3);
        ++i;
    }

    // Impact Rings
    ringImpactShader.use();
    ringImpactShader.setMatrix4("V", view);
    ringImpactShader.setMatrix4("P", projection);

    glBindVertexArray(ringVAO);
    for (size_t i = 0; i < impactRings.size(); ) {
        ImpactRing& r = impactRings[i];
        r.life -= deltaTime;
        if (r.life <= 0.0f) {
            impactRings.erase(impactRings.begin() + i);
            continue;
        }

        float t = 1.0f - (r.life / r.initialLife);
        float radius = 0.1f + (1.0f - t) * 0.5f;
        glm::mat4 M = glm::translate(glm::mat4(1.0f), r.position);
        M = glm::scale(M, glm::vec3(radius));

        ringImpactShader.setMatrix4("M", M);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        i++;
    }
    glBindVertexArray(0);

    // Restore transparency 
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
}


// =========================================================================
// MAIN ROUTINE
// =========================================================================

int main() {  
    std::cout << "Initializing OpenGL Application..." << std::endl;

    // 1. GLFW & GLAD INITIALIZATION
    GLFWwindow* window = initOpenGL(SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!window) return -1;

    // 2. SHADERS & OBJECTS SETUP
    std::cout << "Loading model..." << std::endl;
    ObjectManager objectManager;
    
    ShaderFilePaths shaderPaths;
    shaderPaths.addFragmentShader(PATH_TO_SHADERS "/textureLighting.frag");
    shaderPaths.addVertexShader(PATH_TO_SHADERS "/fishTextureLighting.vert");
    Shader textureLightingShader(shaderPaths);
    
    textureLightingShader.use();
    textureLightingShader.setFloat("shininess", 32.0f);
    textureLightingShader.setFloat("light.ambient_strength", 0.1f);
    textureLightingShader.setFloat("light.diffuse_strength", 1.8f);
    textureLightingShader.setFloat("light.specular_strength", 1.0f);
    textureLightingShader.setFloat("light.constant", 1.0f);
    textureLightingShader.setFloat("light.linear", 0.14f);
    textureLightingShader.setFloat("light.quadratic", 0.07f);
    textureLightingShader.setVector3f("light.light_pos", glm::vec3(0.0f, 0.0f, 0.0f));

    Shader textureShader(PATH_TO_SHADERS "/texture.vert", PATH_TO_SHADERS "/texture.frag");
    Shader crosshairShader(PATH_TO_SHADERS "/crosshair.vert", PATH_TO_SHADERS "/crosshair.frag");

    Shader sphereShader(PATH_TO_SHADERS "/sphere.vert", PATH_TO_SHADERS "/sphere.frag");
    objectManager.addObject("sphere", PATH_TO_OBJECTS "/sphere.obj", sphereShader);
    objectManager.addObject("sunHalo", PATH_TO_OBJECTS "/sphere.obj", sphereShader);

    Shader planetShader(PATH_TO_SHADERS "/planet.vert", PATH_TO_SHADERS "/planet.frag");
    objectManager.addObject("planet", PATH_TO_OBJECTS "/planet.obj", planetShader);

    Shader projectileShader(PATH_TO_SHADERS "/projectile.vert", PATH_TO_SHADERS "/projectile.frag");
    objectManager.addObject("projectile", PATH_TO_OBJECTS "/sphere.obj", projectileShader);
    objectManager.removeOneObject("projectile");

    terrainManager terrain("terrain");

    Shader cubeMapShader = Shader(PATH_TO_SHADERS "/cubemap.vert", PATH_TO_SHADERS "/cubemap.frag");
    objectManager.addObject("cubeMap", PATH_TO_OBJECTS "/cubeMap.obj", cubeMapShader);

    Shader reflexionShader = Shader(PATH_TO_SHADERS "/sphereReflexion.vert", PATH_TO_SHADERS "/sphereReflexion.frag");
    objectManager.addObject("reflectiveSphere", PATH_TO_OBJECTS "/sphere.obj", reflexionShader);
    reflexionShader.use();
    reflexionShader.setFloat("shininess", 32.0f);
    reflexionShader.setVector3f("materialColour", glm::vec3(0.5f, 0.6, 0.8));
    reflexionShader.setFloat("light.ambient_strength", 0.1f);
    reflexionShader.setFloat("light.diffuse_strength", 0.5f);
    reflexionShader.setFloat("light.specular_strength", 0.8f);
    reflexionShader.setFloat("light.constant", 1.0);
    reflexionShader.setFloat("light.linear", 0.14);
    reflexionShader.setFloat("light.quadratic", 0.07);

    Shader asteroidShader(PATH_TO_SHADERS "/asteroid.vert", PATH_TO_SHADERS "/asteroid.frag");

    size_t numModelsTogenerate = 10;
    for (size_t i = 0; i < numModelsTogenerate; i++) {
        objectManager.addObject("fish", PATH_TO_OBJECTS "/small_green_alien.obj", textureLightingShader);
    }
    AsteroidManager asteroidManager(numModelsTogenerate, terrain);
    ObjectsData &dataFish = objectManager.objects.at("fish");


    // 3. TRANSFORMATIONS & GEOMETRY INIT EXTRAS
    ObjectsData &projectileData = objectManager.objects.at("projectile");

    glm::vec3 sunPosition = glm::vec3(50.0f, 20.0f, -50.0f);
    glm::vec3 planetPosition = glm::vec3(25.0f, 25.0f, -25.0f);
    
    ObjectsData &sphereData = objectManager.objects.at("sphere");
    sphereData.modelMatrices[0] = glm::translate(glm::mat4(1.0f), sunPosition);
    sphereData.modelMatrices[0] = glm::scale(sphereData.modelMatrices[0], glm::vec3(0.5f, 0.5f, 0.5f));

    ObjectsData &sunHaloData = objectManager.objects.at("sunHalo");
    sunHaloData.modelMatrices[0] = glm::translate(glm::mat4(1.0f), sunPosition);
    sunHaloData.modelMatrices[0] = glm::scale(sunHaloData.modelMatrices[0], glm::vec3(1.0f, 1.0f, 1.0f));

    ObjectsData& planetData = objectManager.objects.at("planet");
    planetData.modelMatrices[0] = glm::translate(glm::mat4(1.0f), planetPosition);
    planetData.modelMatrices[0] = glm::scale(planetData.modelMatrices[0], glm::vec3(0.8f, 0.8f, 0.8f));

    ObjectsData& refSphereData = objectManager.objects.at("reflectiveSphere");
    refSphereData.modelMatrices[0] = glm::translate(refSphereData.modelMatrices[0], glm::vec3(20.0f, 1.0f, 20.0f));
    refSphereData.modelMatrices[0] = glm::scale(refSphereData.modelMatrices[0], glm::vec3(1.0f, 1.0f, 1.0f));

    // Asteroids instances generation
    unsigned int asteroidAmount = 1000;
    srand((unsigned int)glfwGetTime()); 	
    float radius = 15.0;
    float offset = 5.0f;
    for (unsigned int i = 0; i < asteroidAmount; i++) {
        objectManager.addObject("asteroid", PATH_TO_OBJECTS "/rock.obj", asteroidShader);
        ObjectsData& astData = objectManager.objects.at("asteroid");

        glm::mat4 asteroidModel = glm::mat4(1.0f);
        float angle = (float)i / (float)asteroidAmount * 360.0f;
        float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float x = sin(angle) * radius + displacement;
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float y = displacement * 0.4f; 
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float z = cos(angle) * radius + displacement;
        
        asteroidModel = glm::translate(asteroidModel, (glm::vec3(x, y, z) + planetPosition));
        float scale = (rand() % 20) / 100.0f + 0.05f;
        asteroidModel = glm::scale(asteroidModel, glm::vec3(scale));
        float rotAngle = (float)(rand() % 360);
        asteroidModel = glm::rotate(asteroidModel, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

        astData.modelMatrices[i] = asteroidModel;
    }
    
    // Shader & Buffer for particles / crosshair
    Shader particleShader("shaders/particle.vert", "shaders/particle.frag");
    Shader ringImpactShader("shaders/ringImpact.vert", "shaders/ringImpact.frag");

    float vertices[] = {
        -0.5f, -0.5f, 0.0f,   1.0f, 0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,
         0.0f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f,
    };
    float quad[] = {
        -1, -1, 0,   1, -1, 0,  -1,  1, 0,
        -1,  1, 0,   1, -1, 0,   1,  1, 0
    };

    GLuint particleVAO, particleVBO, ringVAO, ringVBO, cubeMapTexture;
    
    glGenVertexArrays(1, &particleVAO);
    glGenBuffers(1, &particleVBO);
    glBindVertexArray(particleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glGenVertexArrays(1, &ringVAO);
    glGenBuffers(1, &ringVBO);
    glBindVertexArray(ringVAO);
    glBindBuffer(GL_ARRAY_BUFFER, ringVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    glGenTextures(1, &cubeMapTexture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    std::string pathToCubeMap = PATH_TO_TEXTURE "/cubemaps/skybox/";
    std::map<std::string, GLenum> facesToLoad = {
        {pathToCubeMap + "right.png", GL_TEXTURE_CUBE_MAP_POSITIVE_X},
        {pathToCubeMap + "bottom.png",GL_TEXTURE_CUBE_MAP_POSITIVE_Y},
        {pathToCubeMap + "front.png", GL_TEXTURE_CUBE_MAP_POSITIVE_Z},
        {pathToCubeMap + "left.png",  GL_TEXTURE_CUBE_MAP_NEGATIVE_X},
        {pathToCubeMap + "top.png",   GL_TEXTURE_CUBE_MAP_NEGATIVE_Y},
        {pathToCubeMap + "back.png",  GL_TEXTURE_CUBE_MAP_NEGATIVE_Z},
    };
    for (std::pair<std::string, GLenum> pair : facesToLoad) {
        loadCubemapFace(pair.first.c_str(), pair.second);
    }

    ObjectsData& astData2 = objectManager.objects.at("asteroid");
    unsigned int instanceVBO;
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, asteroidAmount * sizeof(glm::mat4), astData2.modelMatrices.data(), GL_STATIC_DRAW);
    
    unsigned int astVAO = astData2.object.VAO;
    glBindVertexArray(astVAO);
    std::size_t vec4Size = sizeof(glm::vec4);
    
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(1 * vec4Size));
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * vec4Size));
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * vec4Size));

    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);
    glVertexAttribDivisor(5, 1);
    glVertexAttribDivisor(6, 1);
    glBindVertexArray(0);

    GLuint crossVAO = 0, crossVBO = 0;
    float crosshairVerts[] = {
        -0.02f, 0.0f,  0.02f, 0.0f,
         0.0f, -0.02f, 0.0f,  0.02f
    };
    glGenVertexArrays(1, &crossVAO);
    glGenBuffers(1, &crossVBO);
    glBindVertexArray(crossVAO);
    glBindBuffer(GL_ARRAY_BUFFER, crossVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(crosshairVerts), crosshairVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    ShaderFilePaths hudShaderPaths;
    hudShaderPaths.addVertexShader(PATH_TO_SHADERS "/hud.vert");
    hudShaderPaths.addFragmentShader(PATH_TO_SHADERS "/hud.frag");
    Shader hudShader(hudShaderPaths);
    hudShader.use();
    objectManager.addObject("hud", PATH_TO_OBJECTS "/weapon_quad.obj", hudShader);
    
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 inverseModel = glm::transpose(glm::inverse(model));
    glm::vec3 light_pos = glm::vec3(1.0, 2.0, 1.5);
    
    double prev = glfwGetTime();
    double prevUpdate = prev;
    int deltaFrame = 0;
    auto fps = [&](double now) {
        double currentDeltaTime = now - prev;
        double timeSinceLastUpdate = now - prevUpdate;
        deltaFrame++;
        if (timeSinceLastUpdate > 1.0) {
            prevUpdate = now;
            prev = now;
            const double fpsCount = (double)deltaFrame / currentDeltaTime;
            deltaFrame = 0;
            std::cout << "\rFPS: " << fpsCount << std::flush;
        }
    };


    // 4. MAIN RENDER LOOP
    std::cout << "Controls: W/A/S/D to move, Arrow keys to rotate, ESC to quit" << std::endl;

    int weaponAnimFrame = 0;
    double lastTime = glfwGetTime();
    double now = lastTime;

    while (!glfwWindowShouldClose(window))
    {   
        now = glfwGetTime();
        float deltaTime = (float)(now - lastTime);
        lastTime = now;

        processInput(window);
        
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // --- PHYSICS & LOGIC UPDATES ---
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = camera.GetProjectionMatrix(camera.Zoom, (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT);

        updatePhysics(deltaTime, terrain);
        asteroidManager.update(dataFish);

        processShooting(now, deltaTime, terrain, weaponAnimFrame);
        updateProjectiles(now, projectileData);


        // --- RENDERING STATIC & OPAQUE OBJECTS ---
        
        // Draw Fish
        uniformSetters setters;
        setters.setFloats.push_back({"time", (float)now});
        setters.setMat4.push_back({"V", view});
        setters.setMat4.push_back({"P", projection});
        setters.setVec3.push_back({"u_view_pos", camera.Position});
        objectManager.drawObject("fish", setters);

        // Draw Sun
        uniformSetters sphereSetters;
        sphereSetters.setVec3.push_back({ "baseColor", glm::vec3(1.0f, 0.12f, 0.03f) });
        sphereSetters.setFloats.push_back({ "time", (float)now });
        sphereSetters.setMat4.push_back({ "V", view });
        sphereSetters.setMat4.push_back({ "P", projection });
        sphereSetters.setVec3.push_back({ "view_pos", camera.Position });
        sphereSetters.setIntegers.push_back({ "isHalo", 0 });
        sphereSetters.setFloats.push_back({ "haloIntensity", 0.0f });
        objectManager.drawObject("sphere", sphereSetters);

        // Draw Halo
        uniformSetters haloSetters;
        haloSetters.setVec3.push_back({ "baseColor", glm::vec3(1.0f, 0.18f, 0.05f) });
        haloSetters.setFloats.push_back({ "time", (float)now });
        haloSetters.setMat4.push_back({ "V", view });
        haloSetters.setMat4.push_back({ "P", projection });
        haloSetters.setVec3.push_back({ "view_pos", camera.Position });
        haloSetters.setIntegers.push_back({ "isHalo", 1 });
        haloSetters.setFloats.push_back({ "haloIntensity", 1.35f });

        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        objectManager.drawObject("sunHalo", haloSetters);
        
        // Restore default transparency modes
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);

        // Draw Planet
        uniformSetters planetSetters;
        planetSetters.setFloats.push_back({ "time", (float)now });
        planetSetters.setMat4.push_back({ "V", view });
        planetSetters.setMat4.push_back({ "P", projection });
        planetSetters.setVec3.push_back({ "u_view_pos", camera.Position });
        planetSetters.setVec3.push_back({ "light.light_pos", sunPosition });
        planetSetters.setIntegers.push_back({ "planetTexture", 0 });
        objectManager.drawObject("planet", planetSetters);

        // Draw Cubemap
        glDepthFunc(GL_LEQUAL);
        glDisable(GL_CULL_FACE);
        uniformSetters cubeMapSetters;
        cubeMapSetters.setMat4.push_back({ "V", view });
        cubeMapSetters.setMat4.push_back({ "P", projection });
        cubeMapSetters.setIntegers.push_back({ "cubeMapTexture", 0 });
        objectManager.drawObject("cubeMap", cubeMapSetters);
        glEnable(GL_CULL_FACE);
        glDepthFunc(GL_LESS);

        // Draw Reflective Sphere
        auto deltaReflec = light_pos + glm::vec3(0.0, 0.0, 2 * std::sin(now));
        uniformSetters reflectiveSetters;
        reflectiveSetters.setMat4.push_back({ "M", model });
        reflectiveSetters.setMat4.push_back({ "itM", inverseModel });
        reflectiveSetters.setMat4.push_back({ "V", view });
        reflectiveSetters.setMat4.push_back({ "P", projection });
        reflectiveSetters.setVec3.push_back({ "u_view_pos", camera.Position });
        reflectiveSetters.setVec3.push_back({ "light.light_pos", deltaReflec });
        objectManager.drawObject("reflectiveSphere", reflectiveSetters);

        // Draw Asteroids Instanced
        asteroidShader.use();
        asteroidShader.setMatrix4("V", view);
        asteroidShader.setMatrix4("P", projection);
        asteroidShader.setInteger("useTexture", 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, astData2.object.textureID);
        glBindVertexArray(astData2.object.VAO);
        glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(astData2.object.indices.size()), GL_UNSIGNED_INT, 0, asteroidAmount);
        glBindVertexArray(0);

        // Render Terrain
        terrain.addImpact(impactRings, (float)now);
        terrain.draw(view, projection, camera.Position, glm::vec3(0.0f, 100.0f, 0.0f));


        // --- RENDERING PARTICLES & TRANSPARENCY ---
        uniformSetters projectileSetters;
        projectileSetters.setFloats.push_back({ "time", (float)now });
        projectileSetters.setMat4.push_back({ "V", view });
        projectileSetters.setMat4.push_back({ "P", projection });
        projectileSetters.setVec3.push_back({ "baseColor", glm::vec3(2.5f, 0.3f, 0.3f) });
        projectileSetters.setVec3.push_back({ "center", glm::vec3(0.0f, -0.5f, -8.0f) });
        projectileSetters.setVec3.push_back({ "view_pos", camera.Position });
        
        objectManager.drawObject("projectile", projectileSetters);
        renderParticlesAndRings(particleShader, ringImpactShader, particleVAO, ringVAO, view, projection, deltaTime);

        
        // --- RENDERING UI & HUD ---
        glDisable(GL_DEPTH_TEST);
        glLineWidth(2.0f);
        crosshairShader.use();
        crosshairShader.setVector3f("u_color", glm::vec3(1.0f, 1.0f, 1.0f));
        glBindVertexArray(crossVAO);
        glDrawArrays(GL_LINES, 0, 4);
        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);

        uniformSetters hudSetters;
        hudSetters.setIntegers.push_back({"weaponFrame", weaponAnimFrame});
        objectManager.drawObject("hud", hudSetters);

        // --- END OF FRAME ---
        fps(now);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 5. CLEANUP
    glDeleteBuffers(1, &particleVBO);
    glDeleteVertexArrays(1, &particleVAO);
    glDeleteBuffers(1, &ringVBO);
    glDeleteVertexArrays(1, &ringVAO);
    glDeleteBuffers(1, &crossVBO);
    glDeleteVertexArrays(1, &crossVAO);
    
    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "\nApplication closed." << std::endl;
    return 0;
}
