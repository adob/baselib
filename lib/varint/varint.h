#pragma once

#include "lib/io.h"
#include "lib/base.h"

namespace lib::varint {
    extern Error ErrOverflow;
    
    uint32 read_uint32(io::IStream &in, error2 &err);
    //uint read_uint(io::IStream &in, error &err);
    
    int32 read_sint32(io::IStream &in, error2 &err);
    
    void write_uint32(io::OStream &out, uint32 i, error2 &err);
    void write_sint32(io::OStream &out, int32 i, error2 &err);
}
