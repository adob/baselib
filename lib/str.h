#pragma once

#include <string>
#include <span>
#include <cstring>

#include "./types.h"
#include "./mem.h"
#include "./assert.h"

namespace lib {
    namespace exceptions {
        void out_of_memory();
        void bad_index(size got, size max);
        void overflow();
    }

    namespace io {
        struct WriterTo;
    }


    struct String;
    struct CString;

    template <int N>
    struct StringPlus;

    // str
    struct str {
        const char *data;
        size       len;

        constexpr str() : data(nil), len(0) {}
        // constexpr str(const str&) = default;
        // constexpr str(str&&) = default;
        //constexpr str(nullptr_t) : data(nil), len(0) {}

        template <usize N>
        constexpr str(const char (&str)[N]) : data(str), len(N-1){}


        template <usize N>
        constexpr str(const byte (&bytes)[N]) : data((const char*)bytes), len(N) { }

        str(std::string const& s) : data(s.data()), len(s.size()) {}
        str(std::string_view s) : data(s.data()), len(s.size()) {}
        explicit constexpr str(std::span<uint8> s) : data((const char *) s.data()), len(s.size()) {}
        //str(std::string&&) = delete;

        inline str(String const& s);
        // str(String &&);

        // template <int N>
        // str(StringPlus<N> const &);

        constexpr str(const char *str, size strlen) : data(str), len(strlen) {}
        constexpr str(const byte *data, size strlen) : data((const char*) (const void *) data), len(strlen) {}

        constexpr str slice(size i) const {
            assert(usize(i) <= usize(len), exceptions::bad_index, i, len);
            return str(data + i, len - i);
        }

        constexpr str slice(size i, size j) const {
            assert(usize(j) <= usize(len), exceptions::bad_index, i, len);
            assert(i <= j, exceptions::bad_index, i, j);
            return str(data+i, j-i);
        }

        constexpr const char *begin() {
            return data;
        }

        constexpr const char *end() {
            return data + len;
        }

        constexpr char operator [] (size i) const {
            assert(usize(i) < usize(len), exceptions::bad_index, i, len-1);
            return data[i];
        }

        constexpr str operator [] (size i, size j) const {
            return slice(i, j);
        }

        constexpr str operator() (size i) const {
            return slice(i);
        }

        constexpr str operator + (size i) const {
            return slice(i);
        }

        constexpr explicit operator bool () const {
            return len != 0;
        }

        operator std::string() const {
            return std::string(data, len);
        }

        constexpr bool operator == (const str other) const {
            if (len != other.len) {
                return false;
            }
            if (data == other.data) {
                return true;
            }

            return ::memcmp(data, other.data, len) == 0;
        }

        template <size N>
        constexpr bool operator == (const char (&p)[N]) const {
            if (len != N-1) {
                return false;
            }
            if (data == p) {
                return true;
            }
            return ::memcmp(data, p, N-1) == 0 && p[N-1] == '\0';
        }

        auto operator<=>(const str other) const {
            std::size_t min_length = std::min(this->len, other.len);
            int cmp = std::memcmp(this->data, other.data, min_length);
    
            if (cmp != 0) {
                return cmp <=> 0;  // Compare based on data
            }
            return this->len <=> other.len;  // Compare based on length
        }

        CString c_str() const;
        std::string std_string() const;

        static str from_c_str(const char *strp) {
            return str(strp, strlen(strp));
        }
    };

    constexpr size len(str s)                { return s.len; }
    constexpr const char *begin(str s)       { return s.data; }
    constexpr const char *end(str s)         { return s.data+s.len; }

    template <size N>
    constexpr bool operator == (const char (&p)[N], str s) {
        return s == std::forward<const char (&)[N]>(p);
    }

    // template <size N>
    // constexpr bool operator != (str s, const char (&p)[N]) {
    //     return !(s == std::forward(p));
    // }

    // template <size N>
    // constexpr bool operator != (const char (&p)[N], str s) {
    //     return !(s == std::forward(p));
    // }

