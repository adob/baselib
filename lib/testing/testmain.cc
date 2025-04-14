#include <fcntl.h>
#include <libelf.h>
#include <gelf.h>
#include <cxxabi.h>
#include <unistd.h>
#include <vector>
#include <dlfcn.h>

#include "lib/debug/debug.h"
#include "lib/fmt/fmt.h"
#include "lib/str.h"
#include "lib/strings/strings.h"
#include "testing.h"
#include "func.h"
#include "../strings.h"
#include "../base.h"
#include "../print.h"

using namespace lib;
using namespace testing;
using namespace detail;

static bool is_testing_func(str sym) {
    if (!strings::has_prefix(sym, "test_")) {
        return false;
    }

    //print "got", sym;

    // size paren_idx = strings::index(sym, '(');
    // if (paren_idx < 0) {
    //     return false;
    // }
    
    return strings::has_suffix(sym, "(lib::testing::T&)");
}

static std::vector<FuncData> get_test_funcs(error err) {
    std::vector<FuncData> test_funcs;

    std::vector<FuncData> all_funcs = get_all_funcs(err);
    if (err) return {};

    for (FuncData &data : all_funcs) {
        String demangled = demangle(data.name);

        if (is_testing_func(demangled)) {
            // print "adding test func %v %#x" % demangle(data.name), (uintptr) data.ptr;
            test_funcs.push_back(data);

            // testing::T t{};
            // ((testfunc)data.ptr)(t);
            // print "OK";
        }
    }

    return test_funcs;
}


int main(int argc, char *argv[]) {
    debug::init();
    testing::init();

    testing::detail::TestDeps deps;
    deps.match_string = [](str pat, str s, error) -> bool {
        return true;
    };
    deps.import_path = []() -> String {
        return "";
    };
    
    std::vector<FuncData> test_funcs = get_test_funcs(error::panic);
    testing::M m;
    m.deps = &deps;
    
    for (FuncData &data : test_funcs) {
        m.tests.push_back({
            .name = demangle(data.name),
            .f = (testfunc) data.ptr,
        });
    }

    return m.run();
}