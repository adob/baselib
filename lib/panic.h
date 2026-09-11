#pragma once
import lib.str;
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
