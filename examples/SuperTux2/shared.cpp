#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#include <SDL2/SDL_keycode.h>
#endif

#include <GL/gl.h>

#include <SDL2/SDL.h>

#include <dlfcn.h>
#include <stdio.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_sdl.h"

#include "../../src/MemoryModifier.h"
#include "../../src/util.h"

static SDL_Window* window = nullptr;

static bool show_window = true;

static SDL_GLContext gl_context;

static Process* self = nullptr;

intptr_t STATUS_ADDR;
const int STATUS_COINS_OFFSET = 0x0;
const int STATUS_LVL_OFFSET = 0x4;
const int STATUS_FIRE_OFFSET = 0x8;


intptr_t POSITION_ADDR;
const int POSITION_X_OFFSET = 0x70;
const int POSITION_Y_OFFSET = 0x74;


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
        ImGui::Begin("Cheat Menu", &show_window);   

        if (ImGui::Button("Max up tux")) {
            self->writeMemory<int>(STATUS_ADDR+STATUS_LVL_OFFSET, 2);
            self->writeMemory<int>(STATUS_ADDR+STATUS_FIRE_OFFSET, 9999);
        }

        if (ImGui::Button("Infinite coins")) {
            self->writeMemory<int>(STATUS_ADDR+STATUS_COINS_OFFSET, 99999);
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
    self = new Process();

    STATUS_ADDR = self->getAddressFromPointerString("[[[/usr/bin/supertux2+0x5b8]+0xf8]+0x20]+0x0");
    POSITION_ADDR = self->getAddressFromPointerString("[[[/usr/bin/supertux2+0x7e0]+0x10]+0x8]+0x0");

    printf("Found status at: %p\n", STATUS_ADDR);
    printf("Found position at: %p\n", POSITION_ADDR);

    printf("Loaded injected lib\n");
}

// Cleanup
void __attribute__ ((destructor)) fini() {
    ImGui::DestroyContext();
    printf("Unload injected lib\n");
}
