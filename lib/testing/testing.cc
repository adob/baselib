#include "testing.h"
#include "lib/io/io.h"
#include "lib/sync/map.h"
#include "matcher.h"
#include "example.h"
#include "benchmark.h"
#include "lib/runtime/debug.h"
#include "lib/runtime/symtab.h"
#include "lib/str.h"
#include "lib/sync/go.h"
#include "lib/sync/lock.h"
#include "lib/strings/strings.h"
#include "lib/flag/flag.h"
#include "lib/testing/fuzz.h"

using namespace lib;
using namespace testing;
using namespace testing::detail;

// https://cs.opensource.google/go/go/+/refs/tags/go1.24.1:src/testing/testing.go

// static bool init_ran = false;





namespace {
	static constexpr str marker = "\x16"; // ^V for framing

	struct ChattyFlag {
		bool on  = false; // -v is set in some form
		bool json = false; // -v=test2json is set, to make output better for test2json
	
	
		String prefix() {
			ChattyFlag &f = *this;
			if (f.json) {
				return str(marker);
			}
			return "";
		}
	} ;

	sync::atomic<int64> parallel_start; // number of parallel tests started
	sync::atomic<int64> parallel_stop; // number of parallel tests stopped

	// std::vector<int> cpu_list;
	// 	testlogFile *os.File

	sync::atomic<uint32> num_failed; // number of test failures

	sync::Map<String, time::time> running; // map[string]time.Time of running, unpaused tests
	// )

	bool testing_testing = false;

	struct {
		int errors() { return 0; }
	} race;
}

namespace lib::testing::detail {
	// 	// Flags, registered during Init.
	// short                *bool
	bool *fail_fast;
	// 	outputDir            *string
	ChattyFlag chatty;
	uint *count;
	// 	coverProfile         *string
	// 	gocoverdir           *string
	String	*match_list;
	String *match;
	String *skip;
	// 	memProfile           *string
	// 	memProfileRate       *int
	// 	cpuProfile           *string
	// 	blockProfile         *string
	// 	blockProfileRate     *int
	// 	mutexProfile         *string
	// 	mutexProfileFraction *int
	// 	panicOnExit0         *bool
	// 	traceFile            *string
	// 	timeout              *time.Duration
	// String	*cpu_list_str;
	int *parallel;
	String *shuffle;
	// 	testlog              *string
	bool *full_path;

	bool have_examples = false; // are there examples?
}

void testing::init() {
	chatty.on = true;

	fail_fast = flag::define<bool>("test.short", false, "run smaller test suite to save time");
	count = flag::define<uint>("test.count", 1, "run tests and benchmarks `n` times");
	match_list = flag::define<String>("test.list", "", "list tests, examples, and benchmarks matching `regexp` then exit");
	match = flag::define<String>("test.run", "", "run only tests and examples matching `regexp`");
	skip = flag::define<String>("test.run", "", "run only tests and examples matching `regexp`");
	parallel = flag::define<int>("test.parallel", runtime::num_cpu(), "run at most `n` tests in parallel");
	shuffle = flag::define<String>("test.shuffle", "off", "randomize the execution order of tests and benchmarks");
	full_path = flag::define<bool>("test.fullpath", false, "show full file names in error messages");
	// full_path = full_path_val;

	init_fuzz_flags();
	init_benchmark_flags();
}
        
// void T::fail() {
//     failed_ = true;
// }

// void T::fatal() {
//     failed_ = true;
//     panic("fatal");
// }

bool testing::short_mode() {
    return false;
}

// void T::operator () (bool b) {
//     if (!b) {
//         errors.push_back({"Failed", debug::backtrace(2) });
//         fail();
//     }
// }
void *Common::run_cleanup(PanicHandling ph) {
    Common &c = *this;
    c.cleanup_started.store(true);
	defer d1 = [&] {
        c.cleanup_started.store(false);
    };

	if (ph == RecoverAndReturnPanic) {
		// defer func() {
		// 	panicVal = recover()
		// }()
	}

	// Make sure that if a cleanup function panics,
	// we still run the remaining cleanup functions.
	defer d2 = [&] {
		c.mu.lock();
		bool recur = c.cleanups.size() > 0;
		c.mu.unlock();
		if (recur) {
			c.run_cleanup(NormalPanic);
		}
	};

	// if c.cancelCtx != nil {
	// 	c.cancelCtx()
	// }

	for (;;) {
		//var cleanup func()
        std::function<void()> cleanup;
		c.mu.lock();
		if (c.cleanups.size() > 0) {
			size last = c.cleanups.size() - 1;
			cleanup = c.cleanups[last];
			c.cleanups.pop_back();
		}
		c.mu.unlock();
		if (!cleanup) {
			//return nil
            return nil;
		}
		cleanup();
	}
}
bool Common::failed() const {
	Common const &c = *this;
	sync::RLock lock(c.mu);

	// if (!c.done && int64(race.Errors()) > c.last_race_errors.load()) {
	// 	// c.mu.RUnlock()
	// 	lock.unlock();
	// 	c.checkRaces();
	// 	c.relock();
	// 	// c.mu.RLock()
	// }

	return c.has_failed;
}
void Common::log_str(str s) {
	Common &c = *this;
	c.log_depth(s, 3); // logDepth + log + public function
}

