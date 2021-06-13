#pragma once

#include "Process.h"

#include <string>

namespace mm {
    class MonoManager {
    private:
        Process& process;
        const MemoryRegion& monobdwgc;

        intptr_t pDomain, pAssembly, pImage;

    public:
        MonoManager(Process& process, const std::string& assembly_name = "Assembly-CSharp");

        intptr_t getDomain() { return pDomain; }

        intptr_t getAssembly() { return pAssembly; }

        intptr_t getImage() { return pImage; }

        intptr_t getEmbeddedFunction(const std::string& name);

        intptr_t getClass(const std::string& class_name);

        uint32_t getFieldOffsetOfClass(intptr_t pClass, const std::string& name);

        intptr_t getMethodOfClass(intptr_t pClass, const std::string& name);
    };
}
