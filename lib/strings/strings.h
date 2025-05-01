#pragma once

#include <vector>

#include "../utf8.h"
#include "lib/io/io.h"
#include "lib/math/math.h"
#include "lib/types.h"

namespace lib::strings {

    // contains_rune reports whether the Unicode code point r is within s.
    bool contains_rune(str s, rune r);

    size index(str haystack, str needle);

    inline size index_byte(str s, char c) {
        const char *p = (const char *)memchr(s.data, c, s.len);
        if (p) {
            return p - s.data;
        } else {
            return -1;
        }
    }

    inline size last_index_byte(str s, char c) {
        const char *p = (const char *)memrchr(s.data, c, s.len);
        if (p) {
            return p - s.data;
        } else {
            return -1;
        }
    }

    // index returns the index of the first instance of needle in haystack, or -1 if needle is not present in haystack.
    size index(str s, rune r);

    template <typename Func>
    size index_func(str s, Func&& f, bool truth = true) {
        size start = 0;
        while (start < len(s)) {
            int wid = 1;
            rune r = rune(s[start]);
            if (r >= utf8::RuneSelf) {
                r = utf8::decode_rune(s(start), wid);
            }
            if (f(r) == truth) {
                return start;
            }
            start += wid;
        }
        return -1;
    }


    // index_rune returns the index of the first instance of the Unicode code point
    // r, or -1 if rune is not present in s.
    // If r is [utf8.RuneError], it returns the first instance of any
    // invalid UTF-8 byte sequence.
    size index_rune(str s, rune r);

    inline size rindex(str s, char c) { 
        const char *p = (const char *) memrchr(s.data, c, s.len);
        if (p) {
            return p - s.data;
        } else {
            return -1;
        }
    }

    String to_lower_ascii(str s);

    constexpr bool has_prefix(str s, str prefix) {
        return len(s) >= len(prefix) && s[0,len(prefix)] == prefix;
    }

    constexpr bool has_prefix(str s, char prefix) {
        return len(s) >= 1 && s[0] == prefix;
    }

    constexpr bool has_suffix(str s, str suffix) {
        return len(s) > len(suffix) && s+(len(s)-len(suffix)) == suffix;
    }

    template <typename Func>
    size rindex(str s, Func&& f, bool truth = true) {
        for (size i = len(s); i > 0; ) {
            int wid;
            rune r = utf8::decode_last_rune(s[0, i], wid);
            i -= wid;
            if (f(r) == truth) {
                return i;
            }
        }
        return -1;
    }

    // template <typename Func>
    // str ltrim(str s, Func&& f) {
    //     size i = index_func(s, f, false);
    //     if (i == -1) {
    //         return "";
    //     }
    //     return s(i);
    // }

    // str ltrim(str s, str cutset);
    // str rtrim(str s, str cutset);
    // str trim(str s, str cutset);

    // template <typename Func>
    // str rtrim(str s, Func&& f) {
    //     size i = rindex(s, f, false);
    //     if (i != -1 && s[i] >= utf8::RuneSelf) {
    //         int wid;
    //         utf8::decode(s(i), wid);
    //         i += wid;
    //     } else {
    //         i++;
    //     }
    //     return s[0,i];
    // }

    // template <typename Func>
    // str trim(str s, Func&& f) {
    //     return rtrim(ltrim(s, f), f);
    // } 

    // str trim(str s);
    // str ltrim(str s);
    // str rtrim(str s);

    // template <typename T>
    // std::string join(T const& t, str sep) {
    //     auto iter = begin(t);
    //     auto iterend = end(t);
    //     if (iter == iterend) {
    //         return "";
    //     }
    //     std::string s = *iter;
    //     ++iter;
    //     while (iter != iterend) {
    //         s += sep;
    //         s += *iter;
    //         ++iter;
    //     }
    //     return s;
    // }

    // template <typename T>
    // String join(T const& t, str sep) {
    //     auto iter = begin(t);
    //     auto iterend = end(t);
    //     String s;
    //     if (iter == iterend) {
    //         return s;
    //     }
    //     s += *iter;
    //     ++iter;
    //     while (iter != iterend) {
    //         s += sep;
    //         s += *iter;
    //         ++iter;
    //     }
    //     return s;
    // }

