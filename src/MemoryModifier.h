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

#include <ncurses.h>
#include <stdio.h>
#include <stdint.h>

#define getBytes(type, data) ({ \
    union { \
        type a; \
        byte bytes[sizeof(type)]; \
    } u; \
    u.a = data; \
    u.bytes; \
})

#define getValue(type, data) ({ \
    union { \
        type a; \
        byte bytes[sizeof(type)]; \
    } u; \
    memcpy(u.bytes, data, sizeof(type)); \
    u.a; \
})

#define getFlipValue(type, data) ({ \
    union { \
        type a; \
        byte bytes[sizeof(type)]; \
    } u; \
    for (int i = 0; i < 4; ++ i) \
        u.bytes[i] = data[3 - i]; \
    u.a; \
})


typedef struct {
    char name[100];
    bool readable;
    bool writable;
    bool executable;
    bool shared;
    uint64_t start;
    uint64_t end;
    int mem;
    // Offsets
} MemoryRegion;

typedef struct {
    long p_id;
    long b_addr;
    long b_size;
    char p_name[1024];
    long history[100];
    int p_mem;
    MemoryRegion *m_regions;
    size_t m_regions_size;
} Process;


typedef enum DataType {
    INT,
    FLOAT,
    STR,
    CHAR
} DataType;

typedef unsigned char byte;

Process* openProcess(char *p_name);

void closeProcess(Process *p);

bool getMemoryRegion(Process *p, char *m_name, MemoryRegion *m_region);

bool writeProcessMemory(MemoryRegion reg, uint64_t address, byte *buffer, const int64_t size);

bool readProcessMemory(MemoryRegion reg, uint64_t address, byte *buffer, const int64_t size);

long searchAddress(MemoryRegion reg, const byte *signature, const uint8_t size);

long searchOffset(MemoryRegion reg, const byte *signature, const uint8_t size);

void searchByValue(Process *p, MemoryRegion reg, uint8_t size, DataType type);

void searchByChange(Process *p, MemoryRegion reg, uint64_t size);

uint64_t getAddressByPattern(MemoryRegion reg, const uint64_t start, byte *pattern, char *mask, const size_t size);

void showRange(Process *p, MemoryRegion *reg, uint64_t address, int start, int end);
