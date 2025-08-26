#include <string.h>
#include <stdio.h>
#include <sys/types.h>

#include "io.h"
#include "lib/mem.h"
#include "util.h"

#include "lib/base.h"
#include "lib/exceptions.h"
#include "lib/print.h"


using namespace lib;
using namespace io;

// static deferror ErrNegativeRead("io: read returned negative count");

#pragma GCC diagnostic ignored "-Wshadow"

// #define LOGF(...) printf(__VA_ARGS__)
#define LOGF(...)

io::ReaderWriter::ReaderWriter(io::ReaderWriter &&other) {
    // printf("<MC>");
    (*this) = std::move(other);
}

io::ReaderWriter& io::ReaderWriter::operator=(ReaderWriter&& other) {
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

byte io::ReaderWriter::read_byte(error err) {
    // print "read_byte";
    if (readptr == readend) [[unlikely]] {
        bool use_readbuf = check_readbuf(1);
        
        if (!use_readbuf) {
            // no read buffer
            byte b = 0;
            size nbytes = direct_read(buf(&b, 1), err).nbytes;
            if (nbytes == 0) {
                err(io::ErrUnexpectedEOF());
            }
            return b;
        }
        
        // print "L70";
        ReadResult r = direct_read(readbuf, err);
        // print "direct_read", r.eof, r.nbytes;
        readptr = readbuf.data;
        readend = readptr + r.nbytes;
        if (r.nbytes == 0) {
            err(io::ErrUnexpectedEOF());
            return 0;
        }
    }

    return *(readptr++);
}

ReadResult io::ReaderWriter::read(buf p, error err) {
    if (readptr == readend) [[unlikely]] {
        // buffer is empty

        bool use_readbuf = check_readbuf(len(p));

        //if (len(p) >= len(readbuf)) {
        if (!use_readbuf) {
            //if (!readbuf && readptr) {
            //   err(EOF);
            //    return 0;
            // }

            // large read goes directly into p
            return direct_read(p, err);
        }

        // one read
        ReadResult r = direct_read(readbuf, err);
        
        LIB_CHECK(usize(r.nbytes) <= usize(len(readbuf)), exceptions::assertion);
        readptr = readbuf.data;
        readend = readptr + r.nbytes;
        if (r.nbytes == 0) {
            return {0, true};
        }

        size amt = std::min(p.len, readend - readptr);
        memmove(p.data, readptr, size_t(amt));
        readptr += amt;
        bool eof = r.eof && amt == r.nbytes;
        
        return ReadResult{amt, eof};
    }

    size amt = std::min(p.len, readend - readptr);
    memmove(p.data, readptr, size_t(amt));
    readptr += amt;
    return ReadResult{amt, false};
}


bool ReaderWriter::check_readbuf(size nbytes) {
    intptr ptr = intptr(readbuf.data);
    if (ptr >= 0) {
        // large read
        return nbytes <= len(readbuf);
    }

    size newsize = -ptr;

    if (nbytes > newsize) {
        // large read; don't allocate
        return false;
    }
    
    readbuf.data = mem::alloc(newsize);
    readbuf.len = newsize;

    readptr = readbuf.begin();
    readend = readbuf.begin();
    
    return true;
}


size io::ReaderWriter::write(str p, error err) {
    //::printf("<W>: [%s]\n", p.c_str().data);
    buf avail(writeptr, writeend - writeptr);

    //::printf("Stream::WRITE1 %s %ld\n", p.c_str().data, len(avail));

    if (len(p) > len(avail)) [[unlikely]] {
        // p is larger than available buffer space
        size n1 = 0;
        if (check_writebuf(len(p))) {
            // printf("BUFFER EXISTS\n");
            // buffer exists
            if (writeptr > writebuf) {
                // printf("BUFFER IS NONEMPTY\n");
                // buffer has data
                // write into buffer and flush buffer
                n1 = copy(avail, p);
                writeptr += n1;
                // printf("COPIED %ld into buffer\n", n1);
                flush(err);
                // printf("FLUSHED\n");
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
            } else if (len(p) < (writeend - writeptr)) {
                // buffer is empty and large enough to hold p
                // this case occurs when buffer is first allocated

                avail = buf(writeptr, writeend - writeptr);
                size n = copy(avail, p);
                writeptr += n;
                return n;
            }
        }

        // buffer is empty and data too large to fit into buffer
        size n2 = direct_write(p, err);
        if (!err && n2 < len(p)) {
            err(io::ErrShortWrite());
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

size io::ReaderWriter::write_byte(byte b, error err) {
    LOGF("io::Stream::write(byte) this %p, writeptr %p; writeend %p\n", this, writeptr, writeend);

    if (writeptr == writeend) {
        bool use_writebuf = check_writebuf(1);

        if (!use_writebuf) {
            // no buffer
            LOGF("io::Stream::write(byte) %p 1\n", this);
            return direct_write(buf(&b, 1), err);
        }
        flush(err);
        if (err) {
            LOGF("io::Stream::write(byte) %p 2\n", this);
            return 0;
        }
    }

    LOGF("io::Stream::write(byte) %p 3\n", this);

    (*writeptr++) = b;
    //print "write writeptr", writeptr;
//     print "write writebuf %v %v %v" % writebuf, writeptr, writeptr - writebuf;
    //print "length after write", (writeptr - writebuf);
//     print "write 1", writeptr - writebuf;
    return 1;
}



bool ReaderWriter::check_writebuf(size nbytes) {
    LOGF("io::Stream::check_writebuf this %lx, ptr %ld; size %ld\n", this, intptr(writebuf), nbytes);
    intptr ptr = intptr(writebuf);
    if (ptr > 0) {
        return true;
    }

    if (ptr == 0) {
        return false;
    }

    size newsize = -ptr;

    if (nbytes > newsize) {
        // large write; don't allocate
        return false;
    }

    writebuf = mem::alloc(newsize);
    writeend = writebuf + newsize;
    writeptr = writebuf;

    return true;
}

size io::ReaderWriter::write_repeated(str data, size cnt, error err) {
    size nn = 0;
    for (size i = 0; i < cnt; i++) {
        nn += write(data, err);
        if (err) {
            return nn;
        }
    }
    return nn;
}

size io::ReaderWriter::write_repeated(char c, size cnt, error err) {
    size nn = 0;
    for (size i = 0; i < cnt; i++) {
        nn += write_byte(c, err);
        if (err) {
            return nn;
        }
    }
    return nn;
}


size io::ReaderWriter::flush(error err) {
    LOGF("io::Stream::flush %p\n", this);
    // printf("<flush>");
    if (!writebuf) {
        return 0;
    }

    buf data(writebuf, writeptr - writebuf);
    LOGF("io::Stream::flush %p flush size %ld\n", this, len(data));
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

    LIB_CHECK((intptr(writebuf) < 0 && writeptr == writeptr) || (writeptr >= writebuf && writeptr <= writeend), exceptions::assertion);
    if (n < len(data) && !err) {
        err(ErrShortWrite());
    }
    if (err) {
        if (n > 0 && n < len(data)) {
            copy(data, data.slice(n));
        }
    }

//     print "data left", writeptr - writebuf;

    return n;
}

void io::ReaderWriter::reset() {
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

void io::ReaderWriter::setbuf(buf buf) {
    LOGF("<setbuf>\n");
    size readptr_idx = readptr - readbuf.data;
    size readend_idx = readend - readbuf.data;
    size writeptr_idx = writeptr - readbuf.data;

    writebuf = 0;
    writeptr = buf.begin() + writeptr_idx;
    writeend = buf.end();

    readbuf = buf;
    readptr = buf.begin() + readptr_idx;
    readend = buf.begin() + readend_idx;
}

void io::ReaderWriter::setbufs(buf readbuf, buf writebuf) {
    LOGF("<setbufs>\n");
    ReaderWriter &rw = *this;
    
    rw.readbuf = readbuf;
    rw.readptr = readbuf.begin();
    rw.readend = readbuf.begin();

    rw.writebuf = writebuf.begin();
    rw.writeptr = writebuf.begin();
    rw.writeend = writebuf.end();
}

void Buffered::resize_readbuf(size newsize) {
    LOGF("io::Buffered::resize_readbuf this %p\n", this);

    intptr ptr = intptr(readbuf.data);
    if (ptr <= 0) {
        ptr = -newsize;
        readbuf.data = (byte*) ptr;
        //readend = (byte*) ptr;
        return;
    }

    size readptr_idx = std::min(readptr - readbuf.data, newsize);
    size readend_idx = std::min(readend - readbuf.data, newsize);

    readbuf.data = mem::realloc(readbuf.data, newsize);
    readbuf.len = newsize;
    readptr = readbuf.begin() + readptr_idx;
    readend = readbuf.begin() + readend_idx;
}

void Buffered::resize_writebuf(size newsize) {
    LOGF("io::Buffered::resize_writebuf this %p; size %ld\n", this, newsize);

    intptr ptr = intptr(writebuf);
    if (ptr <= 0) {
        //LOGF("io::Buffered::resize_writebuf 1\n");
        ptr = -newsize;
        writebuf = (byte*) ptr;
        //writeend = (byte*) ptr;
        return;
    }

    // printf("<resize_writebuf %ld/%lx>", newsize, this);
    size writeidx = writeptr - writebuf;
    // printf("writeidx=%d\n", int(writeidx));

    writebuf = mem::realloc(writebuf, newsize);
    writeptr = writebuf + writeidx;
    writeend = writebuf + newsize;
    // print "resize_writebuf\n";
}

Buffered::~Buffered() {
    if (intptr(readbuf.data) > 0) {
        ::free(readbuf.data);
        readbuf.data = nil;
    }

    if (intptr(writebuf) > 0) {
        ::free(writebuf);
        writebuf = nil;
    }
//     print "destructed";
}

ReadResult io::Buffer::direct_read(lib::buf, error) {
    io::Buffer &b = *this;
    size amt = b.writeptr - b.readptr;
    b.readbuf = lib::buf((byte*) b.readptr, amt);
    return {amt, amt == 0};

    // b.readend = writeptr;
    // size amt = b.readend - b.readptr;
    // print "DIRECT READ amt", amt, size(b.readptr), size(b.readend);
    // if (amt == 0) {
    //     return {0, true};
    // }

    // // readptr is updated by io::Stream::read

    // // lib::str data(b.readptr, amt);
    
    // // size nbytes = copy(buf, data);
    // // bool eof = nbytes == amt;

    // // return {nbytes, eof};
}

size io::Buffer::write(lib::str s) {
    return write(s, error::ignore);
}

size io::Buffer::write(char c) {
    return write_byte(c, error::ignore);
}


// static void dump_buffer(str data) {
//     // ::printf("BUFFER contents: <<<");
//     for (int i = 0; i < len(data); i++) {
//         ::printf("%c", data[i]);
//     }
//     ::printf(">>>\n");
// }

size io::Buffer::direct_write(struct str data, error) {
    LOGF("io::Buffer::direct_write len %ld\n", len(data));
    // ::printf("Buffer::direct_write <%s> readend %ld\n", data.c_str().data, readend - readbuf.data);
    // dump_buffer(readbuf);

    //fmt::printf("Buffer writebuf: <<<%q>>>\n", readbuf);
    //size curlen = length();
    //size newlen = curlen + len(data);
    //LIB_CHECK(newlen >= curlen, exceptions::overflow);

    grow(len(data));

    memmove(writeptr, data.data, usize(data.len));
    writeptr += len(data);
    readend = writeptr;

    // dump_buffer(readbuf);
    return len(data);
}

void io::Buffer::grow(size n) {
    LOGF("io::Buffer::grow(%ld) capacity %ld\n", n, capacity());

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

    this->data = (byte*) ::realloc(this->data, newcap);
    if (this->data == nil) {
        exceptions::out_of_memory();
    }
    LOGF("io::Buffer::grow allocated %p %ld\n", data, newcap);

    struct buf newbuf(this->data, newcap);
    setbuf(newbuf);
}

size io::Buffer::used() const {
    return writeptr - data;
}

size io::Buffer::available() const {
    return writeend - writeptr;
}

size Reader::direct_write(str, error) {
    panic("unimplemented");
    return 0;
}

size io::Buffer::capacity() const {
    return writeend - data;
}

size io::Buffer::length() const {
    return writeptr - readptr;
}

String io::Buffer::to_string() {
    String s;
    s.buffer.data = data;
    s.buffer.len = capacity();
    s.length = length();

    // ::printf("to_string <%s>\n", str(readbuf.data, s.length).c_str().data);

    readbuf = {};
    readptr  = 0;
    readend  = 0;

    writebuf = 0;
    writeptr = 0;
    writeend = 0;
    data = nil;
    return s;
}

str io::Buffer::str() const {
    return readbuf[0, this->length()];
}

buf io::Buffer::buf() {
    return readbuf[0, this->length()];
}

io::Buffer::~Buffer() {
    if (this->data) {
        ::free(this->data);
        this->data = nil;
    }
}

ReadResult Writer::direct_read(buf, error) {
    panic("unimplemented");
    return {};
}


Str::Str(str s) {
    readptr = (const byte *) s.begin();
    readend = (const byte *) s.end();
}

ReadResult Str::direct_read(buf buffer, error err) {
    size amt = readend - readptr;
    if (amt == 0) {
        return {0, true};
    }

    str data(readptr, amt);
    size n = copy(buffer, data);
    readptr += n;
    bool eof = readptr == readend;
    return {n, eof};
}

Buf::Buf(buf b) {
    readptr = b.begin();
    readend = b.end();

    writeptr = b.begin();
    writeend = b.end();
}

buf Buf::bytes() {
    return buf(
        const_cast<byte*>(readptr),
        writeptr - readptr);
}

ReadResult Buf::direct_read(buf buffer, error err) {
    readend = writeptr;

    size amt = readend - readptr;
    if (amt == 0) {
        return {0, true};
    }

    str data(readptr, amt);
    size n = copy(buffer, data);
    readptr += n;
    bool eof = readptr == readend;
    return {n, eof};
}

size Buf::direct_write(str, error err) {
    err(ErrShortBuffer());
    return 0;
}

size Buf::length() const {
    return writeptr - readptr;
}

void WriterTo::fmt(io::Writer &w, error err) const {
    this->write_to(w, err);
}

io::Forwarder::Forwarder(io::Reader &reader, io::Writer &writer)
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


ReadResult io::Forwarder::direct_read(buf bytes, error err) {
    //reader.readptr = readptr;

    return reader.direct_read(bytes, err);

    //readbuf = reader.readbuf;
    //readptr = reader.readptr;
    //readend = reader.readend;

    // return n;
}

size io::Forwarder::direct_write(str data, error err) {
//     print "Forwarder::direct_write";

    //writer.writeptr = writeptr;

    return writer.direct_write(data, err);

    //writebuf = writer.writebuf;
    //writeptr = writer.writeptr;
    //writeend = writer.writeend;

//     print "Forwarder";

    //return n;
}

void io::Forwarder::close(error err) {
    reader.close(err);
    if (err) {
        writer.close(error::ignore);
        return;
    }
    writer.close(err);
}
