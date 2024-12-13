#pragma once

#include <sstream>
#include <tuple>
#include <utility>

#include "../io/io_stream.h"

#ifdef __GXX_RTTI
#include <typeinfo>
extern "C" {
    char*
    __cxa_demangle(const char* __mangled_name, char* __output_buffer,
                   size_t* __length, int* __status);

}
#endif

namespace lib::fmt {
    // template <int>
    // struct EnableIf {
    //     typedef bool type;
    // };

    // template <typename T>
    // constexpr
    // EnableIf<sizeof (
    //     (void (T::*) (io::OStream&, error&) const) &T::fmt
    // )>::type
    // is_formattable(bool) {
    //     return true;
    // }

    template <typename T>
    constexpr bool is_formattable(...) { return false; }

    template <typename T>
    void write(io::OStream &out, T const &t, error err);

    template <typename... Args>
    void printf(str format, const Args &... args);

    template <typename... Args>
    void fprintf(FILE*, str format, const Args & ... args);

    template <typename... Args>
    void fprintf(io::OStream&, str format, const Args & ... args);

    // template <typename... Args>
    // String sprintf(str format, const Args & ... args);


    // stringifier
    template <typename T>
    struct Stringifier final : io::WriterTo  {
        T const& t;
        Stringifier(T const& t) : t(t) {}
        void write_to(io::OStream &out, error) const override;
    };

    template <typename T>
    Stringifier<T> stringifier(T const& t);

//
//     template <typename... Args>
//     String sprintf(str format, const Args & ... args);
//
//     template <typename T>
//     String stringify(T const& t);
//


    struct Fmt {
        io::OStream &out;
        error err;

        size width     = 0;
        int prec       = 0;
        int base       = 10;

        union {
            uint32 flags = 0;
            struct {
                bool upcase       : 1;
                bool plus         : 1;
                bool minus        : 1;
                bool zero         : 1;
                bool sharp        : 1;
                bool space        : 1;
                bool comma        : 1;
                bool apos         : 1;
                bool prec_present : 1;
                bool quoted       : 1;
                bool type         : 1;
            } ;
        } ;

        Fmt(io::OStream &out, error err) : out(out), err(err) {}

        void write(byte b);
        void write(char c);

        void write(bool b);

        void write(short i);
        void write(unsigned short i);
        void write(int i);
        void write(unsigned int i);
        void write(long i);
        void write(unsigned long i);
        void write(long long i);
        void write(unsigned long long i);

        void write(float f);
        void write(double d);

        void write(str s);
        void write(const char *);
        void write(char *);
        void write(const wchar_t *);
        void write(wchar_t *);
        void write(String const&);
        void write(io::WriterTo const&);


        template <typename T>
        void write_integer(T n, bool is_signed);

        template <typename T>
        void write(T const &t, ...) {
            constexpr bool is_c_str = requires {
                { t.c_str() } -> std::convertible_to<const char*>;
            };
            if constexpr (is_c_str) {
                write(t.c_str());
                return;
            }
            constexpr bool is_formattable1 = requires(const T& t2) {
                t.fmt(std::declval<io::OStream&>(), std::declval<error>());
            };
            if constexpr (is_formattable1) {
                struct WriterToWrapper : io::WriterTo {
                    const T &obj;
                    WriterToWrapper(T const& t) : obj(t) {}

                    void write_to(io::OStream &ostream, error err) const override {
                        obj.fmt(ostream, err);
                    }
                };
                WriterToWrapper w(t);
                write((io::WriterTo &) w);

                return;
            }
            constexpr bool is_formattable2 = requires(const T& t2) {
                t.fmt(std::declval<Fmt&>());
            };
            if constexpr (is_formattable2) {
                t.fmt(*this);
                return;
            }

            constexpr bool is_describable = requires {
                t.describe(std::declval<io::OStream&>);
            };
            if constexpr (is_describable) {
                // struct WriterToWrapper : io::WriterTo {
                //     const T &obj;
                //     WriterToWrapper(T const& t) : obj(t) {}

                //     void write_to(io::OStream &ostream, error) const override {
                //         obj.describe(ostream);
                //     }
                // };
                // WriterToWrapper w(t);
                // write((io::WriterTo &) w);
                t.describe(out);

                return;
            }

            constexpr bool is_to_utf8 = requires {
                t.toUtf8();
            };
            if constexpr (is_to_utf8) {
                auto utf8 = t.toUtf8();
                write(str(utf8.data(), utf8.length()));
                return;
            }
            
            using std::begin;
            using std::end;
            
            constexpr bool iterable = requires {
                begin(t);
                end(t);
            };
            if constexpr (iterable) {
                write("[");
                bool first = true;
                for (auto &&e : t) {
                    if (first) {
                        first = false;
                    } else {
                        write(", ");
                    }
                    write(e);
                }
                write("]");
                return;
            }
            
            constexpr bool ostream_printable = requires {
                std::declval<std::ostream>() << t;
            };
            if constexpr (ostream_printable) {
                std::stringstream ss;
                ss << t;
                write(ss.str());
                return;
            }
        #ifdef __GXX_RTTI
            int status;
            char *demangled = __cxa_demangle(typeid(t).name(), 0, 0, &status);
            this->write(str("<"));
            this->write(str::from_c_str(demangled));
            free(demangled);
            this->write(str(">"));
        #else
            (void) t;
            this->write(str("???"));
        #endif
        }

