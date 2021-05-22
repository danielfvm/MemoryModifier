#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <string>

#include <dlfcn.h>
#include <stdio.h>

#include "Renderer.h"
#include "Batch.h"

typedef void (*glfwSwapBuffers_t)(GLFWwindow*);

glfwSwapBuffers_t glfwSwapBuffers_ptr;

static Renderer* renderer;

const char* FRAG = 
"#version 330 core\n"

"layout (location = 0) out vec4 o_Color;\n"

"in vec4 v_Color;\n"

"void main() {\n"
"   o_Color = v_Color;\n"
"}\n";

const char* VERT = 
"#version 330 core\n"

"layout (location = 0) in vec2 a_Coords;\n"
"layout (location = 1) in vec4 a_Color;\n"

"uniform mat4 u_ViewProj;\n"

"out vec4 v_Color;\n"

"void main()\n"
"{\n"
"    v_Color = a_Color;\n"
"    gl_Position = u_ViewProj * vec4(a_Coords, 0.0, 1.0);\n"
"}\n";

void draw(GLFWwindow* window) {

    int width, height;

    glfwGetFramebufferSize(window, &width, &height);

    glm::mat4 m_projGui = glm::ortho(0.0f, (float)width, 0.0f, (float)height, -1.0f, 1.0f);

    renderer->render(m_projGui, width, height);

  //  printf("Window: %p, %d:%d\n", window, width, height);
}

extern "C" void __glfwSwapBuffers (GLFWwindow* win) {
    draw(win);
    glfwSwapBuffers_ptr(win);
}

void __attribute__ ((constructor)) init() {
    glfwSwapBuffers_ptr = (glfwSwapBuffers_t) dlsym(RTLD_DEFAULT, "glfwSwapBuffers");
    printf("Found glfwSwapBuffers at %p\n", glfwSwapBuffers_ptr);

    renderer = new Renderer(FRAG, VERT);

    printf("Shader loaded!\n");
}

void __attribute__ ((destructor)) fini() {
    delete renderer;
    printf("Unload injected lib\n");
}

