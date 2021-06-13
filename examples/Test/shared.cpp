#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dlfcn.h>
#include <stdio.h>

#include "../../src/MemoryModifier.h"
#include "../../src/util.h"

static Process* self = nullptr;

unsigned char funcbackup[15];
uint64_t funcaddr;
typedef int (*test_t)(char c);

test_t orig_test_func;

extern "C" char test (char c) {
    printf("Hello!\n");
    self->writeMemory<unsigned char*>((intptr_t)orig_test_func, funcbackup, 13);

    char ret = orig_test_func(c);

    self->detourFunction((intptr_t)orig_test_func, (uint64_t)test, funcbackup);

    return ret;
}

void __attribute__ ((constructor)) init() {
    self = new Process();

    printf("Found map count: %zu\n", self->getMemoryRegions().size());
    printf("Exe path: %s\n", self->getPathToExe().c_str());

    MemoryRegion main = self->getMemoryRegion(util::findNameByProcessId(getpid()), PROT_READ | PROT_EXEC);

    printf("hook function test: %p\n", test);

    orig_test_func = (test_t)(main.getStart()+0x149);//main.getSymbolAddress("test");
    printf("orig function test: %p\n", orig_test_func);

    printf("detour: %d\n", self->detourFunction((intptr_t)orig_test_func, (uint64_t)test, funcbackup));

    printf("Loaded injected lib\n");
}

// Cleanup
void __attribute__ ((destructor)) fini() {
    printf("Unload injected lib\n");
}
