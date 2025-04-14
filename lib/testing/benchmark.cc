#include "benchmark.h"

#include <unordered_map>

#include "lib/flag/flag.h"
#include "lib/io/io_stream.h"
#include "lib/math/math.h"
#include "lib/runtime/debug.h"
#include "lib/runtime/mstats.h"
#include "lib/sync/go.h"
#include "lib/sync/gang.h"
#include "lib/sync/mutex.h"
#include "lib/sync/waitgroup.h"
#include "lib/testing/matcher.h"
#include "lib/testing/testing.h"
#include "lib/time/time.h"
#include "lib/print.h"

using namespace lib;
using namespace lib::testing;
using namespace testing::detail;

String *detail::match_benchmarks;

static runtime::MemStats mem_stats;

// Global lock to ensure only one benchmark runs at a time.
static sync::Mutex benchmark_lock;

namespace {
	DurationOrCountFlag bench_time = {
		.d = 1 * time::second,
	};
}

static int predict_n(int64 goalns, int64 prev_iters, int64 prevns, int64 last) {
	if (prevns == 0) {
		// Round up to dodge divide by zero. See https://go.dev/issue/70709.
		prevns = 1;
	}

	// Order of operations matters.
	// For very fast benchmarks, prevIters ~= prevns.
	// If you divide first, you get 0 or 1,
	// which can hide an order of magnitude in execution time.
	// So multiply first, then divide.
	int64 n = goalns * prev_iters / prevns;
	// Run more iterations than we think we'll need (1.2x).
	n += n / 5;
	// Don't grow too fast in case we had timing errors previously.
	n = min(n, 100*last);
	// Be sure to run at least one more than last time.
	n = max(n, last+1);
	// Don't run more than 1e9 times. (This also keeps n in int range on 32 bit platforms.)
	n = min(n, int64(1e9));
	return int(n);
}

void detail::init_benchmark_flags() {
	match_benchmarks = flag::define<String>("test.bench", "", "run only benchmarks matching `regexp`");
}

bool B::loop() {
    B &b = *this;
    // print "loop", b.loop_n, b.n;

    if (b.loop_n != 0 && b.loop_n < b.n) {
		b.loop_n++;
		return true;
	}
	return b.loop_slow_path();
}

bool B::loop_slow_path() {
    B &b = *this;
    // print "slow path", b.loop_n, b.n;

    if (b.loop_n == 0) {
        // If it's the first call to b.Loop() in the benchmark function.
        // Allows more precise measurement of benchmark loop cost counts.
        // Also initialize b.N to 1 to kick start loop scaling.
        b.n = 1;
        b.loop_n = 1;
        b.reset_timer();
        // print "slow path ret 1";
        return true;
    }

    // Handles fixed iterations case
    if (b.bench_time.n > 0) {
        if (b.n < b.bench_time.n) {
        b.n = b.bench_time.n;
        b.loop_n++;
        return true;
        }
        b.stop_timer();
        return false;
    }
    // Handles fixed time case
    return b.stop_or_scale_bloop();
}

void lib::testing::B::reset_timer() {
    B &b = *this;

    if (b.extra.empty()) {
        // Allocate the extra map before reading memory stats.
        // Pre-size it to make more allocation unlikely.
        // b.extra = std::unordered_map<String, float64>(16);
		b.extra.reserve(16);
    } else {
        b.extra.clear();
    }
    if (b.timer_on) {
        runtime::read_mem_stats(&mem_stats);
        b.start_allocs = mem_stats.mallocs;
        b.start_bytes = mem_stats.total_alloc;
        b.start = time::now();
    } 
    b.duration = 0;
    b.net_allocs = 0;
    b.net_bytes = 0;
}

