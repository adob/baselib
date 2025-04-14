#pragma once

#include <vector>

#include "lib/error.h"
#include "lib/str.h"


namespace lib::testing {
    namespace detail {
        struct FuncData {
            String name;
            void *ptr;
        } ;

        std::vector<FuncData> get_all_funcs(error);
        String demangle(str mangled);
    }
}
