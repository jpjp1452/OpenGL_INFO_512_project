#include <iostream>

// Include glad before GLFW to avoid header conflicts
#include <glad/glad.h>
#include <GLFW/glfw3.h>

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

const int WIDTH = 800;
const int HEIGHT = 600;

#define MouvementMultiplier 0.6f
#define RotationMultiplier 1.9f
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

void processInput(GLFWwindow *window)
{
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

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        camera.ProcessKeyboardRotation(0.0f, 1.0f, 0.1f * RotationMultiplier);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        camera.ProcessKeyboardRotation(0.0f, -1.0f, 0.1f * RotationMultiplier);
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        camera.ProcessKeyboardRotation(-1.0f, 0.0f, 0.1f * RotationMultiplier);
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        camera.ProcessKeyboardRotation(1.0f, 0.0f, 0.1f * RotationMultiplier);
}


glm::mat4 rotateAtoB(glm::vec3 a, glm::vec3 b,glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f))
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


int main()
{
    std::cout << "Initializing OpenGL Application..." << std::endl;

    // Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // Create window
    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "OpenGL Project", nullptr, nullptr);
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
    glViewport(0, 0, WIDTH, HEIGHT);

    std::cout << "Loading model..." << std::endl;
    ObjectManager objectManager;
    Shader textureShader(PATH_TO_SHADERS "/texture.vert", PATH_TO_SHADERS "/texture.frag");



    Shader sphereShader(PATH_TO_SHADERS "/sphere.vert", PATH_TO_SHADERS "/sphere.frag");
    objectManager.addObject("sphere", PATH_TO_OBJECTS "/sphere.obj", sphereShader);



    float maxTranslation = 5.5f;
    float maxScale = 1.5f;
    float minScale = 0.5f;
    size_t numModelsTogenerate = 200;
    BoidManager boidManager(numModelsTogenerate);
    std::vector<float> fishScales(numModelsTogenerate);

    for (size_t i = 0; i < numModelsTogenerate; i++)
    {
        objectManager.addObject("fish", PATH_TO_OBJECTS "/Untitled.obj", textureShader);
        ObjectsData &data = objectManager.objects.at("fish");
        // Random translation
        float tx = ((rand() / (float)RAND_MAX) - 0.5f) * 2.0f * maxTranslation;
        float ty = ((rand() / (float)RAND_MAX) - 0.5f) * 2.0f * maxTranslation;
        float tz = ((rand() / (float)RAND_MAX) - 0.5f) * 2.0f * maxTranslation;
        // Random scale
        float scale = minScale + (rand() / (float)RAND_MAX) * (maxScale - minScale);
        fishScales[i] = scale;
        boidManager.boids[i].position = glm::vec3(tx, ty, tz);

        glm::mat4 rotation = rotateAtoB(glm::vec3(1.0f, 0.0f, 0.0f), boidManager.boids[i].velocity);
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), boidManager.boids[i].position);
        glm::mat4 scaleM = glm::scale(glm::mat4(1.0f), glm::vec3(scale, scale, scale));
        data.modelMatrices[i] = translation * rotation * scaleM;


    }

    ObjectsData &data = objectManager.objects.at("fish");
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();

    for(size_t i = 0; i < data.object.positions.size(); i++)
    {
        glm::vec4 pos = glm::vec4(data.object.positions[i], 1.0f);
        pos = data.modelMatrices[0] * pos;
        minX = std::min(minX, pos.x);
        maxX = std::max(maxX, pos.x);
        minY = std::min(minY, pos.y);
        maxY = std::max(maxY, pos.y);
        minZ = std::min(minZ, pos.z);
        maxZ = std::max(maxZ, pos.z);
    }



    

    std::cout << "Model bounding box:" << std::endl;
    std::cout << "X: [" << minX << ", " << maxX << "]" << std::endl;
    std::cout << "Y: [" << minY << ", " << maxY << "]" << std::endl;
    std::cout << "Z: [" << minZ << ", " << maxZ << "]" << std::endl;



    // Set the center as camera position
    boidManager.center = camera.Position;
    
    






    ObjectsData &sphereData = objectManager.objects.at("sphere");
    //divide it by 10 
    sphereData.modelMatrices[0] = glm::translate(sphereData.modelMatrices[0], glm::vec3(0.0f, -0.5f, -8.0f));
    sphereData.modelMatrices[0] = glm::scale(sphereData.modelMatrices[0], glm::vec3(0.1f, 0.1f, 0.1f));

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
        if (timeSinceLastUpdate > 0.5)
        {
            prevUpdate = now;
            prev = now;
            const double fpsCount = (double)deltaFrame / deltaTime;
            deltaFrame = 0;
            std::cout << "\rFPS: " << fpsCount << std::flush;
        }
    };
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    double lastTime = glfwGetTime();
    double now = lastTime;
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        // Clear screen
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use shader and set uniforms
        shader.use();

        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = camera.GetProjectionMatrix(camera.Zoom, (float)WIDTH / (float)HEIGHT);

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
        sphereSetters.setVec3.push_back({"baseColor", glm::vec3(0.01f, 0.08f, 0.88f)});
        sphereSetters.setFloats.push_back({"time", now});
        sphereSetters.setMat4.push_back({"V", view});
        sphereSetters.setMat4.push_back({"P", projection});
        sphereSetters.setVec3.push_back({"center", glm::vec3(0.0f, -0.5f, -8.0f)});
        sphereSetters.setVec3.push_back({"view_pos", camera.Position});


        boidManager.center = camera.Position;
 
        boidManager.update((now - lastTime)*1.0f);



        //boidManager.update((now - lastTime)*1.0f);
        ObjectsData &data = objectManager.objects.at("fish");
        for (size_t i = 0; i < boidManager.boids.size(); i++)
        {
            glm::mat4 rotation = rotateAtoB(glm::vec3(1.0f, 0.0f, 0.0f), boidManager.boids[i].velocity);
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), boidManager.boids[i].position);
            glm::mat4 scaleM = glm::scale(glm::mat4(1.0f), glm::vec3(fishScales[i], fishScales[i], fishScales[i]));
            data.modelMatrices[i] = translation * rotation * scaleM;
        }




        objectManager.drawObject("fish", setters);
        glDepthMask(GL_FALSE);
        objectManager.drawObject("sphere", sphereSetters);
        glDepthMask(GL_TRUE);
        fps(now);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "Application closed." << std::endl;
    return 0;
}