void Common::log_depth(str s, int depth) {
	Common &c = *this;
	sync::WLock lock(c.mu);
	if (c.done) {
		// This test has already finished. Try and log this message
		// with our parent. If we don't have a parent, panic.
		for (Common *parent = c.parent; parent != nil; parent = parent->parent) {
			sync::WLock parent_lock(parent->mu);
			if (!parent->done) {
				parent->output += parent->decorate(s, depth+1);
				return;
			}
		}
		panic(fmt::sprintf("Log in goroutine after %s has completed: %s", c.name, s));
	} else {
		if (c.chatty != nil) {
			if (c.bench) {
				// Benchmarks don't print === CONT, so we should skip the test
				// printer and just print straight to stdout.
				fmt::print(c.decorate(s, depth+1));
			} else {
				c.chatty->print(c.name, c.decorate(s, depth+1));
			}

			return;
		}
		c.output += c.decorate(s, depth+1);
	}
}

void Common::fail_now(){
	Common &c = *this;
    // c.checkFuzzFn("FailNow")
	c.fail();

    // Calling runtime.Goexit will exit the goroutine, which
    // will run the deferred functions in this goroutine,
    // which will eventually run the deferred lines in tRunner,
    // which will signal to the test loop that this test is done.
    //
    // A previous version of this code said:
    //
    //	c.duration = ...
    //	c.signal <- c.self
    //	runtime.Goexit()
    //
    // This previous version duplicated code (those lines are in
    // tRunner no matter what), but worse the goroutine teardown
    // implicit in runtime.Goexit was not guaranteed to complete
    // before the test exited. If a test deferred an important cleanup
    // function (like removing temporary files), there was no guarantee
    // it would run on a test failure. Because we send on c.signal during
    // a top-of-stack deferred function now, we know that the send
    // only happens after any other stacked defers have completed.
	sync::WLock lock(c.mu);
	c.finished = true;
	sync::goexit();
} 
	
	
String Common::decorate(str s, int skip) {
	Common &c = *this;
	runtime::Frame frame = c.frame_skip(skip);
	String file = frame.file;
	int line = frame.line;
	if (file != "") {
		if (*full_path) {
			// If relative path, truncate file name at last file name separator.
		} else if (size index = strings::last_index_any(file, "/\\"); index >= 0) {
			file = file+(index+1);
		}
	} else {
		file = "???";
	}
	if (line == 0) {
		line = 1;
	}
	io::Buffer buf;
	// buf := new(strings.Builder)
	// Every line is indented at least 4 spaces.
	buf.write("    ");
	fmt::fprintf(buf, "%s:%d: ", file, line);
	std::vector<str> lines = strings::split(s, "\n");
	if (size l = lines.size(); l > 1 && lines[l-1] == "") {
		// lines = lines.sub[:l-1]
		// lines.erase(lines.begin()+l-1);
		lines.pop_back();
	}
	size i = 0;
	for (str line : lines) {
		if (i > 0) {
			// Second and subsequent lines are indented an additional 4 spaces.
			buf.write("\n        ");
		}
		buf.write(line);
	}
	buf.write('\n');
	return buf.to_string();
}

void Common::fail() {
	Common &c = *this;
	if (c.parent != nil) {
		c.parent->fail();
	}
	sync::WLock lock(c.mu);
	
	// c.done needs to be locked to synchronize checks to c.done in parent tests.
	if (c.done) {
		panic(fmt::sprintf("Fail in goroutine after %s has completed", c.name));
	}
	c.has_failed = true;
}

