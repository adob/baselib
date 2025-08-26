#pragma once

#include "assert.h"
#include "lib/types.h"
#include "buf.h"
#include "concepts.h"
#include "assert.h"
#include "exceptions.h"
#include <initializer_list>
namespace lib {

    template <typename T, size N>
    struct Array {
        T data[N];

        Array<T, N>& operator = (const T (&arr)[N]) {
            memcpy(data, arr, N * sizeof(T));
            return *this;
        }

        constexpr T& operator [] (size n) {
            LIB_CHECK(n < N, exceptions::bad_index, n, N-1);
            return data[n];
        }

        constexpr T const& operator [] (size n) const {
            LIB_CHECK(n < N, exceptions::bad_index, n, N-1);
            return data[n];
        }

        constexpr T* begin() {
            return data;
        }

        constexpr const T* begin() const {
            return data;
        }

        constexpr T* end() {
            return data + N;
        }

        constexpr const T* end() const {
            return data + N;
        }

//         slice<T> slice(size i) const {
//             LIB_CHECK(usize(i) <= N, exceptions::bad_index, i, len);
//             return slice<T>(data+i, len-i);
//         }
//
//         slice<T> slice(size i, size j) const {
//             LIB_CHECK(usize(j) <= usize(len), exceptions::bad_index, i , len);
//             LIB_CHECK(i <= j, exceptions::bad_index, i, j);
//             return slice(data+i, j-i);
//         }
//
//         operator buf () {
//             return buf(data, N);
//         }
//
//         operator str () const {
//             return str(data, N);
//         }

        constexpr bool operator==(Array<T, N> const& other) const {
            if (this == &other) {
                return true;
            }

            for (size i = 0; i < N; i++) {
                if (data[i] != other.data[i]) {
                    return false;
                }
            }

            return true;
        }
    } ;

    template<size N>
    struct Array<byte, N> {
        byte data[N];

        constexpr buf slice(size i) {
            LIB_CHECK(usize(i) <= N, exceptions::bad_index, i , N);
            return buf(data+i, N-i);
        }

        constexpr buf slice(size i, size j) {
            LIB_CHECK(usize(j) <= N, exceptions::bad_index, i , N);
            LIB_CHECK(i <= j, exceptions::bad_index, i, j);
            return buf(data+i, j-i);
        }

        constexpr byte &operator[] (size i) {
            LIB_CHECK(usize(i) < N, exceptions::bad_index, i, N);
            return data[i];
        }

        constexpr buf operator [] (size i, size j) {
            return slice(i, j);
        }

        constexpr buf operator + (size offset) {
            return slice(offset);
        }

        operator buf () {
            return buf(data, N);
        }

        operator str () const {
            return str(data, N);
        }

        constexpr byte* begin() {
            return data;
        }

        constexpr const byte* begin() const {
            return data;
        }

        constexpr byte* end() {
            return data + N;
        }

        constexpr const byte* end() const {
            return data + N;
        }
    };

    template<size N>
    struct Array<char, N> {
        char data[N];

        operator buf () {
            return buf(data, N);
        }

        operator str () const {
            return str(data, N);
        }
    };

    template <typename T, size N>
    constexpr size len(Array<T, N> const&) {
        return N;
    }


    template <typename T>
    struct arr {
        T       *data;
        size     len;

        constexpr arr() : data(nullptr), len(0) {}
        constexpr arr(T *t, usize len) : data(t), len(len) {}

        template <size N>
        constexpr arr(T (&arr)[N])  : data(arr), len(N) { }

        template <size N>
        constexpr arr(T (&&arr)[N]) : data(arr), len(N) { }

        template <concepts::SimpleVector V>
        arr(V &vec) : data(vec.data()), len(vec.size()) {}

        arr slice(size i) const {
            LIB_CHECK(usize(i) <= usize(len), exceptions::bad_index, i, len);
            return arr(data+i, len-i);
        }

        arr slice(size i, size j) const {
            LIB_CHECK(usize(j) <= usize(len), exceptions::bad_index, i , len);
            LIB_CHECK(i <= j, exceptions::bad_index, i, j);
            return arr(data+i, j-i);
        }

        arr operator () (size i) const {
            return slice(i);
        }

        arr operator () (size i, size j) const {
            return slice(i, j);
        }

        T& operator [] (size i) {
            LIB_CHECK(usize(i) < usize(len), exceptions::bad_index, i, len-1);
            return data[i];
        }
        constexpr const T& operator [] (size i) const {
            LIB_CHECK(usize(i) < usize(len), exceptions::bad_index, i, len-1);
            return data[i];
        }

