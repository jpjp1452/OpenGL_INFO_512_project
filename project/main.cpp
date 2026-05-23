#include <iostream>
#include <thread>

// Include glad before GLFW to avoid header conflicts
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/gtc/noise.hpp>  
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


#include "camera.h"
#include "shader.h"
#include "objectManager.h"
#include "boids.h"

#ifndef PATH_TO_OBJECTS
#define PATH_TO_OBJECTS "Model"
#endif

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


const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;

#include "terrainManager.h"
#include "asteriodManager.hpp"
#include <glm/gtc/random.hpp>

#define SPEED_FACTOR 1.0f
#define MouvementMultiplier 0.6f * SPEED_FACTOR
#define RotationMultiplier 2.9f * SPEED_FACTOR
#define JUMP_VELOCITY 0.2f * SPEED_FACTOR

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float gravity = 0.1f;
float verticalVelocity = 0.0f;
bool freeFalling = false;
bool shooting = false;
float delayBetweenShots = 0.5f;
float timeSinceLastShot =delayBetweenShots  +1.0f; 
double mouseX;
double mouseY;
double lastMouseX;
double lastMouseY;

glm::vec3 fallingPosition = camera.Position;



void processInput(GLFWwindow *window)
{   
    lastMouseX = mouseX;
    lastMouseY = mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    float deltaX = mouseX - lastMouseX;
    float deltaY = mouseY - lastMouseY;


    //if left mouse button is pressed, set shooting to true, otherwise set it to false
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS &&  timeSinceLastShot >= delayBetweenShots){
        shooting = true;
    }
    else{
        shooting = false;
    }

    if ((glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) && !freeFalling)
    {
        verticalVelocity = JUMP_VELOCITY;
        freeFalling = true;
        fallingPosition = camera.Position;
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboardMovement(FORWARD, 0.1f * MouvementMultiplier);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboardMovement(BACKWARD, 0.1f * MouvementMultiplier);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboardMovement(LEFT, 0.1f * MouvementMultiplier);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboardMovement(RIGHT, 0.1f * MouvementMultiplier);
    //key up or mouse moves up rotate camera up
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

std::vector<Particle> particles;
std::vector<ImpactRing> impactRings;
std::vector<glm::vec3> projectileStarts;
std::vector<glm::vec3> projectileEnds;
std::vector<float> projectileBirthTimes;

int main() {  

    std::cout << "Initializing OpenGL Application..." << std::endl;

    // Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // Create window
    GLFWwindow *window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "OpenGL Project", nullptr, nullptr);
    if (window == nullptr)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Load OpenGL functions with GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return -1;
    }


    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    std::cout << "Loading model..." << std::endl;
    ObjectManager objectManager;
    //Shader textureLightingShader(PATH_TO_SHADERS "/textureLighting.vert", PATH_TO_SHADERS "/textureLighting.frag");
    
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

    // Simple crosshair shader 
	Shader crosshairShader(PATH_TO_SHADERS "/crosshair.vert", PATH_TO_SHADERS "/crosshair.frag");

    Shader sphereShader(PATH_TO_SHADERS "/sphere.vert", PATH_TO_SHADERS "/sphere.frag");

    objectManager.addObject("sphere", PATH_TO_OBJECTS "/sphere.obj", sphereShader);
    objectManager.addObject("sunHalo", PATH_TO_OBJECTS "/sphere.obj", sphereShader);

    Shader planetShader(PATH_TO_SHADERS "/planet.vert", PATH_TO_SHADERS "/planet.frag");
    objectManager.addObject("planet", PATH_TO_OBJECTS "/planet.obj", planetShader);


    Shader projectileShader(PATH_TO_SHADERS "/projectile.vert", PATH_TO_SHADERS "/projectile.frag");
    objectManager.addObject("projectile", PATH_TO_OBJECTS "/sphere.obj", projectileShader);
    objectManager.removeOneObject("projectile");



    std::string terrainShaderPrefix = "terrain";
    terrainManager terrain(terrainShaderPrefix);


    Shader cubeMapShader = Shader(PATH_TO_SHADERS "/cubemap.vert", PATH_TO_SHADERS "/cubemap.frag");
	objectManager.addObject("cubeMap", PATH_TO_OBJECTS "/cubeMap.obj", cubeMapShader);

	Shader reflexionShader = Shader(PATH_TO_SHADERS "/sphereReflexion.vert", PATH_TO_SHADERS "/sphereReflexion.frag");
	objectManager.addObject("reflectiveSphere", PATH_TO_OBJECTS "/sphere.obj", reflexionShader);

	//Rendering reflective sphere with texture shader
    float ambient = 0.1;
    float diffuse = 0.5;
    float specular = 0.8;
    glm::vec3 materialColour = glm::vec3(0.5f, 0.6, 0.8);

    reflexionShader.use();
    reflexionShader.setFloat("shininess", 32.0f);
    reflexionShader.setVector3f("materialColour", materialColour);
    reflexionShader.setFloat("light.ambient_strength", ambient);
    reflexionShader.setFloat("light.diffuse_strength", diffuse);
    reflexionShader.setFloat("light.specular_strength", specular);
    reflexionShader.setFloat("light.constant", 1.0);
    reflexionShader.setFloat("light.linear", 0.14);
    reflexionShader.setFloat("light.quadratic", 0.07);

	// asteroids shader
	Shader asteroidShader(PATH_TO_SHADERS "/asteroid.vert", PATH_TO_SHADERS "/asteroid.frag");

    float maxTranslation = 5.5f;
    float maxScale = 1.5f;
    float minScale = 0.5f;
    size_t numModelsTogenerate = 10;
    
    for (size_t i = 0; i < numModelsTogenerate; i++)
    {
        objectManager.addObject("fish", PATH_TO_OBJECTS "/small_green_alien.obj", textureLightingShader);
    }
    AsteroidManager asteroidManager(numModelsTogenerate, terrain);

    ObjectsData &dataFish = objectManager.objects.at("fish");

    ObjectsData &projectileData = objectManager.objects.at("projectile");

    ObjectsData &sphereData = objectManager.objects.at("sphere");
    glm::vec3 sunPosition = glm::vec3(50.0f, 20.0f, -50.0f);


    sphereData.modelMatrices[0] = glm::translate(glm::mat4(1.0f), sunPosition);
    sphereData.modelMatrices[0] = glm::scale(sphereData.modelMatrices[0], glm::vec3(0.5f, 0.5f, 0.5f));

	// Set up sun halo
    ObjectsData &sunHaloData = objectManager.objects.at("sunHalo");
    sunHaloData.modelMatrices[0] = glm::translate(glm::mat4(1.0f), sunPosition);
    sunHaloData.modelMatrices[0] = glm::scale(sunHaloData.modelMatrices[0], glm::vec3(1.0f, 1.0f, 1.0f));

	// Set up Planet
    ObjectsData& planetData = objectManager.objects.at("planet");
    glm::vec3 planetPosition = glm::vec3(25.0f, 25.0f, -25.0f);
    planetData.modelMatrices[0] = glm::translate(glm::mat4(1.0f), glm::vec3(25.0f, 25.0f, -25.0f));
    planetData.modelMatrices[0] = glm::scale(planetData.modelMatrices[0], glm::vec3(0.8f, 0.8f, 0.8f));

    ObjectsData& refSphereData = objectManager.objects.at("reflectiveSphere");
	refSphereData.modelMatrices[0] = glm::translate(refSphereData.modelMatrices[0], glm::vec3(20.0f, 1.0f, 20.0f));
	refSphereData.modelMatrices[0] = glm::scale(refSphereData.modelMatrices[0], glm::vec3(1.0f, 1.0f, 1.0f));


    unsigned int asteroidAmount = 1000;
    srand(glfwGetTime()); // initialize random seed	
    float radius = 15.0;
    float offset = 5.0f;
    for (unsigned int i = 0; i < asteroidAmount; i++) {
        objectManager.addObject("asteroid", PATH_TO_OBJECTS "/rock.obj", asteroidShader);
        ObjectsData& asteroidData = objectManager.objects.at("asteroid");

        glm::mat4 asteroidModel = glm::mat4(1.0f);
        // 1. translation: displace along circle with 'radius' in range [-offset, offset]
        float angle = (float)i / (float)asteroidAmount * 360.0f;
        float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float x = sin(angle) * radius + displacement;
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float y = displacement * 0.4f; // keep height of field smaller compared to width of x and z
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float z = cos(angle) * radius + displacement;
        asteroidModel = glm::translate(asteroidModel, (glm::vec3(x, y, z) + planetPosition));

        // 2. scale: scale between 0.05 and 0.25f
        float scale = (rand() % 20) / 100.0f + 0.05;
        asteroidModel = glm::scale(asteroidModel, glm::vec3(scale));

        // 3. rotation: add random rotation around a (semi)randomly picked rotation axis vector
        float rotAngle = (rand() % 360);
        asteroidModel = glm::rotate(asteroidModel, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

        // 4. now add to list of matrices
        asteroidData.modelMatrices[i] = asteroidModel;
    }
    
	// Load particleShader and ringImpactShader
    Shader particleShader("shaders/particle.vert", "shaders/particle.frag");
	Shader ringImpactShader("shaders/ringImpact.vert", "shaders/ringImpact.frag");

    // Create simple triangle geometry
    float vertices[] = {
        // Position             Color
        -0.5f,
        -0.5f,
        0.0f,
        1.0f,
        0.0f,
        0.0f, // Red
        0.5f,
        -0.5f,
        0.0f,
        0.0f,
        1.0f,
        0.0f, // Green
        0.0f,
        0.5f,
        0.0f,
        0.0f,
        0.0f,
        1.0f, // Blue
    };

    float quad[] = {
    -1, -1, 0,
     1, -1, 0,
    -1,  1, 0,

    -1,  1, 0,
     1, -1, 0,
     1,  1, 0
    };

    GLuint particleVAO, particleVBO, ringVAO, ringVBO, cubeMapTexture;
    glGenVertexArrays(1, &particleVAO);
    glGenBuffers(1, &particleVBO);

    glBindVertexArray(particleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);


    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glGenVertexArrays(1, &ringVAO);
    glGenBuffers(1, &ringVBO);

    glBindVertexArray(ringVAO);
    glBindBuffer(GL_ARRAY_BUFFER, ringVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

    // position only
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);


	// cube map texture setup
    glGenTextures(1, &cubeMapTexture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    //stbi_set_flip_vertically_on_load(true);

    std::string pathToCubeMap = PATH_TO_TEXTURE "/cubemaps/skybox/";

    std::map<std::string, GLenum> facesToLoad = {
        {pathToCubeMap + "right.png",GL_TEXTURE_CUBE_MAP_POSITIVE_X},
        {pathToCubeMap + "bottom.png",GL_TEXTURE_CUBE_MAP_POSITIVE_Y},
        {pathToCubeMap + "front.png",GL_TEXTURE_CUBE_MAP_POSITIVE_Z},
        {pathToCubeMap + "left.png",GL_TEXTURE_CUBE_MAP_NEGATIVE_X},
        {pathToCubeMap + "top.png",GL_TEXTURE_CUBE_MAP_NEGATIVE_Y},
        {pathToCubeMap + "back.png",GL_TEXTURE_CUBE_MAP_NEGATIVE_Z},
    };
    //load the six faces
    for (std::pair<std::string, GLenum> pair : facesToLoad) {
        loadCubemapFace(pair.first.c_str(), pair.second);
    }

	// vertex buffer for asteroid instancing (https://learnopengl.com/Advanced-OpenGL/Instancing)
    ObjectsData& asteroidData = objectManager.objects.at("asteroid");
    unsigned int instanceVBO;
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, asteroidAmount * sizeof(glm::mat4), asteroidData.modelMatrices.data(), GL_STATIC_DRAW);

	// keep the VAO of the asteroid model bound to set the instanced vertex attributes
    unsigned int astVAO = asteroidData.object.VAO;
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

    // Crosshair (insert after ringVAO creation)
    GLuint crossVAO = 0, crossVBO = 0;
    {
        float crosshairVerts[] = {
            -0.02f, 0.0f, // horizontal left
             0.02f, 0.0f, // horizontal right
             0.0f, -0.02f, // vertical down
             0.0f,  0.02f  // vertical up
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
    }

    // Main render loop
    std::cout << "Controls: W/A/S/D to move, Arrow keys to rotate, ESC to quit" << std::endl;

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 inverseModel = glm::transpose(glm::inverse(model));
    glm::vec3 light_pos = glm::vec3(1.0, 2.0, 1.5);
    
    double prev = glfwGetTime();
    double prevUpdate = prev;
    int deltaFrame = 0;
    auto fps = [&](double now)
    {
        double deltaTime = now - prev;
        double timeSinceLastUpdate = now - prevUpdate;
        deltaFrame++;
        if (timeSinceLastUpdate > 1.0)
        {
            prevUpdate = now;
            prev = now;
            const double fpsCount = (double)deltaFrame / deltaTime;
            deltaFrame = 0;
            std::cout << "\rFPS: " << fpsCount << std::flush;
        }
    };

    
    
    
    
    shaderPaths = ShaderFilePaths();
    shaderPaths.addVertexShader(PATH_TO_SHADERS "/hud.vert");
    shaderPaths.addFragmentShader(PATH_TO_SHADERS "/hud.frag");
    Shader hudShader(shaderPaths);
    hudShader.use();
    //set the texture uniform to 0
    
    objectManager.addObject("hud", PATH_TO_OBJECTS "/weapon_quad.obj", hudShader);
    
    
    
    
    glm::vec3 projectileDirection ;
    glm::vec3 muzzleOffsetLocal = glm::vec3(0.3f, -0.25f, 0.0f);
    
    
    int weaponAnimFrame = 0;
    double lastTime = glfwGetTime();
    double now = lastTime;
    float inc = 0.0f;
    while (!glfwWindowShouldClose(window))
    {   
        now = glfwGetTime();
        float deltaTime = now - lastTime;
        lastTime = now;

        processInput(window);
        // Clear screen
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = camera.GetProjectionMatrix(camera.Zoom, (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT);

        terrain.update(camera.Position);
        float heightUnderCamera = terrain.height_under_camera(camera.Position);
        //float heightAtCamera2 = terrain.terrainHeightAt(camera.Position);
        float minHeight = heightUnderCamera + 1.5f;
        if (freeFalling)
        {
            verticalVelocity -= gravity * deltaTime;
            fallingPosition.y += verticalVelocity;
            camera.Position.y = fallingPosition.y;
            if (camera.Position.y < minHeight)
            {
                camera.Position.y = minHeight;
                verticalVelocity = 0.0f;
                freeFalling = false;
            }
        }
        else
        {
            camera.Position.y = minHeight;
        }


        inc += 0.01f;

        
        asteroidManager.update(dataFish);

        uniformSetters setters;
        setters.setFloats.push_back({"time", now});
        setters.setMat4.push_back({"V", view});
        setters.setMat4.push_back({"P", projection});
        setters.setVec3.push_back({"u_view_pos", camera.Position});
        objectManager.drawObject("fish", setters);

        // draw sun
        uniformSetters sphereSetters;
        sphereSetters.setVec3.push_back({ "baseColor", glm::vec3(1.0f, 0.12f, 0.03f) });
        sphereSetters.setFloats.push_back({ "time", now });
        sphereSetters.setMat4.push_back({ "V", view });
        sphereSetters.setMat4.push_back({ "P", projection });
        sphereSetters.setVec3.push_back({ "view_pos", camera.Position });
        sphereSetters.setIntegers.push_back({ "isHalo", 0 });
        sphereSetters.setFloats.push_back({ "haloIntensity", 0.0f });
        objectManager.drawObject("sphere", sphereSetters);

        // draw halo 
        uniformSetters haloSetters;
        haloSetters.setVec3.push_back({ "baseColor", glm::vec3(1.0f, 0.18f, 0.05f) });
        haloSetters.setFloats.push_back({ "time", now });
        haloSetters.setMat4.push_back({ "V", view });
        haloSetters.setMat4.push_back({ "P", projection });
        haloSetters.setVec3.push_back({ "view_pos", camera.Position });
        haloSetters.setIntegers.push_back({ "isHalo", 1 });
        haloSetters.setFloats.push_back({ "haloIntensity", 1.35f });

        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        objectManager.drawObject("sunHalo", haloSetters);

        // restore default states after drawing halo
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);


        // draw planet
        uniformSetters planetSetters;
        planetSetters.setFloats.push_back({ "time", now });
        planetSetters.setMat4.push_back({ "V", view });
        planetSetters.setMat4.push_back({ "P", projection });
        planetSetters.setVec3.push_back({ "u_view_pos", camera.Position });
        planetSetters.setVec3.push_back({ "light.light_pos", sunPosition });
        planetSetters.setIntegers.push_back({ "planetTexture", 0 });

        objectManager.drawObject("planet", planetSetters);


        // Draw cubMap
        glDepthFunc(GL_LEQUAL);
        glDisable(GL_CULL_FACE);

        uniformSetters cubeMapSetters;
        cubeMapSetters.setMat4.push_back({ "V", view });
        cubeMapSetters.setMat4.push_back({ "P", projection });
        cubeMapSetters.setIntegers.push_back({ "cubeMapTexture", 0 });
        objectManager.drawObject("cubeMap", cubeMapSetters);

        // Resore default states
        glEnable(GL_CULL_FACE);
        glDepthFunc(GL_LESS);

        // Draw reflective sphere
        auto delta = light_pos + glm::vec3(0.0, 0.0, 2 * std::sin(now));

        uniformSetters reflectiveSetters;
        reflectiveSetters.setMat4.push_back({ "M", model });
        reflectiveSetters.setMat4.push_back({ "itM", inverseModel });
        reflectiveSetters.setMat4.push_back({ "V", view });
        reflectiveSetters.setMat4.push_back({ "P", projection });
        reflectiveSetters.setVec3.push_back({ "u_view_pos", camera.Position });
        reflectiveSetters.setVec3.push_back({ "light.light_pos", delta });
        objectManager.drawObject("reflectiveSphere", reflectiveSetters);

        // Draw asteroids
        asteroidShader.use();
        asteroidShader.setMatrix4("V", view);
        asteroidShader.setMatrix4("P", projection);
        asteroidShader.setInteger("useTexture", 1);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, asteroidData.object.textureID);

        glBindVertexArray(asteroidData.object.VAO);
        glDrawElementsInstanced(
            GL_TRIANGLES,
            static_cast<GLsizei>(asteroidData.object.indices.size()),
            GL_UNSIGNED_INT,
            0,
            asteroidAmount
        );
        glBindVertexArray(0);

 
        if (shooting) {
            timeSinceLastShot = 0.0f;
            weaponAnimFrame = 1;
            shooting = false;

            projectileDirection = camera.Front;

            glm::vec3 weaponOrigin =
                camera.Position +
                (camera.Right * muzzleOffsetLocal.x) +
                (camera.Up * muzzleOffsetLocal.y) +
                (camera.Front * -muzzleOffsetLocal.z);

            const float maxDist = 50.0f;
            const float step = 0.25f;

            // 1) Raycast depuis le centre de la caméra -> point sous le viseur
            glm::vec3 camImpact = camera.Position;
            {
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
            }

            // 2) Direction du tir : de l'arme vers le point trouvé par la caméra
            glm::vec3 shotDir = glm::normalize(camImpact - weaponOrigin);

            // 3) Raycast depuis l'arme le long de shotDir pour obtenir l'impact "physique"
            glm::vec3 impactPoint = weaponOrigin;
            {
                float traveled = 0.0f;
                glm::vec3 p = weaponOrigin;
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
            }

            // Spawn particules (inchangé)
            for (int p = 0; p < 25; p++)
            {
                Particle part;
                part.position = impactPoint;

                float theta = ((rand() % 100) / 100.0f) * 6.28f;
                float phi = ((rand() % 100) / 100.0f) * 3.14f;

                glm::vec3 dir = glm::vec3(
                    cos(theta) * sin(phi),
                    cos(phi),
                    sin(theta) * sin(phi)
                );

                glm::vec3 finalDir = glm::normalize(
                    dir * 1.5f + (-projectileDirection) * 2.0f
                );

                part.velocity = finalDir * (5.0f + (rand() % 300) / 100.0f);
                part.life = 0.2f + (rand() % 100) / 400.0f;
                part.initialLife = part.life;
                part.scale = 0.25f + (rand() % 100) / 500.0f;
                part.rotationAxis = glm::normalize(glm::vec3(dir.x, dir.y, dir.z + 0.1f));
                part.rotationAngle = rand() % 360;

                particles.push_back(part);
            }

            // Trajectoire visuelle du projectile : weaponOrigin -> impactPoint
            projectileStarts.push_back(weaponOrigin);
            projectileEnds.push_back(impactPoint);
            projectileBirthTimes.push_back(now);

            // Anneau d'impact
            ImpactRing ring;
            ring.position = impactPoint;
            ring.life = 0.6f;
            ring.initialLife = ring.life;
            ring.radius = 0.1f;
            impactRings.push_back(ring);
        } else {
            timeSinceLastShot += deltaTime;
            if (timeSinceLastShot>= 0.1f){
                weaponAnimFrame = 2;
                if (timeSinceLastShot >= 0.2f){
                    weaponAnimFrame = 3;
                }
                if (timeSinceLastShot >= 0.3f){
                    weaponAnimFrame = 0;
                }
            }
        }

        for (size_t i = 0; i < projectileEnds.size(); )
        {
            if (now - projectileBirthTimes[i] > 0.15f)
            {
                projectileStarts.erase(projectileStarts.begin() + i);
                projectileEnds.erase(projectileEnds.begin() + i);
                projectileBirthTimes.erase(projectileBirthTimes.begin() + i);
            }
            else
            {
                ++i;
            }
        }

        projectileData.modelMatrices.clear();

        for (size_t i = 0; i < projectileEnds.size(); ++i)
        {
            glm::vec3 start = projectileStarts[i];
            glm::vec3 end = projectileEnds[i];

            glm::vec3 delta = end - start;

            float distance = glm::length(delta);

            glm::vec3 forwardDir;

            if (distance > 0.0001f)
                forwardDir = glm::normalize(delta);
            else
                forwardDir = camera.Front;

            distance = glm::max(distance, 0.15f);

            glm::vec3 midPoint = (start + end) * 0.5f;

            glm::mat4 trans = glm::translate(glm::mat4(1.0f), midPoint);

            glm::mat4 rot = rotateAtoB(
                glm::vec3(0.0f, 0.0f, 1.0f),
                forwardDir
            );

            glm::mat4 scaleMat = glm::scale(
                glm::mat4(1.0f),
                glm::vec3(0.001f, 0.001f, distance * 0.5f)
            );

            projectileData.modelMatrices.push_back(
                trans * rot * scaleMat
            );
        }

        terrain.addImpact(impactRings, now);
        terrain.draw(view, projection, camera.Position, glm::vec3(0.0f, 100.0f, 0.0f));

        // On donne une couleur brillante (ex: rouge vif lumineux)
        uniformSetters projectileSetters;
        projectileSetters.setFloats.push_back({ "time", now });
        projectileSetters.setMat4.push_back({ "V", view });
        projectileSetters.setMat4.push_back({ "P", projection });
        projectileSetters.setVec3.push_back({ "baseColor", glm::vec3(2.5f, 0.3f, 0.3f) });
        projectileSetters.setVec3.push_back({ "center", glm::vec3(0.0f, -0.5f, -8.0f) });
        projectileSetters.setVec3.push_back({ "view_pos", camera.Position });

        // -- Rendu FLUO additif pour le laser --
        glDepthMask(GL_FALSE);                    // Ne pas écrire dans le Z-buffer
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);        // Rendu Additif
        glDisable(GL_CULL_FACE);

        objectManager.drawObject("projectile", projectileSetters);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Restauration standard
        glDepthMask(GL_TRUE);

        // --- MISE À JOUR ET RENDU DES PARTICULES ---
        particleShader.use();
        particleShader.setMatrix4("V", view);
        particleShader.setMatrix4("P", projection);

        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        glBindVertexArray(particleVAO);

        for (size_t i = 0; i < particles.size(); )
        {
            Particle& p = particles[i];

            p.life -= deltaTime;
            if (p.life <= 0.0f)
            {
                particles.erase(particles.begin() + i);
                continue;
            }

            // mouvement simple + bruit léger
            glm::vec3 noise = glm::sphericalRand(1.0f) * 0.2f;
            p.velocity += noise * deltaTime;
            p.velocity *= 0.98f;
            p.position += p.velocity * deltaTime;

            float t = p.life / p.initialLife;

            glm::mat4 model = glm::translate(glm::mat4(1.0f), p.position);

            particleShader.setMatrix4("M", model);
            particleShader.setFloat("size", p.scale * (0.3f + t));

            glDrawArrays(GL_TRIANGLES, 0, 3);

            ++i;
        }

        ringImpactShader.use();
        ringImpactShader.setMatrix4("V", view);
        ringImpactShader.setMatrix4("P", projection);

        glBindVertexArray(ringVAO);

        for (size_t i = 0; i < impactRings.size(); )
        {
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
            glDrawArrays(GL_TRIANGLES, 0, 6); // On dessine l'anneau d'impact

            i++;
        }

        glBindVertexArray(0);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);

        // --------------------------------------------

        // Draw crosshair
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

        fps(now);
        glfwSwapBuffers(window);
        glfwPollEvents();
        //sleep 0.1 seconds

    }

    // Cleanup
    glDeleteBuffers(1, &particleVBO);
    glDeleteVertexArrays(1, &particleVAO);
	glDeleteBuffers(1, &ringVBO);
	glDeleteVertexArrays(1, &ringVAO);
    glDeleteBuffers(1, &crossVBO);
    glDeleteVertexArrays(1, &crossVAO);
    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "Application closed." << std::endl;
    return 0;
}
