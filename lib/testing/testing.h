#pragma once

#include "../base.h"
#include "../fmt.h"
#include "../debug.h"

namespace lib::testing {
    struct T {
        struct ErrorInfo {
            String description;
            debug::Backtrace backtrace;
        };
        
        bool failed = false;
        std::vector<ErrorInfo> errors;
        
        template <typename... Args>
        void errorf(str format, Args&&... args) {
            String s = fmt::sprintf(format, std::forward<Args>(args)...);
            errors.push_back({
                std::move(s),
                //debug::backtrace(2) 
            });
            fail();
        }
        
        void fail();
        
        // void operator () (bool);
    } ; 
    
    using testfunc = void (*) (T&);
    
    struct TestCase {
        testfunc func;
        str      name;
        str      filename;
        int      lineno;
        
        TestCase(str name, testfunc func, str filename, int lineno);
    } ;
}