#include <iostream>

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


const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

#include "terrainManager.h"

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
    
	Shader textureLightingShader = Shader(PATH_TO_SHADERS "/textureLighting.vert", PATH_TO_SHADERS "/textureLighting.frag");

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
    objectManager.addObject("sphere", PATH_TO_OBJECTS "/sphere.obj", sphereShader);
    objectManager.addObject("sunHalo", PATH_TO_OBJECTS "/sphere.obj", sphereShader);

    // Create terrain shader with tessellation
    ShaderFilePaths terrainShaderPaths;
    terrainShaderPaths.addVertexShader(PATH_TO_SHADERS "/terrain.vert");
    terrainShaderPaths.addTessellationControlShader(PATH_TO_SHADERS "/terrain.tcs");
    terrainShaderPaths.addTessellationEvaluationShader(PATH_TO_SHADERS "/terrain.tes");
    terrainShaderPaths.addFragmentShader(PATH_TO_SHADERS "/terrain.frag");
    Shader terrainShader(terrainShaderPaths);
    terrainManager terrain(terrainShader);


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
    size_t numModelsTogenerate = 200;
    BoidManager boidManager(numModelsTogenerate);
    std::vector<float> fishScales(numModelsTogenerate);

    for (size_t i = 0; i < numModelsTogenerate; i++)
    {
        objectManager.addObject("fish", PATH_TO_OBJECTS "/Untitled.obj", textureLightingShader);
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



    ObjectsData &sphereData = objectManager.objects.at("sphere");
    sphereData.modelMatrices[0] = glm::translate(glm::mat4(1.0f), glm::vec3(50.0f, 20.0f, -50.0f));
    sphereData.modelMatrices[0] = glm::scale(sphereData.modelMatrices[0], glm::vec3(0.5f, 0.5f, 0.5f));

	// Set up sun halo
    ObjectsData &sunHaloData = objectManager.objects.at("sunHalo");
    sunHaloData.modelMatrices[0] = glm::translate(glm::mat4(1.0f), glm::vec3(50.0f, 20.0f, -50.0f));
    sunHaloData.modelMatrices[0] = glm::scale(sunHaloData.modelMatrices[0], glm::vec3(1.0f, 1.0f, 1.0f));
    

    ObjectsData& refSphereData = objectManager.objects.at("reflectiveSphere");
	refSphereData.modelMatrices[0] = glm::translate(refSphereData.modelMatrices[0], glm::vec3(20.0f, 1.0f, 20.0f));
	refSphereData.modelMatrices[0] = glm::scale(refSphereData.modelMatrices[0], glm::vec3(1.0f, 1.0f, 1.0f));


    unsigned int asteroidAmount = 1000;
    srand(glfwGetTime()); // initialize random seed	
    float radius = 75.0;
    float offset = 15.0f;
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
        asteroidModel = glm::translate(asteroidModel, glm::vec3(x, y, z));

        // 2. scale: scale between 0.05 and 0.25f
        float scale = (rand() % 20) / 100.0f + 0.05;
        asteroidModel = glm::scale(asteroidModel, glm::vec3(scale));

        // 3. rotation: add random rotation around a (semi)randomly picked rotation axis vector
        float rotAngle = (rand() % 360);
        asteroidModel = glm::rotate(asteroidModel, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

        // 4. now add to list of matrices
        asteroidData.modelMatrices[i] = asteroidModel;
    }



    // Set the center as camera position
    boidManager.center = camera.Position;
    
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

    GLuint VAO, VBO, cubeMapTexture;
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
        glm::mat4 projection = camera.GetProjectionMatrix(camera.Zoom, (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT);

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
        sphereSetters.setVec3.push_back({ "baseColor", glm::vec3(1.0f, 0.12f, 0.03f) });
        sphereSetters.setFloats.push_back({ "time", now });
        sphereSetters.setMat4.push_back({ "V", view });
        sphereSetters.setMat4.push_back({ "P", projection });
        sphereSetters.setVec3.push_back({ "view_pos", camera.Position });
        sphereSetters.setIntegers.push_back({ "isHalo", 0 });
        sphereSetters.setFloats.push_back({ "haloIntensity", 0.0f });

        uniformSetters haloSetters;
        haloSetters.setVec3.push_back({ "baseColor", glm::vec3(1.0f, 0.18f, 0.05f) });
        haloSetters.setFloats.push_back({ "time", now });
        haloSetters.setMat4.push_back({ "V", view });
        haloSetters.setMat4.push_back({ "P", projection });
        haloSetters.setVec3.push_back({ "view_pos", camera.Position });
        haloSetters.setIntegers.push_back({ "isHalo", 1 });
        haloSetters.setFloats.push_back({ "haloIntensity", 1.35f });


        
        boidManager.center = camera.Position;
        //boidManager.update((now - lastTime)*1.0f);

        

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

        // draw sun
        objectManager.drawObject("sphere", sphereSetters);

        // draw halo 
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        objectManager.drawObject("sunHalo", haloSetters);

		// default states restauration
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);


        // Draw terrain
        terrainShader.use();
        glm::mat4 terrainModel = glm::scale(glm::mat4(1.0f), glm::vec3(50.0f, 50.0f, 50.0f));
        terrainModel = terrainModel;
        terrainShader.setMatrix4("M", terrainModel);
        terrainShader.setMatrix4("V", view);
        terrainShader.setMatrix4("P", projection);
        terrainShader.setVector3f("u_view_pos", camera.Position);
        terrainShader.setVector3f("lightPos", camera.Position + glm::vec3(0.0f, 10.0f, 0.0f));
        terrainShader.setInteger("heightMap", 0);
        terrainShader.setInteger("textureBrickColor", 1);
        terrainShader.setInteger("textureBrickBump", 2);
        glPatchParameteri(GL_PATCH_VERTICES, 4);
        terrain.draw();

		// Draw cubMap
		glDepthFunc(GL_LEQUAL);     // Accepte une profondeur de 1.0
		glDisable(GL_CULL_FACE);    // D�sactive le culling car la cam�ra est � l'int�rieur du cube

		cubeMapShader.use();
		cubeMapShader.setMatrix4("V", view);
		cubeMapShader.setMatrix4("P", projection);
		cubeMapShader.setInteger("cubemapTexture", 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
        uniformSetters cubeMapSetters;
		objectManager.drawObject("cubeMap", cubeMapSetters);

		// On restaure les �tats par d�faut
		glEnable(GL_CULL_FACE);
		glDepthFunc(GL_LESS);

		// Draw reflective sphere
        reflexionShader.use();

        reflexionShader.setMatrix4("M", model);
        reflexionShader.setMatrix4("itM", inverseModel);
        reflexionShader.setMatrix4("V", view);
        reflexionShader.setMatrix4("P", projection);
        reflexionShader.setVector3f("u_view_pos", camera.Position);

        auto delta = light_pos + glm::vec3(0.0, 0.0, 2 * std::sin(now));
        shader.setVector3f("light.light_pos", delta);
        uniformSetters reflectiveSetters;

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
