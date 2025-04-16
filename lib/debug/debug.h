#pragma once

#include <exception>
#include <vector>
#include "lib/io/io_stream.h"

namespace lib::debug {
    void init();

    struct Backtrace {
        std::vector<void*> addrs;

        void fmt(io::Writer &out, error err) const;
    } ;

    struct SymInfo {
        String demangled;
        String filename;
        int lineno = 0;
        int colno = 0;
    } ;

    Backtrace backtrace(int offset=0);
    SymInfo get_symbol_info(void *);

    void print_exception(std::exception_ptr excep);
    String format_exception(std::exception_ptr excep);
}