void B::stop_timer() {
  B &b = *this;

  if (b.timer_on) {
    b.duration += time::since(b.start);
	// print "INCREASED DURATION BY %v to %v" % time::since(b.start).nsecs, b.duration.nsecs;
    runtime::read_mem_stats(&mem_stats);	
    b.net_allocs += mem_stats.mallocs - b.start_allocs;
    b.net_bytes += mem_stats.total_alloc - b.start_bytes;
    b.timer_on = false;
  }
}

bool B::stop_or_scale_bloop() {
    B &b = *this;

    time::duration time_elapsed = time::since(b.start);
    if (time_elapsed >= b.bench_time.d) {
        // Stop the timer so we don't count cleanup time
        b.stop_timer();
        return false;
    }
    // Loop scaling
    int64 goalns = b.bench_time.d.nanoseconds();
    int64 prev_iters = int64(b.n);
    b.n = predict_n(goalns, prev_iters, time_elapsed.nanoseconds(), prev_iters);
	// print "b.n set to", b.n;
    b.loop_n++;
  	return true;
}

void B::start_timer() {
    B &b = *this;

    if (!b.timer_on) {
		runtime::read_mem_stats(&mem_stats);
		b.start_allocs = mem_stats.mallocs;
		b.start_bytes = mem_stats.total_alloc;
		b.start = time::now();
		b.timer_on = true;
	}
}

bool B::run1() {
    B &b = *this;

    if (b.bstate != nil) {
		// Extend maxLen, if needed.
		if (size n = len(b.name) + bstate->ext_len + 1; n > bstate->max_len) {
			bstate->max_len = n + 8; // Add additional slack to avoid too many jumps in size.
		}
	}

	sync::go([&]{
		b.run_n(1);
	}).join();

    b.run_n(1);
	if (b.has_failed) {
        String prefix = "";
        if (b.chatty != nil) {
            prefix = b.chatty->prefix();
        }
		fmt::fprintf(*b.w, "%s--- FAIL: %s\n%s", prefix, b.name, b.output);
		return false;
	}
	// Only print the output if we know we are not going to proceed.
	// Otherwise it is printed in processBench.
	b.mu.r_lock();
	bool finished = b.finished;
	b.mu.r_unlock();
	if (b.has_sub.load() || finished) {
		str tag = "BENCH";
		if (b.has_skipped) {
			tag = "SKIP";
		}
		if (b.chatty != nil && (len(b.output) > 0 || finished)) {
			b.trim_output();
            String prefix = "";
            if (b.chatty != nil) {
                prefix = b.chatty->prefix();
            }
			fmt::fprintf(*b.w, "%s--- %s: %s\n%s", prefix, tag, b.name, b.output);
		}
		return false;
	}
	return true;
}
void B::run_n(int n) {
    B &b = *this;
    sync::Lock lock(benchmark_lock);
    
	//ctx, cancelCtx := context.WithCancel(context.Background())
	defer d = [&] {
		b.run_cleanup(NormalPanic);
		b.check_races();
	};
	// Try to get a comparable environment for each run
	// by clearing garbage from previous runs.
	// runtime.GC()
	b.reset_races();
	b.n = n;
	// print "b.n", n;
	b.loop_n = 0;
	//b.ctx = ctx;
	//b.cancelCtx = cancelCtx

	b.parallelism = 1;
	b.reset_timer();
	b.start_timer();

	b.bench_func(b);
	
    b.stop_timer();
	b.previous_n = n;
	b.previous_duration = b.duration;
}

void lib::testing::B::run() {
    B &b = *this;

    // labelsOnce.Do(func() {
	// 	fmt.Fprintf(b.w, "goos: %s\n", runtime.GOOS)
	// 	fmt.Fprintf(b.w, "goarch: %s\n", runtime.GOARCH)
	// 	if b.importPath != "" {
	// 		fmt.Fprintf(b.w, "pkg: %s\n", b.importPath)
	// 	}
	// 	if cpu := sysinfo.CPUName(); cpu != "" {
	// 		fmt.Fprintf(b.w, "cpu: %s\n", cpu)
	// 	}
	// })
	if (b.bstate != nil) {
		// Running go test --test.bench
		b.bstate->process_bench(b); // Must call doBench.
	} else {
		// Running func Benchmark.
		b.do_bench();
	}
}

