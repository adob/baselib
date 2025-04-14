#pragma once

#include "lib/testing/testing.h"
#include "lib/time/time.h"
#include "lib/sync/atomic.h"
#include "matcher.h"
#include "func.h"
#include <functional>
#include <unordered_map>
namespace lib::testing {
    struct B;

    namespace detail {
        extern String *match_benchmarks;

        struct DurationOrCountFlag {
            time::duration d;
            int            n = 0;
            bool           allow_zero = false;
        } ;



        struct BenchState {
            detail::Matcher *match = nil;

            size max_len = 0; // The largest recorded benchmark name.
            size ext_len = 0;// Maximum extension length.

            // process_bench runs bench b for the configured CPU counts and prints the results.
            void process_bench(B &b);
        } ;

        bool
        run_benchmarks(str import_path, std::function<bool(str pat, str s, error)> match_string,
                       view<InternalBenchmark> benchmarks);
        bool run_benchmarks(std::vector<FuncData> const&);
        void pretty_print(io::Writer &w, float64 x, str unit);
        void init_benchmark_flags();
    } ;

    struct BenchmarkResult {
        int n = 0;           // The number of iterations.
        time::duration t;         // The total time taken.
        int64 bytes = 0;         // Bytes processed in one iteration.
        uint64 mem_allocs = 0;        // The total number of memory allocations.
        uint64 mem_bytes = 0 ;      // The total number of bytes allocated.
    
        // // Extra records additional metrics reported by ReportMetric.
        std::unordered_map<String, float64> extra;

        int64 ns_per_op() const;
        
        // AllocsPerOp returns the "allocs/op" metric,
        // which is calculated as r.MemAllocs / r.N.
        int64 allocs_per_op() const;

        // AllocedBytesPerOp returns the "B/op" metric,
        // which is calculated as r.MemBytes / r.N.
        int64 alloced_bytes_per_op() const;

        // String returns a summary of the benchmark results.
        // It follows the benchmark result line format from
        // https://golang.org/design/14313-benchmark-format, not including the
        // benchmark name.
        // Extra metrics override built-in metrics of the same name.
        // String does not include allocs/op or B/op, since those are reported
        // by [BenchmarkResult.MemString].
        String string() const;

        // MemString returns r.AllocedBytesPerOp and r.AllocsPerOp in the same format as 'go test'.
        String mem_string() const;

        //private:
        float64 mb_per_sec() const;
    };

    // A PB is used by run_parallel for running parallel benchmarks.
    struct PB {
        sync::atomic<uint64> *global_n = nil; // shared between all worker goroutines iteration counter
	    uint64 grain = 0;        // acquire that many iterations from globalN at once
	    uint64 cache = 0;        // local cache of acquired iterations
	    uint64 b_n = 0;         // total number of iterations to execute (b.N)

        // Next reports whether there are more iterations to execute.
        bool next();
    } ;


    // B is a type passed to [Benchmark] functions to manage benchmark
    // timing and control the number of iterations.
    //
    // A benchmark ends when its Benchmark function returns or calls any of the methods
    // FailNow, Fatal, Fatalf, SkipNow, Skip, or Skipf. Those methods must be called
    // only from the goroutine running the Benchmark function.
    // The other reporting methods, such as the variations of Log and Error,
    // may be called simultaneously from multiple goroutines.
    //
    // Like in tests, benchmark logs are accumulated during execution
    // and dumped to standard output when done. Unlike in tests, benchmark logs
    // are always printed, so as not to hide output whose existence may be
    // affecting benchmark results.
    struct B : detail::Common {
        String namespace_path;
        detail::BenchState *bstate = nil;
        int n      = 0;
        int previous_n = 0;           // number of iterations in the previous run
	    time::duration previous_duration;  // total duration of the previous run
        std::function<void(B&)> bench_func;
        detail::DurationOrCountFlag bench_time;
        int bytes = 0;
        bool missing_bytes = false; // one of the subbenchmarks does not have bytes set.
        bool timer_on = false;
        bool show_alloc_result = false;
        BenchmarkResult result;
        int parallelism = 0; // RunParallel creates parallelism*GOMAXPROCS goroutines
        uint64 start_allocs = 0; // The initial states of memStats.Mallocs and memStats.TotalAlloc.
        uint64 start_bytes = 0;
        uint64 net_allocs = 0; // The net total of this test after being run.
	    uint64 net_bytes = 0;
        // Extra metrics collected by ReportMetric.
	    std::unordered_map<String, float64> extra = std::unordered_map<String, float64>(16);
        int loop_n = 0;
        


