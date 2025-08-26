#include "../testing/testing.h"
#include "lib/error.h"
#include "lib/io/io.h"

#include "lib/print.h"

using namespace lib;

void test_write_byte(testing::T &t) {
    ErrorRecorder err;
    io::Buffer b;

    b.write_byte('x', err);

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

        io::ReadResult direct_read(buf b, error ) override {
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

void test_static_bufferd(testing::T &t) {
    String out;

    struct Writer : io::StaticBuffered<0, 10> {
        String &out;
        Writer(String &out) : out(out) {}

        size direct_write(str data, error) override {
            // print "direct_write %v %q" % len(data), data;
            out += data;
            return len(data);
        }

        io::ReadResult direct_read(buf, error) override {
            panic("unimplemented");
            return {};
        }
    } writer(out);

    writer.write_byte('a', error::panic);
    writer.write_byte('b', error::panic);
    writer.write("0123456789", error::panic);
    writer.flush(error::panic);

    str expected = "ab0123456789";
    if (out != expected) {
        t.errorf("out got %q; wanted %q", out, expected);
    }
}