/*
    This file is part of MemoryModifier.


    MemoryModifier is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    MemoryModifier is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MemoryModifier.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "../../src/MemoryModifier.h"
#include "../../src/Process.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>



const uint64_t dwLocalPlayer = 0xD8E2CC;
const uint64_t m_iDefaultFOV = 0x332C;

int main(int argc, char** argv) {

    // Open process with process name "magverse"
    Process process("csgo_linux64");

    // Print process id
    printf("PID: %d\n", process.getPID());
    printf("size: %d\n", process.getMemoryRegions().size());

    MemoryRegion client = process.getMemoryRegion("client_client.so", PROT_READ | PROT_EXEC);
    printf("Start: %p\n", client.getStart());

    uint32_t localPlayer;
    int fov;

    process.readMemory<uint32_t>(client.getStart() + dwLocalPlayer, localPlayer, sizeof(uint32_t));
    process.readMemory<int>(localPlayer + m_iDefaultFOV, fov, sizeof(int));

    printf("LocalPlayer: %p\n", localPlayer);
    printf("FOV: %d\n", fov);

    /*

    // Load shared lib and run "test" function from shared.so
    MemoryRegion region_sharedlib = process.loadSharedLibrary("shared.so");
    printf("Shared lib loaded at: %p\n", (void*)region_sharedlib.getStart());

    // Get Global Offset Address pointing to glfwSwapBuffers
    auto result = process.getGlobalOffsetAddress("glfwSwapBuffers")[0];

    // Get our modified "__glfwSwapBuffers" located in our shared libaray.
    uint64_t sharedlib_addr = region_sharedlib.getSymbolAddress("__glfwSwapBuffers");

    printf("sharedlib_addr: %p\n", (void*)sharedlib_addr);

    // Replace the pointer in the GOT with our modified function
    process.writeMemory<uint64_t>(process.getStart() + result.offset, sharedlib_addr, sizeof(uint64_t));
    */

    return 0;
}