runtime::Frame Common::frame_skip(int skip) {
	return {};
	// // If the search continues into the parent test, we'll have to hold
	// // its mu temporarily. If we then return, we need to unlock it.
	// shouldUnlock := false
	// defer func() {
	// 	if shouldUnlock {
	// 		c.mu.Unlock()
	// 	}
	// }()
	// var pc [maxStackLen]uintptr
	// // Skip two extra frames to account for this function
	// // and runtime.Callers itself.
	// n := runtime.Callers(skip+2, pc[:])
	// if n == 0 {
	// 	panic("testing: zero callers found")
	// }
	// frames := runtime.CallersFrames(pc[:n])
	// var firstFrame, prevFrame, frame runtime.Frame
	// for more := true; more; prevFrame = frame {
	// 	frame, more = frames.Next()
	// 	if frame.Function == "runtime.gopanic" {
	// 		continue
	// 	}
	// 	if frame.Function == c.cleanupName {
	// 		frames = runtime.CallersFrames(c.cleanupPc)
	// 		continue
	// 	}
	// 	if firstFrame.PC == 0 {
	// 		firstFrame = frame
	// 	}
	// 	if frame.Function == c.runner {
	// 		// We've gone up all the way to the tRunner calling
	// 		// the test function (so the user must have
	// 		// called tb.Helper from inside that test function).
	// 		// If this is a top-level test, only skip up to the test function itself.
	// 		// If we're in a subtest, continue searching in the parent test,
	// 		// starting from the point of the call to Run which created this subtest.
	// 		if c.level > 1 {
	// 			frames = runtime.CallersFrames(c.creator)
	// 			parent := c.parent
	// 			// We're no longer looking at the current c after this point,
	// 			// so we should unlock its mu, unless it's the original receiver,
	// 			// in which case our caller doesn't expect us to do that.
	// 			if shouldUnlock {
	// 				c.mu.Unlock()
	// 			}
	// 			c = parent
	// 			// Remember to unlock c.mu when we no longer need it, either
	// 			// because we went up another nesting level, or because we
	// 			// returned.
	// 			shouldUnlock = true
	// 			c.mu.Lock()
	// 			continue
	// 		}
	// 		return prevFrame
	// 	}
	// 	// If more helper PCs have been added since we last did the conversion
	// 	if c.helperNames == nil {
	// 		c.helperNames = make(map[string]struct{})
	// 		for pc := range c.helperPCs {
	// 			c.helperNames[pcToName(pc)] = struct{}{}
	// 		}
	// 	}
	// 	if _, ok := c.helperNames[frame.Function]; !ok {
	// 		// Found a frame that wasn't inside a helper function.
	// 		return frame
	// 	}
	// }
	// return firstFrame
}

void Common::skip_now() {
	Common &c = *this;
	// c.checkFuzzFn("SkipNow")
	sync::WLock lock(c.mu);
	c.has_skipped = true;
	c.finished = true;
	lock.unlock();

	sync::goexit();
}

static void list_tests(std::function<bool(str pat, str s, error)> &match_String, view<InternalTest> tests, view<InternalBenchmark> benchmarks, view<InternalFuzzTarget>, view<InternalExample> examples) {
	// if _, err := matchString(*matchList, "non-empty"); err != nil {
	// 	fmt.Fprintf(os.Stderr, "testing: invalid regexp in -test.list (%q): %s\n", *matchList, err)
	// 	os.Exit(1)
	// }

	// for _, test := range tests {
	// 	if ok, _ := matchString(*matchList, test.Name); ok {
	// 		fmt.Println(test.Name)
	// 	}
	// }
	// for _, bench := range benchmarks {
	// 	if ok, _ := matchString(*matchList, bench.Name); ok {
	// 		fmt.Println(bench.Name)
	// 	}
	// }
	// for _, fuzzTarget := range fuzzTargets {
	// 	if ok, _ := matchString(*matchList, fuzzTarget.Name); ok {
	// 		fmt.Println(fuzzTarget.Name)
	// 	}
	// }
	// for _, example := range examples {
	// 	if ok, _ := matchString(*matchList, example.Name); ok {
	// 		fmt.Println(example.Name)
	// 	}
	// }
}

// static void parse_cpu_list() {
// 	for (str val : strings::split(*cpu_list_str, ",")) {
// 		panic("unimplemented");
// 		// val = strings::trim_space(val)
// 		// if val == "" {
// 		// 	continue
// 		// }
// 		// cpu, err := strconv.Atoi(val)
// 		// if err != nil || cpu <= 0 {
// 		// 	fmt.Fprintf(os.Stderr, "testing: invalid value %q for -test.cpu\n", val)
// 		// 	os.Exit(1)
// 		// }
// 		// cpuList = append(cpuList, cpu)
// 	}
// 	if (len(cpu_list) == 0) {
// 		cpu_list.push_back(runtime::num_cpu());
// 	}
// }


static bool should_fail_fast() {
	return *fail_fast && num_failed.load() > 0;
}

static TestState new_test_state(int max_parallel, Matcher *m)  {
	return {
		.match =         m,
		// .start_parallel = make(chan bool),
		.running      =   1, // Set the count to 1 for the main (sequential) test.
		.max_parallel =   max_parallel,
	};
}

