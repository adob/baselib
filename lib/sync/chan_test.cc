#include <array>
#include <pthread.h>
#include <stdatomic.h>
#include <unordered_map>

#include "lib/debug/debug.h"
#include "lib/math/math.h"
#include "lib/runtime/debug.h"
#include "lib/sync/chan.h"
#include "lib/sync/go.h"
#include "lib/sync/gang.h"
#include "lib/sync/waitgroup.h"
#include "lib/testing/testing.h"
#include "lib/testing/benchmark.h"
#include "lib/time/time.h"

#include "lib/print.h"
#include "lib/types.h"


using namespace lib;
using namespace sync;

constexpr bool DebugLog = false;

#define LOG(...) if constexpr (DebugLog) fmt::printf(__VA_ARGS__)
// #define LOG(...)

void test_chan(testing::T &t) {
    int N = 200;
    if (testing::short_mode()) {
		N = 20;
	}

    for (int chan_cap = 0; chan_cap < N; chan_cap++) {
        {
            // Ensure that receive from empty chan blocks.
            Chan<int> c(chan_cap);
            bool recv1 = false;
            bool recv2 = false;

            go g1 = [&]{
                c.recv();
                recv1 = true;
            };

            go g2 = [&]{
                c.recv();
                recv2 = true;
            };

            time::sleep(time::millisecond);
            if (recv1 || recv2) {
                t.fatalf("chan[%d]: receive from empty chan", chan_cap);
            }

            // Ensure that non-blocking receive does not block.
            int r = poll(Recv(c));
            if (r != -1) {
                t.fatalf("chan[%d]: receive from empty chan", chan_cap);
            }

            r = poll(Recv(c));
            if (r != -1) {
                t.fatalf("chan[%d]: receive from empty chan", chan_cap);
            }

            c.send(0);
            c.send(0);
        }

        {
            // Ensure that send to full chan blocks.
            Chan<int> c(chan_cap);
            for (int i = 0; i < chan_cap; i++) {
                c.send(i);
            }
            std::atomic<uint32> sent = 0;
            go g1 = [&]{
                c.send(0);
                sent = 1;
            };
            time::sleep(time::millisecond);
            if (sent != 0) {
                t.fatalf("chan[%d]: send to full chan", chan_cap);
            }

            // Ensure that non-blocking send does not block.
            int r = poll(Send(c, 0));
            if (r != -1) {
                t.fatalf("chan[%d]: send to full chan", chan_cap);
            }
            c.recv();
        }

        {
            // Ensure that we receive 0 from closed chan.
            Chan<int> c(chan_cap);
            for (int i = 0; i < chan_cap; i++) {
                c.send(i);
            }
            c.close();
            
            for (int i = 0; i < chan_cap; i++) {
                int v = c.recv();
                if (v != i) {
                    t.fatalf("chan[%d]: received %v, expected %v", chan_cap, v, i);
                }
            }
            if (int v = c.recv(); v != 0) {
                t.fatalf("chan[%d]: received %v, expected %v", chan_cap, v, 0);
            }

            bool ok;
            if (int v = c.recv(&ok); v != 0 || ok) {
                t.fatalf("chan[%d]: received %v/%v, expected %v/%v", chan_cap, v, ok, 0, false);
            }
        }

        {
            // Ensure that close unblocks receive.
            Chan<int> c(chan_cap);
            Chan<bool> done;
            
            go g1 = [&] {
                bool ok;
                int v = c.recv(&ok);
                done.send(v == 0 && ok == false);
            };
            time::sleep(time::millisecond);
            c.close();

            if (!done.recv()) {
                t.fatalf("chan[%d]: received non zero from closed chan", chan_cap);
            }
        }

        {
            // Send 100 integers,
            // ensure that we receive them non-corrupted in FIFO order.
            Chan<int> c(chan_cap);
            go g1 = [&] {
                for (int i = 0; i < 100; i++) {
                    c.send(i);

                }
            };
            for (int i = 0; i < 100; i++) {
                int v = c.recv();
                if (v != i) {
                    t.fatalf("chan[%d]: received %v, expected %v", chan_cap, v, i);
                }
            }

            // Same, but using recv2.
            go g2 = [&] {
                for (int i = 0; i < 100; i++) {
                    c.send(i);
                }
            };
            for (int i = 0; i < 100; i++) {
                bool ok;
                int v = c.recv(&ok);
                if (!ok) {
                    t.fatalf("chan[%d]: receive failed, expected %v", chan_cap, i);
                }
                if (v != i) {
                    fmt::printf("chan[%d]: received %v, expected %v\n", chan_cap, v, i);
                    t.fatalf("chan[%d]: received %v, expected %v", chan_cap, v, i);
                }
            }

            // Send 1000 integers in 4 goroutines,
            // ensure that we receive what we send.
            const int P = 4;
            const int L = 1000;
            Gang g;
            for (int p = 0; p < P; p++) {
                g.go([&]{
                    for (int i = 0; i < L; i++) {
                        c.send(i);
                    }
                });
            }
            Chan<std::unordered_map<int, int>> done;
            
            for (int p = 0; p < P; p++) {
                g.go([&]{
                    std::unordered_map<int, int> recv;
                    for (int i = 0; i < L; i++) {
                        int v = c.recv();
                        recv[v] = recv[v] + 1;
                    }
                    done.send(recv);
                });
            }
            std::unordered_map<int, int> recv;
            for (int p = 0; p < P; p++) {
                for (auto &&[k, v] : done.recv()) {
                    recv[k] = recv[k] + v;
                }
            }
            if (recv.size() != L) {
                t.fatalf("chan[%d]: received %v values, expected %v", chan_cap, recv.size(), L);
            }
            for (auto &&[_, v] : recv) {
                if (v != P) {
                    t.fatalf("chan[%d]: received %v values, expected %v", chan_cap, v, P);
                }
            }
        }

        {
            // Test len/cap.
            Chan<int> c(chan_cap);
            if (c.length() != 0 || c.capacity != chan_cap) {
                t.fatalf("chan[%d]: bad len/cap, expect %v/%v, got %v/%v", chan_cap, 0, chan_cap, c.length(), c.capacity);
            }
            for (int i = 0; i < chan_cap; i++) {
                c.send(i);
            }
            if (c.length() != chan_cap || c.capacity != chan_cap) {
                t.fatalf("chan[%d]: bad len/cap, expect %v/%v, got %v/%v", chan_cap, chan_cap, chan_cap, c.length(), c.capacity);
            }
        }
    }
}

