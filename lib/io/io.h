#pragma once

#include "lib/error.h"
#include "lib/array.h"

namespace lib::io {
    struct Writer;
    struct Reader;

    struct ReadResult {
        size nbytes = 0;
        bool eof    = false;
    } ;

    struct ReaderWriter {
        constexpr ReaderWriter() = default;

        ReaderWriter(ReaderWriter const&) = delete;
        ReaderWriter(ReaderWriter &&other);

        ReaderWriter& operator=(ReaderWriter const&) = delete;
        ReaderWriter& operator=(ReaderWriter&&);

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

        // if writebuf == 0: there is no write buffer
        // if writebuf < 0 and writebuf == writeptr: write buffer has not been created yet but should be -writebuf
        // if wirtebuf > 0: write buffer exists

        // if readbuf.data == 0: there is no read buffer
        // if readbuf.data < 0 and readend == readbuf.data: read buffer has not been created yet
        // if readbuf.data > 0: read buffer exsits

        void setbuf(buf);
        void setbufs(buf readbuf, buf writebuf);

      public:
        virtual ReadResult direct_read(buf bytes, error err) = 0;
        virtual size       direct_write(str data, error err) = 0;

        ReadResult read(buf bytes, error err);

        size write(str data, error err);
        size write_byte(byte byte, error err);

        size write_repeated(str data, size cnt, error err);
        size write_repeated(char c, size cnt, error err);

        byte read_byte(error err);

        size write_available();
        size flush(error err);

        virtual void close(error) {}

        inline operator Writer&();
        inline operator Reader&();

        void reset();

        virtual ~ReaderWriter() {}

        friend struct Forwarder;

    private:
        bool check_readbuf(size);
        bool check_writebuf(size);
    };

    struct Reader : ReaderWriter {
    private:
        size direct_write(str, error) override final;
    };

    struct Writer : ReaderWriter {
        ReadResult direct_read(buf, error) override final;
    };

    ReaderWriter::operator Writer&() {
        return static_cast<Writer&>(*this);
    }

    ReaderWriter::operator Reader&() {
        return static_cast<Reader&>(*this);
    }

    struct Buffered : ReaderWriter {
        Buffered() = default;
        Buffered(Buffered &&other) = default;
        Buffered& operator=(Buffered &&) = default;
        ~Buffered();

    //protected:
        void resize_readbuf(size newsize);
        void resize_writebuf(size newsize);
    };

    template<int readbuf_size, int writebuf_size>
    struct StaticBuffered : ReaderWriter {
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

    struct Buffer : ReaderWriter {
        byte *data = nil;
        ReadResult direct_read(buf bytes, error err) override;
        size       direct_write(str data, error err) override;

        using ReaderWriter::write;
        size write(str data);
        size write(char c);

        void grow(size n);

        size capacity() const;
        size used() const;

        // available returns how many bytes are unused in the buffer.
        // i.e. number of bytes available for writing
        size available() const;

        // length returns the number of bytes of the unread portion of the buffer; b.Len() == len(b.Bytes()).
        // i.e. number of bytes available for reading
        // given by writeptr - readptr;
        size length() const;

        String to_string();
        lib::str str() const;
        lib::buf buf();

        ~Buffer();
    };

    struct Str : Reader {
        Str(str s);

        ReadResult direct_read(buf bytes, error err) override;
    };

    // needs optimization to reuse the String's buffer
    // struct StringStream : Stream {
    //     String &contents;

    //     StringStream(String &s) : contents(s) {}

    //     size direct_read(buf bytes, error err) override;
    //     size direct_write(str data, error err) override;
    // } ;

    struct Buf : ReaderWriter {
        Buf(buf b);

        // bytes returns bytes available for reading
        // given by buf(readptr, writeptr - readptr)
        buf bytes();

        ReadResult direct_read(buf bytes, error err) override;
        size       direct_write(str data, error err) override;

        // length returns the number of bytes of the unread portion of the buffer; b.Len() == len(b.Bytes()).
        // i.e. number of bytes available for reading
        // given by writeptr - readptr;
        size length() const;
    };

    struct WriterTo {
        virtual void write_to(io::Writer &out, error) const = 0;
        virtual ~WriterTo() {}

        void fmt(io::Writer &w, error) const;
    };

    struct Forwarder : ReaderWriter {
        Reader &reader;
        Writer &writer;

        Forwarder(Reader &reader, Writer &writer);

        ReadResult direct_read(buf bytes, error err) override;
        size       direct_write(str data, error err) override;

        void close(error) override;
    };

    

}

namespace lib {
    // StringPlus

    template <int N>
    struct StringPlus : io::WriterTo {
        std::array<str, N> data;
        String materialized;

        // template <typename ...Args>
        constexpr StringPlus(str a, str b) : data({a, b}) {
            static_assert(N == 2, "wrong numbe of args");
        }

        constexpr StringPlus(StringPlus<N-1> a, str b) {
            std::copy(a.data.begin(), a.data.end(), this->data.begin());
            data[N-1] = b;
        }

        void write_to(io::Writer &w, error err) const override {
            for (str s : data) {
                w.write(s, err);
                if (err) {
                    return;
                }
            }
        }

        constexpr StringPlus<N+1> operator + (str other) {
            return StringPlus<N+1>(*this, other);
        }

        operator String() {
            io::Buffer b;
            this->write_to(b, error::ignore);
            String materialized = b.to_string();
            return materialized;
        }

        operator str() {
            io::Buffer b;
            this->write_to(b, error::ignore);
            this->materialized = b.to_string();
            return str(materialized);
        }
    };

    constexpr StringPlus<2> operator + (str a, str b) {
        return StringPlus<2>(a, b);
    }

    template <int N>
    String& String::operator = (StringPlus<N> const& other) {
        return *this = String(other);
    }

    // template <int N>
    // str::str(StringPlus<N> const &o) {
    //     io::Buffer b;
    //     o.write_to(b, error::ignore);
    //     o.materialized = b.to_string();
    //     this->data = o.materialized.buffer.data;
    //     this->len = o.materialized.length;
    // }
}