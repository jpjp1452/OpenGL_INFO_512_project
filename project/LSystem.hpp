#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include "shader.h"

#ifndef PATH_TO_SHADERS
#define PATH_TO_SHADERS "shaders"
#endif

struct Rule
{
    std::string from;
    std::string to;
};

struct RotationXYZ
{
    float x;
    float y;
    float z;
};
struct DecreasingFactors
{
    float growing;
    float branching;
};

struct GrowingFactors // not really needed since we can just scale
{
    float x;
    float y;
    float z;
};

struct ProceduralParameters
{
    std::string inputString;
    std::vector<Rule> rules;
    size_t iterations;
    RotationXYZ angles;
    GrowingFactors growingFactors;
    DecreasingFactors decreaseFactors;
    float leafStartFactor;
    float amplificator;
};

// find from in input and replace it by to
void replace(std::string &input, const std::string &from, const std::string &to)
{
    if (from.empty())
    {
        return;
    }

    size_t pos = 0;
    const size_t from_length = from.length();
    const size_t to_length = to.length();
    while ((pos = input.find(from, pos)) != std::string::npos)
    {
        input.replace(pos, from_length, to);
        pos += to_length;
    }
}

// apply the rules to the input string for a given number of iterations, then replace all non-terminal characters by 'F' and return the final string
std::string applyRules(std::string inputString, const std::vector<Rule> &rules, size_t iterations)
{
    std::string currentString = inputString;
    for (size_t i = 0; i < iterations; i++)
    {
        for (const auto &rule : rules)
        {
            replace(currentString, rule.from, rule.to);
        }
    }
    std::vector<Rule> removedNonTerminalCharacter;
    for (const auto &rule : rules)
    {
        Rule newRule;
        newRule.from = rule.from;
        newRule.to = "F";
        removedNonTerminalCharacter.push_back(newRule);
    }
    for (const auto &rule : removedNonTerminalCharacter)
    {
        replace(currentString, rule.from, rule.to);
    }
    std::cout << "Final string after " << iterations << " iterations: " << currentString << std::endl;
    return currentString;
}

struct Segment
{
    glm::vec3 prevStart; // the start of the previous segment
    glm::vec3 start;   // the start of the segment
    glm::vec3 end;     // the end of the segment
    float factor;       // the factor to apply to radius when converting to a cylinder
    float prevFactor;   // the factor of the previous segment, used to have smoother transition between segments of different factors
};

// + - rotation towards x axis
// * / rotation towards y axis
// ^ & rotation towards z axis
void rotateDirectionToX(glm::vec3 &direction, float angle)
{
    direction = glm::rotateX(direction, glm::radians(angle));
}
void rotateDirectionToY(glm::vec3 &direction, float angle)
{
    direction = glm::rotateY(direction, glm::radians(angle));
}
void rotateDirectionToZ(glm::vec3 &direction, float angle)
{
    direction = glm::rotateZ(direction, glm::radians(angle));
}



// recursive function to simulate stack behavior of brackets, returns the index of next index to read after
//add segment to segments vector when reading F, and update the current direction when reading + - * / ^ &, when reading [ call recursively and update the current direction and branching segment, when reading ] return
size_t convertToSegmentsHelper(const std::string &lSystemString, std::vector<Segment> &segments, GrowingFactors &growingFactors, DecreasingFactors &decreaseFactor, RotationXYZ &angles, size_t startAt, float factor, Segment branchingSegment, glm::vec3 currentDirection)
{
    float currentFactor = factor;
    for (size_t i = startAt; i < lSystemString.length(); i++)
    {
        char c = lSystemString[i];
        if (c == 'F')
        {
            Segment newSegment;
            newSegment.prevStart = branchingSegment.start;
            newSegment.start = branchingSegment.end;

            glm::vec3 growth(currentDirection.x * growingFactors.x, currentDirection.y * growingFactors.y, currentDirection.z * growingFactors.z);
            // growth *= currentFactor;
            newSegment.end = branchingSegment.end + growth;
            newSegment.prevFactor = branchingSegment.factor;
            currentFactor *= decreaseFactor.growing;
            newSegment.factor = currentFactor;
            segments.push_back(newSegment);
            branchingSegment = newSegment;
        }
        else if (c == '+')
        {
            rotateDirectionToX(currentDirection, angles.x);
        }
        else if (c == '-')
        {
            rotateDirectionToX(currentDirection, -angles.x);
        }
        else if (c == '*')
        {
            rotateDirectionToY(currentDirection, angles.y);
        }
        else if (c == '/')
        {
            rotateDirectionToY(currentDirection, -angles.y);
        }
        else if (c == '^')
        {
            rotateDirectionToZ(currentDirection, angles.z);
        }
        else if (c == '&')
        {
            rotateDirectionToZ(currentDirection, -angles.z);
        }
        else if (c == '[')
        {
            size_t newStartAt = i + 1;
            i = convertToSegmentsHelper(lSystemString, segments, growingFactors, decreaseFactor, angles, newStartAt, currentFactor * decreaseFactor.branching, branchingSegment, currentDirection);
        }
        else if (c == ']')
        {
            return i;
        }
    }
    return lSystemString.length() - 1;
}