void nonblock_recv_race_case(testing::T &t, int choice) {
    Chan<int> c(1);
    c.send(1);
    
    go g1 = [&] {
        if (choice == 0) time::sleep(time::millisecond);
        int r = poll(
            Recv(c)
        );
        if (r == -1) {
            t.error("chan is not ready");
        }
    };

    if (choice == 1) time::sleep(time::millisecond);
    c.close();
    if (choice == 2) time::sleep(time::millisecond);
    c.recv();
    
    if (t.failed()) {
        return;
    }
    
}

void test_nonblock_recv_race0(testing::T &t) {
    nonblock_recv_race_case(t, 0);
}
void test_nonblock_recv_race1(testing::T &t) {
    nonblock_recv_race_case(t, 1);
}
void test_nonblock_recv_race2(testing::T &t) {
    nonblock_recv_race_case(t, 2);
}

void test_nonblock_recv_race(testing::T &t) {
	int n = 10000;
	if (testing::short_mode()) {
		n = 100;
	}

	for (int i = 0; i < n; i++) {
        Chan<int> c(1);
		c.send(1);
		
		go g1 = [&] {
            int r = poll(
                Recv(c)
            );
            if (r == -1) {
                t.error("chan is not ready");
            }
		};

        c.close();
		c.recv();
		
		if (t.failed()) {
			return;
		}
	}
}

void test_unbuffered_sleep_send_recv(testing::T &t) {
    Chan<int> c(0);

    go g1 = [&] {
        time::sleep(time::millisecond);
        c.send(42);
    };

    
    int i = c.recv();

    if (i != 42) {
        t.errorf("c.rev() = %d, expected 42", i);
    }
}

void test_unbuffered_send_sleep_recv(testing::T &t) {
    Chan<int> c(0);

    go g1 = [&] {
        c.send(42);
    };

    time::sleep(time::millisecond);
    int i = c.recv();

    if (i != 42) {
        t.errorf("c.rev() = %d, expected 42", i);
    }
}

