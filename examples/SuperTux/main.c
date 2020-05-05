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

#include <MemoryModifier.h>
#include <ncurses.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


int main() {
    Process *p;
    MemoryRegion heap;

    long addr, addr2;

    // open process
    if ((p = openProcess("supertux2")) == NULL) {
        return EXIT_FAILURE;
    }

    if (getMemoryRegion(p, "[heap]", &heap) == 0) {
        return EXIT_FAILURE;
    }

    byte *pattern = "\x21\0\0\0\0\0\0\0\x21\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0????01\0\0\0\0\0\0\0\0\0\0????01\0\0";
    byte *mask = "xxxxxxxxxxxxxxxxxxxxxxxx????01xxxxxxxxxx????01xx";
    byte *pattern2 = "\x98?????\0\0??????\0\0?????????????????????????\0\0\0????????????????\0\xe0?\x44\xe0?????\0\0??????\0\0\0???\0?????????\0\0??????\0\0??????\0\0???\0?\xe0?\x44?\0\0\0\0\0\0\0";
    byte *mask2 = "x?????xx??????xx?????????????????????????xxx????????????????xx?xx?????xx??????xxx???x?????????xx??????xx??????xx???x?x?x?xxxxxxx";

    // search pattern, coins & player state
    if (!(addr = getAddressByPattern(heap, 0, pattern, mask, strlen(mask)))) {
        fprintf(stderr, "Failed loading pattern1\n");
        return EXIT_FAILURE;
    }

    // search pattern, onground, player position, ...
    if (!(addr2 = getAddressByPattern(heap, 0, pattern2, mask2, strlen(mask2)))) {
        fprintf(stderr, "Failed loading pattern2\n");
        return EXIT_FAILURE;
    }

    // start ncurses & disable delay for getch
    initscr();
    nodelay(stdscr, TRUE);

    printw("Found pattern at %p\n", addr);
    printw("Found pattern2 at %p\n", addr2);
    printw("Press Q to quit\n", addr);

    // set payload until q is pressed
    while (getch() != 'q') {
        writeProcessMemory(heap, heap.start + addr - 80, getBytes(int, 9999), 4);   // coins
        writeProcessMemory(heap, heap.start + addr - 76, getBytes(int, 2), 4);      // player state
        writeProcessMemory(heap, heap.start + addr - 72, getBytes(int, 1), 4);      // enable fire shooting
        writeProcessMemory(heap, heap.start + addr2 + 0x40, getBytes(byte, 1), 1);    // fly hack, setting onground true
    } 

    // close ncurses & clear up memory
    endwin();
    closeProcess(p);

    return EXIT_SUCCESS;
}
