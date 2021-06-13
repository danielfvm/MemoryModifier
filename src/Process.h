#pragma once

#include <sys/mman.h>
#include <memory>
#include <type_traits>
#include <unistd.h>
#include <string.h>
#include <string>
#include <vector>

#include "MemoryRegion.h"

namespace mm {
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

        intptr_t getStart() { return m_regions.front().getStart(); }

        intptr_t getEnd() { return m_regions.at(m_regions.size()-2).getEnd(); }

        const std::vector<MemoryRegion>& getMemoryRegions() { return m_regions; }

        const MemoryRegion& getMemoryRegion(const std::string& name, int prot);

        const MemoryRegion& getMemoryRegionFromProtection(int prot);

        const MemoryRegion& getMemoryRegionFromAddress(intptr_t addr);

        const MemoryRegion& loadSharedLibrary(const std::string& path);

        bool unloadSharedLibrary(const MemoryRegion& library);

        uint64_t runFunction(intptr_t address, uint64_t p1 = 0, uint64_t p2 = 0, uint64_t p3 = 0, uint64_t p4 = 0);

        uint64_t getLibcFunction(const std::string& name);

        intptr_t runMalloc(const char* value, size_t len);

        intptr_t runMalloc(const std::string& value);

        void runFree(intptr_t addr);

        // long writePayload(const MemoryRegion& m, const char *signature, uint64_t signature_size, const char *payload, size_t payload_size);

        std::vector<Relocation> getGlobalOffsetAddress(const std::string& name);

        uint64_t scanPatternAddress(intptr_t addr, const char* pattern, const char* mask, size_t pattern_size);

        int detourFunction(intptr_t orig_addr, intptr_t new_addr, unsigned char* backup = nullptr);

        intptr_t getAddressFromPointerString(std::string pointer);

        template<typename T>
        bool writeMemory(intptr_t address, const T& buffer, size_t size = sizeof(T)) {
            void* from = std::is_pointer<T>::value ? (void*)buffer : (void*)std::addressof(buffer);

            if (m_mem == 0) {
                const MemoryRegion& region = getMemoryRegionFromAddress(address);

                if (mprotect((void*)region.getStart(), region.getSize(), PROT_READ | PROT_WRITE) != 0) {
                    return false;
                }

                bool success = memcpy((void*)address, from, size) != nullptr;

                if (mprotect((void*)region.getStart(), region.getSize(), region.getProt()) != 0) {
                    return false;
                }

                return success;
            } else {
                lseek(m_mem, address, SEEK_SET);

                char* bytes = (char*) malloc(size);

                if (memcpy(bytes, from, size) == nullptr) {
                    return false;
                }

                if (!write(m_mem, bytes, size)) {
                    return false;
                }

                free(bytes);

                lseek(m_mem, 0, SEEK_SET);

                return true;
            }
        }

        template<typename T>
        bool readMemory(intptr_t address, const T& buffer, size_t size = sizeof(T)) {
            void* to = std::is_pointer<T>::value ? (void*)buffer : (void*)std::addressof(buffer);

            if (m_mem == 0) {
                return memcpy(to, (void*)address, size) != nullptr;
            } else {
                lseek(m_mem, address, SEEK_SET);

                char* bytes = (char*) malloc(size);

                if (!read(m_mem, bytes, size)) {
                    free(bytes);
                    return false;
                }

                memcpy(to, bytes, size);

                free(bytes);

                lseek(m_mem, 0, SEEK_SET);

                return true;
            }
        }
    };
}
