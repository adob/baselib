#pragma once

#include <stdlib.h>
#include "types.h"
#include "buf.h"

namespace lib::exceptions {
    void out_of_memory();
}

namespace lib::mem {
    byte* alloc(size);
    byte* realloc(byte *data, size newsize);
}
