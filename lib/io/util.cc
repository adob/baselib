#include "util.h"

#include "lib/error.h"
#include "lib/io/io.h"
#include "lib/math/math.h"

using namespace lib;
using namespace lib::io;

#pragma GCC diagnostic ignored "-Wshadow"

size io::read_full(Reader &input, buf buffer, error err) {
    return read_at_least(input, buffer, len(buffer), err);
}

size io::read_at_least(Reader &in, buf buffer, size min, error err) {
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

size io::discard(Reader &in, size nbytes, error err) {
    Array<byte, 512> buffer;
    size amt = math::min(nbytes, len(buffer));
    size bytes_read = 0;

    while (amt > 0) {
        bytes_read += io::read_full(in, buffer[0, amt], err);
        if (err) {
            return bytes_read;
        }

        amt -= bytes_read;
    }

    return bytes_read;
}