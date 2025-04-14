#pragma once

#include <string>

#include "./str.h"
#include "./types.h"

#ifdef __cpp_exceptions
// #include "../deps/backward-cpp/backward.hpp"
// namespace backward {
//     class StackTrace;
// }
#endif

namespace lib::io {
    struct Reader;
}


namespace lib {
    struct str;
    struct error;

#ifdef __cpp_exceptions
    struct Exception {
        //backward::StackTrace *stacktrace = nil;
        
        virtual void fmt(io::Reader &out, error err) const = 0;

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
        size got;
        size max;

        //BadIndex();
        //BadIndex(size got);
        BadIndex(size got, size max);

        void fmt(io::Reader &out, error err) const override;
    };

    struct BadMemAccess : Exception {
        void *ptr;

        BadMemAccess(void *ptr);

        void fmt(io::Reader &out, error err) const override;
    };

    struct NullMemAccess : Exception {
        NullMemAccess();

        void fmt(io::Reader &out, error err) const override;
    };

    struct AssertionFailed : Exception {
        String msg;
        AssertionFailed(str);

        void fmt(io::Reader &out, error err) const override;
    };

    struct OutOfMem : Exception {
        void fmt(io::Reader &out, error err) const override;
    };

    struct Panic : Exception {
        String msg;
        Panic() {}
        Panic(str);

        void fmt(io::Reader &out, error err) const override;
    };
#endif
}
