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

#define SPEED_FACTOR 1.0f
#define MouvementMultiplier 0.6f * SPEED_FACTOR
#define RotationMultiplier 2.9f * SPEED_FACTOR
#define JUMP_VELOCITY 0.2f * SPEED_FACTOR

#define PROJECTILE_SPEED 1.5f * SPEED_FACTOR
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float gravity = 0.1f;
float verticalVelocity = 0.0f;
bool freeFalling = false;
bool shooting = false;
float delayBetweenShots = 0.02f;
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







int main()
{  
/*
    int maxY = 512;
    int maxX = 512;
    unsigned char *dataC = new unsigned char[maxX * maxY*3];
    for (int y = 0; y < maxY; y++)
    {
        for (int x = 0; x < maxX; x++)
        {
            float value = glm::perlin(glm::vec2(x, y) * 0.01f);
            unsigned colorValue = (value + 1.0f) * (127.5f*3);
            dataC[(y * maxX + x) * 3 + 0] = (colorValue > 255) ? 255 : colorValue;
            colorValue -= dataC[(y * maxX + x) * 3 + 0];
            dataC[(y * maxX + x) * 3 + 1] = (colorValue > 255) ? 255 : colorValue;
            colorValue -= dataC[(y * maxX + x) * 3 + 1];
            dataC[(y * maxX + x) * 3 + 2] = (colorValue > 255) ? 255 : colorValue;
        }
    }
    // Save the image with stb_image_write
    stbi_write_png("perlin_noise.png", maxX, maxY, 3, dataC, maxX*3);
    delete[] dataC;
    exit(0);
*/


    







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


    Shader sphereShader(PATH_TO_SHADERS "/sphere.vert", PATH_TO_SHADERS "/sphere.frag");

    size_t how_many_spheres = 1;
    objectManager.addObject("sphere", PATH_TO_OBJECTS "/sphere.obj", sphereShader);
    objectManager.removeOneObject("sphere");









    std::string terrainShaderPrefix = "terrain";
    terrainManager terrain(terrainShaderPrefix);



    float maxTranslation = 5.5f;
    float maxScale = 1.5f;
    float minScale = 0.5f;
    size_t numModelsTogenerate = 10;
    for (size_t i = 0; i < numModelsTogenerate; i++)
    {
        objectManager.addObject("fish", PATH_TO_OBJECTS "/small_green_alien.obj", textureLightingShader);
    }
    AsteroidManager asteroidManager(numModelsTogenerate, terrain);

    ObjectsData &data = objectManager.objects.at("fish");


    ObjectsData &sphereData = objectManager.objects.at("sphere");
    //divide it by 10 

    // Load shader
    Shader shader("shaders/basic.vert", "shaders/basic.frag");

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

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Main render loop
    std::cout << "Controls: W/A/S/D to move, Arrow keys to rotate, ESC to quit" << std::endl;

    glm::mat4 model = glm::mat4(1.0f);
    
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

    double lastTime = glfwGetTime();
    double now = lastTime;
    float inc = 0.0f;




    shaderPaths = ShaderFilePaths();
    shaderPaths.addVertexShader(PATH_TO_SHADERS "/hud.vert");
    shaderPaths.addFragmentShader(PATH_TO_SHADERS "/hud.frag");
    Shader hudShader(shaderPaths);
    hudShader.use();
    //set the texture uniform to 0

    objectManager.addObject("hud", PATH_TO_OBJECTS "/weapon_quad.obj", hudShader);




    glm::vec3 projectileDirection ;
    glm::vec3 projectilePosition ;
    std::vector<glm::vec3> projectileDirections;
    std::vector<glm::vec3> projectilePositions;







    int weaponAnimFrame = 0;
    while (!glfwWindowShouldClose(window))
    {   
        // Clear screen
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        processInput(window);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = camera.GetProjectionMatrix(camera.Zoom, (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT);

        terrain.update(camera.Position);
        float heightUnderCamera = terrain.height_under_camera(camera.Position);
        //float heightAtCamera2 = terrain.terrainHeightAt(camera.Position);



        float minHeight = heightUnderCamera + 1.5f;
        if (freeFalling)
        {
            verticalVelocity -= gravity * (now - lastTime);
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
        terrain.draw(view, projection, camera.Position, glm::vec3(0.0f, 100.0f, 0.0f));


        inc += 0.01f;

        
        asteroidManager.update(data);






        // Use shader and set uniforms
        shader.use();
        shader.setMatrix4("M", model);
        shader.setMatrix4("V", view);
        shader.setMatrix4("P", projection);

        // Render triangle
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        lastTime = now;
        now = glfwGetTime();




        uniformSetters setters;
        setters.setFloats.push_back({"time", now});
        setters.setMat4.push_back({"V", view});
        setters.setMat4.push_back({"P", projection});
        setters.setVec3.push_back({"u_view_pos", camera.Position});
        
        uniformSetters sphereSetters;
        //base color is blue
        sphereSetters.setVec3.push_back({"baseColor", glm::vec3(0.88f, 0.88f, 0.18f)});
        sphereSetters.setFloats.push_back({"time", now});
        sphereSetters.setMat4.push_back({"V", view});
        sphereSetters.setMat4.push_back({"P", projection});
        sphereSetters.setVec3.push_back({"center", glm::vec3(0.0f, -0.5f, -8.0f)});
        sphereSetters.setVec3.push_back({"view_pos", camera.Position});
        objectManager.drawObject("fish", setters);



        uniformSetters hudSetters;

        if (shooting){
            timeSinceLastShot = 0.0f;
            weaponAnimFrame = 1;
            shooting = false;
            projectileDirection = camera.Front;
            projectilePosition = camera.Position;
            projectileDirections.push_back(projectileDirection);
            projectilePositions.push_back(projectilePosition);
            objectManager.addObject("sphere", PATH_TO_OBJECTS "/sphere.obj", sphereShader);




        }
        else{
            timeSinceLastShot += now - lastTime;
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
        for (size_t i = 0; i < projectilePositions.size(); i++){
            float distance = glm::length(projectilePositions[i] - camera.Position);
            float heightUnderProjectile = terrain.terrainHeightAt(projectilePositions[i]);
            float dif = projectilePositions[i].y - heightUnderProjectile;
            std::cout << "diff: " << dif << std::endl;


            if (distance > 50.0f || projectilePositions[i].y < heightUnderProjectile){
                projectilePositions.erase(projectilePositions.begin() + i);
                projectileDirections.erase(projectileDirections.begin() + i);
                i--;
                objectManager.removeOneObject("sphere");
            }
            else{
                projectilePositions[i] += projectileDirections[i] * PROJECTILE_SPEED;
                projectilePositions[i].y -= gravity * (now - lastTime);
            }
        }

        std::cout << "Number of projectiles: " << projectilePositions.size() << std::endl;
        std::cout << "modelMatrices size: " << sphereData.modelMatrices.size() << std::endl;
        for (size_t i = 0; i <sphereData.modelMatrices.size(); i++){
            sphereData.modelMatrices[i] = glm::translate(glm::mat4(1.0f), projectilePositions[i]) * glm::scale(glm::mat4(1.0f), glm::vec3(0.02f));
        }

  

        
        glDepthMask(GL_FALSE);
        objectManager.drawObject("sphere", sphereSetters);
        glDepthMask(GL_TRUE);




        hudSetters.setIntegers.push_back({"weaponFrame", weaponAnimFrame});
        objectManager.drawObject("hud", hudSetters);




        fps(now);
        glfwSwapBuffers(window);
        glfwPollEvents();
        //sleep 0.1 seconds
    }

    // Cleanup
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "Application closed." << std::endl;
    return 0;
}
