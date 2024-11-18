#pragma once

#include <vector>
#include "../base.h"

namespace lib::debug {
    void init();

    struct Backtrace {
        std::vector<void*> addrs;
    } ;

    struct SymInfo {
        String demangled;
        String filename;
        int lineno = 0;
        int colno = 0;
    } ;

    Backtrace backtrace(int offset=0);
    SymInfo get_symbol_info(void *);
}