void B::launch() {
    B &b = *this;

    // // Signal that we're done whether we return normally
	// // or by FailNow's runtime.Goexit.
	// defer d = [&]{
	// 	b.signal.send(true);
	// };

	// b.Loop does its own ramp-up logic so we just need to run it once.
	// If b.loopN is non zero, it means b.Loop has already run.
	if (b.loop_n == 0) {
		// Run the benchmark for at least the specified amount of time.
		if (b.bench_time.n > 0) {
			// We already ran a single iteration in run1.
			// If -benchtime=1x was requested, use that result.
			// See https://golang.org/issue/32051.
			if (b.bench_time.n > 1) {
				b.run_n(b.bench_time.n);
			}
		} else {
			time::duration d = b.bench_time.d;
			for (int64 n = 1; !b.has_failed && b.duration < d && n < int64(1e9);) {
				// print "DURATION %d" % d.nanoseconds();
				int64 last = n;
				// Predict required iterations.
				int64 goalns = d.nanoseconds();
				int64 prev_iters = b.n;
				n = int64(predict_n(goalns, prev_iters, b.duration.nanoseconds(), last));
				b.run_n(int(n));
			}
		}
	}
	b.result = BenchmarkResult{b.n, b.duration, b.bytes, b.net_allocs, b.net_bytes, b.extra};
}
bool B::run(str name, std::function<void(B &)> const &f) {
    B &b = *this;
    
    // Since b has subbenchmarks, we will no longer run it as a benchmark itself.
	// Release the lock and acquire it on exit to ensure locks stay paired.
	b.has_sub.store(true);
	benchmark_lock.unlock();
	defer d1 = [&] { benchmark_lock.lock(); };

    String bench_name = b.name;
	bool ok = true, partial = false;
	if (b.bstate != nil) {
		std::tie(bench_name, ok, partial) = b.bstate->match->full_name(&b, name);
	}
	if (!ok) {
		return true;
	}
	// var pc [maxStackLen]uintptr
	// n := runtime.Callers(2, pc[:])
	B sub {};	
    // signal:  make(chan bool),
    sub.name =    bench_name;
    sub.parent =  &b;
    sub.level =   b.level + 1;
    //.creator: pc[:n],
    sub.w =       b.w;
    sub.chatty =  b.chatty;
    sub.bench =   true;
    
    // importPath: b.importPath,
    sub.bench_func =  f;
    sub.bench_time =  b.bench_time;
    sub.bstate =     b.bstate;
	if (partial) {
		// Partial name match, like -bench=X/Y matching BenchmarkX.
		// Only process sub-benchmarks, if any.
		sub.has_sub.store(true);
	}

	// if (b.chatty != nil) {
	// 	labelsOnce.Do(func() {
	// 		fmt.Printf("goos: %s\n", runtime.GOOS)
	// 		fmt.Printf("goarch: %s\n", runtime.GOARCH)
	// 		if b.importPath != "" {
	// 			fmt.Printf("pkg: %s\n", b.importPath)
	// 		}
	// 		if cpu := sysinfo.CPUName(); cpu != "" {
	// 			fmt.Printf("cpu: %s\n", cpu)
	// 		}
	// 	})

	// 	if !hideStdoutForTesting {
	// 		if b.chatty.json {
	// 			b.chatty.Updatef(benchName, "=== RUN   %s\n", benchName)
	// 		}
	// 		fmt.Println(benchName)
	// 	}
	// }

	if (sub.run1()) {
		sub.run();
	}
	b.add(sub.result);
	return !sub.has_failed;
}
void B::add(BenchmarkResult const &other) {
    B &b = *this;
    BenchmarkResult &r = b.result;

	// The aggregated BenchmarkResults resemble running all subbenchmarks as
	// in sequence in a single benchmark.
	r.n = 1;
	r.t += time::duration(other.ns_per_op());
	if (other.bytes == 0) {
		// Summing Bytes is meaningless in aggregate if not all subbenchmarks
		// set it.
		b.missing_bytes = true;
		r.bytes = 0;
	}
	if (!b.missing_bytes) {
		r.bytes += other.bytes;
	}
	r.mem_allocs += uint64(other.allocs_per_op());
	r.mem_bytes += uint64(other.alloced_bytes_per_op());
}

