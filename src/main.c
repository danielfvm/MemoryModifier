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

#define MEMORY_MODIFYER_VERSION "0.1-a1"

#include "MemoryModifier.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>
#include <unistd.h>

#include <fcntl.h>

// 0x1ac78 x
// 0x1ac7c z
// 0x1ac80 y
// 0x1ADA8 amo
// 0x1AD50 heal
// 0x1ac84 yaw
// 0x1ac88 pitch
// 0x1ae7c shoot
// 0xa3b448 jump
// 0x1acac onground

// 0x1acc4 w
// 0x1acd0, strange mem, 65536w, 1a, 256d

/*
long getPointerOffset(long offset) {
    long pointer;

    if ((pointer = searchOffset(p, getBytes(long int, h_p->b_addr + offset), sizeof(long int))) == 0)
        return 0;
    return pointer;
}
*/

/*
void show(Process *p, long *array, int size) {
    char number[4];
    int i;

    while (1) {
        printf("\033[2J\033[;H");
        for (i = 0; i < size; ++ i) {
            readProcessMemory(p, p->b_addr + array[i], number, 4);
            printf("%p\t%f:%d\n", array[i], getValue(float, number), getValue(int, number));
        }
        usleep(100 * 1000);
    }
}

byte* combineBytes(int size, int count, ...) {
    byte *bytes;
    byte *bPart;
    void *p;
    int i;

    if ((bytes = malloc(count * size)) == NULL)
        return NULL;

    // va_start
    p = (void *) &count + sizeof count;

    // va_arg
    for (i = 0; i < count; ++ i) {
        memcpy(bytes + i * size, (byte*)p, size);
        p += 8;
    }

    return bytes;
}

long getStaticPointer(Process* p, const long address) {
    byte memory[8];
    long pointer;
    long addrPointer;

    byte *buffer = malloc(address - p->b_addr);
    readProcessMemory(p, p->b_addr, buffer, address - p->b_addr);

    for (pointer = p->b_addr; pointer < address; pointer += 4) {
        memcpy(memory, buffer + pointer - p->b_addr, 8);
        addrPointer = getValue(long int, memory);
        for (long addr = address; addr > address - 0xFFFF; addr -= 4) {
            if (addrPointer == addr)
                return pointer;
        }
    }

    free(buffer);

    return 0;
}
*/

