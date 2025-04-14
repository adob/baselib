#include <stdio.h>
#include "sharedlib/array.h"
#include "./quote.h"
#include "sharedlib/utf8.h"
#ifdef __GXX_RTTI
#include <typeinfo>
extern "C" {
    char*
    __cxa_demangle(const char* __mangled_name, char* __output_buffer,
                   size_t* __length, int* __status);
    
}
#endif

xxx

namespace lib::fmt { 
    

    
    struct Fmt : Opts {
        io::OStream& out;
        error err;
        
        Fmt(io::OStream& out, error err) : out(out), err(err) {}
        Fmt(io::OStream& out, error err, Opts const& opts) : Opts(opts), out(out), err(err) {}
        
        void pad(str s) {
            pad(s, len(s));
        }
        
        template<typename T>
        void pad(T s, size runecount){
            if (width == 0 || runecount >= width) {
                out.write(s, err);
                return;
            }
            bool left = !minus;
            size padamt = width - runecount;
            char padchar = zero ? '0' : ' ';
            
            if (left) {
                out.write_repeated(padchar, padamt, err);
                out.write(s, error::ignore());
            } else {
                out.write(s, error::ignore());
                out.write_repeated(padchar, padamt, err);
            }
        }
        
        void pad(io::WriterTo const& writable) {
            if (width == 0) {
                 writable.write_to(out, err);
                 return;
            }
            
            bool left = !minus;
            char padchar = zero ? '0' : ' ';
            
            if (left) {
                size runecount = utf8::count(writable);
                size padamt = width - runecount;
                if (padamt > 0) {
                    out.write_repeated(padchar, padamt, err);
                }
                writable.write_to(out, err);
            } else {
                utf8::RuneCounter counter(out);
                writable.write_to(counter, err);
                size padamt = width - counter.runecount;
                if (padamt > 0) {
                    out.write_repeated(padchar, padamt, err);
                }
            }
        }
        
        template <typename T>
        void write_integer(T n) {
            char buf[256];
            static const str ldigits = "0123456789abcdefghijklmnopqrstuvwxyz";
            static const str udigits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
            const str digits = upcase ? udigits : ldigits;
            char sep = comma ? ',' : apos ? '\'' : 0;
            int nsep = (base == 16 || base == 2) ? 5 : 4;
            
            bool negative = n < 0;
            if (negative)
                n = -n;
            
            size i = sizeof buf;
            size ncount = 0;
            if (n == 0) {
                buf[--i] = '0';
                ncount++;
            } else {
                do {
                    ncount++;
                    if (sep && (ncount % nsep) == 0) {
                        buf[--i] = sep;
                        ncount++;
                    }
                    buf[--i] = digits[n % base];
                    n /= base;
                } while (n > 0);
            }
                
            size pad_prec = prec > ncount ? prec - ncount : 0;
            size sharp_extra = !sharp ? 0
                                    : (base == 16 || base == 2) ? 2
                                        : base == 8 ? 1
                                            : 0;
            ncount += pad_prec + sharp_extra;
            if (negative||plus||space) ncount++;
            size pad_width = width > ncount ? width-ncount : 0;

            if (pad_width > 0) {
                char padchar = zero ? '0' : ' ';
                out.write_repeated(padchar, pad_width, err);
            }
            if (negative)
                out.write('-', error::ignore());
            else if (plus)
                out.write('+', error::ignore());
            else if (space)
                out.write(' ', error::ignore());
            if (sharp) {
                if (base == 8)       out.write('0', err);
                else if (base == 2)  out.write("0b", err);
                else if (base == 16) out.write("0x", err);
            }
            if (pad_prec > 0) {
                out.write_repeated('0', pad_prec, err);
            }
            out.write(str(buf+i, sizeof(buf)-i), err);
        }
        
        void write(char c, bool) {
            if (quoted) {
                pad(quote(c));
            } else {
                pad(c, 1);
            }
        }
        
        void write(char32_t c, bool) {
            if (quoted) {
                pad(quote(c));
            } else {
                Array<char, utf8::UTFMax> buf;
                pad(utf8::encode(c, buf));
            }
        }
        
        void write(char16_t c, bool) {
            write((char32_t) c, true);
        }
        
        void write(wchar_t c, bool) {
            write((char32_t) c, true);
        }
        
        void write(signed char n, bool) {
            write_integer<signed long long>(n);
        }
        
        void write(unsigned char n) {
            write_integer<unsigned long long>(n);
        }
        
        void write(signed short int n, bool) {
            write_integer<signed long long>(n);
        }
        
        void write(unsigned short int n, bool) {
            write_integer<unsigned long long>(n);
        }
        
        void write(signed int n, bool) {
            write_integer<signed long long>(n);
        }
        
        void write(unsigned int n, bool) {
            write_integer<unsigned long long>(n);
        }
        
