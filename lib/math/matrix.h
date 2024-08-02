#pragma once

#include "lib/base.h"
#include "lib/exceptions.h"
#include "lib/types.h"
#include "lib/io/io_stream.h"
#include "lib/fmt/fmt.h"

namespace lib::math {
    template <typename T, size N>
    struct Matrix {
        T data[N][N] = {};

        T& operator[] (size i, size j) {
            return data[i][j];
        }

        T operator[] (size i, size j) const {
            return data[i][j];
        }

        void mul(Matrix<T, N> const& m1, Matrix<T, N> const& m2) {
            for (size i = 0; i < N; i++) {
                for (size j = 0; j < N; j++) {
                    for (size k = 0; k < N; k++) {
                        (*this)[i, j] += m1[i, k] * m2[k, j];
                    }
                }
            }
        }

        Matrix<T, N> transposed() const {
            Matrix<T, N> m;

            for (size i = 0; i < N; i++) {
                for (size j = 0; j < N; j++) {
                    m[i, j] = (*this)[j, i];
                }
            }

            return m;
        }

        template <size M>
        Matrix<T, M> resized() const {
            Matrix<T, M> m;

            //assert(M <= N, exceptions::assertion());
            for (size i = 0; i < M; i++) {
                for (size j = 0; j < M; j++) {
                    m[i, j] = (*this)[i, j];
                }
            }

            return m;
        }

        void fmt(io::OStream &out, error &err) const {
            fmt::Fmt f(out, err);
            f.width = 20;
            //f.zero = true;

            for (size i = 0; i < N; i++) {
                f.write("[");
                for (size j = 0; j < N; j++) {
                    //f.write_integer((unsigned long) (*this)[i, j], false);
                    f.write((*this)[i, j]);
                }
                fmt::fprintf(out, "]\n");
            }
        }
    } ;
    template <typename T, size N>
    Matrix<T, N> operator * (Matrix<T, N> const& m1, Matrix<T, N> const& m2) {
        Matrix<T, N> m;
        m.mul(m1, m2);
        return m;
    }

    using Matrix4x4 = Matrix<float64, 4>;
    using Matrix3x3 = Matrix<float64, 3>;

}