int
main () {
    Process* p = openProcess("supertux");
    printf("Name: %s\nPID: %d\n", p->p_name, p->p_id);

    MemoryRegion m_region_heap;
    if (!getMemoryRegion(p, "[heap]", &m_region_heap)) {
        printf("Error!\n");
        return 1;
    }

    printf("MemoryRegion: %s\nStart addr: %p\nEnd addr: %p\n", m_region_heap.name, m_region_heap.start, m_region_heap.end);

    closeProcess(p);


    // Super tux
/*
    byte *pattern = "\0\0\0\0\x98????\x55\0\0?????\x55\0\0?????????????????????????\0\0\0?????????????????\x55\0\0\xe0????\x55\0\0?????\x55\0\0\0\0\0\0\x04\x55\0\0?????\x55\0\0?????\x55\0\0?????\x55\0\0???\0\0\x55\0\0\0\0\0\0";
    byte *mask = "xxxx?????xxx?????xxx?????????????????????????xxx?????????????????xxx?????xxx?????xxxxxxx?xxx?????xxx?????xxx?????xxx???xxxxxxxxx";
    long l = getAddressByPattern(p, 0, pattern, mask, strlen(mask));
    printf("Pattern found at %p\n", l + 64);

    while (1) {
        writeProcessMemory(p, p->b_addr + l + 64 + 4, getBytes(byte, 1), 1);
        usleep(1 * 1000);
    }
*/

/*
    searchByValue(p, 4, INT, 0x0);
    showRange(p, p->history[0], -32 * 4, 32 * 4); 
*/

/*
    byte *bufferk = malloc(p->b_size);

    readProcessMemory(p, p->b_addr, bufferk, p->b_size);0x8a9610

    int k;
    for (k = 0; k < p->b_size; ++ k) {
        if (memcmp(bufferk + k, "\x06\x09\x0a\x0a\x08\x07\x07\x0a\x0b\x0b\x09\x08", 12) == 0) {
            printf("Signature found!\n");
            writeProcessMemory(p, p->b_addr + k - 22, getBytes(int, 2),  4);
            for (int j = -32; j < 32; ++ j)
                printf("%d: %d\n", j, bufferk[k + j]);
        }
    }

    return 0;
*/


//   showRange(p, 0x8df5e4, -32, 32); 
    
    

//    searchByChange(p, 4, 1, 0x0);
// 0x8daeb0
/*
    searchByValue(p, 4, INT, 0x0);
    long saddr = p->history[0];


    printf("Scanning for statics\n");

    size_t size = 4;
    size_t range = p->b_size / size;

    byte *buffer = malloc(range * size);
    byte *buffer1 = malloc(range * size);
    bool *valid = malloc(range);

    int i, j, r;

    bool n = 0;
    bool cw = 0;

    memset(valid, 1, range);

    if (readProcessMemory(p, p->b_addr, buffer, range * size))
        printf("Success!\n");

    long l;

    printf("Delete none base address ...\n");
    for (l = 0; l < 100; ++ l) {
        n = !n;

        readProcessMemory(p, p->b_addr, cw ? buffer : buffer1, range * size);
        cw = !cw;

        for (i = 0; i < range; ++ i) {
            if (n && valid[i] && memcmp(buffer + i * size, buffer1 + i * size, size) != 0) {
                valid[i] = 0;
            }
        }


        for (i = 0, r = 0; i < range; ++ i) {
            if (valid[i]) {
                r ++;
            }
        }

        printf("%d/100\t%d\n", l / 10, r);
    }

    printf("Delete duplicates ...\n");
    int start = saddr / size - 0xFF;
    for (i = start; i < start + 0xFF * 2; ++ i) {
        printf("%d/%d\n", i - start, 0xFF * 2);
        if (valid[i]) {
            for (j = 0; j < range; ++ j) {
                if (i == j) {
                    continue;
                }
                if (memcmp(buffer + i * size, buffer + j * size, size) == 0) {
                    valid[i] = 0; //1846
                    valid[j] = 0;
                }
            }
        }
    }

    for (r = 0, i = saddr / size - 0xFF; i < saddr / size; ++ i) {
        if (valid[i]) {
            r ++;
        }
    }


    printf("R: %d\n", r);


    printf("Now pls restart client!\n");
    int enter;
    scanf("%d", &enter);

    destroyProcess(p);
    p = createProcess("super", "[heap]");//"System.dll");

    if (readProcessMemory(p, p->b_addr, buffer1, p->b_size))
        printf("Success!\n");
    for (i = start; i < start + 0xFF * 2; ++ i) {
        printf("%d/%d\n", i - start, 0xFF * 2);
        if (valid[i]) {
            bool found = 0;
            for (j = 0; j < p->b_size / 2; ++ j) {
                if (memcmp(buffer + i * size, buffer1 + j * 2, size) == 0) {
                    found = 1;
                    break;
                }
            }
            if (!found)
                valid[i] = 0;
        }
    }



    FILE* file = fopen("base_address_list.txt", "w");

    printf("Storing ...\n");
    for (r = 0, i = saddr / size - 0xFF; i < saddr / size + 0xFF; ++ i) {
        if (valid[i]) {
            r ++;
            for (j = 0; j < size; ++ j) {
                fprintf(file, "\\x%02x", buffer[i * size + j]);
            }
            int offset =  saddr - i * size;
            fprintf(file," %d\n", offset);
        }
    }
    printf("Stored: %d\n", r);

    fclose(file);
*/
/*
    int stop;
    printf("Continue? Restart Client ... ");
    scanf("%d", &stop);

    p = createProcess("super", "[heap]");
*/



/*
    int i;

    for (i = 0; i < 0xFFFFFFF; i += 2) {
        if (memcmp("Sunny", buffer + i, 5) == 0) {
            break;
        }
    }

  */  

    return EXIT_SUCCESS;
}
