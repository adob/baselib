#pragma once

#include "str.h"
#include "error.h"

namespace lib {
    namespace io {
        struct WriterTo;
    }
    
    [[noreturn]] void panic();
    void panic(str msg);
    void panic(io::WriterTo const&);
    void panic(Error const& e);
}