void test_unbuffered_sleep_send_select_recv(testing::T &t) {
    //time::sleep(10*time::second);
    Chan<int> c(0);

    go g1 = [&] {
        time::sleep(time::millisecond);
        c.send(42);
    };

    
    int i = 0;
    select(Recv(c, &i));

    if (i != 42) {
        t.errorf("select(Read(c)) = %d, expected 42", i);
    }
}

void test_unbuffered_sleep_send_select_recv2(testing::T &t) {
    Chan<int> c1(0);
    Chan<int> c2(0);

    go g1 = [&] {
        time::sleep(time::millisecond);
        c1.send(42);
    };

    
    int i1 = 0;
    int i2 = 0;
    select(Recv(c1, &i1), Recv(c2, &i2));

    if (i1 != 42) {
        t.errorf("select(Read(c1)) = %d, expected 42", i1);
    }
}

void test_unbuffered_send_sleep_select_recv(testing::T &t) {
    Chan<int> c(0);

    go g1 = [&] {
        c.send(42);
    };

    
    time::sleep(time::millisecond);

    int i = 0;
    select(Recv(c, &i));

    if (i != 42) {
        t.errorf("select(Read(c)) = %d, expected 42", i);
    }
}

void test_unbuffered_sleep_select_send_recv(testing::T &t) {
    Chan<int> c(0);

    go g1 = [&] {
        time::sleep(time::millisecond);
        select(Send(c, 42));
    };

    int i = c.recv();

    if (i != 42) {
        t.errorf("select(Read(c)) = %d, expected 42", i);
    }
}

void test_unbuffered_select_send2_sleep_recv(testing::T &t) {
    Chan<int> c1(0);
    Chan<int> c2(0);

    go g1 = [&] {
        select(Send(c1, 42), Send(c2, 43));
    };
    
    time::sleep(time::millisecond);
    int i = c1.recv();

    if (i != 42) {
        t.errorf("select(Read(c1)) = %d, expected 42", i);
    }
}

void test_unbuffered_select_send_sleep_recv(testing::T &t) {
    //print "test_unbuffered_select_send_sleep_recv";
    Chan<int> c(0);

    go g1 = [&] {
        select(Send(c, 42));
    };

    
    time::sleep(time::millisecond);
    int i = c.recv();

    if (i != 42) {
        t.errorf("select(Read(c)) = %d, expected 42", i);
    }
}

// This test checks that select acts on the state of the channels at one
// moment in the execution, not over a smeared time window.
// In the test, one goroutine does:
//
//	create c1, c2
//	make c1 ready for receiving
//	create second goroutine
//	make c2 ready for receiving
//	make c1 no longer ready for receiving (if possible)
//
// The second goroutine does a non-blocking select receiving from c1 and c2.
// From the time the second goroutine is created, at least one of c1 and c2
// is always ready for receiving, so the select in the second goroutine must
// always receive from one or the other. It must never execute the default case.
void test_nonblock_select_race(testing::T &t) {
	int n = 100000;
	if (testing::short_mode()){
		n = 1000;
	}
    Chan<bool> done(1);
	for (int i = 0; i < n; i++) {
        Chan<int> c1(1);
        Chan<int> c2(1);
        c1.send(1);
		go g1 = [&] {
            int r = poll(
                Recv(c1),
                Recv(c2)
            );
            if (r == -1) {
                done.send(false);
            }
            
            done.send(true);
		};
        c2.send(1);
		poll(Recv(c1));
		
        if (!done.recv()) {
            t.fatal("no chan is ready");
        }
	}
}


// Same as test_nonblock_select_race, but close(c2) replaces c2 <- 1.
void test_nonblock_select_race_2(testing::T &t) {
	int n = 100000;
	if (testing::short_mode()) {
		n = 1000;
	}
    Chan<bool> done(1);
	for (int i = 0; i < n; i++) {
        Chan<int> c1(1);
        Chan<int> c2;
        c1.send(1);
		go g1 = [&] {
            int r = poll(
                Recv(c1),
                Recv(c2)
            );
            if (r == -1) {
                done.send(false);
            }
            done.send(true);
		};
        c2.close();
        poll(Recv(c1));
        if (!done.recv()) {
            t.fatal("no chan is ready");
        }
	}
}

