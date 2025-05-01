#pragma once

#include "lib/types.h"
#include "lib/io/io.h"

namespace lib::strconv {
    enum Flag {
        FlagPlus  = 1,
        FlagSharp = 1 << 1,
        FlagSpace = 1 << 2,
        FlagMinus = 1 << 3,
        FlagZero  = 1 << 4,
    } ;

    struct Float64Formatter : io::WriterTo {
        float64 f;
        char fmt;
        int prec;
        int flags;
        int width;

        Float64Formatter(float64 f, char fmt, int prec, int flags, int width) : f(f), fmt(fmt), prec(prec), flags(flags), width(width) {}
        void write_to(io::Writer &out, error err) const override;
    } ;

    struct Float32Formatter : io::WriterTo {
        float32 f;
        char fmt;
        int prec;
        int flags;
        int width;

        Float32Formatter(float32 f, char fmt, int prec, int flags, int width) : f(f), fmt(fmt), prec(prec), flags(flags), width(width) {}
        void write_to(io::Writer &out, error err) const override;
    } ;

    // format_float converts the floating-point number f to a string,
    // according to the format fmt and precision prec. It rounds the
    // result assuming that the original was obtained from a floating-point
    // value of bitSize bits (32 for float32, 64 for float64).
    //
    // The format fmt is one of
    //   - 'b' (-ddddp±ddd, a binary exponent),
    //   - 'e' (-d.dddde±dd, a decimal exponent),
    //   - 'E' (-d.ddddE±dd, a decimal exponent),
    //   - 'f' (-ddd.dddd, no exponent),
    //   - 'g' ('e' for large exponents, 'f' otherwise),
    //   - 'G' ('E' for large exponents, 'f' otherwise),
    //   - 'x' (-0xd.ddddp±ddd, a hexadecimal fraction and binary exponent), or
    //   - 'X' (-0Xd.ddddP±ddd, a hexadecimal fraction and binary exponent).
    //
    // The precision prec controls the number of digits (excluding the exponent)
    // printed by the 'e', 'E', 'f', 'g', 'G', 'x', and 'X' formats.
    // For 'e', 'E', 'f', 'x', and 'X', it is the number of digits after the decimal point.
    // For 'g' and 'G' it is the maximum number of significant digits (trailing
    // zeros are removed).
    // The special precision -1 uses the smallest number of digits
    // necessary such that ParseFloat will return f exactly.
    // The exponent is written as a decimal integer;
    // for all formats other than 'b', it will be at least two digits.
    Float64Formatter format_float(float64 f, char fmt, int prec, int flags = 0, int width = 0);
    Float32Formatter format_float(float32 f, char fmt, int prec, int flags = 0, int width = 0);
}