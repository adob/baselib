#include "pipe.h"

#include "lib/error.h"
#include "lib/io/io.h"
#include "lib/io/util.h"
#include "lib/sync/go.h"
#include "lib/sync/gang.h"
#include "lib/testing/testing.h"
#include "lib/strings/strings.h"

#include <memory>

using namespace lib;
using namespace lib::io;
using namespace lib::testing;

// https://cs.opensource.google/go/go/+/refs/tags/go1.24.2:src/io/pipe_test.go

static void check_write(T &t, io::Writer &w, str data, sync::Chan<int> &c) {
    size n = w.write(data, [&](Error &err) {
        t.errorf("write: %v", err);
    });
    if (n != len(data)) {
        t.errorf("short write: %d != %d", n, len(data));
    }
    c.send(0);
}

// Test a single read/write pair.
void test_pipe1(T &t) {
    sync::Chan<int> c;
	auto [r, w] = pipe();
    Array<byte, 64> b;
    sync::go g1 = [&]{
        check_write(t, *w, "hello, world", c);
    };
	auto [n, eof] = r->read(b, [&](Error &err) {
        t.errorf("read: %v", err);
    });
    if (n != 12 || b[0,12] != "hello, world" || eof) {
        t.errorf("bad read: got %q, eof %v", b[0,n], eof);
    }
    c.recv();
    r->close(error::ignore);
    w->close(error::ignore);
}

static void reader(T &t, io::Reader &r, sync::Chan<size> &c) {
	Array<byte, 64> b;
	for (;;) {
		auto [n, eof] = r.read(b, [&](Error &err) {
            t.errorf("read: %v", err);
        });
        if (eof) {
            c.send(0);
            break;
        }
		c.send(n);
	}
}

// Test a sequence of read/write pairs.
void test_pipe2(testing::T &t) {
    sync::Chan<size> c;
    auto [r, w] = pipe();
	sync::go g1 = [&] { reader(t, *r, c); };
	Array<byte, 64> b = {};

	for (int i = 0; i < 5; i++) {
		buf p = b[0, 5+i*10];
		size n = w->write(p, [&](Error &err) {
            t.errorf("write: %v", err);
        });
		if (n != len(p)) {
			t.errorf("wrote %d, got %d", len(p), n);
        }
		size nn = c.recv();
		if (nn != n) {
			t.errorf("wrote %d, read got %d", n, nn);
		}
	}
	w->close(error::ignore);
	size nn = c.recv();
	if (nn != 0) {
		t.errorf("final read got %d", nn);
	}
}

struct PipeReturn {
	size n = 0;
	ErrorRecorder err;
} ;

// Test a large write that requires multiple reads to satisfy.
static void writer(Writer &w, str data, sync::Chan<PipeReturn> &c) {
    ErrorRecorder rec;
	size n = w.write(data, rec);
	w.close(error::ignore);
	c.send(PipeReturn{n, rec});
}

void test_pipe3(T &t) {
	sync::Chan<PipeReturn> c;
	auto [r, w] = pipe();
    lib::Buffer wdat(128);;
	for (int i = 0; i < len(wdat); i++) {
		wdat[i] = byte(i);
	}
	sync::go g1 = [&] { writer(*w, wdat, c); };
    lib::Buffer rdat(1024);
	size tot = 0;
	for (size n = 1; n <= 256; n *= 2) {
		auto [nn, eof] = r->read(rdat[tot, tot+n], [&](Error &err) {
            t.fatalf("read: %v", err);
        });

		// only final two reads should be short - 1 byte, then 0
		size expect = n;
		if (n == 128) {
			expect = 1;
		} else if (n == 256) {
			expect = 0;
			if (!eof) {
				t.fatalf("read at end: !eof");
			}
		}
		if (nn != expect) {
			t.fatalf("read %d, expected %d, got %d", n, expect, nn);
		}
		tot += nn;
	}
	PipeReturn pr = c.recv();
	if (pr.n != 128 || pr.err) {
		t.fatalf("write 128: %d, %v", pr.n, pr.err);
	}
	if (tot != 128) {
		t.fatalf("total read %d != 128", tot);
	}
	for (int i = 0; i < 128; i++) {
		if (rdat[i] != byte(i)) {
			t.fatalf("rdat[%d] = %d", i, rdat[i]);
		}
	}
}

// Test read after/before writer close.

struct PipeTest {
	bool async;
    std::unique_ptr<Error> err;
	bool close_with_error;
} ;

