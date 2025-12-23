#pragma once

#include "lib/str.h"
#include "lib/types.h"

namespace lib {

    template <size N>
    struct InlineString {
        byte bytes[N];
        size    length = 0;

        InlineString()      : length(0) {}
        InlineString(str s) : length(std::min(s.len, N)) {
            memcpy(bytes, s.data, length);
        }
        InlineString(buf s) : length(std::min(s.len, N)) {
            memcpy(bytes, s.data, length);
        }

        InlineString(std::string const& s) : length(std::min(size(s.size()), N)) {
            memcpy(bytes, s.data(), length);
        }

        // InlineString(io::WriterTo const&);

        template <usize M>
        constexpr InlineString(const char (&str)[M]) : length(std::min(size(N), size(M)-1)) {
            static_assert(M - 1 <= N, "too long");
            memcpy(bytes, str, length);
        }

        template <usize M>
        constexpr InlineString(const byte (&str)[M]) : length(std::min(size(N), size(M))) {
            static_assert(M <= N, "too long");
            memcpy(bytes, str, length);
        }

        InlineString(InlineString<N> const &other) {
            (*this) = other;
        }

        // explicit String(rune r);

        // explicit String(size cap) :
        //     buffer(cap),
        //     length(0) {}

        // String(String &&other) {
        //     buffer = std::move(other.buffer);
        //     length = other.length;
        //     other.length = 0;
        // }

        // void ensure(size newcap) {
        //     if (newcap <= buffer.len) {
        //         return;
        //     }
        //     if (newcap < 32) {
        //         newcap = 32;
        //     }
        //     buffer.resize(std::max(newcap, buffer.len*2));
        // }

        void append(str s) {
            size newlen = std::min(length + s.len, N);
            // LIB_CHECK(newlen >= length, exceptions::overflow);

            memcpy(bytes + length, s.data, newlen);
            length = newlen;
        }

        void append(char c) {
            size newlen = std::min(length + 1, N);

            bytes[length] = c;
            length = newlen;
        }

        // // expand expands string length by additional number of bytes
        // // and returns the newly-allocated data. New data is uninitialized.
        // buf expand(size additional) {
        //     size newlen = length += additional;
        //     ensure(newlen);

        //     buf b = buffer[length, newlen];
        //     length = newlen;
        //     return b;
        // }

        constexpr size cap() const {
            return N;
        }

        // const char *c_str() const {
        //     const_cast<String*>(this)->ensure(length+1);
        //     buffer[length] = '\0';
        //     return (char *) buffer.data;
        // }

        // std::string std_string() const;

        constexpr str slice(size i) const {
            LIB_CHECK(usize(i) <= usize(length), exceptions::bad_index, i, length);
            return str(bytes+i, length-i);
        }

        constexpr buf slice(size i) {
            LIB_CHECK(usize(i) <= usize(length), exceptions::bad_index, i, length);
            return buf(bytes+i, length-i);
        }

        constexpr str slice(size i, size j) const  {
            LIB_CHECK(usize(j) <= usize(length), exceptions::bad_index, i , length);
            LIB_CHECK(i <= j, exceptions::bad_index, i, j);
            return str(bytes+i, j-i);
        }

        constexpr buf slice(size i, size j) {
            LIB_CHECK(usize(j) <= usize(length), exceptions::bad_index, i , length);
            LIB_CHECK(i <= j, exceptions::bad_index, i, j);
            return buf(bytes+i, j-i);
        }

        constexpr buf data() {
            return buf(bytes, N);
        }

        constexpr str operator + (size i) const {
            return slice(i);
        }

        constexpr InlineString<N>& operator += (str s) {
            append(s);
            return *this;
        }

        constexpr InlineString<N>& operator += (char c) {
            append(c);
            return *this;
        }

        InlineString<N>&& operator+(this String &&s, String const &other) {
            s.append(other);
            return s;
        }

        InlineString<N>& operator += (io::WriterTo const &writer_to);

        InlineString<N>& operator = (str s) {
            length = std::min(N, len(s));
            memmove(bytes, s.data, length);
            
            return *this;
        }

        InlineString<N>& operator = (buf b) {
            return *this = str(b);
        }

        InlineString<N>& operator = (const InlineString<N>& other) {
            length = other.length;
            memmove(bytes, other.bytes, length);
            return *this;
        }

        // String& operator = (String&& other) {
        //     buffer = std::move(other.buffer);
        //     length = other.length;
        //     other.length = 0;
        //     return *this;
        // }

        // template <int N>
        // String& operator = (StringPlus<N> const&);

        template <usize M>
        InlineString<N>& operator =(const char (&s)[M]) {
            length = std::min(N, size(M-1));
            memmove(bytes, s, length);
            return *this;
        }

        constexpr byte operator [] (size i) const {
            LIB_CHECK(usize(i) < usize(length), exceptions::bad_index, i,length-1);
            return bytes[i];
        }

        constexpr byte& operator [] (size i) {
            LIB_CHECK(usize(i) < usize(length), exceptions::bad_index, i,length-1);
            return ((byte*) bytes)[i];
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

            if (this->bytes == (byte*) s.data) {
                return true;
            }
            return ::memcmp(this->bytes, s.data, this->length) == 0;
        }

        constexpr bool operator == (InlineString<N> const& other) const {
            return ::memcmp(bytes, other.bytes, length) == 0;
        }

        template <size M>
        constexpr bool operator == (const char (&p)[M]) const {
            if (this->length != M-1) {
                return false;
            }
            return ::memcmp(this->bytes, p, M-1) == 0 && p[M-1] == '\0';
        }

        auto operator<=>(const String &other) const {
            std::size_t min_length = std::min(this->length, other.length);
            int cmp = std::memcmp(this->bytes, other.buffer.data, min_length);

            if (cmp != 0) {
                return cmp <=> 0;  // Compare based on data
            }
            return this->length <=> other.length;  // Compare based on length
        }

        constexpr operator str() const {
            return str(bytes, length);
        }

        constexpr operator buf() {
            return buf(bytes, length);
        }
    };

}