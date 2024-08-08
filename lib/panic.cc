#include "panic.h"

#ifdef __cpp_exceptions
#define BACKWARD_HAS_UNWIND 1
#define BACKWARD_HAS_DW 1

#define BACKWARD_HAS_BFD 0
#define BACKWARD_HAS_DWARF 0
#define BACKWARD_HAS_BACKTRACE 0
#define BACKWARD_HAS_BACKTRACE_SYMBOL 0
#define BACKWARD_HAS_LIBUNWIND 0
#include "../deps/backward-cpp/backward.hpp"
#endif
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

void lib::panic(error err) {
    auto s = fmt::stringifier(err);
    panic(s);
}

void lib::panic(deferror const& e) {
    panic(e.msg);
}

void lib::panic(str msg) {
    (void) msg;
#ifdef __cpp_exceptions
    fmt::fprintf(stderr, "call to panic with msg %q\n", msg);
    exceptions::Panic ex(msg);

    if (ex.stacktrace) {
        backward::Printer p;
        p.print(*ex.stacktrace, stderr);
    }
    throw ex;
#else
    printf("PANIC\n");
    abort();
#endif
}


void lib::panic(io::WriterTo const& writable) {
    io::Buffer buffer;
    writable.write_to(buffer, error::ignore());

    String s = buffer.to_string();
    panic(s);
}