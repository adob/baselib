#include "benchmark.h"
#include "lib/io/io.h"
#include "lib/runtime/debug.h"
#include "lib/testing/benchmark.h"

using namespace lib;
using namespace lib::testing;


struct {
	float64 v;
	String expected;
} pretty_print_tests[] ={
	{0, "         0 x"},
	{1234.1, "      1234 x"},
	{-1234.1, "     -1234 x"},
	{999.950001, "      1000 x"},
	{999.949999, "       999.9 x"},
	{99.9950001, "       100.0 x"},
	{99.9949999, "        99.99 x"},
	{-99.9949999, "       -99.99 x"},
	{0.000999950001, "         0.001000 x"},
	{0.000999949999, "         0.0009999 x"}, // smallest case
	{0.0000999949999, "         0.0001000 x"},
};

void test_pretty_print(testing::T &t) {
	for (auto &&tt : pretty_print_tests) {
		io::Buffer buf;
		detail::pretty_print(buf, tt.v, "x");
		if (tt.expected != buf.str()) {
			t.errorf("pretty_print(%v, \"x\"): expected %q, actual %q", tt.v, tt.expected, buf.str());
		}
	}
}

void test_result_string(testing::T &t) {
	// Test fractional ns/op handling
	testing::BenchmarkResult r = {
		.n = 100,
		.t = 240 * time::nanosecond,
	};
	if (r.ns_per_op() != 2) {
		t.errorf("ns_per_op: expected 2, actual %v", r.ns_per_op());
	}
    str want = "     100\t         2.400 ns/op";
    String got = r.string();
	if (want != got) {
		t.errorf("string: expected %q, actual %q", want, got);
	}

	// Test sub-1 ns/op (issue #31005)
	r.t = 40 * time::nanosecond;
    want = "     100\t         0.4000 ns/op";
    got = r.string();
	if (want != got) {
		t.errorf("string: expected %q, actual %q", want, got);
	}

	// Test 0 ns/op
	r.t = 0;
    want = "     100";
    got = r.string();
	if (want != got) {
		t.errorf("string: expected %q, actual %q", want, got);
	}
}

void test_run_parallel(testing::T &t) {
	if (testing::short_mode()) {
		t.skip("skipping in short mode");
	}
	testing::benchmark([&](testing::B &b) {
		std::atomic<uint32> procs = uint32(0);
		std::atomic<uint64> iters = uint64(0);
		b.set_parallelism(3);
		b.run_parallel([&](testing::PB &pb) {
			procs.fetch_add(1);
			while (pb.next()) {
				iters.fetch_add(1);
			}
		});
		if (uint32 want = uint32(3 * runtime::num_cpu()); procs.load() != want) {
			t.errorf("got %v procs, want %v", procs.load(), want);
		}
		if (iters.load() != uint64(b.n)) {
			t.errorf("got %v iters, want %v", iters.load(), b.n);
		}
	});
}

// func TestRunParallelFail(t *testing.T) {
// 	testing.Benchmark(func(b *testing.B) {
// 		b.RunParallel(func(pb *testing.PB) {
// 			// The function must be able to log/abort
// 			// w/o crashing/deadlocking the whole benchmark.
// 			b.Log("log")
// 			b.Error("error")
// 		})
// 	})
// }

// func TestRunParallelFatal(t *testing.T) {
// 	testing.Benchmark(func(b *testing.B) {
// 		b.RunParallel(func(pb *testing.PB) {
// 			for pb.Next() {
// 				if b.N > 1 {
// 					b.Fatal("error")
// 				}
// 			}
// 		})
// 	})
// }

// func TestRunParallelSkipNow(t *testing.T) {
// 	testing.Benchmark(func(b *testing.B) {
// 		b.RunParallel(func(pb *testing.PB) {
// 			for pb.Next() {
// 				if b.N > 1 {
// 					b.SkipNow()
// 				}
// 			}
// 		})
// 	})
// }

// func TestBenchmarkContext(t *testing.T) {
// 	testing.Benchmark(func(b *testing.B) {
// 		ctx := b.Context()
// 		if err := ctx.Err(); err != nil {
// 			b.Fatalf("expected non-canceled context, got %v", err)
// 		}

