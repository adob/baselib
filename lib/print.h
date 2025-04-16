#pragma once
#include <stdio.h>
#include "lib/error.h"
#include "lib/fmt/fmt.h"
#include "lib/os/stdio.h"

namespace prettyprint {
    using namespace lib;
    
    struct PrintFormatted {
        ::lib::fmt::State printer;
        IgnoringError error_handler;
        
        PrintFormatted(io::Writer &f, str format) :  printer(f, format, error_handler) {}
        
        template <typename T>
        PrintFormatted& operator , (T const &t) {
            printer << t;
            return *this;
        }
    } ;
    
    struct PrintUnformatted {
        io::Writer &out;
        PrintUnformatted(io::Writer &f) : out(f) {}
        
        template <typename T>
        PrintUnformatted& operator , (T const &t) {
            out.write(' ', error::ignore);
            ::lib::fmt::write(out, t, error::ignore);
            return *this;
        }
    } ;
    
    struct PrintUndecided : PrintFormatted {
        
        PrintUndecided(io::Writer &f, str format) : PrintFormatted(f, format) {}
        
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
            printer.out.write(format, error::ignore);
            
            pu, t;
            
            printer.begin = printer.end;
            return pu;
        }
        
        ~PrintUndecided() {
            str s(printer.begin, printer.end - printer.begin);
            if (s) {
                printer.out.write(s, error::ignore);
            }
        }

    };
    
    struct Print {
        fmt::BufferedWriter out;
        Print(io::Writer &f) : out(f) {}
        
        template <size_t N>
        PrintUndecided operator * (const char (&str)[N]) {
            PrintUndecided p(out, str);
            return p;
        }
    
        
        template <typename T>
        PrintUnformatted operator * (T const& t) {
            PrintUnformatted p(out);
            ::lib::fmt::write(out, t, error::ignore);
            return p;
        }
        
        ~Print() {
             out.write('\n', error::ignore);
             out.flush(error::ignore);
        }

    } ;
}

#define print  (::prettyprint::Print(::lib::os::stdout))*
#define eprint (::prettyprint::Print(::lib::os::stderr))*
