#ifndef SHADER_H
#define SHADER_H

#include <string>

#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective

class Shader {
private:
    int programID;

public:
    Shader(const char* fragmentCode, const char* vertexCode);
    ~Shader();

    void bind();
    void unbind();

    void set_uniform(const int location, bool b);
    void set_uniform(const int location, float f);
    void set_uniform(const int location, float f0, float f1);
    void set_uniform(const int location, float f0, float f1, float f2);
    void set_uniform(const int location, float f0, float f1, float f2, float f3);
    void set_uniform(const int location, const glm::mat4& matrix);

    int get_uniform(const std::string& name);
};

#endif // SHADER_H