void B::trim_output() {
    B &b = *this;

    // The output is likely to appear multiple times because the benchmark
	// is run multiple times, but at least it will be seen. This is not a big deal
	// because benchmarks rarely print, but just in case, we trim it if it's too long.
	const int max_newlines = 10;
	for (int nl_count = 0, j = 0; j < len(b.output); j++) {
		if (b.output[j] == '\n') {
			nl_count++;
			if (nl_count >= max_newlines) {
				b.output = b.output[0,j];
                b.output += "\n\t... [output truncated]\n";
				break;
			}
		}
	}
}

void B::run_parallel(std::function<void(PB&)> body) {
	B &b = *this;
	if (b.n == 0) {
		return; // Nothing to do when probing.
	}
	// Calculate grain size as number of iterations that take ~100µs.
	// 100µs is enough to amortize the overhead and provide sufficient
	// dynamic load balancing.
	uint64 grain = 0;
	if (b.previous_n > 0 && b.previous_duration > 0) {
		grain = 100'000 * uint64(b.previous_n) / uint64(b.previous_duration.nanoseconds());
	}
	if (grain < 1) {
		grain = 1;
	}
	// We expect the inner loop and function call to take at least 10ns,
	// so do not do more than 100µs/10ns=1e4 iterations.
	if (grain > 10'000) {
		grain = 10'000;
	}

	sync::atomic<uint64> n;
	int num_procs = b.parallelism * runtime::num_cpu();
	sync::Gang g;
	
	for (int p = 0; p < num_procs; p++) {
		g.go([&] {
			PB pb = {
				.global_n = &n,
				.grain    = grain,
				.b_n      = uint64(b.n),
			};
			body(pb);
		});
	}
	g.join();
	if (n.load() <= uint64(b.n) && !b.failed()) {
		b.fatal("run_parallel: body exited without pb.Next() == false");
	}
}


int64 BenchmarkResult::ns_per_op() const {
    BenchmarkResult const& r = *this;
    auto v = r.extra.find("ns/op");
    if (v != r.extra.end()) {
        return int64(v->second);
    }
	if (r.n <= 0) {
		return 0;
	}
	return r.t.nanoseconds() / int64(r.n);
}
int64 BenchmarkResult::allocs_per_op() const {
    BenchmarkResult const& r = *this;
    auto v = r.extra.find("allocs/op");
    if (v != r.extra.end()) {
		return int64(v->second);
	}
	if (r.n <= 0) {
		return 0;
	}
	return int64(r.mem_allocs) / int64(r.n);
}

int64 BenchmarkResult::alloced_bytes_per_op() const {
    BenchmarkResult const& r = *this;
    auto v = r.extra.find("B/op");
    if (v != r.extra.end()) {
		return int64(v->second);
	}
	if (r.n <= 0) {
		return 0;
	}
	return int64(r.mem_bytes) / int64(r.n);
}

float64 BenchmarkResult::mb_per_sec() const {
    BenchmarkResult const& r = *this;
    auto v = r.extra.find("MB/s");
    if (v != r.extra.end()) {
		return v->second;
	}
	if (r.bytes <= 0 || r.t <= 0 || r.n <= 0) {
		return 0;
	}
	return (float64(r.bytes) * float64(r.n) / 1e6) / r.t.seconds();
}


