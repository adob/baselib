#pragma once

#include "lib/io.h"
#include "lib/base.h"

namespace lib::varint {
    struct ErrOverflow : ErrorBase<ErrOverflow, "varint overflow"> {};
    
    uint32 read_uint32(io::Writer &in, error err);
    //uint read_uint(io::IStream &in, error &err);
    
    int32 read_sint32(io::Writer &in, error err);
    
    void write_uint32(io::Reader &out, uint32 i, error err);
    void write_sint32(io::Reader &out, int32 i, error err);
}
