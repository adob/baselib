#include "mem.h"

#include <stdlib.h>

#include "exceptions.h"

using namespace lib;
using namespace lib::mem;

byte *mem::alloc(size size) {
    void *p = ::malloc(size);
    if (!p) {
        exceptions::out_of_memory();
    }
    return (byte*) p;
}

byte *mem::realloc(byte *p, size newsize) {
    p = (byte*) ::realloc(p, newsize);
    if (!p) {
        exceptions::out_of_memory();
    }
    return (byte*) p;
}

void mem::touch(str s) {

    volatile char * volatile ptr = (volatile char * volatile)s.data; // forces a read
    const char *end = s.data + s.len;
    
    while (ptr != end) {
        volatile char x = *ptr;
        (void) x;
        ptr++;
    }
}

void mem::touch(void *p) {
    volatile byte * volatile ptr = (volatile byte * volatile) p;
    volatile byte x = *ptr;
    (void) x;
}