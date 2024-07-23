#pragma once

#include "str.h"
#include "error.h"

namespace lib {
    namespace io {
        struct WriterTo;
    }
    
    void panic();
    void panic(str msg);
    void panic(io::WriterTo const&);
    void panic(error e);
    void panic(deferror const&);
}
