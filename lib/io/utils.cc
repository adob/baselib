#include "utils.h"

#include "lib/error.h"
#include "lib/io/io_stream.h"
#include "lib/print.h"

using namespace lib;
using namespace io;

#pragma GCC diagnostic ignored "-Wshadow"

// namespace lib::io {
//     Error EOF("EOF");
//     Error ErrUnexpectedEOF("unexpected EOF");
//     Error ErrShortWrite("short write");
//     Error ErrShortBuffer("short buffer");
    
//     Error ErrIO("IO error");
// }

size io::read_full(io::IStream &input, buf buffer, error err) {
    return read_at_least(input, buffer, len(buffer), err);
}

size io::read_at_least(io::IStream &in, buf buffer, size min, error err) {
    if (len(buffer) < min) {
        err(ErrShortBuffer());
        return 0;
    }
    
    size n = 0;
    while (n < min) {
        //in.read(buf(), ErrorReporter([](const Error &) {}));
        //in.read(buf(), ErrorReporter([](const Error &) {});
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
