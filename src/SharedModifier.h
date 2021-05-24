#pragma once

#include <sys/mman.h>
#include <stdint.h>
#include <assert.h>
#include <dlfcn.h>
#include <string>
#include <elf.h>

#include "util.h"

namespace SharedModifier {

    template <typename T>
    T getSymbolAddress(const char* filename, const char* symbol)
    {
        void* handle = dlopen(filename, RTLD_NOW);
        T result = reinterpret_cast<T>(dlsym(handle, symbol));
        dlclose(handle);

        return result;
    }

    inline void**& getvtable(void* inst, size_t offset = 0)
    {
        return *reinterpret_cast<void***>((size_t)inst + offset);
    }

    inline const void** getvtable(const void* inst, size_t offset = 0)
    {
        return *reinterpret_cast<const void***>((size_t)inst + offset);
    }

    template<typename Fn>
    inline Fn getvfunc(const void* inst, size_t index, size_t offset = 0)
    {
        return reinterpret_cast<Fn>(getvtable(inst, offset)[index]);
    }


    inline std::vector<MemoryRegion> getMemoryRegions() {
        return util::findMemoryRegionsByProcessId(getpid());
    }


    template<typename T>
    bool writeMemory(MemoryRegion region, uint64_t address, const T& buffer, uint64_t size = sizeof(T)) {
        if (mprotect((void*)region.getStart(), region.getSize(), PROT_READ | PROT_WRITE) != 0) {
            return false;
        }

        puts("write");

        if (std::is_pointer<T>::value) {
            memcpy((void*)address, (void*)buffer, size);
        } else {
            memcpy((void*)address, (void*)std::addressof(buffer), size);
        }
        puts("finish");

        if (mprotect((void*)region.getStart(), region.getSize(), region.getProt()) != 0) {
            return false;
        }

        return true;
    }


    template<typename T>
    void readMemory(uint64_t address, const T& buffer, uint64_t size = sizeof(T)) {
        if (std::is_pointer<T>::value) {
            memcpy((void*)buffer, (void*)address, size);
        } else {
            memcpy((void*)std::addressof(buffer), (void*)address, size);
        }
    }

    inline MemoryRegion getMemoryRegion(const std::string& name, int prot) {
        auto m_regions = getMemoryRegions();
        for (auto it = m_regions.begin(); it != m_regions.end(); ++ it) {
            if (strstr(it->getName().c_str(), name.c_str()) != nullptr && it->getProt() == prot) {
                return *it;
            }
        }
        throw std::runtime_error("Failed to find MemoryRegion: " + name + " " + std::to_string(prot));
    }

    inline MemoryRegion getMemoryRegionFromAddress(uint64_t addr) {
        auto m_regions = getMemoryRegions();
        for (auto it = m_regions.begin(); it != m_regions.end(); ++ it) {
            if (it->getStart() <= addr && addr <= it->getEnd()) {
                return *it;
            }
        }
        throw std::runtime_error("Failed to find MemoryRegion with addr: " + std::to_string(addr));
    }

    inline std::vector<Relocation> getGlobalOffsetAddress(const std::string& name) {
        MemoryRegion mr = getMemoryRegion(util::findNameByProcessId(getpid()), PROT_READ);

        // Get file size
        FILE *file = fopen(mr.getName().c_str(), "r");

        if (file == nullptr) {
            throw std::runtime_error("Failed to open shared lib: " + mr.getName());
        }

        if (fseek(file, 0, SEEK_END)) {
            fclose(file);
            throw std::runtime_error("Failed to determine size of shared lib: " + mr.getName());
        }
        long file_size = ftell(file);

        // Load file to memory
        void *bytes = mmap(nullptr, (size_t)file_size, PROT_READ, MAP_PRIVATE, fileno(file), 0);
        if (bytes == nullptr) {
            fclose(file);
            throw std::runtime_error("Failed to open shared lib with mmap: " + mr.getName());
        }
        fclose(file);

        // Magic number
        const unsigned char expected_magic[] = {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3};

        Elf64_Ehdr elf_hdr;
        memmove(&elf_hdr, bytes, sizeof(elf_hdr));

        // Check if it is an elf file
        if (memcmp(elf_hdr.e_ident, expected_magic, sizeof(expected_magic)) != 0) {
            throw std::runtime_error("Target is not an ELF executable");
        }

        // Check if file is elf64
        if (elf_hdr.e_ident[EI_CLASS] != ELFCLASS64) {
            throw std::runtime_error("Only ELF-64 is supported");
        }

        // Check if its an x86_64 architecture
        if (elf_hdr.e_machine != EM_X86_64) {
            throw std::runtime_error("Only x86-64 is supported");
        }

        // Search offsets
        size_t dynstr_off = 0;    // stores symbol names
        size_t dynsym_off = 0;    // stores address to string stored in dynstr
        size_t rela_off = 0;      // stores offset and idx to dynsym
        size_t rela_sz = 0;       // size of rela

        for (uint16_t i = 0; i < elf_hdr.e_shnum; i++) {
            size_t offset = elf_hdr.e_shoff + i * elf_hdr.e_shentsize;
            Elf64_Shdr shdr;
            memmove(&shdr, (char*)bytes + offset, sizeof(shdr));

            switch (shdr.sh_type) {
            case SHT_SYMTAB:
            case SHT_STRTAB:
                if (!dynstr_off) dynstr_off = shdr.sh_offset;
                break;
            case SHT_DYNSYM:
                dynsym_off = shdr.sh_offset;
                break;
            case SHT_RELA:
                rela_off = shdr.sh_offset;
                rela_sz = shdr.sh_size;
                break;
            }
        }

        // validate offsets
        assert(dynstr_off);
        assert(dynsym_off);
        assert(rela_off);

        std::vector<Relocation> relocations;

        for (size_t j = 0; j * sizeof(Elf64_Rela) < rela_sz; j++) {
            Elf64_Rela rela;
            size_t absoffset = rela_off + j * sizeof(Elf64_Rela);
            memmove(&rela, (char*)bytes + absoffset, sizeof(rela));

            Elf64_Sym sym;
            absoffset = dynsym_off + ELF64_R_SYM(rela.r_info) * sizeof(Elf64_Sym);
            memmove(&sym, (char*)bytes + absoffset, sizeof(sym));

            if (strcmp((char*)bytes + dynstr_off + sym.st_name, name.c_str()) == 0) {
                relocations.push_back(Relocation {
                    .offset = rela.r_offset,
                    .info = rela.r_info,
                    .addend = rela.r_addend,
                });
            }
        }

        munmap(bytes, file_size);

        return relocations;
    }
}
