#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>

#ifndef PATH_TO_SHADERS
#define PATH_TO_SHADERS "shaders"
#endif
// slightly modified version of the shader class used in the exercices, with support for geometry and tessellation shaders

std::string readFile(const char* filePath) {
    std::ifstream file;
    std::stringstream buffer;

    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        file.open(filePath);
        buffer << file.rdbuf();
        file.close();
    } catch (std::ifstream::failure& e) {
        std::cerr << "ERROR::FILE_NOT_SUCCESFULLY_READ: " << e.what() << std::endl;
    }
    return buffer.str();
}


struct ShaderFilePaths {
    std::map<GLenum, std::string> paths;
    void addFragmentShader(const std::string& path) {
        paths[GL_FRAGMENT_SHADER] = path;
    }
    void addVertexShader(const std::string& path) {
        paths[GL_VERTEX_SHADER] = path;
    }
    void addTessellationControlShader(const std::string& path) {
        paths[GL_TESS_CONTROL_SHADER] = path;
    }
    void addTessellationEvaluationShader(const std::string& path) {
        paths[GL_TESS_EVALUATION_SHADER] = path;
    }
    void addGeometryShader(const std::string& path) {
        paths[GL_GEOMETRY_SHADER] = path;
    }
};


struct ShaderCode{
    std::map<GLenum, std::string> code;
    void addFragmentShader(const std::string& c) {
        code[GL_FRAGMENT_SHADER] = c;
    }
    void addVertexShader(const std::string& c) {
        code[GL_VERTEX_SHADER] = c;
    }
    void addTessellationControlShader(const std::string& c) {
        code[GL_TESS_CONTROL_SHADER] = c;
    }
    void addTessellationEvaluationShader(const std::string& c) {
        code[GL_TESS_EVALUATION_SHADER] = c;
    }
    void addGeometryShader(const std::string& c) {
        code[GL_GEOMETRY_SHADER] = c;
    }
};




class Shader
{
public:
	GLuint ID;
    Shader() = default;

    Shader(ShaderFilePaths shaderPaths)
    {   
        //check if there is altest the vertex and fragment shader
        if (shaderPaths.paths.find(GL_VERTEX_SHADER) == shaderPaths.paths.end()) {
            throw std::invalid_argument("Shader: vertex shader path not provided");
        }
        if (shaderPaths.paths.find(GL_FRAGMENT_SHADER) == shaderPaths.paths.end()) {
            throw std::invalid_argument("Shader: fragment shader path not provided");
        }
        std::vector<GLuint> compiledShaders;
        for (const auto& pair : shaderPaths.paths) {
            GLenum type = pair.first;
            const std::string& path = pair.second;
            std::string code = readFile(path.c_str());
            GLuint shaderID = compileShader(code, type);
            compiledShaders.push_back(shaderID);
        }
        ID = compileProgram(compiledShaders);
        for (GLuint shader : compiledShaders) {
            glDeleteShader(shader);
        }
    }


    Shader(ShaderCode shaderCodes)
    {   
        std::vector<GLuint> compiledShaders;
        for (const auto& pair : shaderCodes.code) {
            GLenum type = pair.first;
            const std::string& code = pair.second;
            GLuint shaderID = compileShader(code, type);
            compiledShaders.push_back(shaderID);
        }
        ID = compileProgram(compiledShaders);
        for (GLuint shader : compiledShaders) {
            glDeleteShader(shader);
        }
    }



	Shader(const char* vertexPath, const char* fragmentPath)
	{   

        std::cout << "Loading shader from paths: " << vertexPath << " and " << fragmentPath << std::endl;

        // 1. retrieve the vertex/fragment source code from filePath
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;
        // ensure ifstream objects can throw exceptions:
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        std::cout << "Reading shader files..." << std::endl;
        try
        {
            // open files
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;
            // read file's buffer contents into streams
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();
            // close file handlers
            vShaderFile.close();
            fShaderFile.close();
            // convert stream into string
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        }
        catch (std::ifstream::failure& e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << e.what() << std::endl;
        }
        std::cout << "Shader files read successfully." << std::endl;
        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

        GLuint vertex = compileShader(vertexCode, GL_VERTEX_SHADER);
        GLuint fragment = compileShader(fragmentCode, GL_FRAGMENT_SHADER);
        ID = compileProgram(vertex, fragment);
	}

