#pragma once

#include "assert.h"
#include "str.h"
#include "buf.h"
#include "assert.h"
#include "exceptions.h"
namespace lib {

    template <typename T, size N>
    struct Array {
        T data[N];

        Array<T, N>& operator = (const T (&arr)[N]) {
            memcpy(data, arr, N * sizeof(T));
            return *this;
        }

        constexpr T& operator [] (size n) {
            assert(n < N, exceptions::bad_index, n, N-1);
            return data[n];
        }

        constexpr T const& operator [] (size n) const {
            assert(n < N, exceptions::bad_index, n, N-1);
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
//             assert(usize(i) <= N, exceptions::bad_index, i, len);
//             return slice<T>(data+i, len-i);
//         }
//
//         slice<T> slice(size i, size j) const {
//             assert(usize(i) <= usize(len), exceptions::bad_index, i , len);
//             assert(i <= j, exceptions::bad_index, i, j);
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
            assert(usize(i) <= N, exceptions::bad_index, i , N);
            return buf(data+i, N-i);
        }

        constexpr buf slice(size i, size j) {
            assert(usize(i) <= N, exceptions::bad_index, i , N);
            assert(i <= j, exceptions::bad_index, i, j);
            return buf(data+i, j-i);
        }

        constexpr byte operator[] (size i) {
            assert(usize(i) < N, exceptions::bad_index, i);
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

        constexpr byte* end() {
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
    struct array {
        T       *arr;
        size     len;


        constexpr array() : arr(nullptr), len(0) {}
        constexpr array(T *t, usize len) : arr(t), len(len) {}

        template <size N>
        constexpr array(T (&arr)[N])  : arr(arr), len(N) { }

        template <size N>
        constexpr array(T (&&arr)[N]) : arr(arr), len(N) { }

        array slice(size i) const {
            assert(usize(i) <= usize(len), exceptions::bad_index, i, len);
            return array(arr+i, len-i);
        }

        array slice(size i, size j) const {
            assert(usize(i) <= usize(len), exceptions::bad_index, i , len);
            assert(i <= j, exceptions::bad_index, i, j);
            return array(arr+i, j-i);
        }

        array operator () (size i) const {
            return slice(i);
        }

        array operator () (size i, size j) const {
            return slice(i, j);
        }

        T& operator [] (size i) {
            assert(usize(i) < usize(len), exceptions::bad_index, i, len-1);
            return arr[i];
        }
        constexpr const T& operator [] (size i) const {
            assert(usize(i) < usize(len), exceptions::bad_index, i, len-1);
            return arr[i];
        }

        constexpr T* begin() {
            return arr;
        }

        constexpr const T* begin() const {
            return arr;
        }

        constexpr T* end() {
            return arr + len;
        }

        constexpr const T* end() const {
            return arr + len;
        }

    //     friend constexpr T *begin(array arr) { return arr.arr; }

    //     friend constexpr T *end(array arr) { return arr.arr + arr.len; }
    };

    template <typename T>
    constexpr size len(array<T> arr) {
        return arr.len;
    }

}
