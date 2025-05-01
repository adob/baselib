#include "../testing/testing.h"
#include "lib/error.h"
#include "lib/io/io.h"

#include "lib/print.h"

using namespace lib;

void test_write_byte(testing::T &t) {
    ErrorRecorder err;
    io::Buffer b;

    b.write('x', err);

    if (b.str() != "x" || err) {
        t.errorf("Buffer::write('x') = %q, err = %q; want \"x\"", b.str(), err);
    }

}

void test_read_byte(testing::T &t) {
    ErrorRecorder err;

    io::Str b("hello");

    char c = b.read_byte(err);
    if (c != 'h' || err)  {
        t.errorf("StrStream::read_byte() = %q, err = %q; want 'h'", c, err);
    }
}

void test_Buffered_read_byte(testing::T &t) {
    struct MyBuffered : io::Buffered {
        str data = "hello";

        io::ReadResult direct_read(buf b, error 
        ) override {
            size nbytes = copy(b, data);

            return io::ReadResult{.nbytes = nbytes, .eof = 0};
        }

        size direct_write(str data, error) override {
            //return copy(this->buffer, data);
            return 0;
        }
    } ;

    MyBuffered mb;
    mb.resize_readbuf(10);
    mb.resize_writebuf(10);

    ErrorRecorder err;
    char c = mb.read_byte(err);
    if (c != 'h' || err)  {
        t.errorf("Buffered::read_byte() = %q, err = %q; want 'h'", c, err);
    }
}