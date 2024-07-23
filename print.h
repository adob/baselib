#pragma once
#include <stdio.h>
#include "io.h"
#include "fmt.h"

namespace prettyprint {
    using namespace lib;
    
    struct PrintFormatted {
        fmt::State printer;
        
        PrintFormatted(io::OStream &f, str format) :  printer(f, format) {}
        
        template <typename T>
        PrintFormatted& operator , (T const &t) {
            printer << t;
            return *this;
        }
    } ;
    
    struct PrintUnformatted {
        io::OStream &out;
        PrintUnformatted(io::OStream &f) : out(f) {}
        
        template <typename T>
        PrintUnformatted& operator , (T const &t) {
            out.write(' ', error::ignore());
            fmt::write(out, t, error::ignore());
            return *this;
        }
    } ;
    
    struct PrintUndecided : PrintFormatted {
        
        PrintUndecided(io::StdStream &f, str format) : PrintFormatted(f, format) {}
        
        template <typename T>
        PrintFormatted& operator % (T const& t) {
            PrintFormatted &pf = *this;
            pf, t;
            return pf;
        }
        
        template <typename T>
        PrintUnformatted operator , (T const& t) {
            PrintUnformatted pu(printer.out);
            
            str format = str(printer.begin, printer.end - printer.begin);
            printer.out.write(format, error::ignore());
            
            pu, t;
            
            printer.begin = printer.end;
            return pu;
        }
        
        ~PrintUndecided() {
            str s(printer.begin, printer.end - printer.begin);
            if (s) {
                printer.out.write(s, error::ignore());
            }
        }

    };
    
    struct Print {
        io::StdStream &out;
        Print(io::StdStream &f) : out(f) {}
        
        template <size_t N>
        PrintUndecided operator * (const char (&str)[N]) {
            PrintUndecided p(out, str);
            return p;
        }
    
        
        template <typename T>
        PrintUnformatted operator * (T const& t) {
            PrintUnformatted p(out);
            p, t;
            return p;
        }
        
        ~Print() {
             out.write('\n', error::ignore());
        }

    } ;
}

#define print  (::prettyprint::Print(::lib::io::out))*
#define eprint (::prettyprint::Print(::lib::io::err))*
