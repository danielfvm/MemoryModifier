// Linux SuperTux Cheat

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
    //byte *pattern2 = "\0\0\0\0\x98????\x55\0\0?????\x55\0\0?????????????????????????\0\0\0?????????????????\x55\0\0\xe0????\x55\0\0?????\x55\0\0\0\0\0\0\x04\x55\0\0?????\x55\0\0?????\x55\0\0?????\x55\0\0???\0\0\x55\0\0\0\0\0\0";
    //byte *mask2 = "xxxxx????xxx?????xxx?????????????????????????xxx?????????????????xxxx????xxx?????xxxxxxxxxxx?????xxx?????xxx?????xxx???xxxxxxxxx";

    // search pattern, coins & player state
    if (!(addr = getAddressByPattern(heap, 0, pattern, mask, strlen(mask)))) {
        fprintf(stderr, "Failed loading pattern1\n");
        return EXIT_FAILURE;
    }

    // search pattern, onground, player position, ...
    /*if (!(addr2 = getAddressByPattern(heap, 0, pattern2, mask2, strlen(mask2)))) {
        fprintf(stderr, "Failed loading pattern2\n");
        return EXIT_FAILURE;
    }*/

    // start ncurses & disable delay for getch
    initscr();
    nodelay(stdscr, TRUE);

    printw("Found pattern at %p\n", addr);
    printw("Press Q to quit\n", addr);

    // set payload until q is pressed
    while (getch() != 'q') {
        writeProcessMemory(heap, heap.start + addr - 80, getBytes(int, 9999), 4);   // coins
        writeProcessMemory(heap, heap.start + addr - 76, getBytes(int, 2), 4);      // player state
        writeProcessMemory(heap, heap.start + addr - 72, getBytes(int, 1), 4);      // enable fire shooting
       // writeProcessMemory(heap, heap.start + addr2 + 68, getBytes(byte, 1), 1);    // fly hack, setting onground true
    } 

    // close ncurses & clear up memory
    endwin();
    closeProcess(p);

    return EXIT_SUCCESS;
}
