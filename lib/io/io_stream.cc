#include <string.h>
#include <stdio.h>

#include "io_stream.h"
#include "utils.h"

#include "lib/base.h"
#include "lib/exceptions.h"
#include "lib/print.h"


using namespace lib;
using namespace io;

// static deferror ErrNegativeRead("io: read returned negative count");

StdStream io::out(stdout);
StdStream io::err(stderr);

#pragma GCC diagnostic ignored "-Wshadow"

io::Stream::Stream(io::Stream &&other) {
    // printf("<MC>");
    (*this) = std::move(other);
}

io::Stream& io::Stream::operator=(Stream&& other) {
    // printf("<MA>");
    readbuf = other.readbuf;
    readptr = other.readptr;
    readend = other.readend;
    writebuf = other.writebuf;
    writeptr = other.writeptr;
    writeend = other.writeend;

    other.readbuf = {};
    other.readptr = nil;
    other.readend = nil;
    other.writebuf = nil;
    other.writeptr = nil;
    other.writeend = nil;

    return *this;
}

byte io::Stream::read_byte(error &err) {
    if (readptr == readend) [[unlikely]] {
        if (!readbuf) {
            // if (readptr) {
            //    err(EOF);
            //    return 0;
            // }

            // no read buffer
            byte b = 0;
            direct_read(buf(&b, 1), err);
            return b;
        }

        size n = direct_read(readbuf, err);
        readptr = readbuf.data;
        readend = readptr + n;
        if (n == 0) {
            return 0;
        }
    }

    return *(readptr++);
}

size io::Stream::read(buf p, error &err) {
    if (readptr == readend) [[unlikely]] {
        // buffer is empty

        if (len(p) >= len(readbuf)) {
            //if (!readbuf && readptr) {
            //   err(EOF);
            //    return 0;
            // }

            // large read goes directly into p
            return direct_read(p, err);
        }

        // one read
        size n = direct_read(readbuf, err);
        
        assert(usize(n) <= usize(len(readbuf)), exceptions::assertion);
        readptr = readbuf.data;
        readend = readptr + n;
        if (n == 0) {
            return 0;
        }
    }

    size amt = std::min(p.len, readend - readptr);
    memmove(p.data, readptr, size_t(amt));
    readptr += amt;
    return amt;
}


size io::Stream::write(str p, error &err) {
    // printf("<W>");
    buf avail(writeptr, writeend - writeptr);

    // ::printf("Stream::WRITE1 %s %d\n", p.c_str().data, len(avail));

    if (len(p) > len(avail)) {
        // p is larger than available buffer space
        size n1 = 0;
        if (writebuf && writeptr > writebuf) {
            // buffer has data
            // write into buffer and flush buffer
            n1 = copy(avail, p);
            flush(err);
            if (err) {
                return n1;
            }

            p = p.slice(n1);

            avail = buf(writeptr, writeend - writeptr);
            if (len(p) <= len(avail)) {
                // remaining p can fit into buffer
                size n2 =  copy(avail, p);
                writeptr += n2;
                return n1 + n2;
            }

            // else write directly
        }

        // buffer is empty and data too large to fit into buffer
        size n2 = direct_write(p, err);
        if (n2 < len(p)) {
            err(io::ErrShortWrite);
        }
        // ::printf("Stream::WRITE2 now=<%s>\n", str(readbuf.data, writeptr - readptr).c_str().data);
        return n1 + n2;
    }

    // write into buffer
    size n = copy(avail, p);
    writeptr += n;
//     print "write of len", len(p), writeptr - writebuf;

    // ::printf("Stream::CURRENT %s\n", str(this->writebuf, 100).c_str().data);
    // ::printf("Stream::WRITE3 size=%d\n", int(writeend - writeptr));
    return n;
}

size io::Stream::write(byte b, error &err) {
    // printf("<1-%d/%lx/%lx>", writeptr == writeend, this, writeptr);
    if (writeptr == writeend) {
        if (!writeptr) {
            // no buffer
            return direct_write(buf(&b, 1), err);
        }
        flush(err);
        if (err) {
            return 0;
        }
    }

    (*writeptr++) = b;
    //print "write writeptr", writeptr;
//     print "write writebuf %v %v %v" % writebuf, writeptr, writeptr - writebuf;
    //print "length after write", (writeptr - writebuf);
//     print "write 1", writeptr - writebuf;
    return 1;
}

