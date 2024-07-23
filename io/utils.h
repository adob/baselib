#pragma once

#include "lib/base.h"
#include "io_stream.h"

namespace lib::io {
    using namespace lib;
    
    extern deferror EOF;
    extern deferror ErrUnexpectedEOF;
    extern deferror ErrShortWrite;
    extern deferror ErrShortBuffer;
    
    extern deferror ErrIO;
    
    size read_full(io::IStream &in, buf buffer, error &err);
    
    size read_at_least(io::IStream &in, buf buffer, size min, error &err);
}
