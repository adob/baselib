module;
#ifdef ARDUINO
#include "stdio_impl+arduino.h"
#else
#include "stdio_impl+posix.h"
#endif

export module lib.io:stdio;
import lib.str;
import lib.error;
import lib.types;
import :core;
import <stdio.h>;



// Disabled: named-module imports do not propagate these macro changes to consumers.
/*
#define SECOND(a, b, ...) b

#define IS_PROBE(...) SECOND(__VA_ARGS__, 0)
#define PROBE() ~, 1

#define CAT(a,b) a ## b
#define CAT2(a, b) CAT(a, b)

#define NOT(x) IS_PROBE(CAT(_NOT_, x))
#define _NOT_0 PROBE()

//#define BOOL(x) NOT(NOT(x))

#define X(...) 0
#define CHECK1(arg1, arg2) CHECK2(arg1, arg2)
#define CHECK2(arg1, arg2) arg1 arg2

#define TEST_stdout 1
#define TEST_stderr 1
#define TEST_stdin 1

#define IS_PARENS(arg) NOT(CHECK1(X, arg))
#define IS_SELF(arg) CAT2(TEST_, arg)

#define NS_DECL(ARG) namespace { \
    inline FILE *builtin_##ARG() { \
        return ARG; \
    } \
} \

#define STRUCT_DECL(ARG) struct { \
    operator FILE*() const { \
        return builtin_##ARG(); \
    } \
} inline ARG __attribute__((unused));

#if !IS_PARENS(stdout)
    #if !IS_SELF(stdout)
        NS_DECL(stdout)
        #undef stdout
        STRUCT_DECL(stdout)
    #endif
#else
    NS_DECL(stdout)
    #undef stdout
    STRUCT_DECL(stdout)
#endif

#if !IS_PARENS(stderr)
    #if !IS_SELF(stderr)
        NS_DECL(stderr)
        #undef stderr
        STRUCT_DECL(stderr)
    #endif
#else
    NS_DECL(stderr)
    #undef stderr
    STRUCT_DECL(stderr)
#endif

#if !IS_PARENS(stdin)
    #if !IS_SELF(stdin)
        NS_DECL(stderr)
        #undef stdin
        STRUCT_DECL(stdin)
    #endif
#else
    NS_DECL(stdin)
    #undef stdin
    STRUCT_DECL(stdin)
#endif

#undef SECOND
#undef IS_PROBE
#undef PROBE
#undef CAT
#undef CAT2
#undef NOT
#undef _NOT_0
#undef X
#undef CHECK1
#undef CHECK2
//#undef BOOL
#undef TEST_stdout
#undef TEST_stderr
#undef IS_PARENS
#undef IS_SELF
#undef NS_DECL
#undef STRUCT_DECL

*/

export extern "C++"
namespace lib::io {
    struct StdStream : io::ReaderWriter {
        FILE *file;
        int fd;
  
        constexpr StdStream(FILE *file, int fd) : file(file), fd(fd) {}
  
        io::ReadResult direct_read(buf bytes, error err) override;
        size           direct_write(str data, error err) override;
    };

    extern StdStream out;
    extern StdStream err;
    extern StdStream in;
}