        void write(signed long int n, bool) {
            write_integer<signed long long>(n);
        }
        
        void write(unsigned long int n, bool) {
            write_integer<unsigned long long>(n);
        }
        
        void write(signed long long int n, bool) {
            write_integer<signed long long>(n);
        }
        
        void write(unsigned long long int n, bool) {
            write_integer<unsigned long long>(n);
        }
        
//             void write (__int128 n) {
//                 write_integer(n);
//             }
//             
//             void write(unsigned __int128 n) {
//                 write_integer(n);
//             }
        
        void write(double n, bool) {
            char buf[256];
            int count = snprintf(buf, sizeof buf, "%g", n);
            pad(str(buf, count), count);
        }
        
        void write(float f, bool) {
            write((double) f, true);
        }
        
        void write(long double f, bool) {
            write((double)f, true);
        }
        
        void write(bool b, bool) {             
            b ? pad("true") : pad("false");
        }
        
        void write(str s, bool) {
            if (quoted) {
                pad(quote(s));
            } else {
                pad(s, utf8::count(s));
            }
        }
        
        void write(char *charp, bool) {
            write(str::from_c_str(charp), true);
        }
        
        void write(const char *charp, bool) {
            write(str::from_c_str(charp), true);
        }
        
        void write(std::string const& s, bool) {
            write(str(s), true);
        }
        
        void write(String const& s, bool) {
            write(str(s), true);
        }
    
        template < int > 
        struct EnableIf { 
            typedef void type; 
        };
        
        template <typename T>
        typename EnableIf< 
            sizeof (
                (void (T::*)(Fmt&) const) &T::fmt
            )
        >::type
        write(T const& t, bool) {
            t.fmt(*this);
        }
        
        template <typename T>
        void write(T *p, bool) {
                char buf[1024];
//             #ifdef __GXX_RTTI   
//                 int status;
//                 char *demangled = __cxa_demangle(typeid(p).name(), 0, 0, &status);
//                 int cnt = ::snprintf(buf, sizeof buf, "<%s at %p>", demangled, p);
//                 free(demangled);
//             #else
                int cnt = snprintf(buf, sizeof buf, "%p", p);
//             #endif
                write(str(buf, cnt), true);
        }
        
        
//         template <typename T>
//         void write_type(T const& t) {
//         #ifdef __GXX_RTTI
//             int status;
//             char *demangled = __cxa_demangle(typeid(t).name(), 0, 0, &status);
//             write(str(demangled));
//             free(demangled);
//         #else
//             write("???");
//         #endif
//         }
        
        
        #ifdef __GXX_RTTI
        void write(std::type_info const& tinfo, bool) {
            int status;
            char *demangled = __cxa_demangle(tinfo.name(), 0, 0, &status);
            write(str::from_c_str(demangled), true);
            free(demangled);
        }
        #endif
        
        
        template <typename T>
        void write(T const& t, ...) {
            #ifdef __GXX_RTTI
            int status;
            char *demangled = __cxa_demangle(typeid(t).name(), 0, 0, &status);
            write('<', true);
            write(str::from_c_str(demangled), true);
            free(demangled);
            write('>', true);
            #else
            (void) t;
            write("???", true);
            #endif
        }
        
        template <typename T>
        Fmt &operator << (T const& t) {
            write(t, true);
            return *this;
        }
        
    } ;
    
    
    
    template <size N, typename... Args> struct PrinterT;
    
    template <size N, typename Arg, typename... Args>
    struct PrinterT<N, Arg, Args...> : PrinterT<N+1, Args...> {
        Arg const& arg;
        
        PrinterT(Arg const& a, const Args&... args) : PrinterT<N+1, Args...>(args...), arg(a) {}
        
        bool doprint(size n, Fmt& out) {
            if (n == N) {
//                 if (out.type)
//                     out.write_type(arg);
//                 else
                    out.fmt_string(arg, true);
                return true;
            }
            return static_cast<PrinterT<N+1, Args...>>(*this).doprint(n, out);
        }
        
        size getsize() {
            return static_cast<PrinterT<N+1, Args...>>(*this).getsize();
        }
        
        size touint(int n, Fmt&) {
            return n;
        }
        
        template <typename T>
        size touint(T const&, Fmt& out) {
            char buf[256];
            ::snprintf(buf, sizeof buf, "<![can't convert arg %d to int]>", (int)N+1);
            out.write(buf, true);
            return 0;
        }
        
        size getuint(size n, Fmt& out) {
            if (n == N) {
                return touint(arg, out);
            }
            return static_cast<PrinterT<N+1, Args...>>(*this).getuint(n, out);
        }
    } ;
    
    template <size N>
    struct PrinterT<N> {
        
