#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dlfcn.h>
#include <stdio.h>

#include <sys/socket.h>

extern "C" ssize_t __send (int socket, const void* buffer, size_t length, int flags) {
    printf("send: %zu\n", length);
    for (int i = 0; i < length; ++ i)
        printf("%02x ", (unsigned char)((char*)buffer)[i]);
    return send(socket, buffer, length, flags);
}

void __attribute__ ((constructor)) init() {
    printf("Loaded injected lib\n");
}

// Cleanup
void __attribute__ ((destructor)) fini() {
    printf("Unload injected lib\n");
}

