#include "mem.h"

#include <stdlib.h>

#include "exceptions.h"

using namespace lib;

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
