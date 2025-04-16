#pragma once

#include <sstream>
#include <tuple>
#include <utility>

#include "lib/io/io_stream.h"
#include "lib/errors/errors.h"
#include "lib/os/stdio.h"

#ifdef __GXX_RTTI
#include <typeinfo>

extern "C" {
    char*
    __cxa_demangle(const char* __mangled_name, char* __output_buffer,
                   size_t* __length, int* __status);

}
#endif

namespace lib::fmt {
    struct BufferedWriter : io::StaticBuffered<0, 512> {
        io::Writer &out;

        BufferedWriter(io::Writer &out) : out(out) {}
        virtual io::ReadResult direct_read(buf bytes, error err) override;
        virtual size           direct_write(str data, error err) override;
    } ;
    
    template <typename T>
    constexpr bool is_formattable(...) { return false; }

    template <typename T>
    void write(io::Writer &out, T const &t, error err);

    template <typename... Args>
    void printf(str format, const Args &... args);

    template <typename... Args>
    void fprintf(FILE*, str format, const Args & ... args);

    template <typename... Args>
    void fprintf(io::Writer&, str format, const Args & ... args);

    // template <typename... Args>
    // String sprintf(str format, const Args & ... args);


    // stringifier
    template <typename T>
    struct Stringifier final : io::WriterTo  {
        T const& t;
        Stringifier(T const& t) : t(t) {}

        void write_to(io::Writer &out, error) const override;

        operator String() {
            io::Buffer buffer;
            write_to(buffer, error::ignore);
            return buffer.to_string();
       }
    };

    template <typename T>
    Stringifier<T> stringify(T const& t);

    struct Fmt {
        io::Writer &out;
        error err;
        // bool erroring = false;

        int wid      = 0;
        int prec     = 0;
        int base      = 10;
        rune verb     = 'v';

        union {
            uint32 flags = 0;
            struct {
                bool wid_present  : 1;
                bool prec_present : 1;
                // bool upcase       : 1;
                bool minus        : 1;
                bool plus         : 1;
                bool zero         : 1;
                bool sharp        : 1;
                bool space        : 1;
                bool comma        : 1;
                bool apos         : 1;
                // bool type         : 1;
                
                // For the formats %+v %#v, we set the plusV/sharpV flags
                // and clear the plus/sharp flags since %+v and %#v are in effect
                // different, flagless formats set at the top level.
                bool plus_v  : 1;
                bool sharp_v : 1;
            } ;
        } ;

        Fmt(io::Writer &out, error err) : out(out), err(err) {}

        void write(unsigned char b);
        void write(char c);
        void write(char32_t c);

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
        void write_char(T n, bool is_signed);

