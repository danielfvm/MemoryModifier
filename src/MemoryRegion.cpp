#include "MemoryRegion.h"

#include <stdexcept>

#include <sys/mman.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <elf.h>

namespace mm {
    uint64_t MemoryRegion::getSymbolOffset(const std::string& name) const {

        // Get file size
        FILE *file = fopen(m_name.c_str(), "r");

        if (file == nullptr) {
            throw std::runtime_error("Failed to open shared lib: " + m_name);
        }

        if (fseek(file, 0, SEEK_END)) {
            fclose(file);
            throw std::runtime_error("Failed to determine size of shared lib: " + m_name);
        }
        long file_size = ftell(file);

        // Load file to memory
        void *bytes = mmap(nullptr, (size_t)file_size, PROT_READ, MAP_PRIVATE, fileno(file), 0);
        if (bytes == nullptr) {
            fclose(file);
            throw std::runtime_error("Failed to open shared lib with mmap: " + m_name);
        }
        fclose(file);

        // Magic number
        const unsigned char expected_magic[] = {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3};

        Elf64_Ehdr elf_hdr;
        memmove(&elf_hdr, bytes, sizeof(elf_hdr));

        // Check if it is an elf file
        if (memcmp(elf_hdr.e_ident, expected_magic, sizeof(expected_magic)) != 0) {
            throw std::runtime_error("Target is not an ELF executable");
            return 1;
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
        size_t dynsym_sz = 0;     // stores address to string stored in dynstr

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
                dynsym_sz = shdr.sh_size;
                break;
            }
        }

        // validate offsets
        assert(dynstr_off);
        assert(dynsym_off);

        for (size_t j = 0; j * sizeof(Elf64_Sym) < dynsym_sz; j++) {
            Elf64_Sym sym;
            size_t absoffset = dynsym_off + j * sizeof(Elf64_Sym);
            memmove(&sym, (char*)bytes + absoffset, sizeof(sym));

            if (strcmp((char*)bytes + dynstr_off + sym.st_name, name.c_str()) == 0) {
                return sym.st_value;
            }
        }

        munmap(bytes, file_size);

        throw std::runtime_error("Symbol name '" + name + "' not found in shared lib: " + m_name);
    }

    intptr_t MemoryRegion::getSymbolAddress(const std::string& name) const {
        return m_start + getSymbolOffset(name);
    }

    uint64_t MemoryRegion::scanPatternOffset(const char* signature, const char* mask, size_t size) const {
        // Get file size
        FILE *file = fopen(m_name.c_str(), "r");

        if (file == nullptr) {
            throw std::runtime_error("Failed to open shared lib: " + m_name);
        }

        if (fseek(file, 0, SEEK_END)) {
            fclose(file);
            throw std::runtime_error("Failed to determine size of shared lib: " + m_name);
        }
        long filesize = ftell(file);

        // Load file to memory
        void *bytes = mmap(nullptr, (size_t)filesize, PROT_READ, MAP_PRIVATE, fileno(file), 0);
        if (bytes == nullptr) {
            fclose(file);
            throw std::runtime_error("Failed to open shared lib with mmap: " + m_name);
        }
        fclose(file);

        for (long offset = 0; offset < filesize; offset += 1) {
            for (size_t i = 0; i < size; i ++) {
                if (mask[i] != '?' && signature[i] != ((char*)bytes + offset)[i]) {
                    break;
                } else if (i == size - 1) {
                    return offset;
                }
            }
        }

        munmap(bytes, filesize);

        // We didnt find pattern
        return 0;
    }

    intptr_t MemoryRegion::scanPatternAddress(const char* signature, const char* mask, size_t size) const {
        return m_start + scanPatternOffset(signature, mask, size);
    }
}
