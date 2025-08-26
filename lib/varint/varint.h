#pragma once

#include "lib/io/io.h"
#include <type_traits>

namespace lib::varint {
    struct ErrOverflow : ErrorBase<ErrOverflow, "varint overflow"> {};
    
    uint32 read_uint32(io::Reader &in, error err);
    // int decode_uint32(str data, uint32 *out, error err);

    uint64 read_uint64(io::Reader &in, error err);
    // int decode_uint64(str data, uint64 *out, error err);

    void skip(io::Reader &in, error err);
    void skip(str *data, error err);
    
    int32 read_sint32(io::Reader &in, error err);
    int64 read_sint64(io::Reader &in, error err);
    // int decode_sint32(str data, int32 *out, error err);
    
    void write_uint32(io::Writer &out, uint32 i, error err);
    void write_sint32(io::Writer &out, int32 i, error err);
    void write_uint64(io::Writer &out, uint64 i, error err);
    void write_sint64(io::Writer &out, int64 i, error err);

    // template <typename T>
    // T read(io::Reader &in, error err) {
    //     if constexpr (sizeof(T) == 4) {
    //         if constexpr (std::is_signed_v<T>) {
    //             return read_sint32(in, err);
    //         } else {
    //             return read_uint32(in, err);
    //         }
    //     } else if constexpr (sizeof(T) == 8) {
    //         if constexpr (std::is_signed_v<T>) {
    //             return read_sint64(in, err);
    //         } else {
    //             return read_uint64(in, err);
    //         }
    //     } else {
    //         static_assert(false, "bad type");
    //     }
    //     return 0;
    // }

    template <typename T>
    T read_unsigned(io::Reader &in, error err) {
        if constexpr (sizeof(T) == 4) {
            return read_uint32(in, err);
        } else if constexpr (sizeof(T) == 8) {
            return read_uint64(in, err);
        } else {
            static_assert(false, "bad type");
        }
        return 0;
    }

    template <typename T>
    T read_signed(io::Reader &in, error err) {
        if constexpr (sizeof(T) == 4) {
            return read_sint32(in, err);
        } else if constexpr (sizeof(T) == 8) {
            return read_sint64(in, err);
        } else {
            static_assert(false, "bad type");
        }
        return 0;
    }

    // template <typename T>
    // void write(io::Writer &out, T val, error err) {
    //     if constexpr (sizeof(T) == 4) {
    //         if constexpr (std::is_signed_v<T>) {
    //             write_sint32(out, val, err);
    //         } else {
    //             write_uint32(out, val, err);
    //         }
    //     } else if constexpr (sizeof(T) == 8) {
    //         if constexpr (std::is_signed_v<T>) {
    //             write_sint64(out, val, err);
    //         } else {
    //             write_uint64(out, val, err);
    //         }
    //     } else {
    //         static_assert(false, "bad type");
    //     }
    // }

    template <typename T>
    void write_unsigned(io::Writer &out, T val, error err) {
        if constexpr (sizeof(T) == 4) {
            write_uint32(out, val, err);
        } else if constexpr (sizeof(T) == 8) {
            write_uint64(out, val, err);
        } else {
            static_assert(false, "bad type");
        }
    }

    template <typename T>
    void write_signed(io::Writer &out, T val, error err) {
        if constexpr (sizeof(T) == 4) {
            write_sint32(out, val, err);
        } else if constexpr (sizeof(T) == 8) {
            write_sint64(out, val, err);
        } else {
            static_assert(false, "bad type");
        }
    }
}
