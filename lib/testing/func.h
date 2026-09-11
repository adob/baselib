#pragma once

#include <vector>

import lib.error;
import lib.str;
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
