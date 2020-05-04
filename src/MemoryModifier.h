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

bool getMemoryRegion(Process* p, char* m_name, MemoryRegion* m_region);

bool signaturePayload(MemoryRegion reg, const byte *signature, byte *payload, const uint32_t siglen, const uint32_t paylen, const uint32_t bsize, const int offset);

bool writeProcessMemory(MemoryRegion reg, uint64_t address, byte *buffer, const int64_t size);

bool readProcessMemory(MemoryRegion reg, uint64_t address, byte *buffer, const int64_t size);

long searchAddress(MemoryRegion reg, const byte *signature, const uint8_t size);

long searchOffset(MemoryRegion reg, const byte *signature, const uint8_t size);

void searchByValue(Process *p, MemoryRegion reg, uint8_t size, DataType type);

void searchByChange(Process *p, MemoryRegion reg, uint64_t size);

uint64_t getAddressByPattern(MemoryRegion reg, const uint64_t start, byte *pattern, char *mask, const size_t size);

void showRange(MemoryRegion reg, uint64_t address, int start, int end);
