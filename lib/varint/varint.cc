#include "varint.h"
#include "lib/error.h"
#include "lib/io/io.h"
#include "lib/io/util.h"

using namespace lib;

uint32 varint::read_uint32(io::Reader &in, error  err) {
    byte b = in.read_byte(err);
    if (b < 0x80) {
        return uint32(b);
    }
    
    uint_fast8_t bitpos = 7;
    uint32 result = b & 0x7F;
    
    do {
        
        b = in.read_byte(err);
        if (err) {
            return 0;
        }
        
        if (bitpos >= 32) {
            // Note: The varint could have trailing 0x80 bytes, or 0xFF for negative.
            byte sign_extension = (bitpos < 63) ? 0xFF : 0x01;
            bool valid_extension = ((b & 0x7F) == 0x00 ||
                        ((result >> 31) != 0 && b == sign_extension));

            if (bitpos >= 64 || !valid_extension) {
                err(ErrOverflow());
                return 0;
            }
        } else if (bitpos == 28) {
            if ((b & 0x70) != 0 && (b & 0x78) != 0x78) {
                err(ErrOverflow());
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

uint64 varint::read_uint64(io::Reader &in, error err) {
    byte b;
    uint_fast8_t bitpos = 0;
    uint64_t result = 0;
    
    do {
        b = in.read_byte(err);
        if (err) {
            return 0;
        }

        if (bitpos >= 63 && (b & 0xFE) != 0) {
            err(ErrOverflow());
        }

        result |= (uint64_t)(b & 0x7F) << bitpos;
        bitpos = (uint_fast8_t)(bitpos + 7);
    } while (b & 0x80);
    
    return result;
}

void varint::skip(str *data, error err) {
    size n = len(*data);
    size i = 0;
    byte b;
    do {
        if (i >= n) {
            err(io::ErrUnexpectedEOF());
        }
        b = (*data)[i];
        i++;
    } while (b & 0x80);

    *data = (*data)+i;
}

void varint::skip(io::Reader &in, error err) {
    byte b;
    do {
        b = in.read_byte(err);
        if (err) {
            return;
        }
    } while (b & 0x80);
}

int32 varint::read_sint32(io::Reader &in, error err) {
    uint32 value = read_uint32(in, err);
    
    if (value & 1) {
        return ~(value >> 1);
    }
    
    return value >> 1;
}

int64 varint::read_sint64(io::Reader &in, error err) {
    uint64 value = read_uint64(in, err);
    
    if (value & 1) {
        return ~(value >> 1);
    }
    
    return value >> 1;
}

void varint::write_uint32(io::Writer &out, uint32 i, error err) {
    while (i >= 0x80) {
        out.write_byte(byte(i) | 0x80, err);
        if (err) {
            return;
        }
        i >>= 7;
    }
    
    out.write_byte(byte(i), err);
}

void varint::write_uint64(io::Writer &out, uint64 i, error err) {
    while (i >= 0x80) {
        out.write_byte(byte(i) | 0x80, err);
        if (err) {
            return;
        }
        i >>= 7;
    }
    
    out.write_byte(byte(i), err);
}

void varint::write_sint32(io::Writer &out, int32 value, error err) {
    uint32 zigzagged;
    const uint32 mask = -1 >> 1;
    
    if (value < 0) {
        zigzagged = ~((uint32(value) & mask) << 1);
    } else {
        zigzagged = uint32(value) << 1;
    }
    
    return write_uint32(out, zigzagged, err);
}

void varint::write_sint64(io::Writer &out, int64 value, error err) {
    uint64 zigzagged;
    const uint64 mask = -1 >> 1;
    
    if (value < 0) {
        zigzagged = ~((uint64(value) & mask) << 1);
    } else {
        zigzagged = uint64(value) << 1;
    }
    
    return write_uint64(out, zigzagged, err);
}