    void panic(str);



//     // std::string += str
//     inline std::string& operator +=(std::string &s1, str s2) {
//         s1.append(s2.data, s2.len);
//         return s1;
//     }

//     template <typename T>
//     struct slice {
//         T       *arr;
//         size     len;
//
//
//         constexpr slice() : arr(nullptr), len(0) {}
//         constexpr slice(T *t, usize len) : arr(t), len(len) {}
//
//         template <size N>
//         constexpr slice(T (&arr)[N])  : arr(arr), len(N) { }
//
//         template <size N>
//         constexpr slice(T (&&arr)[N]) : arr(arr), len(N) { }
//
//         slice slice(size i) const {
//             assert(usize(i) <= usize(len), exceptions::bad_index, i, len);
//             return slice(arr+i, len-i);
//         }
//
//         slice slice(size i, size j) const {
//             assert(usize(j) <= usize(len), exceptions::bad_index, i , len);
//             assert(i <= j, exceptions::bad_index, i, j);
//             return slice(arr+i, j-i);
//         }
//
//         slice operator () (size i) const {
//             return slice(i);
//         }
//
//         slice operator () (size i, size j) const {
//             return slice(i, j);
//         }
//
//         T& operator [] (size i) {
//             assert(usize(i) < usize(len), exceptions::bad_index, i, len-1);
//             return arr[i];
//         }
//         constexpr const T& operator [] (size i) const {
//             assert(usize(i) < usize(len), exceptions::bad_index, i, len-1);
//             return arr[i];
//         }
//
//     //     friend constexpr T *begin(slice arr) { return arr.arr; }
//
//     //     friend constexpr T *end(slice arr) { return arr.arr + arr.len; }
//     };
//
//     template <typename T>
//     constexpr size len(slice<T> arr) {
//         return arr.len;
//     }


    // buf
    struct buf {
        byte *data = nil;
        ssize len = 0   ;

        constexpr buf() : data(0), len(0) {}

//         template <typename T>
//         explicit constexpr buf(T &t) {
//             data = (byte*) &t;
//             len = sizeof(t);
//         }

        constexpr buf(char *data, size len) : data( (byte*) (void*) data), len(len) {}

        constexpr buf(byte *data, size len) : data(data), len(len) {}

        template <size N> constexpr buf(char (&str)[N]) : data((byte*)str), len(N) {}

        template <size N> constexpr buf(uint8 (&bytes)[N]) : data(bytes), len(N) {}

        constexpr buf slice(size i) const {
            assert(usize(i) <= usize(len), exceptions::bad_index, i, len);
            return buf(data+i, len-i);
        }

        constexpr buf slice(size i, size j) const  {
            assert(usize(j) <= usize(len), exceptions::bad_index, i , len);
            assert(i <= j, exceptions::bad_index, i, j);
            return buf(data+i, j-i);
        }

        constexpr byte* begin() {
            return data;
        }

        constexpr const byte* begin() const {
            return data;
        }

        constexpr byte* end() {
            return data + len;
        }

        constexpr const byte* end() const {
            return data + len;
        }

        void clear() {
            data = 0;
            len = 0;
        }

        constexpr byte& operator [] (size i) {
            assert(usize(i) < usize(len), exceptions::bad_index, i,len-1);
            return data[i];
        }

        constexpr buf operator [] (size i, size j) const {
            return slice(i, j);
        }

        constexpr buf operator () (size i) const {
            return slice(i);
        }

        constexpr explicit operator bool () const {
            return len != 0;
        }

        constexpr buf operator + (size offset) {
            return slice(offset);
        }

        operator str () const {
            return str((char*)data, len);
        }
    };


    // constexpr size len(buf b)                { return b.len; }
    constexpr const byte *begin(buf b)       { return b.data; }
    constexpr const byte *end(buf b)         { return b.data+b.len; }

//     // mem
//     namespace mem {
//         inline buf alloc(size n) {
//             byte *data = (byte*) ::malloc(n);
//             if (data == nil) {
//                 exceptions::out_of_memory();
//             }
//
//             return buf(data, n);
//         }
//
//         inline buf realloc(byte *ptr, size n) {
//             byte *data = (byte*) ::realloc(n);
//             if (data == nil) {
//                 exceptions::out_of_memory();
//             }
//
//             return buf(data, n);
//         }
//
//         inline void free(buf buffer) {
//             ::free(buffer.data);
//         }
//     }

    struct Buffer : buf {
        Buffer(Buffer const&) = delete;
        Buffer(Buffer &&other) : buf(other.data, other.len) {
            other.data = nil;
            other.len = 0;
        }

        Buffer() : buf() {}
        Buffer(size n) : buf((byte*) ::malloc(n), n) {
            if (data == nil) {
                exceptions::out_of_memory();
            }
        }

        void resize(size n) {
            data = (byte*) ::realloc(data, n);
            if (data == nil) {
                exceptions::out_of_memory();
            }
            len = n;
        }

        Buffer& operator = (Buffer &&other) {
            // if (this == &other) {
            //     return *this;
            // }
            this->~Buffer();
            data = other.data;
            len = other.len;

            other.data = nil;
            other.len = 0;

            return *this;
        }