void test_self_select(testing::T t) {
	// Ensure that send/recv on the same chan in select
	// does not crash nor deadlock.
	//defer runtime.GOMAXPROCS(runtime.GOMAXPROCS(2))
	
    for (int chan_cap : (int[]){0, 10}) {
        WaitGroup wg;
		wg.add(2);
		Chan<int> c(chan_cap);
		std::deque<go> g;

		for (int p = 0; p < 2; p++) {
			//p := p

			g.emplace_back([&t, &wg, &c, p, chan_cap]{
				defer d = [&]{ wg.done(); };

				for (int i = 0; i < 1000; i++) {
                    int v = 0;

					if (p == 0 || i%2 == 0) {
                        int r = select(
                            Send(c, p),
                            Recv(c, &v)
                        );
                        if (r == 1) {
                            if (chan_cap == 0 && v == p) {
								t.errorf("self receive");
								return;
							}
                        }
					} else {
                        int v = 0;
                        int r = select(
                            Recv(c, &v),
                            Send(c, p)
                        );
                        if (r == 0) {
                            if (chan_cap == 0 && v == p) {
								t.errorf("self receive");
								return;
							}
                        }
					}
				}
			});
            
		}
        
		wg.wait();
	}
}

void test_select_stress(testing::T &) {
	//defer runtime.GOMAXPROCS(runtime.GOMAXPROCS(10))

    Array<Chan<int>, 4> c = {
        Chan<int>(),
        Chan<int>(),
        Chan<int>(2),
        Chan<int>(3)
    };
	int N = int(1e5);
	if (testing::short_mode()) {
		N /= 10;
	}

	// There are 4 goroutines that send N values on each of the chans,
	// + 4 goroutines that receive N values on each of the chans,
	// + 1 goroutine that sends N values on each of the chans in a single select,
	// + 1 goroutine that receives N values on each of the chans in a single select.
	// All these sends, receives and selects interact chaotically at runtime,
	// but we are careful that this whole construct does not deadlock.
	sync::WaitGroup wg;
	wg.add(10);
    std::deque<go> g;
	for (int k = 0; k < 4; k++) {
		//k := k
		g.emplace_back([k, N, &c, &wg] {
			for (int i = 0; i < N; i++) {
                LOG("SENDING %d 0...\n", pthread_self());
				c[k].send(0);
                LOG("SENT %d 0...\n", pthread_self());
			}
			wg.done();
		});
		g.emplace_back([k, N, &c, &wg] {
			for (int i = 0; i < N; i++) {
				c[k].recv();
			}
			wg.done();
		});
	}
	g.emplace_back([N, &c, &wg] {
		Array<int, 4> n = {0,0,0,0};
		Array<Chan<int>*, 4> c1 = {&c[0], &c[1], &c[2], &c[3]};
		for (int i = 0; i < 4*N; i++) {
            int r = select(
                Send(c1[3], 0),
                Send(c1[2], 0),
                Send(c1[0], 0),
                Send(c1[1], 0)
            );

			switch(r) {
			case 0:
				n[3]++;
				if (n[3] == N) {
					c1[3] = nil;
				}
                break;
			case 1:
				n[2]++;
				if (n[2] == N) {
					c1[2] = nil;
				}
                break;
			case 2:
				n[0]++;
				if (n[0] == N) {
					c1[0] = nil;
				}
                break;
			case 3:
				n[1]++;
				if (n[1] == N) {
					c1[1] = nil;
				}
                break;
			}
		}
        LOG("DONE SENDING\n");
		wg.done();
	});
	g.emplace_back([N, &c, &wg] {
		Array<int, 4> n = {0,0,0,0};
		Array<Chan<int>*, 4> c1 = {&c[0], &c[1], &c[2], &c[3]};
		for (int i = 0; i < 4*N; i++) {
            LOG("%d BEFORE RECV %#x %#x %#x %#x\n", pthread_self(), (uintptr) c1[0], (uintptr) c1[1], (uintptr) c1[2], (uintptr) c1[3]);
            int r = select(
                Recv(c1[0]),
                Recv(c1[1]),
                Recv(c1[2]),
                Recv(c1[3])
            );
            LOG("%d AFTER RECV %#x %#x %#x %#x\n", pthread_self(), (uintptr) c1[0], (uintptr) c1[1], (uintptr) c1[2], (uintptr) c1[3]);
			switch (r) {
			case 0:
				n[0]++;
				if (n[0] == N) {
					c1[0] = nil;
				}
                break;
			case 1:
				n[1]++;
				if (n[1] == N) {
					c1[1] = nil;
				}
                break;
			case 2:
				n[2]++;
				if (n[2] == N) {
					c1[2] = nil;
				}
                break;
			case 3:
				n[3]++;
				if (n[3] == N) {
					c1[3] = nil;
				}
                break;
			}
		}
		wg.done();
	});
	wg.wait();
}

