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

#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char** argv) {

    // Open process with process name "magverse"
    Process process("a.out");

    // Print process id
    printf("PID: %d\n", process.getPID());

    // Load shared lib and run "test" function from shared.so
    const MemoryRegion& region_sharedlib = process.loadSharedLibrary("shared.so");
    printf("Shared lib loaded at: %p\n", (void*)region_sharedlib.getStart());

    // Get Global Offset Address pointing to SDL_GL_SwapWindow
    //auto result = process.getGlobalOffsetAddress("test")[0];

    //printf("hook addr: %p\n", (void*)result.offset);

    return 0;
}