// func (p pipeTest) String() string {
// 	return fmt.Sprintf("async=%v err=%v closeWithError=%v", p.async, p.err, p.closeWithError)
// }

PipeTest pipe_tests[] = {
	{true, nil, false},
	{true, nil, true},
	{true, std::make_unique<ErrShortWrite>(), true},
	{false, nil, false},
	{false, nil, true},
	{false, std::make_unique<ErrShortWrite>(), true},
};

static void delay_close(T &t, auto &cl, sync::Chan<int> &ch, PipeTest const &tt) {
	time::sleep(1 * time::millisecond);
    ErrorRecorder err;
	if (tt.close_with_error && tt.err) {
		cl.close_with_error(*tt.err);
	} else {
		cl.close([&](Error &err) {
            t.errorf("delayClose: %v", err);
        });
	}
	ch.send(0);
}

void test_pipe_read_close(T &t) {
    sync::Gang g;
	for (PipeTest &tt : pipe_tests) {
        sync::Chan<int> c(1);
		auto [r, w] = pipe();
		if (tt.async) {
			g.go([&] { delay_close(t, *w, c, tt); });
		} else {
			delay_close(t, *w, c, tt);
		}
        lib::Buffer b(64);
		
        ErrorRecorder want_err;
        ErrorRecorder got_err;
        if (tt.err) {
            want_err.report(*tt.err);
        }

        auto [n, eof] = r->read(b, got_err);
        c.recv();
		
		if (want_err != got_err) {
			t.errorf("read from closed pipe: %q; want %q", got_err, want_err);
		}
		if (n != 0 || !eof) {
			t.errorf("read on closed pipe returned %d, eof %v", n, eof);
		}
        ErrorRecorder err;
        r->close(err);
		if (err) {
			t.errorf("r.close: %v", err);
		}
	}
}

// Test close on Read side during Read.
void test_pipe_read_close2(T &t) {
    sync::Chan<int> c(1);
	// c := make(chan int, 1)
    auto [r, _] = pipe();
	ErrorRecorder err;
    sync::go g1 = [&]{ delay_close(t, *r, c, PipeTest{}); };
	
	auto [n, eof] = r->read(lib::Buffer(64), err);
    c.recv();
	
	if (n != 0 || !eof || !err.is(ErrClosedPipe())) {
		t.errorf("read from closed pipe: %v, %v, err %q; want %v, %v, err %q", n, eof, err, 0, true, ErrClosedPipe());
	}
}

// Test write after/before reader close.

void test_pipe_write_close(T &t) {
    sync::Gang g;
	for (auto &&tt : pipe_tests) {
        sync::Chan<int> c(1);
		auto [r, w] = pipe();
		if (tt.async) {
			g.go([&] { delay_close(t, *r, c, tt); });
		} else {
			delay_close(t, *r, c, tt);
		}
        ErrorRecorder err;
		size n = w->write("hello, world", err);
		c.recv();
        ErrorRecorder expect;
        if (tt.err) {
            expect.report(*tt.err);
        } else {
            expect.report(ErrClosedPipe());
        }
		
		if (err != expect) {
			t.errorf("write on closed pipe with: got error %q; want %q", err, expect);
		}
		if (n != 0) {
			t.errorf("write on closed pipe returned %d", n);
		}
        ErrorRecorder close_err;
        w->close(close_err);
        if (close_err) {
            t.errorf("w.Close: %v", close_err);
        }
	}
}

// Test close on Write side during Write.
void test_pipe_write_close2(T &t) {
    sync::Chan<int> c(1);
	auto [_, w] = pipe();
	sync::go g1 = [&] { delay_close(t, *w, c, PipeTest{}); };
    ErrorRecorder err;
	size n = w->write(lib::Buffer(64), err);
	c.recv();
	if (n != 0 || !err.is(ErrClosedPipe())) {
		t.errorf("write to closed pipe: %v, %v want %v, %v", n, err, 0, ErrClosedPipe());
	}
}

void test_write_empty(T&) {
	auto [r, w] = pipe();
	sync::go g1 = [&] {
		w->write("", error::ignore);
		w->close(error::ignore);
	};
    Array<byte, 2> b = {};
	read_full(*r, b[0,2], error::ignore);
	r->close(error::ignore);
}

