// #ifdef __cpp_exceptions
// #define BACKWARD_HAS_UNWIND 1
// #define BACKWARD_HAS_DW 1

// #define BACKWARD_HAS_BFD 0
// #define BACKWARD_HAS_DWARF 0
// #define BACKWARD_HAS_BACKTRACE 0
// #define BACKWARD_HAS_BACKTRACE_SYMBOL 0
// #define BACKWARD_HAS_LIBUNWIND 0
// #include "../deps/backward-cpp/backward.hpp"
// #endif

#include "panic.h"
#include "lib/io/io.h"
#include "lib/str.h"

#include "fmt.h"


#include "lib/panic.h"
#include "exceptions.h"

#include <stdio.h>

using namespace lib;

void lib::panic() {
    #ifdef __cpp_exceptions
        fmt::fprintf(stderr, "call to panic\n");
        throw exceptions::Panic();
    #else
        printf("PANIC\n");
        abort();
    #endif
}

void lib::panic(Error const& e) {
    io::Buffer b;
    e.fmt(b, error::ignore);

#ifdef __cpp_exceptions
    exceptions::Panic ex(b.to_string());
    
    throw ex;
#else
    panic(b.str());
#endif
}

void lib::panic(str msg) {
    (void) msg;

#ifdef __cpp_exceptions
    exceptions::Panic ex(msg);
    //ex.stacktrace->skip_n_firsts(1);

    throw ex;
#else
    fmt::fprintf(stderr, "call to panic with msg %q\n", msg);
    abort();
#endif
}


void lib::panic(io::WriterTo const& writable) {
    io::Buffer b;
    writable.write_to(b, error::ignore);

    panic(b.str());
}