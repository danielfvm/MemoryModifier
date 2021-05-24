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

#include "../../src/MemoryModifier.h"
#include "../../src/util.h"

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
    Process self;

    printf("Found map count: %zu\n", self.getMemoryRegions().size());

    uint64_t offset = self.getGlobalOffsetAddress("SDL_GL_SwapWindow")[0].offset;
    printf("offset: %p\n", offset);

    printf("Exe path: %s\n", self.getPathToExe().c_str());

    MemoryRegion main = self.getMemoryRegion(util::findNameByProcessId(getpid()), PROT_READ);

    printf("sharedlib_addr: %p\n", __SDL_GL_SwapWindow);

    uint64_t addr;
    self.readMemory(main.getStart() + offset, addr, sizeof(uint64_t));
    printf("found addr: %p\n", addr);

    // Change got to our function
    self.writeMemory(main.getStart() + offset, (uint64_t)__SDL_GL_SwapWindow, sizeof(uint64_t));

    printf("Loaded injected lib\n");
}

// Cleanup
void __attribute__ ((destructor)) fini() {
    ImGui::DestroyContext();
    printf("Unload injected lib\n");
}