        ~Buffer() {
            if (data != nil) {
                ::free(data);
                data = nil;
            }
        }

        Buffer& operator = (Buffer const &other) = delete;
    };

    // String
    struct  String {
        mutable Buffer  buffer;
        size    length = 0;

        String()      : buffer(), length(0) {}
        String(str s) :
                buffer(s.len),
                length(s.len) {
            memcpy(buffer.data, s.data, s.len);
        }
        String(buf s) :
                buffer(s.len),
                length(s.len) {
            memcpy(buffer.data, s.data, s.len);
        }

        String(std::string const& s) : buffer(s.size()), length(s.length()) {
             memcpy(buffer.data, s.data(), s.size());
        }

        String(io::WriterTo const&);

        template <usize N>
        constexpr String(const char (&str)[N]) : buffer(N-1), length(N-1) {
            memcpy(buffer.data, str, N-1);
        }

        String(String const &other) {
            (*this) = other;
        }

        explicit String(rune r);

        explicit String(size cap) :
            buffer(cap),
            length(0) {}

        String(String &&other) {
            buffer = std::move(other.buffer);
            length = other.length;
            other.length = 0;
        }

        void ensure(size newcap) {
            if (newcap <= buffer.len) {
                return;
            }
            if (newcap < 32) {
                newcap = 32;
            }
            buffer.resize(std::max(newcap, buffer.len*2));
        }

        void append(str s) {
            size newlen = length + s.len;
            // assert(newlen >= length, exceptions::overflow);

            ensure(newlen);
            memcpy(buffer.data + length, s.data, s.len);
            length = newlen;
        }

        void append(char c) {
            size newlen = length + 1;
            ensure(newlen);

            buffer.data[length] = c;
            length = newlen;
        }

        // expand expands string length by additional number of bytes
        // and returns the newly-allocated data. New data is uninitialized.
        buf expand(size additional) {
            size newlen = length += additional;
            ensure(newlen);

            buf b = buffer[length, newlen];
            length = newlen;
            return b;
        }

        constexpr size cap() const {
            return len(buffer);
        }

        const char *c_str() const {
            const_cast<String*>(this)->ensure(length+1);
            buffer[length] = '\0';
            return (char *) buffer.data;
        }

        std::string std_string() const;

        constexpr str slice(size i) const {
            assert(usize(i) <= usize(length), exceptions::bad_index, i, length);
            return str(buffer.data+i, length-i);
        }

        constexpr buf slice(size i) {
            assert(usize(i) <= usize(length), exceptions::bad_index, i, length);
            return buf(buffer.data+i, length-i);
        }

        constexpr str slice(size i, size j) const  {
            assert(usize(j) <= usize(length), exceptions::bad_index, i , length);
            assert(i <= j, exceptions::bad_index, i, j);
            return str(buffer.data+i, j-i);
        }

        constexpr buf slice(size i, size j) {
            assert(usize(j) <= usize(length), exceptions::bad_index, i , length);
            assert(i <= j, exceptions::bad_index, i, j);
            return buf(buffer.data+i, j-i);
        }

        constexpr str operator + (size i) const {
            return slice(i);
        }

        constexpr String& operator += (str s) {
            append(s);
            return *this;
        }

        constexpr String& operator += (char c) {
            append(c);
            return *this;
        }

        String&& operator+(this String &&s, String const &other) {
            s.append(other);
            return s;
        }

        String& operator += (io::WriterTo const &writer_to);

        String& operator = (str s) {
            ensure(s.len);

            memmove(buffer.data, s.data, s.len);
            length = s.len;
            return *this;
        }

        String& operator = (buf b) {
            return *this = str(b);
        }

        String& operator = (const String& other) {
            if (other.length > buffer.len) {
                buffer.resize(other.length);
            }
            length = other.length;
            if (length > 0) {
                memcpy(buffer.data, other.buffer.data, length);
            }
            return *this;
        }

        String& operator = (String&& other) {
            buffer = std::move(other.buffer);
            length = other.length;
            other.length = 0;
            return *this;
        }

        template <int N>
        String& operator = (StringPlus<N> const&);

        template <usize N>
        String& operator =(const char (&s)[N]) {
            ensure(N-1);
            memmove(buffer.data, s, N-1);
            length = N-1;
            return *this;
        }

        constexpr char operator [] (size i) const {
            assert(usize(i) < usize(length), exceptions::bad_index, i,length-1);
            return buffer.data[i];
        }

        constexpr char& operator [] (size i) {
            assert(usize(i) < usize(length), exceptions::bad_index, i,length-1);
            return ((char*) buffer.data)[i];
        }

