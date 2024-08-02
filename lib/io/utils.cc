#include "utils.h"

#include "lib/print.h"

using namespace lib;
using namespace io;

#pragma GCC diagnostic ignored "-Wshadow"

namespace lib::io {
    deferror EOF("EOF");
    deferror ErrUnexpectedEOF("unexpected EOF");
    deferror ErrShortWrite("short write");
    deferror ErrShortBuffer("short buffer");
    
    deferror ErrIO("IO error");
}

size io::read_full(io::IStream &input, buf buffer, error &err) {
    return read_at_least(input, buffer, len(buffer), err);
}

size io::read_at_least(io::IStream &in, buf buffer, size min, error &err) {
    if (len(buffer) < min) {
        err(ErrShortBuffer);
        return 0;
    }
    
    size n = 0;
    while (n < min) {
        n += in.read(buffer.slice(n), err);
        if (err) {
            break;
        }
    }
    
    if (n < min && err == io::EOF) {
        err(io::ErrUnexpectedEOF);
    }
    
    return n;
}
