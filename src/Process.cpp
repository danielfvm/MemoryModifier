#include "Process.h"
#include "ptrace.h"
#include "util.h"

#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <string>

#include <sys/ptrace.h>
#include <sys/mman.h>
#include <sys/user.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>  
#include <stdio.h>
#include <elf.h>


namespace mm {
    Process::Process(pid_t pid) {
        openProcess(pid);
    }

    Process::Process(const std::string& name) {
        openProcess(util::findProcessIdByName(name));
    }

    Process::Process() {
        m_pid = getpid();
        m_name = util::findNameByProcessId(m_pid);
        m_regions = util::findMemoryRegionsByProcessId(m_pid);
        m_pathtoexe = util::findPathToExeByProcessId(m_pid);
        m_mem = 0;
    }

    Process::~Process() {
        close(m_mem);
    }

    void Process::openProcess(pid_t pid) {
        m_pid = pid;
        m_name = util::findNameByProcessId(m_pid);
        m_regions = util::findMemoryRegionsByProcessId(m_pid);

        char memfile[100];
        sprintf(memfile, "/proc/%d/mem", m_pid);

        // Open filedescriptor for the process memory file
        if ((m_mem = open(memfile, O_RDWR)) < 0) {
            throw std::runtime_error("Failed to inject into memory, forgot sudo?");
        }
    }

    const MemoryRegion& Process::getMemoryRegion(const std::string& name, int prot) {
        for (auto it = m_regions.begin(); it != m_regions.end(); ++ it) {
            if (strstr(it->getName().c_str(), name.c_str()) != nullptr && it->getProt() == prot) {
                return *it;
            }
        }
        throw std::runtime_error("Failed to find MemoryRegion: " + name + " " + std::to_string(prot));
    }

    const MemoryRegion& Process::getMemoryRegionFromProtection(int prot) {
        for (auto it = m_regions.begin(); it != m_regions.end(); ++ it) {
            if (it->getProt() == prot) {
                return *it;
            }
        }
        throw std::runtime_error("Failed to find MemoryRegion with prot: " + std::to_string(prot));
    }

    const MemoryRegion& Process::getMemoryRegionFromAddress(intptr_t addr) {
        for (auto it = m_regions.begin(); it != m_regions.end(); ++ it) {
            if (it->getStart() <= addr && addr <= it->getEnd()) {
                return *it;
            }
        }
        throw std::runtime_error("Failed to find MemoryRegion with addr: " + std::to_string(addr));
    }

    const MemoryRegion& Process::loadSharedLibrary(const std::string& filename) {
        char* cfullpath = realpath(filename.c_str(), nullptr);

        // Check if file exists
        if (cfullpath == nullptr) {
            throw std::runtime_error("Couldn't find shared library: " + filename);
        }

        std::string fullpath = std::string(cfullpath);

        uint64_t handle;

        if (m_mem == 0) {
            handle = (uint64_t) dlopen(cfullpath, RTLD_LAZY);

            // If handle is zero, __libc__dlopen_mode failed opening shared lib
            if (handle == 0) {
                throw std::runtime_error("Failed to run __libc_dlopen_mode for shared library: " + fullpath);
            }
        } else {
            uint64_t __libc_dlopen_mode_addr = getLibcFunction("__libc_dlopen_mode");
            uint64_t free_addr = getLibcFunction("free");
            uint64_t path_addr = runMalloc(fullpath);

            // If shared library has already been loaded, first unload it
            try {
                unloadSharedLibrary(getMemoryRegion(fullpath, PROT_READ));
            } catch(...) {}

            // Call __libc_dlopen_mode function
            handle = runFunction(__libc_dlopen_mode_addr, path_addr, RTLD_LAZY);

            // If handle is zero, __libc__dlopen_mode failed opening shared lib
            if (handle == 0) {
                throw std::runtime_error("Failed to run __libc_dlopen_mode for shared library: " + fullpath);
            }

            // Free path string from memory
            runFunction(free_addr, path_addr);

            // Update MemoryRegions, we should find our new loaded
            // shared library with getMemoryRegion after this.
            m_regions = util::findMemoryRegionsByProcessId(m_pid);
        }

        try {
            const MemoryRegion& region = getMemoryRegion(fullpath, PROT_READ);

            // If we are here, we sucessfully loaded our shared library.
            // Now we do some hacky trick to get the handler back for unloading.
            // We do so by replacing the first bytes of the start address with
            // our pointer to the handle.
            writeMemory<uint64_t>(region.getStart(), handle, sizeof(uint64_t));

            return region;
        } catch (...) {
            throw std::runtime_error("Couldn't find recently loaded shared library: " + fullpath);
        }
    }