        constexpr str operator [] (size i, size j) const {
            return slice(i, j);
        }

        constexpr buf operator [] (size i, size j) {
            return slice(i, j);
        }
        

        constexpr bool operator == (str s) const {
            if (this->length != s.len) {
                return false;
            }

            if (this->buffer.data == (byte*) s.data) {
                return true;
            }
            return ::memcmp(this->buffer.data, s.data, this->length) == 0;
        }

        constexpr bool operator == (String const& other) const {
            return *this == str(other.buffer.data, other.length);
        }

        template <size N>
        constexpr bool operator == (const char (&p)[N]) const {
            if (this->length != N-1) {
                return false;
            }
            return ::memcmp(this->buffer.data, p, N-1) == 0 && p[N-1] == '\0';
        }

        auto operator<=>(const String &other) const {
            std::size_t min_length = std::min(this->length, other.length);
            int cmp = std::memcmp(this->buffer.data, other.buffer.data, min_length);
    
            if (cmp != 0) {
                return cmp <=> 0;  // Compare based on data
            }
            return this->length <=> other.length;  // Compare based on length
        }
    };

    // String
    struct CString {
        Buffer  buffer;
        size    length;

        CString()      : buffer(), length(0) {}
        CString(str s) :
                buffer(s.len+1),
                length(s.len) {
            memcpy(buffer.data, s.data, s.len);
            
            buffer[len(s)] = '\0';
        }

        CString(std::string const& s) : buffer(s.size()), length(s.length()) {
             memcpy(buffer.data, s.data(), s.size()+1);
        }

        template <usize N>
        constexpr CString(const char (&str)[N]) : buffer(N-1), length(N-1) {
            memcpy(buffer.data, str, N);
        }

        CString(CString const &other) {
            (*this) = other;
        }

        explicit CString(size cap) :
            buffer(cap+1),
            length(0) {}

        CString(CString &&other) {
            buffer = std::move(other.buffer);
            length = other.length;
            other.length = 0;
        }

        void ensure(size newcap) {
            newcap += 1;
            if (newcap <= buffer.len) {
                return;
            }
            if (newcap < 32) {
                newcap = 32;
            }
            buffer.resize(std::max(newcap, buffer.len*2));
        }

        void append(str s) {
            size newlen = length + s.len;
            assert(newlen >= length, exceptions::overflow);

            ensure(newlen);
            memcpy(buffer.data + length, s.data, s.len);
            length = newlen;
            buffer[length] = '\0';
        }

        const char *c_str() const {
            return (char *) buffer.data;
        }

        std::string std_string() const;

        CString& operator += (str s) {
            append(s);
            return *this;
        }

        CString& operator = (str s) {
            ensure(s.len);

            memmove(buffer.data, s.data, s.len);
            length = s.len;
            buffer[length] = '\0';
            return *this;
        }

        CString& operator = (const CString& other) {
            if (other.buffer.len == 0) {
                this->length = 0;
                return *this;
            }
            if (other.length > buffer.len) {
                buffer.resize(other.length+1);
            }
            length = other.length;
            memcpy(buffer.data, other.buffer.data, length+1);

            return *this;
        }

        CString& operator = (CString&& other) {
            buffer = std::move(other.buffer);
            length = other.length;
            other.length = 0;
            return *this;
        }

        template <usize N>
        CString& operator =(const char (&s)[N]) {
            ensure(N-1);
            memmove(buffer.data, s, N);
            length = N-1;
            return *this;
        }

        constexpr operator str() {
            return buffer[0, length];
        }

        constexpr operator char *() {
            return (char*) buffer.data;
        }
    };

    // constexpr bool operator == (str s1, String const& s2) {
    //   return s2 == s1;
    // }

    str::str(String const& s) :
            data((const char *) s.buffer.data),
            len(s.length) {}

    // struct CString {
    //     char *data;
    //     size len;

    //     CString(str s);
    //     CString(CString const&) = delete;

    //     operator const char *() { return data; }

    //     CString& operator = (CString const&) = delete;

    //     ~CString();
    // };

    inline size copy(buf dst, str src) {
        size amt = std::min(len(dst), len(src));
        ::memmove(dst.data, src.data, amt);
        
        return amt;
    }

    // inline bool operator==(String const&a, String const&) {
    //     return false;
    // }
}

template <>
struct std::hash<lib::String> {
    std::size_t operator()(const lib::String &s) const {
        std::hash<string_view> h;
        return h(std::string_view((char*) s.buffer.data, s.length));
    }
} ;