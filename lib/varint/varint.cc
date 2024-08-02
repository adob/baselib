#include "varint.h"

using namespace lib;

deferror varint::ErrOverflow("varint overflow");

//const int MaxVarintLen64 = 10;

uint32 varint::read_uint32(io::IStream &in, error &err) {
    byte b = in.read_byte(err);
    if (b < 0x80) {
        return uint32(b);
    }
    
    uint_fast8_t bitpos = 7;
    uint32 result = b & 0x7F;
    
    do {
        b = in.read_byte(err);
        if (err) {
            if (err == io::EOF) {
                err(io::ErrUnexpectedEOF);
            }
            return 0;
        }
        
        if (bitpos >= 32) {
            /* Note: The varint could have trailing 0x80 bytes, or 0xFF for negative. */
            byte sign_extension = (bitpos < 63) ? 0xFF : 0x01;
            bool valid_extension = ((b & 0x7F) == 0x00 ||
                        ((result >> 31) != 0 && b == sign_extension));

            if (bitpos >= 64 || !valid_extension) {
                err(ErrOverflow);
                return 0;
            }
        } else if (bitpos == 28) {
            if ((b & 0x70) != 0 && (b & 0x78) != 0x78) {
                err(ErrOverflow);
                return 0;
            }
            result |= (uint32_t)(b & 0x0F) << bitpos;
        } else {
            result |= (uint32_t)(b & 0x7F) << bitpos;
        }
        bitpos = (uint_fast8_t)(bitpos + 7);
    } while (b & 0x80);
    
    return result;
}

int32 varint::read_sint32(io::IStream &in, error &err) {
    uint32 value = read_uint32(in, err);
    
    if (value & 1) {
        return ~(value >> 1);
    }
    
    
    return value >> 1;
}

// uint varint::read_uint(io::IStream &in, error &err) {
//     return varint::read_uint32(in, err);
// }

void varint::write_uint32(io::OStream &out, uint32 i, error &err) {
    while (i >= 0x80) {
        out.write(byte(i) | 0x80, err);
        if (err) {
            return;
        }
        i >>= 7;
    }
    
    out.write(byte(i), err);
}

void varint::write_sint32(io::OStream &out, int32 value, error &err) {
    uint32 zigzagged;
    const uint32 mask = -1 >> 1;
    
    if (value < 0) {
        zigzagged = ~((value & mask) << 1);
    } else {
        zigzagged = value << 1;
    }
    
    return write_uint32(out, zigzagged, err);
}
