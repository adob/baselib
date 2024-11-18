#pragma once

#include "../base.h"
#include "../utf8.h"

namespace lib::strings {
    
    size index(str haystack, str needle);
    
    inline size index(str s, char c) { 
        const char *p = (const char *) memchr(s.data, c, s.len);
        if (p) {
            return p - s.data;
        } else {
            return -1;
        }
    }
    
    inline size index(str s, rune r) {
        if (r < utf8::RuneSelf) {
            return index(s, (char) r);
        }
        
        size start = 0;
        while (start < len(s)) {
            int wid = 1;
            rune rr = rune(s[start]);
            if (rr >= utf8::RuneSelf) {
                rr = utf8::decode(s+start, wid);
            }
                
            if (rr == r) {
                return start;
            }
                
            start += wid;
        }
        return -1;
    }
    
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
    size index(str s, Func&& f, bool truth = true) {
        size start = 0;
        while (start < len(s)) {
            int wid = 1;
            rune r = rune(s[start]);
            if (r >= utf8::RuneSelf) {
                r = utf8::decode(s(start), wid);
            }
            if (f(r) == truth) {
                return start;
            }
            start += wid;
        }
        return -1;
    }
    
    template <typename Func>
    size rindex(str s, Func&& f, bool truth = true) {
        for (size i = len(s); i > 0; ) {
            int wid;
            rune r = utf8::decode_last(s[0, i], wid);
            i -= wid;
            if (f(r) == truth) {
                return i;
            }
        }
        return -1;
    }
    
    template <typename Func>
    str ltrim(str s, Func&& f) {
        size i = index(s, f, false);
        if (i == -1) {
            return "";
        }
        return s(i);
    }
    
    str ltrim(str s, str cutset);
    str rtrim(str s, str cutset);
    str trim(str s, str cutset);
    
    template <typename Func>
    str rtrim(str s, Func&& f) {
        size i = rindex(s, f, false);
        if (i != -1 && s[i] >= utf8::RuneSelf) {
            int wid;
            utf8::decode(s(i), wid);
            i += wid;
        } else {
            i++;
        }
        return s[0,i];
    }
    
    template <typename Func>
    str trim(str s, Func&& f) {
        return rtrim(ltrim(s, f), f);
    } 
    
    str trim(str s);
    str ltrim(str s);
    str rtrim(str s);
    
    template <typename T>
    std::string join(T const& t, str sep) {
        auto iter = begin(t);
        auto iterend = end(t);
        if (iter == iterend) {
            return "";
        }
        std::string s = *iter;
        ++iter;
        while (iter != iterend) {
            s += sep;
            s += *iter;
            ++iter;
        }
        return s;
    }
    
    template <typename T>
    String join(T const& t, str sep) {
        auto iter = begin(t);
        auto iterend = end(t);
        String s;
        if (iter == iterend) {
            return s;
        }
        s += *iter;
        ++iter;
        while (iter != iterend) {
            s += sep;
            s += *iter;
            ++iter;
        }
        return s;
    }

    inline String replace_all(str s, char old, char replacement) {
        String result = s;

        for (size i = 0; i < len(s); i++) {
            if (s[i] == old) {
                result[i] = replacement;
            }
        }

        return result;
    }
    
}