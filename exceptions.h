#pragma once

#include <string>

#include "./str.h"
#include "./types.h"
#include "./exception.h"

#ifdef __cpp_exceptions
#include "backward.hpp"
namespace backward {
    class StackTrace;
}
#endif

namespace lib {
    struct str;

#ifdef __cpp_exceptions
    struct Exception {
        String                msg;
        backward::StackTrace *stacktrace = nil;

        Exception();
        ~Exception();
    };
#endif
}

namespace lib::exceptions {
    
    void out_of_memory();
    void bad_index(size got);
    void bad_index(size got, size max);
    void assertion();
    void overflow();

#ifdef __cpp_exceptions
    struct BadIndex : Exception {
        BadIndex();
        BadIndex(size got);
        BadIndex(size got, size max);
    };

    struct BadMemAccess : Exception {
        BadMemAccess(void *ptr);
    };

    struct NullMemAccess : Exception {
        NullMemAccess();
    };

    struct AssertionFailed : Exception {
        AssertionFailed(str);
    };

    struct OutOfMem : Exception {

    };

    struct Panic : Exception {
        Panic() {}
        Panic(str);
    };
#endif
}
