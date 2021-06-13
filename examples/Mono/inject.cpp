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

#include "../../src/Process.h"
#include "../../src/MonoManager.h"

#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>


int main(int argc, char** argv) {

    // Open process
    Process process("SwallowTheSea");

    // Print process id
    printf("Found process: %d\n", process.getPID());

    // Init mono
    /*
    MonoManager mono(process);

    intptr_t cPlayerController = mono.getClass("PlayerController");
    intptr_t mDamage = mono.getMethodOfClass(cPlayerController, "Damage");
    uint32_t oStaminaCoolDownTime = mono.getFieldOffsetOfClass(cPlayerController, "_staminaCoolDownTime");

    printf("PlayerController::Damage() %p\n", mDamage);
    printf("PlayerController::_staminaCoolDownTime %p\n", oStaminaCoolDownTime);

    sleep(1);
    */

    // Instead deatouring the function we can disable it
    // by replacing it with a ret:
    //      process.writeMemory(addr, 0xc3, 1);

    puts("Loading shared.so");
    MemoryRegion region = process.loadSharedLibrary("shared.so");
    intptr_t mHook = region.getSymbolAddress("Damage");
    printf("new adr: %p\n", mHook);

//    printf("detour result: %d\n", process.detourFunction(mDamage, mHook));

    puts("Injection finish!");
}