void test_select_fairness(testing::T &t) {
	const int trials = 10000;
	// if runtime.GOOS == "linux" && runtime.GOARCH == "ppc64le" {
	// 	testenv.SkipFlaky(t, 22047)
	// }
    Chan<byte> c1(trials+1);
    Chan<byte> c2(trials+1);
	for (int i = 0; i < trials+1; i++) {
		c1.send(1);
		c2.send(2);
        // print "sent", i;
	}
    Chan<byte> c3;
	Chan<byte> c4;
	Chan<byte> out;
	Chan<byte> done;
	go g1([&] {
		for(;;) {
            byte b = 0x00;
            select(
                Recv(c3, &b),
                Recv(c4, &b),
                Recv(c1, &b),
                Recv(c2, &b)
            );
            int i = select(
                Send(out, b),
                Recv(done)
            );
            if (i == 1) {
                return;
            }
		}
	});
    int cnt1 = 0, cnt2 = 0;
	for (int i = 0; i < trials; i++) {
        switch (byte b = out.recv(); b) {
        case 1:
            cnt1++;
            break;
        case 2:
            cnt2++;
            break;
        default:
            t.fatalf("unexpected value %d on channel", b);
        }
	}
	// If the select in the goroutine is fair,
	// cnt1 and cnt2 should be about the same value.
	// See if we're more than 10 sigma away from the expected value.
	// 10 sigma is a lot, but we're ok with some systematic bias as
	// long as it isn't too severe.
	const float64 mean = trials * 0.5;
	const float64 variance = trials * 0.5 * (1 - 0.5);
	float64 stddev = math::sqrt(variance);
    // print "mean %f; variance %f; stddev %f; cnt1 %d; cnt2 %d" % mean, variance, stddev, cnt1, cnt2;
	if (math::abs(float64(cnt1-mean)) > 10*stddev) {
		t.errorf("unfair select: in %d trials, results were %d, %d", trials, cnt1, cnt2);
	}
    done.close();
}

void test_lock_same_channel(testing::T &) {
    sync::Chan<int> c;

    go g1 = [&] {
        c.recv();
    };

    select(
        Send(c, 1),
        Send(c, 2)
    );
}

void test_pseudo_random_send(testing::T &t) {
	int n = 100;
    for (int chan_cap : (int[]){0, n}) {
        Chan<int> c(chan_cap);
		std::vector<int> l(n);
		Mutex m;
        m.lock();

		go g1 = [&]{
			for (int i = 0; i < n; i++) {
				// runtime.Gosched()
				l[i] = c.recv();
			}
			m.unlock();
		};

		for (int i = 0; i < n; i++) {
			select(
                Send(c, 1),
                Send(c, 0)
            );
		}
		m.lock(); // wait
		int n0 = 0;
		int n1 = 0;
		for (int i :l) {
			n0 += (i + 1) % 2;
			n1 += i;
		}
		if (n0 <= n/10 || n1 <= n/10) {
			t.errorf("Want pseudorandom, got %d zeros and %d ones (chan cap %d)", n0, n1, chan_cap);
		}
	}
}

void test_multi_consumer(testing::T &t) {
	const int nwork = 23;
	const int niter = 271828;

	Array<int, 10> pn = {2, 3, 7, 11, 13, 17, 19, 23, 27, 31};

	Chan<int> q(nwork*3);
    Chan<int> r(nwork*3);

	// workers
    WaitGroup wg;
    Gang g;
	for (int i = 0; i < nwork; i++) {
		wg.add(1);
		g.go([&, i](int w) {
            for (;;) {
                bool ok;
                LOG("recv worker %v receiving...\n", i);
                int v = q.recv(&ok);
                if (!ok) {
                    LOG("recv worker %v done\n", i);
                    break;
                }
                if (pn[w%len(pn)] == v) {
                    // mess with the fifo-ish nature of range
					//runtime.Gosched()
				}
				r.send(v);
            }
			wg.done();
        }, i);
	}
	// feeder & closer
	int expect = 0;
	g.go([&]{
		for (int i = 0; i < niter; i++) {
			int v = pn[i%len(pn)];
			expect += v;
			q.send(v);
		}
        LOG("SENDER closing...\n");
		q.close();  // no more work
        LOG("done sending\n");
		wg.wait(); // workers done
		r.close();  // ... so there can be no more results
	});
    // time::sleep(time::hour);
    // return;
	// consume & check
	int n = 0;
	int s = 0;
	for (;;) {
        bool ok;
        int v = r.recv(&ok);
        if (!ok) {
            break;
        }
		n++;
		s += v;
	}
	if (n != niter || s != expect) {
		t.errorf("Expected sum %d (got %d) from %d iter (saw %d)",
			expect, s, niter, n);
	}
}