// static ChattyPrinter new_chatty_printer(io::Writer *w)  {
// 	return &chattyPrinter{w: w, json: chatty.json}
// }

// callerName gives the function name (qualified with a package path)
// for the caller after skip frames (where 0 means the current function).
static String caller_name(int skip) {
	// var pc [1]uintptr
	// n := runtime.Callers(skip+2, pc[:]) // skip + runtime.Callers + callerName
	// if n == 0 {
	// 	panic("testing: zero callers found")
	// }
	// return pcToName(pc[0])
	return "caller name";
}


void detail::t_runner(T &t, std::function<void(T&)> const &fn) {
	t.runner = caller_name(0);

	// When this goroutine is done, either because fn(t)
	// returned normally or because a test failure triggered
	// a call to runtime.Goexit, record the duration and send
	// a signal saying that the test is done.
	defer d1 = [&] {
		t.check_races();

		// TODO(#61034): This is the wrong place for this check.
		if (t.failed()) {
			num_failed.add(1);
		}

		// Check if the test panicked or Goexited inappropriately.
		//
		// If this happens in a normal test, print output but continue panicking.
		// tRunner is called in its own goroutine, so this terminates the process.
		//
		// If this happens while fuzzing, recover from the panic and treat it like a
		// normal failure. It's important that the process keeps running in order to
		// find short inputs that cause panics.
		
		// err := recover()
		void *err = nil;
		bool signal = true;

		sync::RLock lock(t.mu);
		bool finished = t.finished;
		lock.unlock();
		if (!finished && err == nil) {
			// err = errNilPanicOrGoexit
			for (Common *p = t.parent; p != nil; p = p->parent) {
				sync::RLock lock(p->mu);
				finished = p->finished;
				lock.unlock();
				if (finished) {
					if (!t.is_parallel) {
						t.errorf("%v: subtest may have called FailNow on a parent test", err);
						err = nil;
					}
					signal = false;
					break;
				}
			}
		}

		if (err != nil && t.tstate->is_fuzzing) {
			panic("unimplemented");
			// prefix := "panic: "
			// if err == errNilPanicOrGoexit {
			// 	prefix = ""
			// }
			// t.Errorf("%s%s\n%s\n", prefix, err, string(debug.Stack()))
			// t.mu.Lock()
			// t.finished = true
			// t.mu.Unlock()
			// err = nil
		}

		// Use a deferred call to ensure that we report that the test is
		// complete even if a cleanup function calls t.FailNow. See issue 41355.
		bool did_panic = false;
		defer d2 = [&] {
			// Only report that the test is complete if it doesn't panic,
			// as otherwise the test binary can exit before the panic is
			// reported to the user. See issue 41479.
			if (did_panic) {
				return;
			}
			if (err != nil) {
				//panic(err);
				panic("unimplemented");
			}
			running.del(t.name);
			if (t.is_parallel) {
				parallel_stop.add(1);
			}
			t.signal.send(signal);
		};

		auto do_panic = [](void *any) {
			panic("unimplemented");
			// t.Fail()
			// if r := t.runCleanup(recoverAndReturnPanic); r != nil {
			// 	t.Logf("cleanup panicked with %v", r)
			// }
			// // Flush the output log up to the root before dying.
			// for root := &t.common; root.parent != nil; root = root.parent {
			// 	root.mu.Lock()
			// 	root.duration += highPrecisionTimeSince(root.start)
			// 	d := root.duration
			// 	root.mu.Unlock()
			// 	root.flushToParent(root.name, "--- FAIL: %s (%s)\n", root.name, fmtDuration(d))
			// 	if r := root.parent.runCleanup(recoverAndReturnPanic); r != nil {
			// 		fmt.Fprintf(root.parent.w, "cleanup panicked with %v", r)
			// 	}
			// }
			// didPanic = true
			// panic(err)
		};
		if (err != nil) {
			do_panic(err);
		}

		t.duration += time::since(t.start);

		if (len(t.sub) > 0) {
			// Run parallel subtests.

			// Decrease the running count for this test and mark it as no longer running.
			t.tstate->release();
			running.del(t.name);

			// Release the parallel subtests.
			t.barrier.close();
			// Wait for subtests to complete.
			for (T *sub : t.sub) {
				sub->signal.recv();
			}

			// Run any cleanup callbacks, marking the test as running
			// in case the cleanup hangs.
			time::time cleanup_start = time::now();
			running.store(t.name, cleanup_start);
			void *err = t.run_cleanup(RecoverAndReturnPanic);
			t.duration += time::since(cleanup_start);
			if (err != nil) {
				do_panic(err);
			}
			t.check_races();
			if (!t.is_parallel) {
				// Reacquire the count for sequential tests. See comment in Run.
				t.tstate->wait_parallel();
			}
		} else if (t.is_parallel) {
			// Only release the count for this test if it was run as a parallel
			// test. See comment in Run method.
			t.tstate->release();
		}
		t.report(); // Report after all subtests have finished.

		// Do not lock t.done to allow race detector to detect race in case
		// the user does not appropriately synchronize a goroutine.
		t.done = true;
		if (t.parent != nil && !t.has_sub.load()) {
			t.set_ran();
		}
	};
	defer d2 = [&] {
		if (len(t.sub) == 0) {
			t.run_cleanup(NormalPanic);
		}
	};

	t.start = time::now();
	t.reset_races();
	fn(t);

	// code beyond here will not be executed when FailNow is invoked
	t.mu.lock();
	t.finished = true;
	t.mu.unlock();
}


