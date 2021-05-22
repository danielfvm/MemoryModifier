#include "shader.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <iostream>

Shader::Shader(const char* fragmentCode, const char* vertexCode) {
    
    char infoLog[1024];
    int success;
    
    // Load fragment shader
    int fragmentID = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentID, 1, &fragmentCode, NULL);
    glCompileShader(fragmentID);
    glGetShaderiv(fragmentID, GL_COMPILE_STATUS, &success);

    if (!success) 
    {
        glGetShaderInfoLog(fragmentID, 1024, NULL, infoLog);
        std::cout << infoLog << std::endl;
        throw std::runtime_error(infoLog);
    }

    int vertexID;

    // Load vertex shader
    try {
        vertexID = glCreateShader(GL_VERTEX_SHADER);
    }
    catch (std::exception e) { std::cout << e.what() << std::endl; }
    
    glShaderSource(vertexID, 1, &vertexCode, NULL);
    glCompileShader(vertexID);
    glGetShaderiv(vertexID, GL_COMPILE_STATUS, &success);

    if (!success) 
    {
        glGetShaderInfoLog(vertexID, 1024, NULL, infoLog);
        throw std::runtime_error(infoLog);
    }

    // Create shader program
    programID = glCreateProgram();
    glAttachShader(programID, vertexID);
    glAttachShader(programID, fragmentID);
    glLinkProgram(programID);
    glGetProgramiv(programID, GL_LINK_STATUS, &success);

    if (!success) 
    {
        glGetProgramInfoLog(programID, 1024, NULL, infoLog);
        throw std::runtime_error(infoLog);
    }

    // Delete shaders as they're already linked to program
    glDeleteShader(vertexID);
    glDeleteShader(fragmentID);
}

Shader::~Shader() 
{
    glDeleteShader(programID);
}

void Shader::bind() 
{ 
    glUseProgram(programID); 
}

void Shader::unbind() 
{
    glUseProgram(0); 
}

void Shader::set_uniform(const int location, bool b)
{
    glUniform1i(location, b);
}

void Shader::set_uniform(const int location, float f) 
{
    glUniform1f(location, f); 
}

void Shader::set_uniform(const int location, float f0, float f1) 
{
    glUniform2f(location, f0, f1); 
}

void Shader::set_uniform(const int location, float f0, float f1, float f2)
{
    glUniform3f(location, f0, f1, f2); 
}

void Shader::set_uniform(const int location, float f0, float f1, float f2, float f3)
{
    glUniform4f(location, f0, f1, f2, f3); 
}

void Shader::set_uniform(const int location, const glm::mat4& matrix)
{
    glUniformMatrix4fv(location, 1, GL_FALSE, &matrix[0][0]); 
}

int Shader::get_uniform(const std::string& name)
{
    return glGetUniformLocation(programID, name.c_str());
}
