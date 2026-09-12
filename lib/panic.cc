export module lib.panic;
import lib.str;

// Preserve compatibility with declarations in the remaining headers.
export extern "C++" {
#include "panic_impl.h"
namespace lib {
    struct Error;
    namespace io {
        struct WriterTo;
    }

    [[noreturn]] void panic();
    void panic(str msg);
    void panic(io::WriterTo const&);
    void panic(Error const& e);
}
}
