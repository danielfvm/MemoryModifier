#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <GL/gl.h>

#include <dlfcn.h>
#include <stdio.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_glfw.h"

typedef void (*glfwSwapBuffers_t)(GLFWwindow*);

glfwSwapBuffers_t glfwSwapBuffers_ptr;

static GLFWwindow* window = nullptr;

static bool show_window = true;

// This function is being replaced by mm with the
// originial "glfwSwapBuffers" function by replacing
// the pointer in GOT.
extern "C" void __glfwSwapBuffers (GLFWwindow* win) {

    if (window == nullptr) {
        window = win;

        // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
        // GL ES 2.0 + GLSL 100
        const char* glsl_version = "#version 100";
#elif defined(__APPLE__)
        // GL 3.2 + GLSL 150
        const char* glsl_version = "#version 150";
#else
        // GL 3.0 + GLSL 130
        const char* glsl_version = "#version 130";
#endif

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);
    }

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (show_window) {
        // Pass a pointer to our bool variable
        ImGui::Begin("ImGui Menu", &show_window);   
        ImGui::Text("Was injected from outside!");
        if (ImGui::Button("Close Me"))
            show_window = false;
        ImGui::End();
    }

    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Call original function to update window
    glfwSwapBuffers_ptr(win);
}

void __attribute__ ((constructor)) init() {
    glfwSwapBuffers_ptr = (glfwSwapBuffers_t) dlsym(RTLD_DEFAULT, "glfwSwapBuffers");
    printf("Found glfwSwapBuffers at %p\n", glfwSwapBuffers_ptr);
}

// Cleanup
void __attribute__ ((destructor)) fini() {
    //ImGui::DestroyContext();
    printf("Unload injected lib\n");
}