std::tuple<bool, bool> detail::run_tests(
		std::function<bool(str pat, str s, error)> match_string,
		view<InternalTest> tests,
		time::time deadline ) {
	
	bool ran = false;
	bool ok = true;
	// for (int procs : cpu_list) {
		// runtime.GOMAXPROCS(procs)
	for (uint i = uint(0); i < *count; i++) {
		if (should_fail_fast()) {
			break;
		}
		if (i > 0 && !ran) {
			// There were no tests to run on the first
			// iteration. This won't change, so no reason
			// to keep trying.
			break;
		}
		// ctx, cancelCtx := context.WithCancel(context.Background())
		Matcher matcher = new_matcher(match_string, *match, "-test.run", *skip);
		TestState tstate = new_test_state(*parallel, &matcher);
		tstate.deadline = deadline;
		T t;
		t.w = & (io::Writer&) os::stdout;
		t.tstate = &tstate;
		//  = {
		// 	{
		// 		// .signal =    make(chan bool, 1),
		// 		// barrier:   make(chan bool),
		// 		.w =         os::stdout,
		// 		// ctx:       ctx,
		// 		// cancelCtx: cancelCtx,
		// 	},
		// 	.tstate = tstate,
		// };
		ChattyPrinter chatty_printer(t.w);
		if (verbose()) {
			t.chatty = &chatty_printer;
		}
		t_runner(t, [&](T &t) {
			for (InternalTest const &test : tests) {
				t.run(test.name, test.f);
			}
		});
		if (sync::poll(sync::Recv(t.signal)) == -1) {
			panic("internal error: tRunner exited without sending on t.signal");
		}
		ok = ok && !t.failed();
		ran = ran || t.ran;
	}
	// }
	return {ran, ok};
}

static std::tuple<bool, bool> run_fuzz_tests(
	TestDeps *deps,
	view<InternalFuzzTarget> tests,
	time::time deadline ) {
	
	return {true, true};
}

