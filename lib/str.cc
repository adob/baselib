#include "str.h"
#include "lib/utf8/encode.h"
#include "lib/utf8/utf8.h"
#include "lib/io/io_stream.h"

using namespace lib;

struct StringWriter : io::Writer {
    String &s;

    StringWriter(String &s) : s(s) {
        this->readbuf.data = s.buffer.data;
        this->writeptr     = s.buffer.data + s.length;
        this->writeend     = s.buffer.data + s.buffer.len;
    }

    size direct_write(str data, error err) override {
        grow(len(data));
        memmove(writeptr, data.data, usize(data.len));
        this->writeptr += len(data);
        return len(data);
    }

    void grow(size n) {
        size newcap = used() + n;
        size curcap = capacity();
        if (newcap < curcap*2) {
            newcap = curcap*2;
        }
        if (newcap < 32) {
            newcap = 32;
        }

        byte *newdata = (byte*) ::realloc(readbuf.data, newcap);
        if (newdata == nil) {
            exceptions::out_of_memory();
        }

        buf newbuf(newdata, newcap);
        setbuf(newbuf);
    }

    size used() const {
        return writeptr - readbuf.data;
    }

    size capacity() {
        return writeptr - readbuf.data;
    }

    void finish() {
        this->s.buffer.data = this->readbuf.data;
        this->s.buffer.len  = this->capacity();
        this->s.length = this->used();
    }
} ;

String::String(rune r) : buffer(utf8::UTFMax), length(0) {
    this->length = len(utf8::encode(buffer, r));
}

String::String(io::WriterTo const &w) {
    io::Buffer buffer;
    w.write_to(buffer, error::ignore);
    *this = buffer.to_string();
}

String& String::operator+=(io::WriterTo const &writer_to) {
    StringWriter sw(*this);
    writer_to.write_to(sw, error::ignore);
    sw.finish();

    return *this;
}

std::string str::std_string() const {
    return std::string(data, len);
}

std::string String::std_string() const {
    return std::string((char*) buffer.data, length);
}

CString str::c_str() const {
    return CString(*this);
}

// CString::CString(str s) {
//     len = s.len;
//     data = (char *) malloc(len+1);
//     if (data == nil) {
//         exceptions::out_of_memory();
//     }
//     memmove(data, s.data, s.len);
//     data[len] = '\0';
// }

// CString::~CString() {
//     free(data);
// }
