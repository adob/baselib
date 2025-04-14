#pragma once

#include "lib/base.h"
#include "io_stream.h"

namespace lib::io {
    using namespace lib;

    //struct EOF : ErrorBase<EOF, "EOF"> {};
    struct ErrUnexpectedEOF : ErrorBase<ErrUnexpectedEOF, "unexpected EOF"> {};
    struct ErrShortWrite    : ErrorBase<ErrShortWrite, "short write"> {};
    struct ErrShortBuffer   : ErrorBase<ErrShortBuffer, "short buffer"> {};
    struct ErrIO            : ErrorBase<ErrIO, "IO error"> {};
    
    size read_full(io::Writer &in, buf buffer, error err);
    
    size read_at_least(io::Writer &in, buf buffer, size min, error err);
}
