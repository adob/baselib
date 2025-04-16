#pragma once

#include <stdlib.h>
#include "types.h"
#include "buf.h"

namespace lib::exceptions {
    void out_of_memory();
}

namespace lib {
    struct str;
}

namespace lib::mem {
    byte* alloc(size);
    byte* realloc(byte *data, size newsize);

    // touch touches memory for TSan (Thread Sanitizer) purposes
    void touch(str s);
    void touch(void *p);
}
