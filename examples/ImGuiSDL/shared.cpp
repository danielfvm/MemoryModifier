#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <GL/gl.h>

#include <SDL2/SDL.h>

#include <dlfcn.h>
#include <stdio.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_sdl.h"

static SDL_Window* window = nullptr;

static bool show_window = true;

static SDL_GLContext gl_context;

// This function is being replaced by mm with the
// originial "glfwSwapWindow" function by replacing
// the pointer in GOT.
extern "C" void __SDL_GL_SwapWindow (SDL_Window* win) {

	static SDL_GLContext original_context = SDL_GL_GetCurrentContext();

    if (window == nullptr) {
        window = win;

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        if (gl_context == nullptr) {
            gl_context = SDL_GL_CreateContext(window);
        }

        // Setup Platform/Renderer backends
        ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
        ImGui_ImplOpenGL3_Init("#version 130");
    }

    SDL_GL_MakeCurrent(window, gl_context);

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);
    ImGui::NewFrame();

    if (show_window) {
        // Pass a pointer to our bool variable
        ImGui::Begin("ImGui Menu", &show_window);   
        ImGui::Text("Was injected from outside!");
        if (ImGui::Button("Close Me")) {
            show_window = false;
        }
        ImGui::End();
    }

    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Call original function to update window
    SDL_GL_MakeCurrent(window, original_context);
    SDL_GL_SwapWindow(win);
}

void __attribute__ ((constructor)) init() {
    printf("Loaded injected lib\n");
}

// Cleanup
void __attribute__ ((destructor)) fini() {
    ImGui::DestroyContext();
    printf("Unload injected lib\n");
}