    bool Process::unloadSharedLibrary(const MemoryRegion& library) {
        uint64_t handle;
        readMemory<uint64_t>(library.getStart(), handle, sizeof(uint64_t));

        // Cant be a handle if its zero
        if (handle == 0) {
            return false;
        }

        // Check if our handle points to a valid position in memory
        uint64_t addr;
        if (!readMemory<uint64_t>(handle, addr, sizeof(uint64_t))) {
            return false;
        }

        // Check if handle points to a pointer that points to the start 
        // address of our library
        if (addr != library.getStart()) {
            return false;
        }

        // Run __libc_dlclose on handle
        bool was_unloaded;
       
        if (m_mem == 0) {
            was_unloaded = dlclose((void*)handle);
        } else {
            was_unloaded = runFunction(getLibcFunction("__libc_dlclose"), handle) == 0;
        }

        // Update memory regions, if __libc__dlclose was able to close a lib
        if (was_unloaded) {
            m_regions = util::findMemoryRegionsByProcessId(m_pid);
        }

        return was_unloaded;
    }

    uint64_t Process::runFunction(intptr_t address, uint64_t p0, uint64_t p1, uint64_t p2, uint64_t p3) {
        if (m_mem == 0) {
            typedef uint64_t (*func_prototype)(uint64_t, uint64_t, uint64_t, uint64_t);
            return ((func_prototype)address)(p0, p1, p2, p3);
        }

        struct REG_TYPE regs;
        struct REG_TYPE regs_backup;
        memset(&regs_backup, 0, sizeof(struct REG_TYPE));
        memset(&regs, 0, sizeof(struct REG_TYPE));

        // Attach to process
        ptrace_attach(m_pid);

        // Read register and make a copy
        ptrace_getregs(m_pid, &regs);
        memcpy(&regs_backup, &regs, sizeof(struct REG_TYPE));

        uint64_t exec_addr = getMemoryRegion(m_name, PROT_READ | PROT_EXEC).getStart() + sizeof(long);

        // Overwrite registers with our new code address and parameters
        regs.rip = exec_addr + 2;
        regs.rax = address;
        regs.rdi = p0;
        regs.rsi = p1;
        regs.rdx = p2;
        regs.rcx = p3;
        
        // Apply changes to register
        ptrace_setregs(m_pid, &regs);

        // 55                      push   rbp
        // 48 89 e5                mov    rbp, rsp
        // ff d0                   call   rax
        // 5d                      pop    rbp
        // cc                      int3 
        unsigned char code[9] = "\x55\x48\x89\xE5\xFF\xd0\x5D\xCC";
        unsigned char code_backup[9];
        size_t code_size = 8;

        // Make backup of code at exec_addr, and overwrite it with our own code
        readMemory<unsigned char*>(exec_addr, code_backup, code_size);
        writeMemory<unsigned char*>(exec_addr, code, code_size);

        // Execute it
        ptrace_cont(m_pid);

        // Read registers so we can get the return value from rax
        ptrace_getregs(m_pid, &regs);

        // Restore backup code, registers and detach from process
        writeMemory<unsigned char*>(exec_addr, code_backup, code_size);
        ptrace_setregs(m_pid, &regs_backup);
        ptrace_detach(m_pid);

        // Return result of function
        return regs.rax;
    }

