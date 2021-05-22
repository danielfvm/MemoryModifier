#include "Process.h"
#include "util.h"

#include <stdexcept>
#include <fcntl.h>  

#include "ptrace.h"

#include <sys/ptrace.h>
#include <sys/mman.h>
#include <sys/user.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <elf.h>

Process::Process(pid_t pid) {
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

Process::Process(const std::string& name) {
    m_pid = util::findProcessIdByName(name);
    m_name = util::findNameByProcessId(m_pid);
    m_regions = util::findMemoryRegionsByProcessId(m_pid);

    char memfile[100];
    sprintf(memfile, "/proc/%d/mem", m_pid);

    // Open filedescriptor for the process memory file
    if ((m_mem = open(memfile, O_RDWR)) < 0) {
        throw std::runtime_error("Failed to inject into memory, forgot sudo?");
    }
}


const MemoryRegion& Process::getMemoryRegion(const std::string& name, const char flags[4]) {
    for (auto it = m_regions.begin(); it != m_regions.end(); ++ it) {
        if (strstr(it->getName().c_str(), name.c_str()) != nullptr && memcmp(it->getFlags(), flags, 4) == 0) {
            return *it;
        }
    }
    throw std::runtime_error("Failed to find MemoryRegion: " + name + " " + flags);
}

const MemoryRegion& Process::getMemoryRegion(const char flags[4]) {
    for (auto it = m_regions.begin(); it != m_regions.end(); ++ it) {
        if (memcmp(it->getFlags(), flags, 4) == 0) {
            return *it;
        }
    }
    throw std::runtime_error(std::string("Failed to find MemoryRegion with flags: ") + flags);
}

const MemoryRegion& Process::getMemoryRegion(uint64_t addr) {
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

    uint64_t __libc_dlopen_mode_addr = getLibcFunction("__libc_dlopen_mode");
    uint64_t free_addr = getLibcFunction("free");
    uint64_t path_addr = mallocString(fullpath);

    // If shared library has already been loaded, first unload it
    try {
        unloadSharedLibrary(getMemoryRegion(fullpath, "r--p"));
    } catch(...) {}


    // Call __libc_dlopen_mode function
    uint64_t handle = runFunction(__libc_dlopen_mode_addr, path_addr, RTLD_LAZY);

    // If handle is zero, __libc__dlopen_mode failed opening shared lib
    if (handle == 0) {
        throw std::runtime_error("Failed to run __libc_dlopen_mode for shared library: " + fullpath);
    }

    // Free path string from memory
    runFunction(free_addr, path_addr);

    // Update MemoryRegions, we should find our new loaded
    // shared library with getMemoryRegion after this.
    m_regions = util::findMemoryRegionsByProcessId(m_pid);

    try {
        const MemoryRegion& region = getMemoryRegion(fullpath, "r--p");

        // If we are here, we sucessfully loaded our shared library.
        // Now we do some hacky trick to get the handler back for unloading.
        // We do so by replacing the first bytes of the start address with
        // our pointer to the handle.
        writeMemory<uint64_t>(region.getStart(), handle, sizeof(uint64_t));

        return region;
    } catch(std::runtime_error e) {
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
    bool was_unloaded = runFunction(getLibcFunction("__libc_dlclose"), handle) == 0;

    // Update memory regions, if __libc__dlclose was able to close a lib
    if (was_unloaded) {
        m_regions = util::findMemoryRegionsByProcessId(m_pid);
    }

    return was_unloaded;
}

Process::~Process() {
    close(m_mem);
}

uint64_t Process::runFunction(const uint64_t address, uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5, uint64_t p6) {
    struct REG_TYPE regs;
    struct REG_TYPE regs_backup;
    memset(&regs_backup, 0, sizeof(struct REG_TYPE));
	memset(&regs, 0, sizeof(struct REG_TYPE));

    // Attach to process
    ptrace_attach(m_pid);

    // Read register and make a copy
    ptrace_getregs(m_pid, &regs);
    memcpy(&regs_backup, &regs, sizeof(struct REG_TYPE));

    uint64_t exec_addr = getMemoryRegion(m_name, "r-xp").getStart() + sizeof(long);

    // Write parameters
    regs.rax = address;
    regs.rip = exec_addr;
    regs.rdi = p1;
    regs.rsi = p2;
    regs.rdx = p3;
    regs.rcx = p4;
    regs.r8 = p5;
    regs.r9 = p6;
    ptrace_setregs(m_pid, &regs);

    // Make backup of code at current rip address and overwrite with
    // our own machine 0xccd0ff which looks in assambl like this:
    //      call rax        ff d0 
    //      int3            cc
    uint64_t code_backup;
    readMemory<uint64_t>(exec_addr, code_backup, 8);
    writeMemory<uint64_t>(exec_addr, 0xccd0ff, 8);

    // We can now continue runing the process which will exectute
    // our function and will stop as soon as it reaches int3
    ptrace_cont(m_pid);

    // Read registers so we can get the return value from rax
    ptrace_getregs(m_pid, &regs);

    // Restore backup code, registers and detach from process
    writeMemory<uint64_t>(exec_addr, code_backup, 8);
    ptrace_setregs(m_pid, &regs_backup);
    ptrace_detach(m_pid);

    return regs.rax;
}

uint64_t Process::getLibcFunction(const std::string& name) {
    MemoryRegion libc = getMemoryRegion("libc-", "r--p");
    uint64_t addr = libc.getSymbolAddress(name);
    return addr;
}

uint64_t Process::mallocString(const std::string& text) {
    uint64_t str_addr = runFunction(getLibcFunction("malloc"), text.size() + 1);

    // Write our string to our address, the string is NUL terminated
    writeMemory<const char*>(str_addr, text.c_str(), text.size() + 1);

    // Return address to string, should be freed after use!
    return str_addr;
}

std::vector<Relocation> Process::getGlobalOffsetAddress(const std::string& name) {

    MemoryRegion mr = getMemoryRegion(m_name, "r--p");

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
