#include "MemoryRegion.h"

#include <sstream>
#include <sys/mman.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h> 
#include <dirent.h>
#include <dlfcn.h>

#include <stdexcept>
#include <string>
#include <vector>

namespace mm { 
    namespace util {
        pid_t findProcessIdByName(const std::string& name);

        std::string findNameByProcessId(pid_t pid);

        std::string findPathToExeByProcessId(pid_t pid);

        std::vector<MemoryRegion> findMemoryRegionsByProcessId(pid_t pid);

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
        uint64_t findFunctionAddress(const std::string& path, const std::string& funcName);

        template<typename T> 
        T convHexStr(const char* hexstr, bool silent = false) {
            std::stringstream ss;
            T val;

            if (memcmp(hexstr, "0x", 2) == 0) {
                ss << std::hex << (hexstr + 2);
            } else {
                ss << std::hex << hexstr;
            }

            ss >> val;

            if (!silent && ss.fail()) {
                throw std::runtime_error("Failed to convert number from hexstring: " + std::string(hexstr));
            }

            return val;
        }
    } 
}
