module;
#include "func_impl.h"

export module lib.testing.func;
import <vector>;

export import lib.error;
export import lib.str;

export extern "C++" {
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

}