        bool doprint(size n, Fmt& out) {
            char buf[256];
            ::snprintf(buf, sizeof buf, "%%(MISSING ARG %d)", (int)n+1);
            out.write(buf, true);
            return false;
        }
        
        size getuint(size, Fmt& out) {
            out.write("<missing int arg>", true);
            return 0;
        }
        
        size getsize() { return N; }
    } ;
            
    template <typename... Args>
    String sprintf(str format, const Args &... args) {
        PrinterT<0, Args...> printer(args...);
        io::MemStream outstream;
        size argi  = 0;
        size lasti = 0;
        size i     = (size) -1;
        int state  = 0;
        bool percent = false;
        Fmt out(outstream, error::ignore());
        size pos = (size) -1;
        
        for (char c : format) {
            i++;
            if (c == '%') {
                outstream.write(format[lasti, i], error::ignore());
                lasti = i;
                percent = !percent;
                state = 0;
                out.flags     = 0;
                out.wid     = 0;
                out.prec      = 0;
                out.base      = 10;
                pos = (size) -1;
            } else if (percent) {
                switch (c) {
                    case 'X': 
                        out.upcase = true;
                        [[fallthrough]];
                    case 'x':
                        out.base = 16;
                        break;
                    case 'O':
                        out.upcase = true;
                        [[fallthrough]];
                    case 'o':
                        out.base = 8;
                        break;
                    case 'b':
                        out.base = 2;
                        break;
                    case 'q':
                        out.quoted = true;
                        break;
                    case 't':
                        out.type = true;
                        break;
                    case ',':
                        out.comma = true;
                        continue;
                    case '\'':
                        out.apos = true;
                        continue;
                    case '"':
                        out.quoted = true;
                        continue;
                    case '+':
                        out.plus = true;
                        continue;
                    case '-':
                        out.minus = true;
                        continue;
                    case '#':
                        out.sharp = true;
                        continue;
                    case ' ':
                        out.space = true;
                        continue;
                    case '.': 
                        state = 2; 
                        continue;
                    case '*':
                        if (state < 2) {
                            out.wid = printer.getuint(argi++, out);
                        } else {
                            out.prec = printer.getuint(argi++, out);
                        }
                        continue;
                    case '(':
                        continue;
                    case ')':
                        if (state < 2) {
                            out.wid = printer.getuint(out.wid, out);
                        } else {
                            out.prec  = printer.getuint(out.prec, out);
                        }
                        continue;
                    case '[':
                        pos = 0;
                        state = 3;
                        continue;
                    case ']':
                        continue;
                    case '0': 
                        if (state == 0) {
                            out.zero = true; 
                            continue;
                        }
                        [[fallthrough]];
                    default: 
                        if ('0' <= c && c <= '9') {
                            if (state == 0)
                                state = 1;
                            if (state == 1)
                                out.wid = out.wid*10 + c-'0';
                            else if (state == 2) {
                                out.prec_present = true;
                                out.prec = out.prec*10 + c-'0';
                            } else {
                                pos = pos*10 + c-'0';
                            }
                                
                        } else {
                            break;
                        }
                        continue;
                }
                printer.doprint(pos != (size) -1 ? pos : argi++, out);
                lasti = i + 1;
                percent = false;
            }
        }
        if (++i != lasti) {
            outstream.write(format[lasti, i], error::ignore());
        }
        return outstream.to_string();
    } // sprintf()
    

//     template <typename... Args>
//     str sprintf(Allocator& alloc, str format, const Args & ... args) {
//         return internal::sprintf(alloc, format, args...);
//     }

    template <typename... Args>
    void printf(str format, const Args & ... args) {
        fprintf(stdout, format, args...);
    }

//     template <typename T, typename... Args>
//     void writef(T& t, str format, const Args & ... args) {
//         std::string s;
//         str s = internal::sprintf(alloc, format, args...);
//         io::write(t, s);
//     }

    template <typename... Args>
    void fprintf(FILE *file, str format, const Args  & ... args) {
        String s = sprintf(format, args...);
        ::fwrite(s.buffer.data, 1, s.length, file);
        //io::write(file, s);
    }

//     template <typename... Args>
//     String sprintf(str format, const Args &... args) {
//         return sprintf(format, args...);
//     }

//     template <typename T>
//     std::string stringify(T const& t) {
//         return stringify(t, alloc);
//     }

    template <typename T>
    String stringify(T const& t) {
        io::MemStream s;
        Fmt printer(s, error::ignore());
        printer.fmt_string(t, true);
        
        return s.to_string();
    }
    
    template <typename T>
    void Stringifier<T>::write_to(io::OStream &out, error &err) const {
        Fmt printer(out, err);
        printer.fmt_string(t, true);
    }
    
    template <typename T>
    Stringifier<T> stringifier(T const& t) {
        return Stringifier<T>(t);
    }
} // namespace fmt




