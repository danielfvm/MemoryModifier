#include "MonoManager.h"

#include <stdexcept>

namespace mm {
    MonoManager::MonoManager(Process& process, const std::string& assembly_name) 
        : process(process), monobdwgc(process.getMemoryRegion("libmonobdwgc-2.0.so", PROT_READ | PROT_EXEC)) {

        intptr_t mGetRootDomain = getEmbeddedFunction("mono_get_root_domain");
        intptr_t mDomainAssemblyOpen = getEmbeddedFunction("mono_domain_assembly_open");
        intptr_t mAssemblyGetImage = getEmbeddedFunction("mono_assembly_get_image");

        intptr_t pAssemblyName = process.runMalloc(assembly_name);

        pDomain = process.runFunction(mGetRootDomain);
        if (pDomain == 0) {
            process.runFree(pAssemblyName);
            throw std::runtime_error("Failed to get root domain");
        }

        pAssembly = process.runFunction(mDomainAssemblyOpen, pDomain, pAssemblyName);
        if (pAssembly == 0) {
            process.runFree(pAssemblyName);
            throw std::runtime_error("Failed to open assembly");
        }

        pImage = process.runFunction(mAssemblyGetImage, pAssembly);
        if (pImage == 0) {
            process.runFree(pAssemblyName);
            throw std::runtime_error("Failed to get image");
        }

        process.runFree(pAssemblyName);
    }

    intptr_t MonoManager::getEmbeddedFunction(const std::string& name) {
        return monobdwgc.getSymbolAddress(name);
    }

    intptr_t MonoManager::getClass(const std::string& class_name) {
        intptr_t mClassFromName = getEmbeddedFunction("mono_class_from_name");

        intptr_t pClassName = process.runMalloc(class_name);
        intptr_t pEmpty = process.runMalloc("");
        intptr_t pClass = process.runFunction(mClassFromName, pImage, pEmpty, pClassName);

        process.runFree(pClassName);
        process.runFree(pEmpty);

        return pClass;
    }

    uint32_t MonoManager::getFieldOffsetOfClass(intptr_t pClass, const std::string& name) {
        intptr_t mClassGetFieldFromName = getEmbeddedFunction("mono_class_get_field_from_name");
        intptr_t mFieldGetOffset = getEmbeddedFunction("mono_field_get_offset");

        intptr_t pFieldName = process.runMalloc(name);
        uint64_t pField = process.runFunction(mClassGetFieldFromName, pClass, pFieldName);

        if (pField == 0) {
            process.runFree(pFieldName);
            return 0;
        }

        uint32_t offset = process.runFunction(mFieldGetOffset, pField);

        process.runFree(pFieldName);

        return offset;
    }


    intptr_t MonoManager::getMethodOfClass(intptr_t pClass, const std::string& name) {
        intptr_t mClassGetMethodFromName = monobdwgc.getSymbolAddress("mono_class_get_method_from_name");
        intptr_t mCompileMethod = monobdwgc.getSymbolAddress("mono_compile_method");

        intptr_t pMethodName = process.runMalloc(name);
        uint64_t pMethod = process.runFunction(mClassGetMethodFromName, pClass, pMethodName, -1);

        if (pMethod == 0) {
            process.runFree(pMethodName);
            return 0;
        }

        intptr_t address = process.runFunction(mCompileMethod, pMethod);

        process.runFree(pMethodName);

        return address;
    }
}