void test_select_duplicate_channel(testing::T &) {
	// This test makes sure we can queue a G on
	// the same channel multiple times.
	Chan<int> c;
	Chan<int> d;
	Chan<int> e;

	// goroutine A
	go g1 = [&]{
		select(
		    Recv(c),
		    Recv(c),
		    Recv(d)
        );
		e.send(9);
	};
	time::sleep(time::millisecond); // make sure goroutine A gets queued first on c

	// goroutine B
	go g2 = [&] {
		c.recv();
	};
	time::sleep(time::millisecond); // make sure goroutine B gets queued on c before continuing

	d.send(7); // wake up A, it dequeues itself from c.  This operation used to corrupt c.recvq.
	e.recv();    // A tells us it's done
	c.send(8); // wake up B.  This operation used to fail because c.recvq was corrupted (it tries to wake up an already running G instead of B)
}

void benchmark_make_chan(testing::B &b) {
    struct Struct0 {} ;
    struct Struct32 { int64 a, b, c, d; };
    struct Struct40 { int64 a, b, c, d, e; };

	b.run("byte", [&](testing::B &b){        
		for (int i = 0; i < b.n; i++) {
			Chan<byte> x(8);
            x.close();
		}
	});
	b.run("int", [&](testing::B &b) {
		for (int i = 0; i < b.n; i++) {
            Chan<int> x(8);
            x.close();
		}
	});
	b.run("ptr", [&](testing::B &b) {
		for (int i = 0; i < b.n; i++) {
            Chan<Array<byte,8>> x;
			x.close();
		}
	});
	b.run("struct",  [&](testing::B &b) {
        b.run("0", [&](testing::B &b) {
            for (int i = 0; i < b.n; i++) {
                Chan<Struct0> x(8);
                x.close();
            }
        });
        b.run("32", [&](testing::B &b) {
            for (int i = 0; i < b.n; i++) {
                Chan<Struct32> x(8);
                x.close();
            }
        });
        b.run("40", [&](testing::B &b) {
            for (int i = 0; i < b.n; i++) {
                Chan<Struct40> x(8);
                x.close();
            }
        });
	});
}


void benchmark_chan_nonblocking(testing::B &b) {
    Chan<int> myc ;
	b.run_parallel([&](testing::PB &pb) {   
		while (pb.next()) {
            sync::poll(sync::Recv(myc));
		}
	});
}

void benchmark_select_uncontended(testing::B &b) {
	b.run_parallel([&](testing::PB &pb){
        Chan<int> myc1(1);
        Chan<int> myc2(1);
        myc1.send(0);
	    while (pb.next()) {
            int i = sync::select(
                sync::Recv(myc1),
                sync::Recv(myc2)
            );
            switch (i) {
                case 0:
                    myc2.send(0);
                    break;
                case 1:
                    myc1.send(0);
                    break;
            }
		}
	});
}

void benchmark_select_sync_contended(testing::B &b) {
    Chan<int> myc1;
	Chan<int> myc2;
    Chan<int> myc3;
    Chan<int> done;
    Gang g;
	b.run_parallel([&](testing::PB &pb) {
        g.go([&] {
            for (;;) {
                int i = select(
                    Send(myc1, 0),
                    Send(myc2, 0),
                    Send(myc3, 0),
                    Recv(done)
                );
                if (i == 3) {
                    return;
                }
            }
            
        });
		while (pb.next()) {
            select(
                Recv(myc1),
                Recv(myc2),
                Recv(myc3)
            );
		}
	});
    done.close();
}