void test_write_after_writer_close(T &t) {
	auto [r, w] = pipe();
    sync::Chan<bool> done;
    ErrorRecorder write_err;
	sync::go g1 = [&] {
        w->write("hello", [&](Error &err) { 
            t.errorf("got error: %q; expected none", err);
        });
		w->close(error::ignore);
        w->write("world", write_err);
		done.send(true);
	};

    lib::Buffer buf(100);
	String result;
	size n = read_full(*r, buf, [&](Error &err) {
        if (!err.is<ErrUnexpectedEOF>()) {
            t.fatalf("got: %q; want: %q", err, ErrUnexpectedEOF());
        }
    });
	result = buf[0,n];
	done.recv();

	if (result != "hello") {
		t.errorf("got: %q; want: %q", result, "hello");
	}
	if (!write_err.is(ErrClosedPipe())) {
		t.errorf("got: %q; want: %q", write_err, ErrClosedPipe());
	}
}

void test_pipe_close_error(T &t) {
    struct TestError1 : ErrorBase<TestError1, "TestError1"> {};
    struct TestError2 : ErrorBase<TestError2, "TestError2"> {};

	PipePair p = pipe();
	p.r->close_with_error(TestError1{});
    if (ErrorRecorder err; p.w->write("x", err), !err.is(TestError1())) {
        t.errorf("write error: got %v, want TestError1", err);
    }
	p.r->close_with_error(TestError2{});
	if (ErrorRecorder err; p.w->write("x", err), !err.is(TestError2())) {
		t.errorf("write error: got %v, want TestError1", err);
	}

	p = pipe();
	p.w->close_with_error(TestError1{});
	if (ErrorRecorder err; p.r->read(buf(), err), !err.is(TestError1{})) {
		t.errorf("read error: got %v, want TestError1", err);
	}
	p.w->close_with_error(TestError2{});
	if (ErrorRecorder err; p.r->read(buf(), err), !err.is(TestError1{})) {
		t.errorf("read error: got %v, want TestError1", err);
	}
}

static String sort_bytes_in_groups(str b, int n) {
    std::vector<str> groups;
	// var groups [][]byte
	while (len(b) > 0) {
        groups.push_back(b[0,n]);
        b = b+n;
	}
    std::sort(groups.begin(), groups.end());
    
    String out;
    for (str s : groups) {
        out += s;
    }
    return out;
}


void test_pipe_cconcurrent(T &t) {
	const str input     = "0123456789abcdef";
	const int count     = 8;
    const int read_size = 2;
    sync::Gang g;

	t.run("write", [&](T &t) {
		auto [r, w] = pipe();

		for (int i = 0; i < count; i++) {
			g.go([&] {
				time::sleep(time::millisecond); // Increase probability of race
                ErrorRecorder err;
                size n = w->write(input, err);
				if (n != len(input) || err) {
					t.errorf("write() = (%d, %v); want (%d, nil)", n, err, len(input));
				}
			});
		}

		lib::Buffer buf(count*len(input));
		for (size i = 0; i < len(buf); i += read_size) {
            ErrorRecorder err;
            size n = r->read(buf[i, i+read_size], err).nbytes;
			if (n != read_size || err) {
				t.errorf("read() = (%d, %v); want (%d, nil)", n, err, read_size);
			}
		}

		// Since each Write is fully gated, if multiple Read calls were needed,
		// the contents of Write should still appear together in the output.
		String got = buf;
		String want = strings::repeat(input, count);
		if (got != want) {
			t.errorf("got: %q; want: %q", got, want);
		}
	});

	t.run("read", [&](T &t) {
		auto [r, w] = pipe();

		sync::Chan<String> c(int(count*len(input)/read_size));
		for (int i = 0; i < c.capacity; i++) {
		    g.go([&] {
				time::sleep(time::millisecond); // Increase probability of race
				lib::Buffer buf(read_size);
                ErrorRecorder err;
                size n = r->read(buf, err).nbytes;
				if (n != read_size || err) {
					t.errorf("read() = (%d, %v); want (%d, nil)", n, err, read_size);
				}
				c.send(buf);
			});
		}

		for (int i = 0; i < count; i++) {
            ErrorRecorder err;
            size n = w->write(input, err);
			if (n != len(input) || err) {
				t.errorf("write() = (%d, %v); want (%d, nil)", n, err, len(input));
			}
		}

		// Since each read is independent, the only guarantee about the output
		// is that it is a permutation of the input in readSized groups.
		// lib::Buffer got := make([]byte, 0, count*len(input))
        String got;
		for (int i = 0; i < c.capacity; i++) {
            got += c.recv();
		}
		got = sort_bytes_in_groups(got, read_size);
        String want = strings::repeat(input, count);
		want = sort_bytes_in_groups(want, read_size);
		if (got != want) {
			t.errorf("got: %q; want: %q", got, want);
		}
	});
}