// 		var innerCtx context.Context
// 		b.Run("inner", func(b *testing.B) {
// 			innerCtx = b.Context()
// 			if err := innerCtx.Err(); err != nil {
// 				b.Fatalf("expected inner benchmark to not inherit canceled context, got %v", err)
// 			}
// 		})
// 		b.Run("inner2", func(b *testing.B) {
// 			if !errors.Is(innerCtx.Err(), context.Canceled) {
// 				t.Fatal("expected context of sibling benchmark to be canceled after its test function finished")
// 			}
// 		})

// 		t.Cleanup(func() {
// 			if !errors.Is(ctx.Err(), context.Canceled) {
// 				t.Fatal("expected context canceled before cleanup")
// 			}
// 		})
// 	})
// }

// func ExampleB_RunParallel() {
// 	// Parallel benchmark for text/template.Template.Execute on a single object.
// 	testing.Benchmark(func(b *testing.B) {
// 		templ := template.Must(template.New("test").Parse("Hello, {{.}}!"))
// 		// RunParallel will create GOMAXPROCS goroutines
// 		// and distribute work among them.
// 		b.RunParallel(func(pb *testing.PB) {
// 			// Each goroutine has its own bytes.Buffer.
// 			var buf bytes.Buffer
// 			for pb.Next() {
// 				// The loop body is executed b.N times total across all goroutines.
// 				buf.Reset()
// 				templ.Execute(&buf, "World")
// 			}
// 		})
// 	})
// }

// func TestReportMetric(t *testing.T) {
// 	res := testing.Benchmark(func(b *testing.B) {
// 		b.ReportMetric(12345, "ns/op")
// 		b.ReportMetric(0.2, "frobs/op")
// 	})
// 	// Test built-in overriding.
// 	if res.NsPerOp() != 12345 {
// 		t.Errorf("NsPerOp: expected %v, actual %v", 12345, res.NsPerOp())
// 	}
// 	// Test stringing.
// 	res.N = 1 // Make the output stable
// 	want := "       1\t     12345 ns/op\t         0.2000 frobs/op"
// 	if want != res.String() {
// 		t.Errorf("expected %q, actual %q", want, res.String())
// 	}
// }

// func ExampleB_ReportMetric() {
// 	// This reports a custom benchmark metric relevant to a
// 	// specific algorithm (in this case, sorting).
// 	testing.Benchmark(func(b *testing.B) {
// 		var compares int64
// 		for b.Loop() {
// 			s := []int{5, 4, 3, 2, 1}
// 			slices.SortFunc(s, func(a, b int) int {
// 				compares++
// 				return cmp.Compare(a, b)
// 			})
// 		}
// 		// This metric is per-operation, so divide by b.N and
// 		// report it as a "/op" unit.
// 		b.ReportMetric(float64(compares)/float64(b.N), "compares/op")
// 		// This metric is per-time, so divide by b.Elapsed and
// 		// report it as a "/ns" unit.
// 		b.ReportMetric(float64(compares)/float64(b.Elapsed().Nanoseconds()), "compares/ns")
// 	})
// }

// func ExampleB_ReportMetric_parallel() {
// 	// This reports a custom benchmark metric relevant to a
// 	// specific algorithm (in this case, sorting) in parallel.
// 	testing.Benchmark(func(b *testing.B) {
// 		var compares atomic.Int64
// 		b.RunParallel(func(pb *testing.PB) {
// 			for pb.Next() {
// 				s := []int{5, 4, 3, 2, 1}
// 				slices.SortFunc(s, func(a, b int) int {
// 					// Because RunParallel runs the function many
// 					// times in parallel, we must increment the
// 					// counter atomically to avoid racing writes.
// 					compares.Add(1)
// 					return cmp.Compare(a, b)
// 				})
// 			}
// 		})

// 		// NOTE: Report each metric once, after all of the parallel
// 		// calls have completed.

// 		// This metric is per-operation, so divide by b.N and
// 		// report it as a "/op" unit.
// 		b.ReportMetric(float64(compares.Load())/float64(b.N), "compares/op")
// 		// This metric is per-time, so divide by b.Elapsed and
// 		// report it as a "/ns" unit.
// 		b.ReportMetric(float64(compares.Load())/float64(b.Elapsed().Nanoseconds()), "compares/ns")
// 	})
// }