void benchmark_select_async_contended(testing::B &b) {
	int procs = runtime::num_cpu();
    Chan<int> myc1(procs);
    Chan<int> myc2(procs);
	b.run_parallel([&](testing::PB &pb) {
        myc1.send(0);
		while (pb.next()) {
            int i = select(
                Recv(myc1),
                Recv(myc2)
            );
            switch (i) {
            case 0:
                myc2.send(0);
                break;
            case 1:
                myc1.send(0);
                break;
            }
		}
	});
}

void benchmark_select_nonblock(testing::B &b) {
    Chan<int> myc1;
    Chan<int> myc2;
	Chan<int> myc3(1);
    Chan<int> myc4(1);
    b.run_parallel([&](testing::PB &pb) {
        while (pb.next()) {
            poll(
                Recv(myc1)
            );

            poll(
                Send(myc2, 0)
            );

            poll(
                Recv(myc3)
            );

            poll(
                Send(myc4, 0)
            );
        }
    });
}

// func BenchmarkChanUncontended(b *testing.B) {
// 	const C = 100
// 	b.RunParallel(func(pb *testing.PB) {
// 		myc := make(chan int, C)
// 		for pb.Next() {
// 			for i := 0; i < C; i++ {
// 				myc <- 0
// 			}
// 			for i := 0; i < C; i++ {
// 				<-myc
// 			}
// 		}
// 	})
// }

// func BenchmarkChanContended(b *testing.B) {
// 	const C = 100
// 	myc := make(chan int, C*runtime.GOMAXPROCS(0))
// 	b.RunParallel(func(pb *testing.PB) {
// 		for pb.Next() {
// 			for i := 0; i < C; i++ {
// 				myc <- 0
// 			}
// 			for i := 0; i < C; i++ {
// 				<-myc
// 			}
// 		}
// 	})
// }

// func benchmarkChanSync(b *testing.B, work int) {
// 	const CallsPerSched = 1000
// 	procs := 2
// 	N := int32(b.N / CallsPerSched / procs * procs)
// 	c := make(chan bool, procs)
// 	myc := make(chan int)
// 	for p := 0; p < procs; p++ {
// 		go func() {
// 			for {
// 				i := atomic.AddInt32(&N, -1)
// 				if i < 0 {
// 					break
// 				}
// 				for g := 0; g < CallsPerSched; g++ {
// 					if i%2 == 0 {
// 						<-myc
// 						localWork(work)
// 						myc <- 0
// 						localWork(work)
// 					} else {
// 						myc <- 0
// 						localWork(work)
// 						<-myc
// 						localWork(work)
// 					}
// 				}
// 			}
// 			c <- true
// 		}()
// 	}
// 	for p := 0; p < procs; p++ {
// 		<-c
// 	}
// }

// func BenchmarkChanSync(b *testing.B) {
// 	benchmarkChanSync(b, 0)
// }

// func BenchmarkChanSyncWork(b *testing.B) {
// 	benchmarkChanSync(b, 1000)
// }

// func benchmarkChanProdCons(b *testing.B, chanSize, localWork int) {
// 	const CallsPerSched = 1000
// 	procs := runtime.GOMAXPROCS(-1)
// 	N := int32(b.N / CallsPerSched)
// 	c := make(chan bool, 2*procs)
// 	myc := make(chan int, chanSize)
// 	for p := 0; p < procs; p++ {
// 		go func() {
// 			foo := 0
// 			for atomic.AddInt32(&N, -1) >= 0 {
// 				for g := 0; g < CallsPerSched; g++ {
// 					for i := 0; i < localWork; i++ {
// 						foo *= 2
// 						foo /= 2
// 					}
// 					myc <- 1
// 				}
// 			}
// 			myc <- 0
// 			c <- foo == 42
// 		}()
// 		go func() {
// 			foo := 0
// 			for {
// 				v := <-myc
// 				if v == 0 {
// 					break
// 				}
// 				for i := 0; i < localWork; i++ {
// 					foo *= 2
// 					foo /= 2
// 				}
// 			}
// 			c <- foo == 42
// 		}()
// 	}
// 	for p := 0; p < procs; p++ {
// 		<-c
// 		<-c
// 	}
// }

// func BenchmarkChanProdCons0(b *testing.B) {
// 	benchmarkChanProdCons(b, 0, 0)
// }

// func BenchmarkChanProdCons10(b *testing.B) {
// 	benchmarkChanProdCons(b, 10, 0)
// }