        constexpr arr operator + (size offset) const {
            return slice(offset);
        }

        constexpr T* begin() {
            return data;
        }

        constexpr const T* begin() const {
            return data;
        }

        constexpr T* end() {
            return data + len;
        }

        constexpr const T* end() const {
            return data + len;
        }

        constexpr bool operator == (const arr<T> &other) const {
            if (len != other.len) {
                return false;
            }

            for (size i = 0; i < len; i++) {
                if (data[i] != other.data[i]) {
                    return false;
                }
            }

            return true;
        }
    };

    template <typename T>
    using view = arr<const T>;

    // template <typename T>
    // struct view : arr<const T> {

    //     constexpr view() {}
    //     constexpr view(const T *t, usize len) : arr<const T>(t, len) {}

    //     template <size N>
    //     constexpr view(const T (&data)[N])  : arr<const T>(data, N) {}

    //     constexpr view(std::initializer_list<T> list) : arr<const T>(list.begin(), list.size()) {}

    //     // view slice(size i) const {
    //     //     LIB_CHECK(usize(i) <= usize(len), exceptions::bad_index, i, len);
    //     //     return arr(data+i, len-i);
    //     // }

    //     // view slice(size i, size j) const {
    //     //     LIB_CHECK(usize(j) <= usize(len), exceptions::bad_index, i , len);
    //     //     LIB_CHECK(i <= j, exceptions::bad_index, i, j);
    //     //     return arr(data+i, j-i);
    //     // }

    //     // view operator () (size i) const {
    //     //     return slice(i);
    //     // }

    //     // view operator () (size i, size j) const {
    //     //     return slice(i, j);
    //     // }

    //     // T& operator [] (size i) {
    //     //     LIB_CHECK(usize(i) < usize(len), exceptions::bad_index, i, len-1);
    //     //     return data[i];
    //     // }
    //     // constexpr const T& operator [] (size i) const {
    //     //     LIB_CHECK(usize(i) < usize(len), exceptions::bad_index, i, len-1);
    //     //     return data[i];
    //     // }

    //     // constexpr view operator + (size offset) const {
    //     //     return slice(offset);
    //     // }

    //     // constexpr T* begin() {
    //     //     return data;
    //     // }

    //     // constexpr const T* begin() const {
    //     //     return data;
    //     // }

    //     // constexpr T* end() {
    //     //     return data + len;
    //     // }

    //     // constexpr const T* end() const {
    //     //     return data + len;
    //     // }

    //     // constexpr bool operator == (const arr<T> &other) const {
    //     //     if (len != other.len) {
    //     //         return false;
    //     //     }

    //     //     for (size i = 0; i < len; i++) {
    //     //         if (data[i] != other.data[i]) {
    //     //             return false;
    //     //         }
    //     //     }

    //     //     return true;
    //     // }
    // };

    // template <typename T>
    // struct view {
    //     const T  *data;
    //     size     len;


    //     constexpr view() : data(nullptr), len(0) {}
    //     constexpr view(const T *t, size len) : data(t), len(len) {}

    //     template <size N>
    //     constexpr view(const T (&data)[N])  : data(data), len(N) { }

    //     template <size N>
    //     constexpr view(T (&&data)[N]) : data(data), len(N) { }

    //     view slice(size i) const {
    //         LIB_CHECK(usize(i) <= usize(len), exceptions::bad_index, i, len);
    //         return view(data+i, len-i);
    //     }

    //     view slice(size i, size j) const {
    //         LIB_CHECK(usize(j) <= usize(len), exceptions::bad_index, i , len);
    //         LIB_CHECK(i <= j, exceptions::bad_index, i, j);
    //         return array(data+i, j-i);
    //     }

    //     view operator () (size i) const {
    //         return slice(i);
    //     }

    //     view operator () (size i, size j) const {
    //         return slice(i, j);
    //     }

    //     constexpr const T& operator [] (size i) const {
    //         LIB_CHECK(usize(i) < usize(len), exceptions::bad_index, i, len-1);
    //         return data[i];
    //     }

    //     constexpr view operator + (size offset) {
    //         return slice(offset);
    //     }

    //     constexpr view operator + (size offset) const {
    //         return slice(offset);
    //     }


    //     constexpr const T* begin() const {
    //         return data;
    //     }

    //     constexpr const T* end() const {
    //         return data + len;
    //     }

    // };

    template <typename T>
    constexpr size len(arr<T> arr) {
        return arr.len;
    }

    template <typename T>
    constexpr size len(view<T> arr) {
        return arr.len;
    }

    template <concepts::Sizeable T>
    constexpr size len(T const &t) {
        return t.size();
    }

}