        template <typename T>
        Fmt &operator << (T const& t) {
            write(t);
            return *this;
        }
    };

    template <typename T>
    Stringifier<T> stringifier(T const& t) {
        return Stringifier<T>(t);
    }

    template <typename T>
    void Stringifier<T>::write_to(io::OStream &out, error err) const {
        fmt::write(out, t, err);
    }

    template <typename T>
    void write(io::OStream &out, T const &t, error err) {
        Fmt fmt(out, err);
        fmt.write(t);
    }

//     struct Opts {
//         size width     = 0;
//         int prec      = 0;
//         int base      = 10;
//
//         union {
//             uint32 flags = 0;
//             struct {
//                 bool upcase     : 1;
//                 bool plus       : 1;
//                 bool minus      : 1;
//                 bool zero       : 1;
//                 bool sharp      : 1;
//                 bool space      : 1;
//                 bool comma      : 1;
//                 bool apos       : 1;
//                 bool prec_present : 1;
//                 bool quoted     : 1;
//                 bool type       : 1;
//             } ;
//         } ;
//
//         Opts() : width(0), prec(0), base(10), flags(0) {}
//
//         struct Fmt : Opts {
//             io::OStream& out;
//             error err;
//
//             Fmt(io::OStream& out, error err) : out(out), err(err) {}
//             Fmt(io::OStream& out, error err, Opts const& opts) : Opts(opts), out(out), err(err) {}
//
//
//         private:
//             void pad(str s, size runecount);
//             void pad(io::WriteTo const&);
//
//             void write_int(int32);
//             void write_int(uint32);
//             void write_int(int64);
//             void write_int(uint64);
//
//         };
//     } ;

    struct State : Fmt {
        const char *begin;
        const char *end;

        State(io::OStream &out, str format);

        template <typename T>
        State &operator << (T const &t) {
            if (!advance()) {
                return *this;
            }

            write(t);
            return *this;
        }

        bool advance();
        void flush();
    };

    template <typename... Args>
    void printf(str format, const Args  & ... args) {
        fprintf(io::stdout, format, args...);
    }

    template <typename... Args>
    void fprintf(FILE *file, str format, const Args  & ... args) {
        io::StdStream out(file);
        fprintf(out, format, args...);
    }

    template <typename... Args>
    void fprintf(io::OStream &out, str format, const Args  & ... args) {
        State printer(out, format);

        (printer << ... << args).flush();
    }

    // template <typename... Args>
    // String sprintf(str format, const Args & ... args) {
    //   io::Buffer buffer;

    //   fprintf(buffer, format, args...);

    //   return buffer.to_string();
    // }

    // template <typename... Args>
    // void sprintf(String &s, str format, const Args & ... args) {
    //     io::StrStream sstream(s);
    //     fprintf(sstream, format, args...);
    // }

    namespace detail {

        template<typename Tuple, usize... I>
        void fprintf_apply(io::OStream &out, str format, Tuple &&t, std::index_sequence<I...>) {
            fmt::fprintf(out, format, std::get<I>(t)...);
        }
    }

    template <typename... Args>
    struct FmtWriterTo : io::WriterTo {
        str format;
        std::tuple<Args...> args;

        FmtWriterTo(str format, const Args & ... args) : format(format), args(args...) {}

        void write_to(io::OStream &out, error) const override {
            detail::fprintf_apply(
                out,
                format,
                args,
                std::make_index_sequence< 
                    std::tuple_size_v<std::tuple<Args...>>>());
        }

        operator String() {
             io::Buffer buffer;
             write_to(buffer, error::ignore);
             return buffer.to_string();
        }
    } ;

    template<typename... Args>
    auto sprintf(str format, const Args & ... args) {
        return FmtWriterTo<Args...>(format, args...);
    }
}

namespace lib {
    template<typename ...Args>
    void error::operator()(str f, const Args &... args) {
        auto writer = fmt::sprintf(f, args...);
        reporter.report(FormattedError(writer));
    }
}

// template <typename... Args>
// void lib::error::operator()(str fmt, const Args  & ...args) {
//     String s = fmt::sprintf(fmt, args...);
//     (*this)(s);
// }


//#include "fmt.inlines.h"

