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
    glm::vec3 prevStart;
    glm::vec3 start;
    glm::vec3 end;
    float factor;
    float prevFactor;
};

struct state
{
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

struct Angle
{
    float x;
    float y;
    float z;
};
struct decreasingFactors
{
    float growing;
    float branching;
};


size_t convertToSegmentsHelper(const std::string &lSystemString, std::vector<Segment> &segments, Angle angles, size_t startAt, float factor,decreasingFactors decreaseFactor, Segment branchingSegment, glm::vec3 currentDirection)
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

            glm::vec3 growth = currentDirection * (currentFactor*4.0f);
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
            i = convertToSegmentsHelper(lSystemString, segments, angles, newStartAt, currentFactor*decreaseFactor.branching, decreaseFactor, branchingSegment, currentDirection);
        }
        else if (c == ']')
        {
            return i;
        }
    }
    return lSystemString.length() - 1;
}

std::vector<Segment> convertToSegments(const std::string &lSystemString, float angleX, float angleY, float angleZ)
{
    int openBrackets = 0;
    std::vector<Segment> segments;
    Segment baseSegment;
    baseSegment.start = glm::vec3(0.0f, 0.0f, 0.0f);
    baseSegment.end = glm::vec3(0.0f, 1.0f, 0.0f);
    baseSegment.factor = 1.0f;
    baseSegment.prevFactor = 1.0f;
    segments.push_back(baseSegment);
    Angle angles;
    angles.x = angleX;
    angles.y = angleY;
    angles.z = angleZ;
    decreasingFactors decreaseFactor;
    decreaseFactor.growing = 0.99f;
    decreaseFactor.branching = 0.80f;
    convertToSegmentsHelper(lSystemString, segments, angles, 0,1.0f, decreaseFactor, baseSegment, glm::vec3(0.0f, 1.0f, 0.0f));
    return segments;
}

class ProceduralObject
{
public:
    std::vector<Segment> segments;
    GLuint VAO, VBO;
    Shader shader;

    ProceduralObject(std::string inputString, std::vector<Rule> rules, size_t iterations, Angle angles, ShaderFilePaths shaderPath)
    {
        std::string output = applyRules(inputString, rules, iterations);
        segments = convertToSegments(output, angles.x, angles.y, angles.z);
        std::cout << "compiling shader" << std::endl;
        shader = Shader(shaderPath);

        std::cout << "ProceduralObject initialized with " << segments.size() << " segments." << std::endl;
        if (segments.size() > 0) {
            std::cout << "Segment 0 start: " << segments[0].start.x << "," << segments[0].start.y << "," << segments[0].start.z << std::endl;
            std::cout << "Segment 0 end: " << segments[0].end.x << "," << segments[0].end.y << "," << segments[0].end.z << std::endl;
        }

        // Create VAO and VBO for each segment

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        glBufferData(GL_ARRAY_BUFFER, sizeof(Segment) * segments.size(), segments.data(), GL_STATIC_DRAW);

        auto att_pos = glGetAttribLocation(shader.ID, "inPrevStart");
        if (att_pos != -1) {
            glEnableVertexAttribArray(att_pos);
            glVertexAttribPointer(att_pos, 3, GL_FLOAT, GL_FALSE, sizeof(Segment), (void *)offsetof(Segment, prevStart));
        }
        auto att_start = glGetAttribLocation(shader.ID, "inStart");
        if (att_start != -1) {
            glEnableVertexAttribArray(att_start);
            glVertexAttribPointer(att_start, 3, GL_FLOAT, GL_FALSE, sizeof(Segment), (void *)offsetof(Segment, start));
        }
        auto att_end = glGetAttribLocation(shader.ID, "inEnd");
        if (att_end != -1) {
            glEnableVertexAttribArray(att_end);
            glVertexAttribPointer(att_end, 3, GL_FLOAT, GL_FALSE, sizeof(Segment), (void *)offsetof(Segment, end));
        }
        auto att_factor = glGetAttribLocation(shader.ID, "inFactor");
        if (att_factor != -1) {
            glEnableVertexAttribArray(att_factor);
            glVertexAttribPointer(att_factor, 1, GL_FLOAT, GL_FALSE, sizeof(Segment), (void *)offsetof(Segment, factor));
        }
        auto att_prevFactor = glGetAttribLocation(shader.ID, "inPrevFactor");
        if (att_prevFactor != -1) {
            glEnableVertexAttribArray(att_prevFactor);
            glVertexAttribPointer(att_prevFactor, 1, GL_FLOAT, GL_FALSE, sizeof(Segment), (void *)offsetof(Segment, prevFactor));
        }   

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

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

        glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, segments.size());
        glBindVertexArray(0);


    }
};
