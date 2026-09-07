#pragma once

#include <cstddef>
#include <cstdio>

namespace lib::fallback {

template <typename = void>
inline const void *memrchr(const void *s, int c, std::size_t n) {
    auto *p = (const unsigned char *)s;

    while (n != 0) {
        --n;
        if (p[n] == (unsigned char)c) {
            return p + n;
        }
    }

    return nullptr;
}

template <typename = void>
inline int fileno(FILE *file) {
    if (file == stdin) {
        return 0;
    }
    if (file == stderr) {
        return 2;
    }
    return 1;
}

} // namespace lib::fallback

using lib::fallback::memrchr;
using lib::fallback::fileno;
