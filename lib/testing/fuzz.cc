#include "fuzz.h"
#include "lib/flag/flag.h"

using namespace lib;
using namespace lib::testing;
using namespace lib::testing::detail;

String *detail::match_fuzz;
String *detail::fuzz_cache_dir;
bool *detail::is_fuzz_worker;

bool detail::run_fuzzing(TestDeps *deps, view<InternalFuzzTarget> fuzzTests) {
    return true;
}
void lib::testing::detail::init_fuzz_flags() {
    match_fuzz = flag::define<String>("test.fuzz", "", "run the fuzz test matching `regexp`");
    fuzz_cache_dir = flag::define<String>("test.fuzzcachedir", "", "directory where interesting fuzzing inputs are stored (for use only by cmd/go)");
    is_fuzz_worker = flag::define<bool>("test.fuzzworker", false, "coordinate with the parent process to fuzz random values (for use only by cmd/go)");
}
