#pragma once
#include "lib/base.h"
#include "lib/array.h"

#ifdef EOF
    const auto EOF_ = EOF;
    #undef EOF
    const auto EOF = EOF_;
#endif

// #ifdef stdout
//     struct {
//         operator FILE* () {
//             return stdout;
//         }
//         #undef stdout
//     } const stdout;
// #endif
//
// #ifdef stderr
//     struct {
//         operator FILE* () {
//             return stderr;
//         }
//         #undef stderr
//     } const stderr;
// #endif
//
// #ifdef stdin
//     struct {
//         operator FILE* () {
//             return stdin;
//         }
//         #undef stdin
//     } const stdin;
// #endif



namespace lib::io {
    struct IStream;
    struct OStream;

    struct Stream {
        constexpr Stream() = default;

        Stream(Stream const&) = delete;
        Stream(Stream &&other);

        Stream& operator=(Stream const&) = delete;
        Stream& operator=(Stream&&);

      protected:
        buf         readbuf;
        const byte *readptr  = 0;
        const byte *readend  = 0;

        byte *writebuf = 0;
        byte *writeptr = 0;
        byte *writeend = 0;

        // read buffer
        //       [<ALREADY READ><NOT YET READ><AVAIL BUFFER>]
        //       ^              ^             ^              ^
        //    readbuf.begin   readptr       readend       readbuf.end 
        
        // write buffer
        //      [<WRITTEN & FLUSHED><AVAIL FOR WRITE>]
        //      ^                   ^                 ^
        //    writebuf           writeptr          writeend

        void setbuf(buf);

      public:
        virtual size direct_read(buf bytes, error &err) = 0;
        virtual size direct_write(str data, error &err) = 0;

        size read(buf bytes, error &err);

        size write(str data, error &err);
        size write(byte byte, error &err);

        size write_repeated(str data, size cnt, error &err);
        size write_repeated(char c, size cnt, error &err);

        byte read_byte(error &err);

        size write_available();
        size flush(error &err);

        virtual void close(error &) {}

        inline operator IStream&();
        inline operator OStream&();

        void reset();

        virtual ~Stream() {}

        friend struct Forwarder;
    };

    struct IStream : Stream {
    private:
        size direct_write(str, error&) override final;
    };

    struct OStream : Stream {
        size direct_read(buf, error&) override final;
    };

    Stream::operator IStream&() {
        return static_cast<IStream&>(*this);
    }

    Stream::operator OStream&() {
        return static_cast<OStream&>(*this);
    }

    struct Buffered : Stream {
        Buffered() = default;
        Buffered(Buffered &&other) = default;
        Buffered& operator=(Buffered &&) = default;
        ~Buffered();

    //protected:
        void resize_readbuf(size newsize);
        void resize_writebuf(size newsize);
    };

    template<int readbuf_size, int writebuf_size>
    struct StaticBuffered : Stream {
        Array<byte, readbuf_size>  backing_readbuf;
        Array<byte, writebuf_size> backing_writebuf;

        StaticBuffered() {
            readbuf = backing_readbuf;
            readptr = readbuf.begin();
            readend = readbuf.begin();

            writebuf = backing_writebuf.begin();
            writeptr = writebuf;
            writeend = backing_writebuf.end();
        }
    };

    struct Buffer : Stream {
        size direct_read(buf bytes, error &err) override;
        size direct_write(str data, error &err) override;

        using Stream::write;
        size write(str data);

        void grow(size n);

        size capacity();
        size used();
        size available();
        size length();

        String to_string();

        ~Buffer();

    };

    struct StrStream : IStream {
        StrStream(str s);

        size direct_read(buf bytes, error &err) override;
    };

    struct BufStream : Stream {
        BufStream(buf b);

        buf bytes();

        size direct_read(buf bytes, error &err) override;
        size direct_write(str data, error &err) override;
    };

    struct StdStream : Stream {
        FILE *file;

        constexpr StdStream(FILE* file) : file(file) {}

        size direct_read(buf bytes, error &err) override;
        size direct_write(str data, error &err) override;
    };

    extern StdStream out;
    extern StdStream err;

    struct WriterTo {
        virtual void write_to(io::OStream &out, error &) const = 0;
        virtual ~WriterTo() {}
    };

    struct Forwarder : Stream {
        IStream &reader;
        OStream &writer;

        Forwarder(IStream &reader, OStream &writer);

        size direct_read(buf bytes, error &err) override;
        size direct_write(str data, error &err) override;

        void close(error &) override;
    };
}