        template <typename T>
        void fmt_integer(T n, bool is_signed, int base, char verb, str digits, bool sharp);

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
                t.fmt(std::declval<io::Writer&>(), std::declval<error>());
            };
            if constexpr (is_formattable1) {
                struct WriterToWrapper : io::WriterTo {
                    const T &obj;
                    WriterToWrapper(T const& t) : obj(t) {}

                    void write_to(io::Writer &w, error err) const override {
                        obj.fmt(w, err);
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

            // constexpr bool is_describable = requires {
            //     t.describe(std::declval<io::OStream&>);
            // };
            // if constexpr (is_describable) {
            //     // struct WriterToWrapper : io::WriterTo {
            //     //     const T &obj;
            //     //     WriterToWrapper(T const& t) : obj(t) {}

            //     //     void write_to(io::OStream &ostream, error) const override {
            //     //         obj.describe(ostream);
            //     //     }
            //     // };
            //     // WriterToWrapper w(t);
            //     // write((io::WriterTo &) w);
            //     t.describe(out);

            //     return;
            // }

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
                for (auto const &e : t) {
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
                { std::declval<std::stringstream>() << t } -> std::same_as<std::stringstream&>;
            };
            if constexpr (ostream_printable) {
                std::stringstream ss;
                ss << t;
                write("(ss)");
                write(ss.str());
                return;
            }

            if constexpr (std::is_pointer_v<T>) {
                if (t == nil) {
                    write("<nil>");
                    return;
                }
                if constexpr (!std::is_same_v<void*, T>) {
                    write(*t);
                    return;
                }
            }

            constexpr bool is_visitable = requires {
                visit([](auto &&) {}, t);
            } ;
            if constexpr (is_visitable) {
                visit([&](auto const &v) {
                    write(v);
                }, t);
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

        // template <typename... Args>
        // void writef(str format, const Args &... args);

        template <typename T>
        Fmt &operator << (T const& t) {
            write(t);
            return *this;
        }

        private:
        // fmt_q formats a string as a double-quoted, escaped string constant.
        // If .sharp is set a raw (backquoted) string may be returned instead
        // if the string does not contain any control characters other than tab.
        void fmt_q(str s);

        // fmtS formats a string.
        void fmt_s(str s);

        void fmt_w(io::WriterTo const &w);

        // fmtSx formats a string as a hexadecimal encoding of its bytes.
        void fmt_sx(str s, str digits);
        void fmt_wx(io::WriterTo const &w, str digits);

        // fmt_c formats an integer as a Unicode character.
        // If the character is not valid Unicode, it will print '\ufffd'.
        template <typename T>
        void fmt_c(T c);

        // fmt_qc formats an integer as a single-quoted, escaped Go character constant.
        // If the character is not valid Unicode, it will print '\ufffd'.
        template <typename T>
        void fmt_qc(T c);

        void fmt_qw(io::WriterTo const &);

        // fmt_unicode formats a uint64 as "U+0078" or with f.sharp set as "U+0078 'x'".
        template <typename T>
        void fmt_unicode(T c);

        // fmt_float formats a float64/float32. It assumes that verb is a valid format specifier
        // for strconv.AppendFloat and therefore fits into a byte.
        void fmt_float(std::variant<float32, float64> v, rune verb, int prec);
        
        void fmt_bad_int_arg();

        // truncate_string truncates the string s to the specified precision, if present.
        str truncate_string(str s);

        void write_padded(io::WriterTo const& writable);
        void write_padded(str s);

        // write_padding generates n bytes of padding.
        void write_padding(size n);
        void write_string(str s);
        void write_byte(byte b);
        void write_repeated(byte b, size count);
        void write_rune(rune r);
        
        void write_float(std::variant<float32,float64>);

        void bad_verb(rune r);
        void handle_star(int n);
    };

    template <typename T>
    Stringifier<T> stringify(T const& t) {
        return Stringifier<T>(t);
    }

    template <typename T>
    void Stringifier<T>::write_to(io::Writer &out, error err) const {
        fmt::write(out, t, err);
    }

    template <typename T>
    void write(io::Writer &out, T const &t, error err) {
        Fmt fmt(out, err);
        fmt.write(t);
    }

    struct State : Fmt {
        const char *begin;
        const char *end;

        State(io::Writer &out, str format, error);

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
        BufferedWriter bw(os::stdout);
        fprintf(bw, format, args...);
        bw.flush(error::ignore);
    }

    template <typename... Args>
    void fprintf(io::Writer &out, error err, str format, const Args  & ... args) {
        State printer(out, format, err);

        (printer << ... << args).flush();
    }

    template <typename... Args>
    void fprintf(io::Writer &out, str format, const Args  & ... args) {
        fprintf(out, error::ignore, format, args...);
    }

    template <typename... Args>
    void fprintf(FILE *file, str format, const Args  & ... args) {
        os::StdStream out(file, ::fileno(file));
        fprintf(out, format, args...);
    }

    // fprint()
    template <typename ...Args>
    void fprint(io::Writer &out, error err, const Args &...arg) {
        Fmt fmt(out, err);
        ( (fmt.write(arg), !err) && ...);
        // int i = 0;
        // ( ((i++ != 0 ? fmt.write(' '):void()), fmt.write(arg), !err) && ...);
    }

    // template <typename ...Args>
    // void fprint(io::Writer &out, const Args &...arg) {
    //     fprint(out, error(error::ignore), arg...);
    // }

    // cat()
    template <typename ...Args>
    void fcat(io::Writer &out, error err, const Args &...arg) {
        Fmt fmt(out, err);
        ( (fmt.write(arg), !err) && ...);
    }

    // print
    template <typename ...Args>
    void print(const Args &...arg) {
        fprint(os::stdout, error::ignore, arg...);
    }

    // fprintln
    template <typename ...Args>
    void fprintln(io::Writer &out, error err, const Args &...arg) {
        Fmt fmt(out, err);
        int i = 0;
        ( ((i++ != 0 ? fmt.write(' '):void()), fmt.write(arg), !err) && ...);
        fmt.write("\n");
    }

    template <typename ...Args>
    void fprintln(io::Writer &out, const Args &...arg) {
        fprintln(out, error(error::ignore), arg...);
    }

    // println
    template <typename ...Args>
    void println(const Args &...arg) {
        ffprint(os::stdout, error::ignore, arg...);
    }

     
    // template <typename... Args>
    // void Fmt::writef(str format, const Args  & ... args) {
    //     State printer(this->out, format, this->err);

    //     (printer << ... << args).flush();
    // }
    
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
        void fprintf_apply(io::Writer &out, error err, str format, Tuple &&t, std::index_sequence<I...>) {
            fmt::fprintf(out, err, format, *std::get<I>(t)...);
        }

        template<typename Tuple, usize... I>
        void sprintln_apply(io::Writer &out, error err, Tuple &&t, std::index_sequence<I...>) {
            fmt::fprintln(out, err, *std::get<I>(t)...);
        }

        template<typename Tuple, usize... I>
        void sprint_apply(io::Writer &out, error err, Tuple &&t, std::index_sequence<I...>) {
            fmt::fprint(out, err, *std::get<I>(t)...);
        }

        template<typename Tuple, usize... I>
        void cat_apply(io::Writer &out, error err, Tuple &&t, std::index_sequence<I...>) {
            fmt::fcat(out, err, *std::get<I>(t)...);
        }
    }

    template <typename... Args>
    struct FmtWriterTo : io::WriterTo {
        str format;
        std::tuple<const Args*...> args;

        FmtWriterTo(str format, const Args & ... args) : format(format), args(&args...) {}

        void write_to(io::Writer &out, error err) const override {
            detail::fprintf_apply(
                out,
                err,
                format,
                args,
                std::make_index_sequence< 
                    std::tuple_size_v<std::tuple<Args...>>>());
        }
    } ;

    template<typename... Args>
    FmtWriterTo<Args...> sprintf(str format, const Args & ... args) {
        return FmtWriterTo<Args...>(format, args...);
    }

    template <typename... Args>
    struct Sprintlner : io::WriterTo {
        std::tuple<const Args*...> args;

        Sprintlner(Args const & ...args) : args(&args...) {}

        void write_to(io::Writer &out, error err) const override {
            detail::sprint_apply(
                out,
                err,
                args,
                std::make_index_sequence< 
                    std::tuple_size_v<std::tuple<Args...>>>());
        }
    } ;

    template <typename... Args>
    struct Sprinter : io::WriterTo {
        std::tuple<const Args*...> args;

        Sprinter(Args const & ...args) : args(&args...) {}

        void write_to(io::Writer &out, error err) const override {
            detail::sprint_apply(
                out,
                err,
                args,
                std::make_index_sequence< 
                    std::tuple_size_v<std::tuple<Args...>>>());
        }
    } ;

    template <typename... Args>
    struct Catter : io::WriterTo {
        std::tuple<const Args*...> args;

        Catter(Args const & ...args) : args(&args...) {}

        void write_to(io::Writer &out, error err) const override {
            detail::cat_apply(
                out,
                err,
                args,
                std::make_index_sequence< 
                    std::tuple_size_v<std::tuple<Args...>>>());
        }
    } ;

    template <typename... Args>
    struct ErrorF : Error {
        str format;
        std::tuple<const Args*...> args;

        ErrorF(str format, const Args & ... args) : format(format), args(&args...) {}

        void fmt(io::Writer &out, error err) const override {
            detail::fprintf_apply(
                out,
                err,
                format,
                args,
                std::make_index_sequence< 
                    std::tuple_size_v<std::tuple<Args...>>>());
        }
    } ;

    // Sprintln formats using the default formats for its operands and returns the resulting string.
    // Spaces are always added between operands and a newline is appended.
    template<typename... Args>
    Sprintlner<Args...> sprintln(const Args & ... args) {
        return Sprintlner<Args...>(args...);
        
    }

    // sprint formats using the default formats for its operands and returns the resulting string.
    // Spaces are always added between operands and a newline is appended.
    template<typename... Args>
    Sprinter<Args...> sprint(const Args & ... args) {
        return Sprinter<Args...>(args...);
        
    }

    template <typename... Args>
    ErrorF<Args...> errorf(str format, Args const &... args) {
        return ErrorF(format, args...);
    }
    
    // cat formats using the default formats for its operands and returns the resulting string.
    // No spaces are added.
    template<typename... Args>
    Catter<Args...> cat(const Args & ... args) {
        return Catter<Args...>(args...);
        
    }
}


namespace lib {
    template<typename ...Args>
    void ErrorReporterInterface::report(str f, const Args &... args) {
        auto writer = fmt::sprintf(f, args...);
        this->report(errors::WriterToError(writer));
    }
}

// template <typename... Args>
// void lib::error::operator()(str fmt, const Args  & ...args) {
//     String s = fmt::sprintf(fmt, args...);
//     (*this)(s);
// }


//#include "fmt.inlines.h"

