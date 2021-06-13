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


#include "MemoryModifier.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

#include "util.h"


int main(int argc, char** argv) {
    Process process("magverse");

    MemoryRegion region = process.getMemoryRegion("magverse", PROT_READ);

    printf("pid: %d\n", process.getPID());

    /*
    uint64_t str = process.mallocString("Hello World from mm!\n");
    printf("malloc'd string\n");
    uint64_t libc_printf_addr = process.getLibcFunction("printf");
    printf("%p\n", libc_printf_addr);
    process.runFunction(libc_printf_addr, str);
    printf("printf\n");
    process.runFunction(process.getLibcFunction("free"), str);
    printf("free\n");
    */

    // load shared lib and run "test" function from shared.so
    
    MemoryRegion region_sharedlib = process.loadSharedLibrary("./examples/ImGuiGLFW/shared.so");
    printf("Shared lib loaded at: %p\n", region_sharedlib.getStart());

    /*
    auto result = process.getGlobalOffsetAddress("glfwSwapBuffers")[0];
    printf("result: %p\n", result.offset);

    uint64_t sharedlib_addr = region_sharedlib.getSymbolAddress("__glfwSwapBuffers");
    printf("sharedlib_addr: %p\n", sharedlib_addr);

    printf("got: %p\n", region.getStart() + result.offset);

    process.writeMemory<uint64_t>(region.getStart() + result.offset, sharedlib_addr, sizeof(uint64_t));
    */
    
    MemoryRegion region_libglfw = process.getMemoryRegion("libglfw", PROT_READ);
    uint64_t orig = region_libglfw.getSymbolAddress("glfwSwapBuffers");
    uint64_t after = region_sharedlib.getSymbolAddress("__glfwSwapBuffers");
    printf("Orig: %p\nAfter: %p\n", orig, after);
    process.detourFunction(orig, after, Process::DetourOption::Replace);
    
/*
    MemoryRegion rxp = process.getMemoryRegion("supertux2", "r-xp");
    uint64_t offset = rxp.scanPatternOffset("\x48\x8b\x3d\x9c\xed\x35\x00", "xxxxxxxxxx", 7);
    printf("Offset: %p\n", offset);
*/
    return 0;
}