static String benchmark_name(str name, int n){
	if (n != 1) {
		return fmt::sprintf("%s-%d", name, n);
	}
	return name;
}

void BenchState::process_bench(B &b) {
    BenchState &s = *this;
    // for i, procs := range cpuList {
        int i = 0;
        int procs = 1;
        int count_n = 1;
        int *count = &count_n;
        bool benchmark_memory_b = false;
        bool *benchmark_memory = &benchmark_memory_b;
        
		for (uint j = 0; j < *count; j++) {
			// runtime.GOMAXPROCS(procs)
			String bench_ame = benchmark_name(b.name, procs);

			// If it's chatty, we've already printed this information.
			if (b.chatty == nil) {
				fmt::fprintf(*b.w, "%-*s\t", s.max_len, bench_ame);
			}
			// Recompute the running time for all but the first iteration.
			if (i > 0 || j > 0) {
				B b {};				
                // signal: make(chan bool),
                b.name =   b.name,
                b.w =      b.w,
                b.chatty = b.chatty,
                b.bench =  true,
                
                b.bench_func = b.bench_func,
                b.bench_time = b.bench_time,
				
				b.run1();
			}
			BenchmarkResult r = b.do_bench();
			if (b.has_failed) {
				// The output could be very long here, but probably isn't.
				// We print it all, regardless, because we don't want to trim the reason
				// the benchmark failed.
                String prefix = "";
                if (b.chatty != nil) {
                    prefix = b.chatty->prefix();
                }
				fmt::fprintf(*b.w, "%s--- FAIL: %s\n%s", prefix, bench_ame, b.output);
				continue;
			}
			String results = r.string();
			if (b.chatty != nil) {
				fmt::fprintf(*b.w, "%-*s\t", s.max_len, bench_ame);
			}
			if (*benchmark_memory || b.show_alloc_result) {
				results += "\t";
                results += r.mem_string();
			}
			fmt::fprintln(*b.w, results);
			// Unlike with tests, we ignore the -chatty flag and always print output for
			// benchmarks since the output generation time will skew the results.
			if (len(b.output) > 0) {
				b.trim_output();
                String prefix = "";
                if (b.chatty != nil) {
                    prefix = b.chatty->prefix();
                }
				fmt::fprintf(*b.w, "%s--- BENCH: %s\n%s", prefix, bench_ame, b.output);
			}
			// if p := runtime.GOMAXPROCS(-1); p != procs {
			// 	fmt.Fprintf(os.Stderr, "testing: %s left GOMAXPROCS set to %d\n", benchName, p)
			// }
			if (b.chatty != nil && b.chatty->json) {
				b.chatty->update("", String(fmt::sprintf("=== NAME  %s\n", "")));
			}
		}
	// }
}

BenchmarkResult B::do_bench() {
    B &b = *this;

    sync::go([&] {
        b.launch();    
    }).join();

    return b.result;
}

void detail::pretty_print(io::Writer &w, float64 x, str unit) {
	// Print all numbers with 10 places before the decimal point
	// and small numbers with four sig figs. Field widths are
	// chosen to fit the whole part in 10 places while aligning
	// the decimal point of all fractional formats.
    str format;
	float64 y = math::abs(x);

	if (y == 0 || y >= 999.95) {
		format = "%10.0f %s";
    } else if (y >= 99.995) {
		format = "%12.1f %s";
	} else if (y >= 9.9995) {
		format = "%13.2f %s";
    } else if (y >= 0.99995) {
		format = "%14.3f %s";
    } else if (y >= 0.099995) {
		format = "%15.4f %s";
	} else if (y >= 0.0099995) {
		format = "%16.5f %s";
    } else if (y >= 0.00099995) {
		format = "%17.6f %s";
	} else {
		format = "%18.7f %s";
	}
	fmt::fprintf(w, format, x, unit);
}