    uint64_t Process::getLibcFunction(const std::string& name) {
        const MemoryRegion& libc = getMemoryRegion("libc-", PROT_READ);
        return libc.getSymbolAddress(name);
    }

    intptr_t Process::runMalloc(const char* value, size_t len) {
        intptr_t addr = runFunction(getLibcFunction("malloc"), len);

        if (addr == 0) {
            throw std::runtime_error("Failed to allocate memory");
        }

        // Write our data to our address
        writeMemory<const char*>(addr, value, len);

        // Return address to string, should be freed after use!
        return addr;
    }

    intptr_t Process::runMalloc(const std::string& value) {
        return runMalloc(value.c_str(), value.length() + 1);
    }

    void Process::runFree(intptr_t addr) {
        runFunction(getLibcFunction("free"), addr);
    }

    std::vector<Relocation> Process::getGlobalOffsetAddress(const std::string& name) {

        MemoryRegion mr = getMemoryRegion(m_name, PROT_READ);

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

    uint64_t Process::scanPatternAddress(intptr_t addr, const char* signature, const char* mask, size_t size) {
        char* memory = (char*) malloc(size * 2);

        for (; addr < getEnd() - size; addr += size) {
            readMemory<char*>(addr, memory, size * 2);
            for (size_t j = 0; j < size; j ++) {
                for (size_t i = 0; i < size; i ++) {
                    if (mask[i] != '?' && signature[i] != memory[i+j]) {
                        break;
                    } else if (i == size - 1) {
                        free(memory);
                        return addr + i + j;
                    }
                }
            }
        }

        free(memory);
        return 0;
    }

    int Process::detourFunction(intptr_t orig_addr, intptr_t new_addr, unsigned char* backup) {
        #define JMP_SIZE 13

        // mov rax, addr
        // call rax
        // ret
        // nop
        unsigned char jmp_code[JMP_SIZE] = { 0x48, 0xb8, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0xff, 0xd0, 0xc3 };

        // Replace NOP with offset to new function
        memcpy(jmp_code + 2, &new_addr, 8);

        if (backup != nullptr && !readMemory<unsigned char*>(orig_addr, backup, JMP_SIZE)) {
            return 0;
        }

        if (!writeMemory<unsigned char*>(orig_addr, jmp_code, JMP_SIZE)) {
            return 0;
        }

        return JMP_SIZE;
    }

    // example for pointerstr: "[[[/usr/bin/supertux2+0x5b8]+0xf8]+0x20]+0x4"
    intptr_t Process::getAddressFromPointerString(std::string pointerstr) {
        pointerstr.erase(std::remove(pointerstr.begin(), pointerstr.end(), '['), pointerstr.end());
        pointerstr.erase(std::remove(pointerstr.begin(), pointerstr.end(), ']'), pointerstr.end());
        pointerstr.erase(std::remove(pointerstr.begin(), pointerstr.end(), ' '), pointerstr.end());

        std::vector<std::string> parts;

        size_t start;
        size_t end = 0;

        while ((start = pointerstr.find_first_not_of('+', end)) != std::string::npos) {
            end = pointerstr.find('+', start);
            parts.push_back(pointerstr.substr(start, end - start));
        }

        const MemoryRegion& region = getMemoryRegion(parts[0], PROT_READ | PROT_WRITE);

        intptr_t pointer = region.getStart();
        uint64_t offset;

        std::stringstream ss;

        for (auto it = parts.begin() + 1; it != parts.end() - 1; ++ it) {
            ss.clear();
            ss << std::hex << (it->c_str() + 2);
            ss >> offset;

            readMemory<intptr_t>(pointer + offset, pointer);
        }

        ss.clear();
        ss << std::hex << (parts.back().c_str() + 2);
        ss >> offset;

        return pointer + offset;
    }
}
