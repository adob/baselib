//#pragma once

#ifdef assert
    #undef assert
#endif

#if defined(NDEBUG)
    #define assert(arg) (static_cast<void> (0))
#else
    #define assert(check, T, ...) ((check) ? (static_cast<void> (0)) : ( T(__VA_ARGS__)))
#endif

