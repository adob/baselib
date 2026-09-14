module;
#include "util_impl.h"

export module lib.io:util;
import :core;
import lib.error;
import lib.str;
import lib.types;


export extern "C++"
namespace lib::io {
    using namespace lib;

    //struct EOF : ErrorBase<EOF, "EOF"> {};
    struct ErrUnexpectedEOF : ErrorBase<ErrUnexpectedEOF, "unexpected EOF"> {};
    struct ErrShortWrite    : ErrorBase<ErrShortWrite, "short write"> {};
    struct ErrShortBuffer   : ErrorBase<ErrShortBuffer, "short buffer"> {};
    struct ErrIO            : ErrorBase<ErrIO, "IO error"> {};
    
    size read_full(Reader &in, buf buffer, error err);
    size discard(Reader &in, size nbytes, error err);
    
    size read_at_least(Reader &in, buf buffer, size min, error err);
}