    Shader(std::string vShaderCode, std::string fShaderCode)
    {
        GLuint vertex = compileShader(vShaderCode, GL_VERTEX_SHADER);
        GLuint fragment = compileShader(fShaderCode, GL_FRAGMENT_SHADER);
        ID = compileProgram(vertex, fragment);
    }

    void use() {
        glUseProgram(ID);
    }
    void setInteger(const GLchar *name, GLint value) {
        glUniform1i(glGetUniformLocation(ID, name), value);
    }
    void setFloat(const GLchar* name, GLfloat value) {
        glUniform1f(glGetUniformLocation(ID, name), value);
    }
    void setVector3f(const GLchar* name, GLfloat x, GLfloat y, GLfloat z) {
        glUniform3f(glGetUniformLocation(ID, name), x, y, z);
    }
    void setVector3f(const GLchar* name, const glm::vec3& value) {
        glUniform3f(glGetUniformLocation(ID, name), value.x, value.y, value.z);
    }
    void setVector4f(const GLchar* name, const glm::vec4& value) {
        glUniform4f(glGetUniformLocation(ID, name), value.x, value.y, value.z, value.w);
	}
    void setMatrix4(const GLchar* name, const glm::mat4& matrix) {
        glUniformMatrix4fv(glGetUniformLocation(ID, name), 1, GL_FALSE, glm::value_ptr(matrix));
    }

private:
    GLuint compileShader(std::string shaderCode, GLenum shaderType)
    {   
        GLuint shader = glCreateShader(shaderType);
        const char* code = shaderCode.c_str();
        glShaderSource(shader, 1, &code, NULL);
        glCompileShader(shader);

        GLchar infoLog[1024];
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::string t = "undetermined";
            if (shaderType == GL_VERTEX_SHADER) {
                t = "vertex shader";
            }
            else if (shaderType == GL_FRAGMENT_SHADER) {
                t = "fragment shader";
            }
            else if (shaderType == GL_TESS_CONTROL_SHADER) {
                t = "tessellation control shader";
            }
            else if (shaderType == GL_TESS_EVALUATION_SHADER) {
                t = "tessellation evaluation shader";
            }
            else if (shaderType == GL_GEOMETRY_SHADER) {
                t = "geometry shader";
            }
            std::cout << "ERROR::SHADER_COMPILATION_ERROR of the " << t << ": " << shaderType << infoLog << std::endl;
        }
        return shader;
    }

    GLuint compileProgram(std::vector<GLuint> shaders)
    {
        GLuint programID = glCreateProgram();
        for (GLuint shader : shaders) {
            glAttachShader(programID, shader);
        }
        glLinkProgram(programID);

        GLchar infoLog[1024];
        GLint success;
        glGetProgramiv(programID, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(programID, 1024, NULL, infoLog);
            std::cout << "ERROR::PROGRAM_LINKING_ERROR:  " << infoLog << std::endl;
        }
        return programID;
    }




    GLuint compileProgram(GLuint vertexShader, GLuint fragmentShader)
    {
        GLuint programID = glCreateProgram();

        glAttachShader(programID, vertexShader);
        glAttachShader(programID, fragmentShader);
        glLinkProgram(programID);


        GLchar infoLog[1024];
        GLint success;
        glGetProgramiv(programID, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(programID, 1024, NULL, infoLog);
            std::cout << "ERROR::PROGRAM_LINKING_ERROR:  " << infoLog << std::endl;
        }
        return programID;
    }

};
#endif