size io::Stream::write_repeated(str data, size cnt, error &err) {
    size nn = 0;
    for (size i = 0; i < cnt; i++) {
        nn += write(data, err);
        if (err) {
            return nn;
        }
    }
    return nn;
}

size io::Stream::write_repeated(char c, size cnt, error &err) {
    size nn = 0;
    for (size i = 0; i < cnt; i++) {
        nn += write(c, err);
        if (err) {
            return nn;
        }
    }
    return nn;
}


size io::Stream::flush(error &err) {
    // printf("<flush>");
    if (!writebuf) {
        return 0;
    }

    buf data(writebuf, writeptr - writebuf);
    //print "data to go", writeptr - writebuf;
    if (!data) {
        return 0;
    }
//     print "flushing datalen", len(data), this;

//     print "before", writebuf, writeptr, writeend;
    size n = direct_write(data, err);
//     print "after", writebuf, writeptr, writeend;
    writeptr -= n;
//     print "direct_write returned", n, writebuf, writeptr, writeend, writeptr >= writebuf && writeptr <= writeend;

//     print "flushing datalen after", writeptr - writebuf;

    assert(writeptr >= writebuf && writeptr <= writeend, exceptions::assertion);
    if (n < len(data) && !err) {
        err(ErrShortWrite);
    }
    if (err) {
        if (n > 0 && n < len(data)) {
            copy(data, data.slice(n));
        }
    }

//     print "data left", writeptr - writebuf;

    return n;
}

void io::Stream::reset() {
    // print "reset";
    readptr  = readend;
    writeptr = writebuf;

    // read
    //       [<ALREADY READ><NOT YET READ><AVAIL BUFFER>]
    //       ^              ^             ^              ^
    //    readbuf.begin   readptr       readend       readbuf.end 
    
    // write
    //      [<WRITTEN & FLUSHED><AVAIL FOR WRITE>]
    //      ^                   ^                 ^
    //    writebuf           writeptr          writeend

}

void io::Stream::setbuf(buf buf) {
    // printf("<setbuf>");
    size readptr_idx = std::min(readptr - readbuf.data, len(buf));
    size readend_idx = std::min(readend - readbuf.data, len(buf));

    // ::printf("setbuf newsize=%d readptr=%d readend=%d\n", int(len(buf)), int(readptr_idx), int(readend_idx));

    writebuf = 0;
    writeptr = buf.begin() + readend_idx;
    writeend = buf.end();

    readbuf = buf;
    readptr = buf.begin() + readptr_idx;
    readend = buf.begin() + readend_idx;
}

void Buffered::resize_readbuf(size newsize) {
    size readptr_idx = std::min(readptr - readbuf.data, newsize);
    size readend_idx = std::min(readend - readbuf.data, newsize);

    readbuf.data = mem::realloc(readbuf.data, newsize);
    readbuf.len = newsize;
    readptr = readbuf.begin() + readptr_idx;
    readend = readbuf.begin() + readend_idx;
}

void Buffered::resize_writebuf(size newsize) {
    // printf("<resize_writebuf %ld/%lx>", newsize, this);
    size writeidx = writeptr - writebuf;
    // printf("writeidx=%d\n", int(writeidx));

    writebuf = mem::realloc(writebuf, newsize);
    writeptr = writebuf + writeidx;
    writeend = writebuf + newsize;
    // print "resize_writebuf\n";
}

Buffered::~Buffered() {
    if (readbuf.data) {
        ::free(readbuf.data);
        readbuf.data = nil;
    }

    if (writebuf) {
        ::free(writebuf);
        writebuf = nil;
    }
//     print "destructed";
}

size io::Buffer::direct_read(buf buf, error &err) {
    readend = writeptr;
    size amt = readend - readptr;
    if (amt == 0) {
        err(io::EOF);
        return 0;
    }

    str data(readptr, amt);
    return copy(buf, data);
}

size io::Buffer::write(str s) {
    return write(s, error::ignore());
}

