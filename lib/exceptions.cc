#include "./exceptions.h"
#include "./str.h"
#include "./panic.h"
#include "lib/io/io.h"
#include "fmt/fmt.h"
//#include "fmt/fmt.h"

using namespace lib;
using namespace exceptions;

void exceptions::out_of_memory() {
    panic("out of memory");
}

void exceptions::bad_index(size got, size max) {
    #if __cpp_exceptions
        throw BadIndex(got, max);
    #else
        panic("bad index");
    #endif
}

void exceptions::assertion() {
    panic("assertion");
}

void exceptions::overflow() {
    panic("overflow");
}


#ifdef __cpp_exceptions
lib::Exception::Exception()
    /* : stacktrace(new backward::StackTrace())*/ {
    
    // stacktrace->load_here(1024);
    // stacktrace->skip_n_firsts(2);
}

lib::Exception::~Exception() {
    // if (stacktrace) {
    //     delete stacktrace;
    //     stacktrace = nil;
    // }
}


BadMemAccess::BadMemAccess(void *ptr) : ptr(ptr) {}

void BadMemAccess::fmt(io::Reader &out, error err) const {
    fmt::fprintf(out, err, "attempt to dereference invalid pointer at %#X", (uintptr) ptr);
}

NullMemAccess::NullMemAccess() {}

void NullMemAccess::fmt(io::Reader &out, error err) const {
    fmt::fprintf(out, err, "null dereference");
}

BadIndex::BadIndex(size got, size max) : got(got), max(max) {}

void BadIndex::fmt(io::Reader &out, error err) const {
    fmt::fprintf(out, err, "index out range [0:%d]: %d", max, got);
}


AssertionFailed::AssertionFailed(str s) : msg(s) {}
void AssertionFailed::fmt(io::Reader &out, error err) const {
    out.write(msg, err);
}

void OutOfMem::fmt(io::Reader &out, error err) const {
    fmt::fprintf(out, err, "out of memory");
}

Panic::Panic(str s) : msg(s) {}
void Panic::fmt(io::Reader &out, error err) const {
    fmt::fprintf(out, err, "panic: %s", msg);
}


#endif