// start from the base segment and apply the rules to convert the final string into segments, then return the vector of segments
std::vector<Segment> convertToSegments(const std::string &lSystemString, DecreasingFactors &decreaseFactor, RotationXYZ &angles, GrowingFactors &growingFactors)
{
    int openBrackets = 0;
    std::vector<Segment> segments;
    Segment baseSegment;
    baseSegment.prevStart = glm::vec3(0.0f, -2.0f, 0.0f);
    baseSegment.start = glm::vec3(0.0f, -1.0f, 0.0f);
    baseSegment.end = glm::vec3(0.0f, 0.0f, 0.0f);
    baseSegment.factor = 1.0f;
    baseSegment.prevFactor = 1.0f;
    convertToSegmentsHelper(lSystemString, segments, growingFactors, decreaseFactor, angles, 0, 1.0f, baseSegment, glm::vec3(0.0f, 1.0f, 0.0f));
    return segments;
}

class ProceduralObject
{
public:
    std::vector<Segment> segments;
    GLuint VAO, VBO;
    GLuint textureID1, textureID2;
    Shader &shader;
    float leafStartFactor;
    float amplificator;

    ProceduralObject(ProceduralParameters params, Shader &shaderPath, std::string tex1, std::string tex2) : shader(shaderPath), textureID1(0), textureID2(0)
    {

        leafStartFactor = params.leafStartFactor;
        amplificator = params.amplificator;

        std::string output = applyRules(params.inputString, params.rules, params.iterations);
        segments = convertToSegments(output, params.decreaseFactors, params.angles, params.growingFactors);
        std::cout << "ProceduralObject initialized with " << segments.size() << " segments." << std::endl;

        // Create VAO and VBO for each segment

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        glBufferData(GL_ARRAY_BUFFER, sizeof(Segment) * segments.size(), segments.data(), GL_STATIC_DRAW);

        auto att_pos = glGetAttribLocation(shader.ID, "inPrevStart");
        if (att_pos != -1)
        {
            glEnableVertexAttribArray(att_pos);
            glVertexAttribPointer(att_pos, 3, GL_FLOAT, GL_FALSE, sizeof(Segment), (void *)offsetof(Segment, prevStart));
        }
        auto att_start = glGetAttribLocation(shader.ID, "inStart");
        if (att_start != -1)
        {
            glEnableVertexAttribArray(att_start);
            glVertexAttribPointer(att_start, 3, GL_FLOAT, GL_FALSE, sizeof(Segment), (void *)offsetof(Segment, start));
        }
        auto att_end = glGetAttribLocation(shader.ID, "inEnd");
        if (att_end != -1)
        {
            glEnableVertexAttribArray(att_end);
            glVertexAttribPointer(att_end, 3, GL_FLOAT, GL_FALSE, sizeof(Segment), (void *)offsetof(Segment, end));
        }
        auto att_factor = glGetAttribLocation(shader.ID, "inFactor");
        if (att_factor != -1)
        {
            glEnableVertexAttribArray(att_factor);
            glVertexAttribPointer(att_factor, 1, GL_FLOAT, GL_FALSE, sizeof(Segment), (void *)offsetof(Segment, factor));
        }
        auto att_prevFactor = glGetAttribLocation(shader.ID, "inPrevFactor");
        if (att_prevFactor != -1)
        {
            glEnableVertexAttribArray(att_prevFactor);
            glVertexAttribPointer(att_prevFactor, 1, GL_FLOAT, GL_FALSE, sizeof(Segment), (void *)offsetof(Segment, prevFactor));
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // Load textures 1 and 2
        unsigned int width, height;
        unsigned char *data;
        if (!tex1.empty())
        {
            data = stbi_load(tex1.c_str(), (int *)&width, (int *)&height, 0, 3);
            if (data)
            {
                glGenTextures(1, &textureID1);
                glBindTexture(GL_TEXTURE_2D, textureID1);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                glGenerateMipmap(GL_TEXTURE_2D);
                stbi_image_free(data);
            }
            else
            {
                std::cout << "Failed to load texture: " << tex1 << std::endl;
            }
        }
        if (!tex2.empty())
        {
            data = stbi_load(tex2.c_str(), (int *)&width, (int *)&height, 0, 3);
            if (data)
            {
                glGenTextures(1, &textureID2);
                glBindTexture(GL_TEXTURE_2D, textureID2);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                glGenerateMipmap(GL_TEXTURE_2D);
                stbi_image_free(data);
            }
            else
            {
                std::cout << "Failed to load texture: " << tex2 << std::endl;
            }
        }
    }
    void draw(uniformSetters setters)
    {
        shader.use();
        for (const auto &setter : setters.setIntegers)
        {
            shader.setInteger(setter.first.c_str(), setter.second);
        }
        for (const auto &setter : setters.setFloats)
        {
            shader.setFloat(setter.first.c_str(), setter.second);
        }
        for (const auto &setter : setters.setVec3)
        {
            shader.setVector3f(setter.first.c_str(), setter.second);
        }
        for (const auto &setter : setters.setMat4)
        {
            shader.setMatrix4(setter.first.c_str(), setter.second);
        }

        shader.setFloat("leafStartFactor", leafStartFactor);
        shader.setFloat("leafAmplification", amplificator);

        shader.setInteger("texture1", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID1);
        shader.setInteger("texture2", 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textureID2);
        glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, segments.size());
        glBindVertexArray(0);
        // Unbind textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    //destructor to delete the VAO, VBO and textures
    ~ProceduralObject()
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        if (textureID1 != 0)        {
            glDeleteTextures(1, &textureID1);
        }
        if (textureID2 != 0)        {
            glDeleteTextures(1, &textureID2);
        }
    }
};