size io::Buffer::direct_write(str data, error&) {
    // ::printf("Buffer::direct_write <%s>\n", data.c_str().data);
    //size curlen = length();
    //size newlen = curlen + len(data);
    //assert(newlen >= curlen, exceptions::overflow);

    grow(len(data));

    memmove(writeptr, data.data, data.len);
    readend += len(data);
    writeptr += len(data);
    return len(data);
}

void io::Buffer::grow(size n) {
    size newcap = used() + n;
    size curcap = capacity();
    if (newcap <= curcap) {
        return;
    }

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

size io::Buffer::used() {
    return writeptr - readbuf.data;
}

size io::Buffer::available() {
    return writeend - writeptr;
}

size IStream::direct_write(str, error &) {
    panic("unimplemented");
    return 0;
}

size OStream::direct_read(buf, error &) {
    panic("unimplemented");
    return 0;
}


size io::Buffer::capacity() {
    return len(readbuf);
}

size io::Buffer::length() {
    return writeptr - readptr;
}

String io::Buffer::to_string() {
    String s;
    s.buffer.data = readbuf.data;
    s.buffer.len = readbuf.len;
    s.length = length();

    // ::printf("to_string <%s>\n", str(readbuf.data, s.length).c_str().data);

    readbuf = {};
    return s;
}

io::Buffer::~Buffer() {
    if (readbuf) {
        ::free(readbuf.data);
        readbuf = {};
    }
}


StrStream::StrStream(str s) {
    readptr = (const byte *) s.begin();
    readend = (const byte *) s.end();
}

size StrStream::direct_read(buf buffer, error &err) {
    size amt = readend - readptr;
    if (amt == 0) {
        err(io::EOF);
        return 0;
    }

    str data(readptr, amt);
    size n = copy(buffer, data);
    readptr += n;
    return n;
}

BufStream::BufStream(buf b) {
    readptr = b.begin();
    readend = b.end();

    writeptr = b.begin();
    writeend = b.end();
}

buf BufStream::bytes() {
    return buf(
        const_cast<byte*>(readptr),
        writeptr - readptr);
}

size BufStream::direct_read(buf buffer, error &err) {
    readend = writeptr;

    size amt = readend - readptr;
    if (amt == 0) {
        err(io::EOF);
        return 0;
    }

    str data(readptr, amt);
    size n = copy(buffer, data);
    readptr += n;
    return n;
}

size BufStream::direct_write(str, error &err) {
    err(ErrShortBuffer);
    return 0;
}

// StdStream

size io::StdStream::direct_read(buf bytes, error &err) {
   size n = ::fread(bytes.data, 1, len(bytes), file);

    if (n != len(bytes)) {
        if (::ferror(file)) {
            err(ErrIO);
            return n;
        }

        if (::feof(file)) {
            err(EOF);
        }
   }

   return n;
}

size io::StdStream::direct_write(str data, error &err) {
   size n = ::fwrite(data.data, 1, len(data), file);

   if (n != len(data)) {
       if (::feof(file)) {
           err(EOF);
       }

       if (::ferror(file)) {
           err(ErrIO);
           return n;
       }

       err(ErrIO);
   }

   return n;
}


io::Forwarder::Forwarder(io::IStream &reader, io::OStream &writer)
        : reader(reader),
          writer(writer) {

//     print "Forwarder", this;

    readbuf = reader.readbuf;
    readptr = reader.readptr;
    readend = reader.readend;

    writebuf = writer.writebuf;
    writeptr = writer.writeptr;
    writeend = writer.writeend;

//     print "Fowarder writebuf", writebuf;
    //print "writeptr", writeptr;
}


size io::Forwarder::direct_read(buf bytes, error &err) {
    //reader.readptr = readptr;

    size n = reader.direct_read(bytes, err);

    //readbuf = reader.readbuf;
    //readptr = reader.readptr;
    //readend = reader.readend;

    return n;
}

size io::Forwarder::direct_write(str data, error &err) {
//     print "Forwarder::direct_write";

    //writer.writeptr = writeptr;

    size n = writer.direct_write(data, err);

    //writebuf = writer.writebuf;
    //writeptr = writer.writeptr;
    //writeend = writer.writeend;

//     print "Forwarder";

    return n;
}

void io::Forwarder::close(error &err) {
    reader.close(err);
    if (err) {
        writer.close(error::ignore());
        return;
    }
    writer.close(err);
}
