#include "util.h"

#include <unistd.h>

#include <stdexcept>
#include <cstring>
#include <string>

namespace mm {
    pid_t util::findProcessIdByName(const std::string& name) {
        struct dirent *entry;

        FILE *statusfile;
        char statusfilename[512];
        char statusstring[32];

        DIR *procdir;

        if ((procdir = opendir("/proc/")) == NULL) {
            throw std::runtime_error("Failed to open directory /proc/\n");
        }

        while ((entry = readdir(procdir)) != NULL) {
            if (atol(entry->d_name) == 0) {
                continue;
            }

            sprintf(statusfilename, "/proc/%s/status", entry->d_name);

            if ((statusfile = fopen(statusfilename, "r")) == NULL) {
                throw std::runtime_error(std::string("Failed to open status file: ") + statusfilename);
            }

            fseek(statusfile, 6, SEEK_SET);
            fgets(statusstring, 32, statusfile);
            fclose(statusfile);

            if (strstr(statusstring, name.c_str()) != NULL) {
                return atol(entry->d_name);
            }
        }

        throw std::runtime_error(std::string("Process not found: ") + name);
    }

    std::string util::findNameByProcessId(pid_t pid) {
        char statusfilename[64];
        char statusstring[32];

        FILE* statusfile;

        sprintf(statusfilename, "/proc/%d/status", pid);

        if ((statusfile = fopen(statusfilename, "r")) == NULL) {
            throw std::runtime_error(std::string("Failed to open status file: ") + statusfilename);
        }

        fseek(statusfile, 6, SEEK_SET);
        fgets(statusstring, 32, statusfile);

        fclose(statusfile);

        // delete \n at the end
        statusstring[strlen(statusstring)-1] = '\0';

        return statusstring;
    }


    std::string util::findPathToExeByProcessId(pid_t pid) {
        char exefile[64];
        char path[64];

        size_t buff_len;

        sprintf(exefile, "/proc/%d/exe", pid);

        if ((buff_len = readlink (exefile, path, 63)) == -1) {
            throw std::runtime_error("Failed to fetch filepath to exe of pid: " + std::to_string(pid));
        }

        return std::string(path);
    }


    std::vector<MemoryRegion> util::findMemoryRegionsByProcessId(pid_t pid) {
        std::vector<MemoryRegion> regions;

        FILE *f_maps;
        char n_maps[512];

        sprintf(n_maps, "/proc/%d/maps", pid);

        if ((f_maps = fopen(n_maps, "r")) == NULL) {
            throw std::runtime_error(std::string("Failed to open file: ") + n_maps);
        }

        char info[1024];
        char name[512];
        uint64_t start;
        uint64_t end;
        char flags[4];

        while (fgets(info, 1024, f_maps)) {
            if (sscanf(info, "%lx-%lx %s %*s %*d:%*d %*d %[^\n]", &start, &end, flags, name) != 4) {
                memset(name, 0, 512);
            }

            int prot = (flags[0] == 'r' ? PROT_READ  : 0) |
                       (flags[1] == 'w' ? PROT_WRITE : 0) |
                       (flags[2] == 'x' ? PROT_EXEC  : 0);

            regions.push_back(MemoryRegion(name, info, prot, start, end));
        }

        fclose(f_maps);

        return regions;
    }


    /*
     * getFunctionAddress()
     *
     * Find the address of a function within our own loaded copy of libc.so.
     *
     * args:
     * - char* funcName: name of the function whose address we want to find
     *
     * returns:
     * - a long containing the address of that function
     *
     */
    uint64_t util::findFunctionAddress(const std::string& path, const std::string& funcName)
    {
        void* handle = dlopen(path.c_str(), RTLD_LAZY);
        void* funcAddr = dlsym(handle, funcName.c_str());

        dlclose(handle);

        return (uint64_t) funcAddr;
    }
}
