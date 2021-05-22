#pragma once

#include <unistd.h>
#include <string.h>
#include <string>
#include <vector>

#include "MemoryRegion.h"


struct Relocation {
    uint64_t offset;
    uint64_t info;
    long addend;
    std::string name;
};


class Process {
private:
    pid_t m_pid;

    int m_mem;

    std::string m_name;

    std::vector<MemoryRegion> m_regions;

    void loadMemoryRegions();

public:
    Process(pid_t pid);

    Process(const std::string& name);

    ~Process();

    pid_t getPID() { return m_pid; }

    std::string& getName() { return m_name; }

    const std::vector<MemoryRegion>& getMemoryRegions() { return m_regions; }

    const MemoryRegion& getMemoryRegion(const std::string& name, const char flags[4]);

    const MemoryRegion& getMemoryRegion(const char flags[4]);

    const MemoryRegion& getMemoryRegion(uint64_t addr);

    const MemoryRegion& loadSharedLibrary(const std::string& path);

    bool unloadSharedLibrary(const MemoryRegion& library);

    uint64_t runFunction(const uint64_t address, uint64_t p1 = 0, uint64_t p2 = 0, uint64_t p3 = 0, uint64_t p4 = 0, uint64_t p5 = 0, uint64_t p6 = 0);

    uint64_t getLibcFunction(const std::string& name);

    uint64_t mallocString(const std::string& text);

    /* scan for address by using provided signature */
    std::vector<struct Relocation> scan(uint64_t from, uint64_t to, const char *signature, uint64_t signature_size);

    long writePayload(const MemoryRegion& m, const char *signature, uint64_t signature_size, const char *payload, uint64_t payload_size);

    std::vector<Relocation> getGlobalOffsetAddress(const std::string& name);

    template<typename T>
    bool writeMemory(uint64_t address, const T& buffer, uint64_t size = sizeof(T)) {
        lseek(m_mem, address, SEEK_SET);

        char* bytes = (char*) malloc(size);
        memcpy(bytes, std::addressof(buffer), size);

        if (std::is_pointer<T>::value) {
            memcpy(bytes, (void*)buffer, size);
        } else {
            memcpy(bytes, (void*)std::addressof(buffer), size);
        }

        if (!write(m_mem, bytes, size)) {
            return false;
        }

        free(bytes);

        lseek(m_mem, 0, SEEK_SET);

        return true;
    }


    template<typename T>
    bool readMemory(uint64_t address, const T& buffer, uint64_t size = sizeof(T)) {
        lseek(m_mem, address, SEEK_SET);

        char* bytes = (char*) malloc(size);

        if (!read(m_mem, bytes, size)) {
            free(bytes);
            return false;
        }

        if (std::is_pointer<T>::value) {
            memcpy((void*)buffer, bytes, size);
        } else {
            memcpy((void*)std::addressof(buffer), bytes, size);
        }

        free(bytes);

        lseek(m_mem, 0, SEEK_SET);

        return true;
    }
};