int M::run() {
	// defer func() {
	// 	code = m.exitCode
	// }()
	M &m = *this;

	// Count the number of calls to m.Run.
	// We only ever expected 1, but we didn't enforce that,
	// and now there are tests in the wild that call m.Run multiple times.
	// Sigh. go.dev/issue/23129.
	m.num_run++;

	// TestMain may have already called flag.Parse.
	if (!flag::parsed()) {
		flag::parse();
	}

	if (chatty.json) {
		// With -v=json, stdout and stderr are pointing to the same pipe,
		// which is leading into test2json. In general, operating systems
		// do a good job of ensuring that writes to the same pipe through
		// different file descriptors are delivered whole, so that writing
		// AAA to stdout and BBB to stderr simultaneously produces
		// AAABBB or BBBAAA on the pipe, not something like AABBBA.
		// However, the exception to this is when the pipe fills: in that
		// case, Go's use of non-blocking I/O means that writing AAA
		// or BBB might be split across multiple system calls, making it
		// entirely possible to get output like AABBBA. The same problem
		// happens inside the operating system kernel if we switch to
		// blocking I/O on the pipe. This interleaved output can do things
		// like print unrelated messages in the middle of a TestFoo line,
		// which confuses test2json. Setting os.Stderr = os.Stdout will make
		// them share a single pfd, which will hold a lock for each program
		// write, preventing any interleaving.
		//
		// It might be nice to set Stderr = Stdout always, or perhaps if
		// we can tell they are the same file, but for now -v=json is
		// a very clear signal. Making the two files the same may cause
		// surprises if programs close os.Stdout but expect to be able
		// to continue to write to os.Stderr, but it's hard to see why a
		// test would think it could take over global state that way.
		//
		// This fix only helps programs where the output is coming directly
		// from Go code. It does not help programs in which a subprocess is
		// writing to stderr or stdout at the same time that a Go test is writing output.
		// It also does not help when the output is coming from the runtime,
		// such as when using the print/println functions, since that code writes
		// directly to fd 2 without any locking.
		// We keep realStderr around to prevent fd 2 from being closed.
		//
		// See go.dev/issue/33419.

		// realStderr = os.Stderr
		// os.Stderr = os.Stdout
	}

	if (*parallel < 1) {
		fmt::fprintln(os::stderr, "testing: -parallel can only be given a positive integer");
		flag::usage();
		return m.exit_code = 2;
	}

	if (*match_fuzz != "" && *fuzz_cache_dir == "") {
		fmt::fprintln(os::stderr, "testing: -test.fuzzcachedir must be set if -test.fuzz is set");
		flag::usage();
		return m.exit_code = 2;
	}

	if (*match_list != "") {
		list_tests(m.deps->match_string, m.tests, m.benchmarks, m.fuzz_targets, m.examples);
		return m.exit_code = 0;
	}

	if (*shuffle != "off") {
		panic("unimplemented");
		// int64 n = 0;
		// // var err error
		// if (*shuffle == "on") {
		// 	n = time::now().unix_nano();
		// } else {
		// 	n, err = strconv.ParseInt(*shuffle, 10, 64)
		// 	if err != nil {
		// 		fmt.Fprintln(os.Stderr, `testing: -shuffle should be "off", "on", or a valid integer:`, err)
		// 		m.exitCode = 2
		// 		return
		// 	}
		// }
		// fmt.Println("-test.shuffle", n)
		// rng := rand.New(rand.NewSource(n))
		// rng.Shuffle(len(m.tests), func(i, j int) { m.tests[i], m.tests[j] = m.tests[j], m.tests[i] })
		// rng.Shuffle(len(m.benchmarks), func(i, j int) { m.benchmarks[i], m.benchmarks[j] = m.benchmarks[j], m.benchmarks[i] })
	}

	// parse_cpu_list();

	m.before();
	defer d1 = [&] { m.after(); };

	// Run tests, examples, and benchmarks unless this is a fuzz worker process.
	// Workers start after this is done by their parent process, and they should
	// not repeat this work.
	if (!*is_fuzz_worker) {
		time::time deadline = m.start_alarm();
		have_examples = len(m.examples) > 0;
		auto [test_ran, test_ok] = run_tests(m.deps->match_string, m.tests, deadline);
		auto [fuzz_targets_ran, fuzz_targets_ok] = run_fuzz_tests(m.deps, m.fuzz_targets, deadline);
		auto [example_ran, example_ok] = run_examples(m.deps->match_string, m.examples);
		m.stop_alarm();
		if (!test_ran && !example_ran && !fuzz_targets_ran && *match_benchmarks == "" && *match_fuzz == "") {
			fmt::fprintln(os::stderr, "testing: warning: no tests to run");
			if (testing_testing && *match != "^$") {
				// If this happens during testing of package testing it could be that
				// package testing's own logic for when to run a test is broken,
				// in which case every test will run nothing and succeed,
				// with no obvious way to detect this problem (since no tests are running).
				// So make 'no tests to run' a hard failure when testing package testing itself.
				fmt::print(chatty.prefix(), "FAIL: package testing must run tests\n");
				test_ok = false;
			}
		}
		bool any_failed = !test_ok || !example_ok || !fuzz_targets_ok || !run_benchmarks(m.deps->import_path(), m.deps->match_string, m.benchmarks);
		if (!any_failed && race.errors() > 0) {
			fmt::print(chatty.prefix(), "testing: race detected outside of test execution\n");
			any_failed = true;
		}
		if (any_failed) {
			fmt::print(chatty.prefix(), "FAIL\n");
			return m.exit_code = 1;
		}
	}

	bool fuzzing_ok = run_fuzzing(m.deps, m.fuzz_targets);
	if (!fuzzing_ok) {
		fmt::print(chatty.prefix(), "FAIL\n");
		if (*is_fuzz_worker) {
			return m.exit_code = FuzzWorkerExitCode;
		} 
		return m.exit_code = 1;
	}
	
	if (!*is_fuzz_worker) {
		fmt::print(chatty.prefix(), "PASS\n");
	}
	return m.exit_code = 0;
}

void lib::testing::M::before() {}
void lib::testing::M::after() {}