    inline String replace_all(str s, char old, char replacement) {
        String result = s;

        for (size i = 0; i < len(s); i++) {
            if (s[i] == old) {
                result[i] = replacement;
            }
        }

        return result;
    }

    template <size N>
    struct Catter : io::WriterTo {
        template <typename T>
        using StrType = str;
        
        std::array<str, N> args;

        template <typename... Args>
        Catter(Args const&... args) : args({args...}) {}

        void write_to(io::Writer &out, error err) const override {
            for (size i = 0; i < N; i++) {
                out.write(args[i], err);
                if (err) {
                    return;
                }
            }
            // apply(out, err, std::make_index_sequence< 
            //     std::tuple_size_v<std::tuple<Args...>>>());
            
        }

    // private:
    //     template <usize... I>
    //     void apply(io::Writer &out, error err, std::index_sequence<I...>) const {
    //         ( (out.write(std::get<I>(args), err), bool(err)) && ... );
    //     }
    } ;

    // template <typename... Args>
    // concept AllStr = (std::same_as<str, Args> && ...);

    

    template <typename... Args>
    Catter<sizeof...(Args)> cat(Args const&...args) {
        
        Catter<sizeof...(Args)> catter {
            args...
        };
        return catter;
    }

    struct Repeater : io::WriterTo {
        str s;
        size count = 0;

        void write_to(io::Writer &out, error err) const override;
    } ;

    // repeat returns a new string consisting of count copies of the string s.
    //
    // It panics if count is negative or if the result of (len(s) * count)
    // overflows.
    Repeater repeat(str s, size count);

    // LastIndex returns the index of the last instance of substr in s, or -1 if substr is not present in s.
    size last_index(str s, str substr);

    // last_index_any returns the index of the last instance of any Unicode code
    // point from chars in s, or -1 if no Unicode code point from chars is
    // present in s.
    size last_index_any(str s, str chars);

    struct Split : Iterator {
        str s;
        str sep;
        size sep_save = 0;
        size n = math::MaxSize;
        size i = 0;

        Split(str s, str sep, size sep_save, size n);

        bool has_next();
        str next();

        operator std::vector<str>() const;
    } ;

    // Split slices s into all substrings separated by sep and returns a slice of
    // the substrings between those separators.
    //
    // If s does not contain sep and sep is not empty, Split returns a
    // slice of length 1 whose only element is s.
    //
    // If sep is empty, Split splits after each UTF-8 sequence. If both s
    // and sep are empty, Split returns an empty slice.
    //
    // It is equivalent to [SplitN] with a count of -1.
    //
    // To split around the first instance of a separator, see [Cut].
    Split split(str s, str sep);

    // SplitN slices s into substrings separated by sep and returns a slice of
    // the substrings between those separators.
    //
    // The count determines the number of substrings to return:
    //   - n > 0: at most n substrings; the last substring will be the unsplit remainder;
    //   - n == 0: the result is nil (zero substrings);
    //   - n < 0: all substrings.
    //
    // Edge cases for s and sep (for example, empty strings) are handled
    // as described in the documentation for [Split].
    //
    // To split around the first instance of a separator, see [Cut].
    Split split_n(str s, str sep, size n);

    // SplitAfter slices s into all substrings after each instance of sep and
    // returns a slice of those substrings.
    //
    // If s does not contain sep and sep is not empty, SplitAfter returns
    // a slice of length 1 whose only element is s.
    //
    // If sep is empty, SplitAfter splits after each UTF-8 sequence. If
    // both s and sep are empty, SplitAfter returns an empty slice.
    //
    // It is equivalent to [SplitAfterN] with a count of -1.
    Split split_after(str s, str sep);

    // SplitAfterN slices s into substrings after each instance of sep and
    // returns a slice of those substrings.
    //
    // The count determines the number of substrings to return:
    //   - n > 0: at most n substrings; the last substring will be the unsplit remainder;
    //   - n == 0: the result is nil (zero substrings);
    //   - n < 0: all substrings.
    //
    // Edge cases for s and sep (for example, empty strings) are handled
    // as described in the documentation for [SplitAfter].
    Split split_after_n(str s, str sep, size n);

    // count counts the number of non-overlapping instances of substr in s.
    // If substr is an empty string, Count returns 1 + the number of Unicode code points in s.
    size count(str s, str substr);
}