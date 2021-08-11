#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

typedef unsigned int uint;

void checkCompileErrors(uint shader, std::string type);

uint CompileShader(std::string vertName, std::string fragName, std::string geomName = "")
{
    std::string vertCode, fragCode, geomCode;
    std::ifstream vertFile, fragFile, geomFile;

    // ensure ifstream objects can throw exceptions:
    vertFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fragFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    geomFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        // open files
        vertFile.open(("shaders/" + vertName + ".vert").c_str());
        fragFile.open(("shaders/" + fragName + ".frag").c_str());
        std::stringstream vertStream, fragStream;
        // read file's buffer contents into streams
        vertStream << vertFile.rdbuf();
        fragStream << fragFile.rdbuf();
        // close file handlers
        vertFile.close();
        fragFile.close();
        // convert stream into string
        vertCode = vertStream.str();
        fragCode = fragStream.str();

        if (geomName != "")
        {
            geomFile.open(("shaders/" + geomName + ".geom").c_str());
            std::stringstream geomStream;
            geomStream << geomFile.rdbuf();
            geomFile.close();
            geomCode = geomStream.str();
        }
    }
    catch (std::ifstream::failure& e)
    {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
    }

    const char* vertexCode = vertCode.c_str();
    const char* fragmentCode = fragCode.c_str();
    // 2. compile shaders
    uint vertex, fragment;
    // vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertexCode, NULL);
    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");
    // fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragmentCode, NULL);
    glCompileShader(fragment);
    checkCompileErrors(fragment, "FRAGMENT");

    uint geometry;
    if(geomName != "")
    {
        const char* geometryCode = geomCode.c_str();
        geometry = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(geometry, 1, &geometryCode, NULL);
        glCompileShader(geometry);
        checkCompileErrors(geometry, "GEOMETRY");
    }

    // shader Program
    uint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    if (geomName != "") glAttachShader(program, geometry);
    glLinkProgram(program);
    checkCompileErrors(program, "PROGRAM");

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    if (geomName != "") glDeleteShader(geometry);

    return program;
} 

// utility function for checking shader compilation/linking errors.
void checkCompileErrors(uint shader, std::string type)
{
    int success;
    char infoLog[1024];
    if (type != "PROGRAM")
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << 
            infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
    else
    {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << 
            infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
}

void shader_SetInt(uint shader, const char* name, int value)
{
    glUniform1i(glGetUniformLocation(shader, name), value);
}

void shader_SetMatrix4(uint shader, const char* name, glm::mat4 value)
{
    glUniformMatrix4fv(glGetUniformLocation(shader, name), 1, false, glm::value_ptr(value));
}

void shader_SetMatrix4x4(uint shader, const char* name, glm::mat4 value)
{
    glUniformMatrix4fv(glGetUniformLocation(shader, name), 1, false, glm::value_ptr(value));
}

void shader_SetVector3f(uint shader, const char* name, glm::vec3 value)
{
    glUniform3f(glGetUniformLocation(shader, name), value.x, value.y, value.z);
}

void shader_SetVector4f(uint shader, const char* name, glm::vec4 value)
{
    glUniform4f(glGetUniformLocation(shader, name), value.x, value.y, value.z, value.w);
}

#endif