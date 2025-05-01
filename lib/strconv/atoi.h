#pragma once

#include "lib/error.h"
#include "lib/io/io.h"
#include "lib/str.h"

namespace lib::strconv {

    // ErrRange indicates that a value is out of range for the target type.
    struct ErrRange : ErrorBase<ErrRange, "value out of range"> {};

    // ErrSyntax indicates that a value does not have the right syntax for the target type.
    struct ErrSyntax : ErrorBase<ErrSyntax, "invalid syntax"> {};

    // IntSize is the size in bits of an int or uint value.
    const int IntSize = sizeof(int) * 8;

    // A NumError records a failed conversion.
    struct NumError : ErrorBase<NumError> {
        str func;               // the failing function (ParseBool, ParseInt, ParseUint, ParseFloat, ParseComplex)
        str num;                // the input
        Error *err = nil;       // the reason the conversion failed (e.g. ErrRange, ErrSyntax, etc.)

        NumError(str func, str num, Error &err) : func(func), num(num), err(&err) {}
        NumError(str func, str num, Error &&err) : func(func), num(num), err(&err) {}
        virtual void fmt(io::Writer &out, error err) const override;
        virtual view<Error *> unwrap() const override;
    } ;


    // Atoi is equivalent to ParseInt(s, 10, 0), converted to type int.
    int atoi(str s, error);

    // ParseInt interprets a string s in the given base (0, 2 to 36) and
    // bit size (0 to 64) and returns the corresponding value i.
    //
    // The string may begin with a leading sign: "+" or "-".
    //
    // If the base argument is 0, the true base is implied by the string's
    // prefix following the sign (if present): 2 for "0b", 8 for "0" or "0o",
    // 16 for "0x", and 10 otherwise. Also, for argument base 0 only,
    // underscore characters are permitted as defined by the Go syntax for
    // [integer literals].
    //
    // The bitSize argument specifies the integer type
    // that the result must fit into. Bit sizes 0, 8, 16, 32, and 64
    // correspond to int, int8, int16, int32, and int64.
    // If bitSize is below 0 or above 64, an error is returned.
    //
    // The errors that ParseInt returns have concrete type [*NumError]
    // and include err.Num = s. If s is empty or contains invalid
    // digits, err.Err = [ErrSyntax] and the returned value is 0;
    // if the value corresponding to s cannot be represented by a
    // signed integer of the given size, err.Err = [ErrRange] and the
    // returned value is the maximum magnitude integer of the
    // appropriate bitSize and sign.
    //
    // [integer literals]: https://go.dev/ref/spec#Integer_literals
    int64 parse_int(str s, int base, int bit_size, error err);

    // ParseUint is like [ParseInt] but for unsigned numbers.
    //
    // A sign prefix is not permitted.
    uint64 parse_uint(str s, int base, int bit_size, error err);
}
