#pragma once

#include <sys/mman.h>
#include <memory>
#include <type_traits>
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

    std::string m_pathtoexe;

    std::vector<MemoryRegion> m_regions;

    void loadMemoryRegions();

public:
    Process(pid_t pid);

    Process(const std::string& name);

    Process();

    ~Process();

    void openProcess(pid_t pid);

    pid_t getPID() { return m_pid; }

    const std::string& getName() { return m_name; }

    const std::string& getPathToExe() { return m_pathtoexe; }

    uint64_t getStart() { return m_regions.front().getStart(); }

    uint64_t getEnd() { return m_regions.at(m_regions.size()-2).getEnd(); }

    const std::vector<MemoryRegion>& getMemoryRegions() { return m_regions; }

    const MemoryRegion& getMemoryRegion(const std::string& name, int prot);

    const MemoryRegion& getMemoryRegionFromProtection(int prot);

    const MemoryRegion& getMemoryRegionFromAddress(uint64_t addr);

    const MemoryRegion& loadSharedLibrary(const std::string& path);

    bool unloadSharedLibrary(const MemoryRegion& library);

    uint64_t runFunction(const uint64_t address, uint64_t p1 = 0, uint64_t p2 = 0, uint64_t p3 = 0, uint64_t p4 = 0, uint64_t p5 = 0, uint64_t p6 = 0);

    uint64_t getLibcFunction(const std::string& name);

    uint64_t mallocString(const std::string& text);

    long writePayload(const MemoryRegion& m, const char *signature, uint64_t signature_size, const char *payload, uint64_t payload_size);

    std::vector<Relocation> getGlobalOffsetAddress(const std::string& name);

    uint64_t scanPatternAddress(uint64_t addr, const char* pattern, const char* mask, size_t pattern_size);

    enum DetourOption {
        Replace,
        Before,
        After
    };

    bool detourFunction(uint64_t from_addr, uint64_t to_addr, DetourOption option);

    template<typename T>
    bool writeMemory(uint64_t address, const T& buffer, uint64_t size = sizeof(T));

    template<typename T>
    bool readMemory(uint64_t address, const T& buffer, uint64_t size = sizeof(T));
};
