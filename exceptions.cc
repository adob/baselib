#include "./exceptions.h"
#include "./str.h"
#include "./panic.h"
//#include "fmt/fmt.h"

using namespace lib;

void exceptions::out_of_memory() {
    panic("out of memory");
}

void exceptions::bad_index(size got) {
    (void) got;
    panic("bad index");
}

void exceptions::bad_index(size got, size max) {
    (void) got;
    (void) max;
    panic("bad_index");
}

void exceptions::assertion() {
    panic("assertion");
}

void exceptions::overflow() {
    panic("overflow");
}


#ifdef __cpp_exceptions
lib::Exception::Exception() : 
    stacktrace(new backward::StackTrace()) {
    
    stacktrace->load_here(1024);
    stacktrace->skip_n_firsts(2);
}

lib::Exception::~Exception() {
    if (stacktrace) {
        delete stacktrace;
        stacktrace = nil;
    }
}


exceptions::BadMemAccess::BadMemAccess(void *ptr) {
    (void) ptr;
//     msg = fmt::sprintf("Attempt to dereference invalid pointer at %#.12x", 
//                         (uintptr) ptr);
}

exceptions::NullMemAccess::NullMemAccess() {
//     msg = "Null dereference";
}

// exceptions::Error::Error(error e) {
//     (void) e;
// //     msg = fmt::sprintf("Error: %s", e);
// }

exceptions::BadIndex::BadIndex() {
//     msg = fmt::sprintf("Bad index");
}

exceptions::BadIndex::BadIndex(size got) {
    (void) got;
//     msg = fmt::sprintf("Bad index: %d", got);
}

exceptions::BadIndex::BadIndex(size got, size max) {
    (void) got;
    (void) max;
//     msg = fmt::sprintf("Index out range [0:%d]: %d", max, got);
}

exceptions::AssertionFailed::AssertionFailed(str s) {
    (void) s;
//     msg = s;
}

exceptions::Panic::Panic(str s) {
    this->msg = s;
    //print "Panic()", msg.length;
}

#endif