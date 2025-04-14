#pragma once

#include "lib/str.h"
#include "lib/testing/testing.h"
#include <functional>
namespace lib::testing {
    namespace detail {
        extern String *match_fuzz;
        // fuzzDuration     durationOrCountFlag
        // minimizeDuration = durationOrCountFlag{d: 60 * time.Second, allowZero: true}
        extern String *fuzz_cache_dir;
        extern bool *is_fuzz_worker;

        // FuzzWorkerExitCode is used as an exit code by fuzz worker processes after an
        // internal error. This distinguishes internal errors from uncontrolled panics
        // and other failures. Keep in sync with internal/fuzz.workerExitCode.
        const int FuzzWorkerExitCode = 70;

        // // corpusDir is the parent directory of the fuzz test's seed corpus within
        // // the package.
        // corpusDir = "testdata/fuzz"

        struct F {

        } ;

        void init_fuzz_flags();

        // runFuzzing runs the fuzz test matching the pattern for -fuzz. Only one such
        // fuzz test must match. This will run the fuzzing engine to generate and
        // mutate new inputs against the fuzz target.
        //
        // If fuzzing is disabled (-test.fuzz is not set), runFuzzing
        // returns immediately.
        bool run_fuzzing(TestDeps *deps, view<InternalFuzzTarget> fuzzTests);
    }
}