#include "lib/debug/debug.h"
#include "lib/strings.h"
#include "benchmark.h"
#include "func.h"
#include "lib/io/io_stream.h"

#include "lib/print.h"
#include "lib/time/time.h"

using namespace lib;
using namespace testing;
using namespace testing::detail;

using BenchFunc = void (testing::B&);

static bool is_benchmark_func(str sym) {
    if (!strings::has_prefix(sym, "benchmark_")) {
        return false;
    }

    //print "got", sym;

    // size paren_idx = strings::index(sym, '(');
    // if (paren_idx < 0) {
    //     return false;
    // }
    
    return strings::has_suffix(sym, "(lib::testing::B&)");
}

static std::vector<FuncData> get_benchmark_funcs(error err) {
    std::vector<FuncData> test_funcs;

    std::vector<FuncData> all_funcs = get_all_funcs(err);
    if (err) return {};

    for (FuncData &data : all_funcs) {
        String demangled = demangle(data.name);

        if (is_benchmark_func(demangled)) {
            test_funcs.push_back(data);
        }
    }

    return test_funcs;
}

// static int predict_n(int64 goalns, int64 prev_iters, int64 prevns, int64 last) {
// 	if (prevns == 0) {
// 		// Round up to dodge divide by zero. See https://go.dev/issue/70709.
// 		prevns = 1;
// 	}

// 	// Order of operations matters.
// 	// For very fast benchmarks, prevIters ~= prevns.
// 	// If you divide first, you get 0 or 1,
// 	// which can hide an order of magnitude in execution time.
// 	// So multiply first, then divide.
// 	int64 n = goalns * prev_iters / prevns;
// 	// Run more iterations than we think we'll need (1.2x).
// 	n += n / 5;
// 	// Don't grow too fast in case we had timing errors previously.
// 	n = min(n, 100*last);
// 	// Be sure to run at least one more than last time.
// 	n = max(n, last+1);
// 	// Don't run more than 1e9 times. (This also keeps n in int range on 32 bit platforms.)
// 	n = min(n, int64(1e9));
// 	return int(n);
// }

// static void run_n(testing::B &b, int n, BenchFunc func) {
//     b.n = n;
// 	b.loop_n = 0;
// 	//b.ctx = ctx
// 	//b.cancelCtx = cancelCtx

// 	// b.parallelism = 1
// 	b.reset_timer();
// 	b.start_timer();

// 	//b.bench_func(b);
//     func(b);
	
//     b.stop_timer();
// 	b.previous_n = n;
// 	b.previous_duration = b.duration;
// }

// static void run_benchmark_func(testing::B &b, FuncData &data) {
//     time::duration goal_duration = 1 * time::second;
//     BenchState state = {};

//     b.name = data.name;
//     b.n = 1;
//     b.loop_n = 0;
//     b.reset_timer();
//     b.start_timer();
//     b.bench_time.d = goal_duration;

//     BenchFunc *func = (BenchFunc*) data.ptr;

//     for (int64 n = 1; !b.failed && b.duration < goal_duration && n < int64(1e9);) {
//         int64 last = n;
//         // Predict required iterations.
//         int64 goalns = goal_duration.nanoseconds();
//         int64 prev_iters = b.n;
//         n = int64(predict_n(goalns, prev_iters, b.duration.nanoseconds(), last));
//         print "predict_n", n;

//         run_n(b,int(n), func);
//         // break;
//     }

//     float64 nsops = float64(b.duration.nanoseconds()) / b.n;
//     printf("%'f ns/op\n", nsops);

//     //BenchmarkResult result = {b.n, b.duration};

// }

int main(int argc, char *argv[]) {
    debug::init();
    testing::init();

    std::vector<FuncData> bench_funcs = get_benchmark_funcs(error::panic);
    if (bench_funcs.size() == 0) {
        print "no benchmark functions found";
        return 0;
    }
    for (auto && f: bench_funcs) {
        String demangled = demangle(f.name);
        size paren = strings::index(demangled, '(');
        if (paren != -1) {
            demangled = demangled[0,paren];
        }
        f.name = demangled;
    }

    // time::duration goal_duration = 1 * time::second;
    // DurationOrCountFlag bench_time = {
    //     .d = goal_duration
    // };
    // BenchState bstate = {};

    // auto bench_func = [&](B &b) {
    //     for (FuncData &benchmark : bench_funcs) {
    //         b.run(benchmark.name, (void(*)(B&)) benchmark.ptr);
    //     }
    // };

    // B main {};
    // main.w      = & ((io::Writer&) io::stdout),
    // main.bench  = true,
    // main.name   = "Main",
    // main.bench_func = bench_func,
    // main.bstate = &bstate,
    // main.bench_time = bench_time,

    // main.run_n(1);
    bool ok = testing::detail::run_benchmarks(bench_funcs);
    if (!ok) {
        print "FAILED";
    } else {
        print "OK";
    }
    
    return 0;
}