        // Loop returns true as long as the benchmark should continue running.
        //
        // A typical benchmark is structured like:
        //
        //	func Benchmark(b *testing.B) {
        //		... setup ...
        //		for b.Loop() {
        //			... code to measure ...
        //		}
        //		... cleanup ...
        //	}
        //
        // Loop resets the benchmark timer the first time it is called in a benchmark,
        // so any setup performed prior to starting the benchmark loop does not count
        // toward the benchmark measurement. Likewise, when it returns false, it stops
        // the timer so cleanup code is not measured.
        //
        // The compiler never optimizes away calls to functions within the body of a
        // "for b.Loop() { ... }" loop. This prevents surprises that can otherwise occur
        // if the compiler determines that the result of a benchmarked function is
        // unused. The loop must be written in exactly this form, and this only applies
        // to calls syntactically between the curly braces of the loop. Optimizations
        // are performed as usual in any functions called by the loop.
        //
        // After Loop returns false, b.N contains the total number of iterations that
        // ran, so the benchmark may use b.N to compute other average metrics.
        //
        // Prior to the introduction of Loop, benchmarks were expected to contain an
        // explicit loop from 0 to b.N. Benchmarks should either use Loop or contain a
        // loop to b.N, but not both. Loop offers more automatic management of the
        // benchmark timer, and runs each benchmark function only once per measurement,
        // whereas b.N-based benchmarks must run the benchmark function (and any
        // associated setup and cleanup) several times.
        bool loop();

        // run_parallel runs a benchmark in parallel.
        // It creates multiple goroutines and distributes b.N iterations among them.
        // The number of goroutines defaults to runtime::num_cpu(). To increase parallelism for
        // non-CPU-bound benchmarks, call [B.SetParallelism] before RunParallel.
        // RunParallel is usually used with the go test -cpu flag.
        //
        // The body function will be run in each goroutine. It should set up any
        // goroutine-local state and then iterate until pb.Next returns false.
        // It should not use the [B.StartTimer], [B.StopTimer], or [B.ResetTimer] functions,
        // because they have global effect. It should also not call [B.Run].
        //
        // RunParallel reports ns/op values as wall time for the benchmark as a whole,
        // not the sum of wall time or CPU time over each parallel goroutine.
        void run_parallel(std::function<void(PB&)> f);

        // StartTimer starts timing a test. This function is called automatically
        // before a benchmark starts, but it can also be used to resume timing after
        // a call to [B.StopTimer].
        void start_timer();

        // StopTimer stops timing a test. This can be used to pause the timer
        // while performing steps that you don't want to measure.
        void stop_timer();

        // ResetTimer zeroes the elapsed benchmark time and memory allocation counters
        // and deletes user-reported metrics.
        // It does not affect whether the timer is running.
        void reset_timer();

        // run benchmarks f as a subbenchmark with the given name. It reports
        // whether there were any failures.
        //
        // A subbenchmark is like any other benchmark. A benchmark that calls run at
        // least once will not be measured itself and will be called once with N=1.
        bool run(str name, std::function<void(B &)> const &body);


        // SetParallelism sets the number of goroutines used by [B.RunParallel] to p*GOMAXPROCS.
        // There is usually no need to call SetParallelism for CPU-bound benchmarks.
        // If p is less than 1, this call will have no effect.
        void set_parallelism(int p);

    protected:
        
        bool loop_slow_path();
        bool stop_or_scale_bloop();

        // runN runs a single benchmark for the specified number of iterations.
        void run_n(int n);

        // run1 runs the first iteration of benchFunc. It reports whether more
        // iterations of this benchmarks should be run.
        bool run1();

        // run executes the benchmark in a separate goroutine, including all of its
        // subbenchmarks. b must not have subbenchmarks.
        void run();

        // launch launches the benchmark function. It gradually increases the number
        // of benchmark iterations until the benchmark runs for the requested benchtime.
        // launch is run by the doBench function as a separate goroutine.
        // run1 must have been called on b.
        void launch();

        void add(BenchmarkResult const &other);

        // trimOutput shortens the output from a benchmark, which can be very long.
        void trim_output();

        BenchmarkResult do_bench();

        friend detail::BenchState;
        friend bool detail::run_benchmarks(const std::vector<detail::FuncData> &);
        friend BenchmarkResult testing::benchmark(std::function<void(B &)>);
    } ;


    // benchmark benchmarks a single function. It is useful for creating
    // custom benchmarks that do not use the "go test" command.
    //
    // If f depends on testing flags, then [Init] must be used to register
    // those flags before calling Benchmark and before calling [flag.Parse].
    //
    // If f calls Run, the result will be an estimate of running all its
    // subbenchmarks that don't call Run in sequence in a single benchmark.
    BenchmarkResult benchmark(std::function<void(B &)>);

    void run_benchmarks();
}