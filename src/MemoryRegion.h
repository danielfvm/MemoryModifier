#pragma once

#include <string.h>
#include <string>
#include <vector>

class MemoryRegion {
private:
    std::string m_name;
    std::string m_info;

    int m_prot; // example for flag: "r-xp"

    uint64_t m_start;
    uint64_t m_end;

    friend inline std::vector<MemoryRegion> getMemoryRegionsByProcessId(pid_t pid);

public:
    MemoryRegion(const std::string& name, const std::string& info, int prot, uint64_t start, uint64_t end)
        : m_name(name), m_info(info), m_prot(prot), m_start(start), m_end(end) {}

    const std::string& getName() const { return m_name; }
    const std::string& getInfo() const { return m_info; }

    const int getProt() const { return m_prot; }

    uint64_t getSymbolOffset(const std::string& name);
    uint64_t getSymbolAddress(const std::string& name);

    uint64_t scanPatternOffset(const char* signature, const char* mask, size_t size) const;
    uint64_t scanPatternAddress(const char* signature, const char* mask, size_t size) const;

    uint64_t getStart() const { return m_start; }
    uint64_t getEnd() const { return m_end; }
    uint64_t getSize() const { return m_end - m_start; }
};