// func BenchmarkChanProdCons100(b *testing.B) {
// 	benchmarkChanProdCons(b, 100, 0)
// }

// func BenchmarkChanProdConsWork0(b *testing.B) {
// 	benchmarkChanProdCons(b, 0, 100)
// }

// func BenchmarkChanProdConsWork10(b *testing.B) {
// 	benchmarkChanProdCons(b, 10, 100)
// }

// func BenchmarkChanProdConsWork100(b *testing.B) {
// 	benchmarkChanProdCons(b, 100, 100)
// }

// func BenchmarkSelectProdCons(b *testing.B) {
// 	const CallsPerSched = 1000
// 	procs := runtime.GOMAXPROCS(-1)
// 	N := int32(b.N / CallsPerSched)
// 	c := make(chan bool, 2*procs)
// 	myc := make(chan int, 128)
// 	myclose := make(chan bool)
// 	for p := 0; p < procs; p++ {
// 		go func() {
// 			// Producer: sends to myc.
// 			foo := 0
// 			// Intended to not fire during benchmarking.
// 			mytimer := time.After(time.Hour)
// 			for atomic.AddInt32(&N, -1) >= 0 {
// 				for g := 0; g < CallsPerSched; g++ {
// 					// Model some local work.
// 					for i := 0; i < 100; i++ {
// 						foo *= 2
// 						foo /= 2
// 					}
// 					select {
// 					case myc <- 1:
// 					case <-mytimer:
// 					case <-myclose:
// 					}
// 				}
// 			}
// 			myc <- 0
// 			c <- foo == 42
// 		}()
// 		go func() {
// 			// Consumer: receives from myc.
// 			foo := 0
// 			// Intended to not fire during benchmarking.
// 			mytimer := time.After(time.Hour)
// 		loop:
// 			for {
// 				select {
// 				case v := <-myc:
// 					if v == 0 {
// 						break loop
// 					}
// 				case <-mytimer:
// 				case <-myclose:
// 				}
// 				// Model some local work.
// 				for i := 0; i < 100; i++ {
// 					foo *= 2
// 					foo /= 2
// 				}
// 			}
// 			c <- foo == 42
// 		}()
// 	}
// 	for p := 0; p < procs; p++ {
// 		<-c
// 		<-c
// 	}
// }

// func BenchmarkReceiveDataFromClosedChan(b *testing.B) {
// 	count := b.N
// 	ch := make(chan struct{}, count)
// 	for i := 0; i < count; i++ {
// 		ch <- struct{}{}
// 	}
// 	close(ch)

// 	b.ResetTimer()
// 	for range ch {
// 	}
// }

// func BenchmarkChanCreation(b *testing.B) {
// 	b.RunParallel(func(pb *testing.PB) {
// 		for pb.Next() {
// 			myc := make(chan int, 1)
// 			myc <- 0
// 			<-myc
// 		}
// 	})
// }

// func BenchmarkChanSem(b *testing.B) {
// 	type Empty struct{}
// 	myc := make(chan Empty, runtime.GOMAXPROCS(0))
// 	b.RunParallel(func(pb *testing.PB) {
// 		for pb.Next() {
// 			myc <- Empty{}
// 			<-myc
// 		}
// 	})
// }

// func BenchmarkChanPopular(b *testing.B) {
// 	const n = 1000
// 	c := make(chan bool)
// 	var a []chan bool
// 	var wg sync.WaitGroup
// 	wg.Add(n)
// 	for j := 0; j < n; j++ {
// 		d := make(chan bool)
// 		a = append(a, d)
// 		go func() {
// 			for i := 0; i < b.N; i++ {
// 				select {
// 				case <-c:
// 				case <-d:
// 				}
// 			}
// 			wg.Done()
// 		}()
// 	}
// 	for i := 0; i < b.N; i++ {
// 		for _, d := range a {
// 			d <- true
// 		}
// 	}
// 	wg.Wait()
// }

// func BenchmarkChanClosed(b *testing.B) {
// 	c := make(chan struct{})
// 	close(c)
// 	b.RunParallel(func(pb *testing.PB) {
// 		for pb.Next() {
// 			select {
// 			case <-c:
// 			default:
// 				b.Error("Unreachable")
// 			}
// 		}
// 	})
// }

int xmain(int, char **) {
    debug::init();
    testing::T t;
    test_chan(t);
    //test_select_duplicate_channel(t);
    return 0;
}