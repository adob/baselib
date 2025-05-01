#include "util.h"

#include "lib/error.h"
#include "lib/io/io.h"

using namespace lib;
using namespace lib::io;

#pragma GCC diagnostic ignored "-Wshadow"

size io::read_full(io::Writer &input, buf buffer, error err) {
    return read_at_least(input, buffer, len(buffer), err);
}

size io::read_at_least(io::Writer &in, buf buffer, size min, error err) {
    if (len(buffer) < min) {
        err(ErrShortBuffer());
        return 0;
    }
    
    size n = 0;
    while (n < min) {
        ReadResult r = in.read(buffer + n, err);
        n += r.nbytes;
        if (err) {
            return n;
        }

        if (r.eof) {
            break;
        }
    }
    
    if (n < min) {
        err(io::ErrUnexpectedEOF());
    }
    
    return n;
}