String BenchmarkResult::string() const {
    BenchmarkResult const& r =*this;

    io::Buffer buf;
	fmt::fprintf(buf, "%8d", r.n);

	// Get ns/op as a float.
    auto result = r.extra.find("ns/op");
    bool ok = result != r.extra.end();
    float64 ns = 0;
    if (ok) {
        ns = result->second;
    } else {
		ns = float64(r.t.nanoseconds()) / float64(r.n);
	}
	if (ns != 0) {
		buf.write('\t');
		pretty_print(buf, ns, "ns/op");
	}

	if (float64 mbs = r.mb_per_sec(); mbs != 0) {
		fmt::fprintf(buf, "\t%7.2f MB/s", mbs);
	}

	// Print extra metrics that aren't represented in the standard
	// metrics.
	std::vector<String> extra_keys;
	for (auto & v: r.extra) {
        String const& key = v.first;
        if (key == "ns/op" || key == "MB/s" || key == "B/op" || key == "allocs/op") {
            // Built-in metrics reported elsewhere.
			continue;
        }
		
		extra_keys.push_back(key);
	}
    std::sort(extra_keys.begin(), extra_keys.end());
	
	for (String &k : extra_keys) {
		buf.write('\t', error::ignore);
		pretty_print(buf, r.extra.at(k), k);
	}

	return buf.to_string();
}

String BenchmarkResult::mem_string() const {
    BenchmarkResult const& r = *this;
    return fmt::sprintf("%8d B/op\t%8d allocs/op",
		r.alloced_bytes_per_op(), r.allocs_per_op());
}

bool detail::run_benchmarks(std::vector<FuncData> const &bench_funcs) {
	time::duration goal_duration = 1 * time::second;
    DurationOrCountFlag bench_time = {
        .d = goal_duration
    };

	auto match_string = [](str pat, str s, error) -> bool {
		return true;
	};

	int maxprocs = runtime::num_cpu();

	Matcher match = new_matcher(match_string, *match_benchmarks, "-test.bench", *skip);
    BenchState bstate = {
		.match = &match,
		.ext_len = len(benchmark_name("", maxprocs)),
	};

    auto bench_func = [&](B &b) {
        for (FuncData const &benchmark : bench_funcs) {
            b.run(benchmark.name, (void(*)(B&)) benchmark.ptr);
        }
    };

    B main {};
    main.w      = & ((io::Writer&) os::stdout),
    main.bench  = true,
    main.name   = "Main",
    main.bench_func = bench_func,
    main.bstate = &bstate,
    main.bench_time = bench_time;
	
	ChattyPrinter chatty_printer(main.w);

	if (verbose()) {
		main.chatty = &chatty_printer;
	}

    main.run_n(1);
	return !main.has_failed;
}

bool PB::next() {
	PB &pb = *this;
	if (pb.cache == 0) {
		uint64 n = pb.global_n->add(pb.grain);
		if (n <= pb.b_n) {
			pb.cache = pb.grain;
		} else if (n < pb.b_n+pb.grain) {
			pb.cache = pb.b_n + pb.grain - n;
		} else {
			return false;
		}
	}
	pb.cache--;
	return true;
}
bool detail::run_benchmarks(
	str import_path,
    std::function<bool(str pat, str s, error)> match_string,
    view<InternalBenchmark> benchmarks) 
{
	return true;
	panic("unimplemented");
	return false;
}

struct DiscardIO : io::Writer {
	size direct_write(str data, error err) override {
		return len(data);
	}
} ;

BenchmarkResult testing::benchmark(std::function<void(B &)> f) {
	B b;
	DiscardIO discard;
	b.w = &discard;
	b.bench_func = f;
	b.bench_time = bench_time;
	if (b.run1()) {
		b.run();
	}
	return b.result;
}
void B::set_parallelism(int p) {
	B &b = *this;
	if (p >= 1) {
		b.parallelism = p;
	}
}
