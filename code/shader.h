#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>

#define SHADER_READ_FILE_ERR 999

typedef unsigned int uint;

void checkCompileErrors(uint shader, std::string type);

uint CompileShader(const char* vertName, const char* fragName, const char* geomName = NULL)
{
    char vertFileName[64], fragFileName[64], geomFileName[64];
    snprintf(vertFileName, sizeof(vertFileName), "shaders/%s.vert", vertName);
    snprintf(fragFileName, sizeof(fragFileName), "shaders/%s.frag", fragName);
    snprintf(geomFileName, sizeof(geomFileName), "shaders/%s.geom", geomName);

    FILE* vertFile, *fragFile, *geomFile; 
    if (fopen_s(&vertFile, vertFileName, "r") != 0) {
        std::cout << "ERROR::VERTEX_SHADER::FILE_NOT_SUCCESFULLY_READ\n";
        return SHADER_READ_FILE_ERR;
    }

    if (fopen_s(&fragFile, fragFileName, "r") != 0) {
        std::cout << "ERROR::FRAGMENT_SHADER::FILE_NOT_SUCCESFULLY_READ\n";
        return SHADER_READ_FILE_ERR;
    }

    char c;

    std::string vertCode;
    while ((c = (char)fgetc(vertFile)) != EOF)
    {
        vertCode += c;
    }
    fclose(vertFile);

    std::string fragCode;
    while ((c = (char)fgetc(fragFile)) != EOF)
    {
        fragCode += c;
    }
    fclose(fragFile);

    std::string geomCode;
    if (geomName)
    {
        if (fopen_s(&geomFile, geomFileName, "r") != 0) {
            std::cout << "ERROR::FRAGMENT_SHADER::FILE_NOT_SUCCESFULLY_READ\n";
            return SHADER_READ_FILE_ERR;
        }

        while ((c = (char)fgetc(geomFile)) != EOF)
        {
            geomCode += c;
        }
        fclose(geomFile);
    }

	const char* vertexCode = vertCode.c_str();
    const char* fragmentCode = fragCode.c_str();
    
    uint vertex, fragment;
    
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertexCode, NULL);
    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");
    
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragmentCode, NULL);
    glCompileShader(fragment);
    checkCompileErrors(fragment, "FRAGMENT");

    uint geometry;
    if(geomName)
    {
        const char* geometryCode = geomCode.c_str();
        geometry = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(geometry, 1, &geometryCode, NULL);
        glCompileShader(geometry);
        checkCompileErrors(geometry, "GEOMETRY");
    }

    uint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    if (geomName) glAttachShader(program, geometry);
    glLinkProgram(program);
    checkCompileErrors(program, "PROGRAM");

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    if (geomName) glDeleteShader(geometry);

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
            infoLog << "\n -- --------------------------------------------------- -- " << "\n";
        }
    }
    else
    {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << 
            infoLog << "\n -- --------------------------------------------------- -- " << "\n";
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