time::time M::start_alarm() {
	return {};
	// if *timeout <= 0 {
	// 	return time.Time{}
	// }

	// deadline := time.Now().Add(*timeout)
	// m.timer = time.AfterFunc(*timeout, func() {
	// 	m.after()
	// 	debug.SetTraceback("all")
	// 	extra := ""

	// 	if list := runningList(); len(list) > 0 {
	// 		var b strings.Builder
	// 		b.WriteString("\nrunning tests:")
	// 		for _, name := range list {
	// 			b.WriteString("\n\t")
	// 			b.WriteString(name)
	// 		}
	// 		extra = b.String()
	// 	}
	// 	panic(fmt.Sprintf("test timed out after %v%s", *timeout, extra))
	// })
	// return deadline
}

bool testing::verbose() {
	return true;
	// Same as in Short.
	if (!flag::parsed()) {
		panic("testing: verbose called before parse");
	}
	return chatty.on;
}
void Common::check_fuzz_fn(str) {
	if (this->in_fuzz_fn) {
		panic(fmt::sprintf("testing: f.%s was called inside the fuzz target, use t.%s instead", name, name));
	}
}
void TestState::release() {
	TestState &s = *this;
	s.mu.lock();
	if (s.num_waiting == 0) {
		s.running--;
		s.mu.unlock();
		return;
	}
	s.num_waiting--;
	s.mu.unlock();
	s.start_parallel.send(true); // Pick a waiting test to be run.
}
void TestState::wait_parallel() {
	TestState &s = *this;
	sync::Lock lock(s.mu);
	if (s.running < s.max_parallel) {
		s.running++;
		return;
	}
	s.num_waiting++;
	lock.unlock();
	s.start_parallel.recv();
}

// fmtDuration returns a string representing d in the form "87.00s".
static String fmt_duration(time::duration d) {
	return fmt::sprintf("%.2fs", d.seconds());
}


void T::report() {
	T &t = *this;
	if (t.parent == nil) {
		return;
	}
	String dstr = fmt_duration(t.duration);
	str format = "--- %s: %s (%s)\n";
	if (t.failed()) {
		t.flush_to_parent(t.name, String(fmt::sprintf(format, "FAIL", t.name, dstr)));
	} else if (t.chatty != nil) {
		String s = fmt::sprintf(format, t.name, dstr);
		if (t.skipped()) {
			t.flush_to_parent(t.name, String(fmt::sprintf(format, "SKIP", t.name, dstr)));
		} else {
			t.flush_to_parent(t.name, String(fmt::sprintf(format, "PASS", t.name, dstr)));
		}
	}
}
void Common::flush_to_parent(str test_name, str s) {
	Common &c = *this;
	Common *p = c.parent;
	sync::WLock plock(p->mu);
	sync::WLock clock(c.mu);

	// if len(c.output) > 0 {
	// 	// Add the current c.output to the print,
	// 	// and then arrange for the print to replace c.output.
	// 	// (This displays the logged output after the --- FAIL line.)
	// 	format += "%s"
	// 	args = append(args[:len(args):len(args)], c.output)
	// 	c.output = c.output[:0]
	// }

	if (c.chatty != nil && (p->w == c.chatty->w || c.chatty->json)) {
		// We're flushing to the actual output, so track that this output is
		// associated with a specific test (and, specifically, that the next output
		// is *not* associated with that test).
		//
		// Moreover, if c.output is non-empty it is important that this write be
		// atomic with respect to the output of other tests, so that we don't end up
		// with confusing '=== NAME' lines in the middle of our '--- PASS' block.
		// Neither humans nor cmd/test2json can parse those easily.
		// (See https://go.dev/issue/40771.)
		//
		// If test2json is used, we never flush to parent tests,
		// so that the json stream shows subtests as they finish.
		// (See https://go.dev/issue/29811.)
		c.chatty->update(test_name, s + c.output);
	} else {
		// We're flushing to the output buffer of the parent test, which will
		// itself follow a test-name header when it is finally flushed to stdout.
		fmt::fprintf(*p->w, error::ignore, "%s%s%s", c.chatty->prefix(), s, c.output);
		// fmt.Fprintf(p.w, c.chatty.prefix()+format, args...)
	}
	c.output = "";
}
void Common::set_ran() {
	Common &c = *this;
	if (c.parent != nil) {
		c.parent->set_ran();
	}
	sync::WLock lock(c.mu);
	c.ran = true;
}


