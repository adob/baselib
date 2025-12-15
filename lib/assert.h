//#pragma once

// #ifdef assert
//     #undef assert
// #endif

#if defined(NDEBUG)
    #define LIB_CHECK(...) (static_cast<void> (0))
#else
    #define LIB_CHECK(check, T, ...) ((check) ? (static_cast<void> (0)) : ( T(__VA_ARGS__)))
#endif