bool T::run(str name, std::function<void(T &)> f) {
	T &ot = *this;
	if (ot.cleanup_started.load()) {
		panic("testing: t.Run called during t.Cleanup");
	}

	ot.has_sub.store(true);
	auto [test_name, ok, _] = ot.tstate->match->full_name(&ot, name);;
	if (!ok || should_fail_fast()) {
		return true;
	}
	// Record the stack trace at the point of this call so that if the subtest
	// function - which runs in a separate stack - is marked as a helper, we can
	// continue walking the stack into the parent test.
	// var pc [maxStackLen]uintptr
	// n := runtime.Callers(2, pc[:])

	// There's no reason to inherit this context from parent. The user's code can't observe
	// the difference between the background context and the one from the parent test.
	// ctx, cancelCtx := context.WithCancel(context.Background())
	T nt;
	nt.name   = test_name;
	nt.parent = &ot;
	nt.level  = ot.level + 1;
	//nt.creator = pc[:n];
	nt.chatty = ot.chatty;
	nt.tstate = ot.tstate;
	
	// t = &T{
	// 	common: common{
	// 		// barrier:   make(chan bool),
	// 		// signal:    make(chan bool, 1),
	// 		name:      testName,
	// 		parent:    &t.common,
	// 		level:     t.level + 1,
	// 		creator:   pc[:n],
	// 		chatty:    t.chatty,
	// 		// ctx:       ctx,
	// 		// cancelCtx: cancelCtx,
	// 	},
	// 	tstate: t.tstate,
	// }
	Indenter indenter(&nt);
	nt.w = &indenter;

	if (nt.chatty != nil) {
		nt.chatty->update(nt.name, String(fmt::sprintf("=== RUN   %s\n", nt.name)));
	}
	running.store(nt.name, time::now());

	// Instead of reducing the running count of this test before calling the
	// tRunner and increasing it afterwards, we rely on tRunner keeping the
	// count correct. This ensures that a sequence of sequential tests runs
	// without being preempted, even when their parent is a parallel test. This
	// may especially reduce surprises if *parallel == 1.
	//go tRunner(nt, f);
	t_runner(nt, f);

	// The parent goroutine will block until the subtest either finishes or calls
	// Parallel, but in general we don't know whether the parent goroutine is the
	// top-level test function or some other goroutine it has spawned.
	// To avoid confusing false-negatives, we leave the parent in the running map
	// even though in the typical case it is blocked.

	if (!nt.signal.recv()) {
		// At this point, it is likely that FailNow was called on one of the
		// parent tests by one of the subtests. Continue aborting up the chain.
		sync::goexit();
	}

	if (nt.chatty != nil && nt.chatty->json) {
		nt.chatty->update(nt.parent->name, String(fmt::sprintf("=== NAME  %s\n", nt.parent->name)));
	}
	return !nt.has_failed;
}
void M::stop_alarm() {

}

size detail::Indenter::direct_write(str b, error) {
	Indenter &w = *this;
	size n = len(b);
	while (len(b) > 0) {
		size end = strings::index_byte(b, '\n');
		if (end == -1) {
			end = len(b);
		} else {
			end++;
		}
		// An indent of 4 spaces will neatly align the dashes with the status
		// indicator of the parent.
		str line = b[0,end];
		if (line[0] == marker[0]) {
			w.c->output.append(marker);
			line = line+1;
		}
		str indent = "    ";
		w.c->output.append(indent);
		w.c->output.append(line);
		b = b+end;
	}
	return n;
}
bool detail::Common::skipped() const {
	Common const &c = *this;
	sync::RLock lock(c.mu);
	return c.has_skipped;
}


// int lib::testing::main_start(detail::TestDeps *deps,
//                          view<detail::InternalTest> tests,
//                          view<detail::InternalBenchmark> benchmarks,
//                          view<detail::InternalFuzzTarget> fuzz_targets,
//                          view<detail::InternalExample> examples) 
// {
// 	// register_cover(deps.init_runtime_coverage());
// 	init();
// 	M m;
// 	m.deps = deps;
// 	m.tests = tests;
// 	m.benchmarks = benchmarks;
// 	m.fuzzTargets = fuzz_targets;
// 	m.examples = examples;
// }
void ChattyPrinter::update(str test_name, str s) {
	ChattyPrinter &p = *this;
	sync::Lock lock(p.last_name_mu);

	// Since the message already implies an association with a specific new test,
	// we don't need to check what the old test name was or log an extra NAME line
	// for it. (We're updating it anyway, and the current message already includes
	// the test name.)
	p.last_name = test_name;
	fmt::fprint(*p.w, error::ignore, p.prefix(), s);
}
void ChattyPrinter::print(str test_name, str s) {
	ChattyPrinter &p = *this;
	sync::Lock lock(p.last_name_mu);

	if (p.last_name == "") {
		p.last_name = test_name;
	} else if (p.last_name != test_name) {
		fmt::fprintf(*p.w, "%s=== NAME  %s\n", p.prefix(), test_name);
		p.last_name = test_name;
	}

	fmt::fprint(*p.w, error::ignore, s);
}

ChattyPrinter::ChattyPrinter(io::Writer *w) : w(w) {
	this->json = chatty.json;
}
