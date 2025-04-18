#include "chan.h"

#include "lib/print.h"
#include "lib/sync/atomic.h"
#include "lib/sync/lock.h"
#include "lib/types.h"
#include <atomic>
#include <ctime>
#include <netinet/in.h>
#include <pthread.h>

using namespace lib;
using namespace sync;
using namespace sync::internal;

// notes
// other implementations
// https://github.com/facebook/folly/blob/main/folly/channels/Channel.h
// https://github.com/andreiavrammsd/cpp-channel/blob/master/include/msd/channel.hpp
// https://github.com/atollk/copper/blob/main/include/copper.h
// https://github.com/Balnian/ChannelsCPP
// https://github.com/navytux/pygolang/blob/6aec47842dfc03c16071fa540d7d9d1ab68573f2/golang/runtime/libgolang.cpp

// https://github.com/cameron314/concurrentqueue

#define LOG(...) if (DebugLog) { fmt::printf(__VA_ARGS__); }
#define LOGX(...) // fmt::printf(__VA_ARGS__);
// #define LOG(...)

static __thread uint64 seed = 0;

static inline uint64_t wyrand(uint64_t *seed){  
    *seed += 0xa0761d6478bd642full; 
    uint64_t see1 = *seed ^ 0xe7037ed1a0b428dbull;
    see1 *= (see1>>32)|(see1<<32);
    return  (*seed*((*seed>>32)|(*seed<<32)))^((see1>>32)|(see1<<32));
  }

static inline uint32_t rotl(const uint32_t x, int k) {
	return (x << k) | (x >> (32 - k));
}

static uint32 cheaprand() {
    if constexpr (sizeof(void*) >= 8) {
        return uint32(wyrand(&seed));
    }

    // Implement xorshift64+: 2 32-bit xorshift sequences added together.
	// Shift triplet [17,7,16] was calculated as indicated in Marsaglia's
	// Xorshift paper: https://www.jstatsoft.org/article/view/v008i14/xorshift.pdf
	// This generator passes the SmallCrush suite, part of TestU01 framework:
	// http://simul.iro.umontreal.ca/testu01/tu01.html
	uint32 *t = (uint32*)(&seed);
	uint32 s1 = t[0];
    uint32 s0 = t[1];

	s1 ^= s1 << 17;
	s1 = s1 ^ s0 ^ s1>>7 ^ s0>>16;

	t[0] = s0;
    t[1] = s1;
	
    return s0 + s1;
    // const uint32_t result = rotl(s[0] + s[3], 7) + s[0];

	// const uint32_t t = s[1] << 9;

	// s[2] ^= s[0];
	// s[3] ^= s[1];
	// s[1] ^= s[2];
	// s[0] ^= s[3];

	// s[2] ^= t;

	// s[3] = rotl(s[3], 11);

	// return result;
}

uint32 cheaprandn(uint32 n) {
    // See https://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
	return uint32((uint64(cheaprand()) * uint64(n)) >> 32);
}

String format(Selector *e) {
    if (e == nil) {
        return "<nil>";
    }
    String s;
    s += fmt::sprintf("%#x", (uintptr) e);

    for (Selector *elem = e->prev; elem != nil; elem = elem->prev) {
        mem::touch(elem);
        s = String(fmt::sprintf("%#x <- ", (uintptr) elem)) + s;
    }

    for (Selector *elem = e->next; elem != nil; elem = elem->next) {
        s += String(fmt::sprintf(" -> %#x", (uintptr) elem));
    }

    return "[" + s + "]";
}

ChanBase::ChanBase(int capacity) : capacity(capacity) {
    LOG("%d %#x chan constructed with capacity %v\n", pthread_self(), (uintptr) this, capacity);
    // fmt::printf("%d %#x chan constructed with capacity %v; receivers %#x; senders %#x\n", pthread_self(), (uintptr) this, capacity,
        // (uintptr) &this->adata.receivers, (uintptr) &this->adata.senders);
}

// assumes channel is open
bool ChanBase::send_nonblocking(this ChanBase &c, void *elem, bool move, Data *data_out) {
    if (c.capacity > 0) {
    reload1:
        Data data = c.adata.load();
        do {
        again1:

            LOG("%d %#x send_nonblocking: got q %v; receivers %#x\n", pthread_self(), (uintptr) &c, data.q, (uintptr) data.receivers);
            if (data.q >= c.capacity) {
                if (data_out) *data_out = data;
                return false;
            }

            if (data.receivers == SelectorClosed) {
                // closed
                LOG("%d %#x send_nonblocking: closed\n", pthread_self(), (uintptr) &c);
                if (data_out) *data_out = data;
                return false;
            }

            if (data.q == 0 && data.receivers != nil) {
                LOG("%d %#x send_nonblocking: special case\n", pthread_self(), (uintptr) &c);

                if (data.receivers == SelectorBusy) {
                    goto reload1;
                }

                bool b = c.adata.compare_and_swap(&data, {.q = data.q, .receivers = SelectorBusy, .senders = data.senders});
                if (!b) {
                    goto again1;
                }
                data.receivers->removed = true;
                // fmt::printf("REMOVING 140 %#x from %#x\n", (uintptr) receiver, (uintptr) &c.adata.receivers);
                if (data.receivers->next != nil) {
                    data.receivers->next->prev = nil;
                }
                
                b = true;
                for (Selector::State state = Selector::New; !data.receivers->active->compare_and_swap(&state, Selector::Done);) {
                    if (state == Selector::Done) {
                        b = false;
                        break;
                    }
                    state = Selector::New;
                    // continue if busy
                }

                c.adata.receivers_head_store(data.receivers->next);
                LOG("%d %#x send_nonblocking: receiver popped atomically %#x\n", pthread_self(), (uintptr) &c, (uintptr) data.receivers);
                if (!b) {
                    data = c.adata.load();
                    goto again1;
                }
                LOGX("send 165 won %#x\n", (uintptr) data.receivers->active);

                if (data.receivers->value != nil) {
                    c.set(data.receivers->value, elem, move);
                }
                if (data.receivers->ok != nil) {
                    *data.receivers->ok = true;
                }
                LOGX("%d about to store 171 %#x\n", pthread_self(), (uintptr) data.receivers->completed);
                data.receivers->completer->store(data.receivers);
                data.receivers->completed->notify();
                return true;
            }
        } while (!c.adata.compare_and_swap(&data, {.q = data.q+1, .receivers = data.receivers, .senders = data.senders }));
        // while(c.adata.update(&data, { .q = data.q+1, .receivers = data.receivers, .senders = data.senders }));
        LOG("%d %#x send_nonblocking_unlocked: q %v -> %v %v\n", pthread_self(), (uintptr) &c, data.q, data.q+1, c.adata.q.load());
        
        c.push(elem, move);
        return true;
    }

    // zero capacity case
reload2:
    Data data = c.adata.load();
again2:
    if (data.receivers == nil) {
        LOG("%d %#x send_nonblocking (cap=0): receivers empty; returning\n", pthread_self(), (uintptr) &c);
        if (data_out) *data_out = data;
        return false;
    }
    if (data.receivers == SelectorClosed) {
        if (data_out) *data_out = data;
        return false;
    }
    if (data.receivers == SelectorBusy) {
        goto reload2;
    }

    bool b = c.adata.receivers_head_compare_and_swap(&data.receivers, SelectorBusy);
    if (!b) {
        LOG("%d %#x send_nonblocking (cap=0): compare-and-swap failed\n", pthread_self(), (uintptr)  &c);
        goto again2;
    }
    data.receivers->removed = true;
    // fmt::printf("REMOVING 203 %#x from %#x\n", (uintptr) receiver, (uintptr) &c.adata.receivers);
    if (data.receivers->next) {
        data.receivers->next->prev = nil;
    }
    
    b = true;
    for (Selector::State state = Selector::New; !data.receivers->active->compare_and_swap(&state, Selector::Done);) {
        if (state == Selector::Done) {
            b = false;
            break;
        }
        state = Selector::New;
        // continue if busy
    }

    c.adata.receivers_head_store(data.receivers->next);
    LOG("%d %#x send_nonblocking (cap=0): receiver popped atomically %#x\n", pthread_self(), (uintptr) &c, (uintptr) data.receivers);
    if (!b) {
        goto reload2;
    }
    LOGX("send 226 won %#x\n", (uintptr) data.receivers->active);

    mem::touch(data.receivers);
    if (data.receivers->value) {
        c.set(data.receivers->value, elem, move);
    }
    if (data.receivers->ok) {
        *data.receivers->ok = true;
    }
    if (data.receivers->OUT_OF_SCOPE) {
        panic("OUT OF SCOPE");
    }
    LOGX("%d about to store 238 %#x\n", pthread_self(), (uintptr) data.receivers);
    mem::touch(data.receivers);
    data.receivers->completer->store(data.receivers);
    data.receivers->completed->notify();
    LOGX("%d store done 238 %#x\n", pthread_self(), (uintptr) data.receivers->completed);
    LOG("%d %#x send_nonblocking (cap=0): notified receiver\n", pthread_self(), (uintptr) &c);
    return true;
}

int ChanBase::send_nonblocking_sel(this ChanBase &c, Selector *sender) {
    if (c.capacity > 0) {
reload1:
    Data data = c.adata.load();
    
again1:

        LOG("%d %#x send_nonblocking: got q %v; receivers %#x\n", pthread_self(), (uintptr) &c, data.q, (uintptr) data.receivers);
        if (data.receivers == SelectorClosed) {
            // closed
            LOG("%d %#x send_nonblocking: closed\n", pthread_self(), (uintptr) &c);
            sender->removed = true;
            return -1;
        }

        if (data.q >= c.capacity) {
            if (data.senders == SelectorBusy) {
                goto reload1;
            }
            if (!c.adata.compare_and_swap(&data, {.q = data.q, .receivers = data.receivers, .senders = SelectorBusy})) {
                goto again1;
            }
            sender->next = data.senders;
            if (data.senders != nil) {
                data.senders->prev = sender;
            }
            c.adata.senders_head_store(sender);
            LOGX("ADDED 269 %#x to %#x\n", (uintptr) &sender, (uintptr) &c.adata.senders);
            return -1;
        }

        if (data.q == 0 && data.receivers != nil) {
            LOG("%d %#x send_nonblocking: special case\n", pthread_self(), (uintptr) &c);

            if (data.receivers == SelectorBusy) {
                goto reload1;
            }

            if (!c.adata.compare_and_swap(&data, {.q = data.q, .receivers = SelectorBusy, .senders = data.senders})) {
                goto again1;
            }

            // lock receiver
            for (Selector::State state = Selector::New; !data.receivers->active->compare_and_swap(&state, Selector::Busy);) {
                if (state == Selector::Done) {
                    data.receivers->removed = true;
                    if (data.receivers->next != nil) {
                        data.receivers->next->prev = nil;
                    }
                    c.adata.receivers_head_store(data.receivers->next);
                    goto reload1;
                }
                state = Selector::New;
                // continue if busy
            }

            // lock sender
            for (Selector::State state = Selector::New; !sender->active->compare_and_swap(&state, Selector::Done);) {
                if (state == Selector::Done) {
                    // undo
                    data.receivers->active->store(Selector::New);
                    c.adata.receivers_head_store(data.receivers);

                    sender->completed->wait();
                    return sender->completer->load()->id;
                }
                state = Selector::New;
                // continue if busy
            }
            LOGX("send 320 won %#x %#x\n", (uintptr) data.receivers, (uintptr) sender->active);

            data.receivers->active->store(Selector::Done);
            data.receivers->removed = true;
            // fmt::printf("REMOVING 287 %#x\n", (uintptr) receiver);
            if (data.receivers->next != nil) {
                data.receivers->next->prev = nil;
            }
            c.adata.receivers_head_store(data.receivers->next);

            LOG("%d %#x send_nonblocking: receiver popped atomically %#x\n", pthread_self(), (uintptr) &c, (uintptr) data.receivers);
            if (data.receivers->value != nil) {
                c.set(data.receivers->value, sender->value, sender->move);
            }
            if (data.receivers->ok != nil) {
                *data.receivers->ok = true;
            }
            LOGX("%d about to store 329 %#x\n", pthread_self(), (uintptr) data.receivers);
            data.receivers->completer->store(data.receivers);
            data.receivers->completed->notify();

            return sender->id;
        }

        // lock sender
        for (Selector::State state = Selector::New; !sender->active->compare_and_swap(&state, Selector::Busy);) {
            if (state == Selector::Done) {
                // undo
                // data.receivers->active->store(Selector::New);
                // c.adata.receivers_head_store(data.receivers);

                sender->completed->wait();
                return sender->completer->load()->id;
            }
            state = Selector::New;
            // continue if busy
        }
        if (!c.adata.compare_and_swap(&data, {.q = data.q+1, .receivers = data.receivers, .senders = data.senders})) {
            sender->active->store(Selector::New);
            goto again1;
        }
        LOGX("send 355 won %#x\n", (uintptr) sender->active);

        sender->active->store(Selector::Done);
        c.push(sender->value, sender->move);
        return sender->id;
    }

    // zero capacity case
reload2:
    Data data = c.adata.load();
again2:
    if (data.receivers == SelectorClosed) {
        sender->removed = true;
        return -1;
    }
    if (data.receivers == nil) {
        if (data.senders == SelectorBusy) {
            goto reload2;
        }
        LOG("%d %#x send_nonblocking (cap=0): receivers empty; returning\n", pthread_self(), (uintptr) &c);
        if (!c.adata.compare_and_swap(&data, {.q = data.q, .receivers = data.receivers, .senders = SelectorBusy})) {
            goto again2;
        }
        sender->next = data.senders;
        if (data.senders != nil) {
            data.senders->prev = sender;
        }
        c.adata.senders_head_store(sender);
        LOGX("ADDED 361%#x to %#x\n", (uintptr) &sender, (uintptr) &c.adata.senders);
        return -1;
    }
    if (data.receivers == SelectorBusy) {
        goto reload2;
    }

    if (!c.adata.receivers_head_compare_and_swap(&data.receivers, SelectorBusy)) {
        LOG("%d %#x send_nonblocking (cap=0): compare-and-swap failed\n", pthread_self(), (uintptr)  &c);
        goto again2;
    }
    
    // lock receiver; receiver -> busy
    for (Selector::State state = Selector::New; !data.receivers->active->compare_and_swap(&state, Selector::Busy);) {
        if (state == Selector::Done) {
            data.receivers->removed = true;
            if (data.receivers->next != nil) {
                data.receivers->next->prev = nil;
            }
            c.adata.receivers_head_store(data.receivers->next);        
            goto reload2;
        }
        state = Selector::New;
        // continue if busy
    }

    // lock sender; sender -> done
    for (Selector::State state = Selector::New; !sender->active->compare_and_swap(&state, Selector::Done);) {
        if (state == Selector::Done) {
            // undo
            data.receivers->active->store(Selector::New);
            c.adata.receivers_head_store(data.receivers);
            
            sender->completed->wait();
            return sender->completer->load()->id;
        }
        state = Selector::New;
        // continue if busy
    }
    LOGX("send 411 won %#x %#x\n", (uintptr) data.receivers->active, (uintptr) sender->active);

    data.receivers->active->store(Selector::Done);
    // fmt::printf("REMOVING 356 %#x\n", (uintptr) receiver);

    data.receivers->removed = true;
    if (data.receivers->next != nil) {
        data.receivers->next->prev = nil;
    }
    c.adata.receivers_head_store(data.receivers->next);
    
    LOG("%d %#x send_nonblocking (cap=0): receiver popped atomically %#x\n", pthread_self(), (uintptr) &c, (uintptr) data.receivers);

    if (data.receivers->value) {
        c.set(data.receivers->value, sender->value, sender->move);
    }
    if (data.receivers->ok) {
        *data.receivers->ok = true;
    }
    LOGX("%d about to store 419 %#x\n", pthread_self(), (uintptr) data.receivers);
    data.receivers->completer->store(data.receivers);
    data.receivers->completed->notify();
    LOG("%d %#x send_nonblocking (cap=0): notified receiver\n", pthread_self(), (uintptr) &c);
    return sender->id;
}


void ChanBase::send_blocking(this ChanBase &c, void *elem, bool move) {
    LOG("%d %#x send_blocking\n", pthread_self(), (uintptr) &c);

again:
    //sync::Lock lock(c.lock);
    Data data;
    bool ok = c.send_nonblocking(elem, move, &data);
    if (ok) {
        LOG("%d %#x send_blocking: send_blocking succeeded\n", pthread_self(), (uintptr) &c);
        return;
    }

    LOG("%d %#x send_blocking: send_blocking failed\n", pthread_self(), (uintptr)  &c);

    if (data.senders == SelectorClosed) {
        panic("send on closed channel");
    }

    if (data.senders == SelectorBusy) {
        goto again;
    }

    bool b = c.adata.compare_and_swap(&data, {.q = data.q, .receivers = data.receivers, .senders = SelectorBusy});
    if (!b) {
        LOG("%d %#x send_blocking: compare_and_swap failed\n", pthread_self(), (uintptr)  &c);
        goto again;
    }

    atomic<Selector::State> active = Selector::New;
    atomic<Selector*> completer = nil;
    Waiter completed;
    LOGX("%d allocated completed 453 %#x\n", pthread_self(), (uintptr) &completed);

    Selector sender = {
        .value = elem,
        .move  = move,
        .active = &active,
        .completer = &completer,
        .completed = &completed,
        .next = data.senders,
    } ;
    if (data.senders != nil) {
        if (data.senders->removed) {
            fmt::printf("WAS REMOVED %#x\n", (uintptr) data.senders);
            panic("removed");
        }

        data.senders->prev = &sender;
    }

    c.adata.senders_head_store(&sender);
    // LOGX("ADDED 428 %#x to %#x\n", (uintptr) &sender, (uintptr) &c.adata.senders);
    // fmt::printf("sender in %#x\n", (uintptr) &sender);

    // lock.unlock();
    LOG("%d %#x send_blocking: waiting\n", pthread_self(), (uintptr)  &c);
    completed.wait();

    // lock.relock();
    c.adata.senders_remove(&sender);

    if (sender.panic) {
        panic("send on closed channel");
    }

    LOG("%d %#x send_blocking: sender going out of scope: %#x\n", pthread_self(), (uintptr) &c, (uintptr) &sender);
    // if constexpr (DebugChecks) {
    //     Data data;
    //     for (;;) {
    //         data = c.adata.load();
    //         while (data.senders == SelectorBusy) {
    //             data = c.adata.load();
    //         }
    //         bool ok = c.adata.senders_head_compare_and_swap(&data.senders, SelectorBusy);
    //         if (ok) {
    //             break;
    //         }
    //     }

    //     fmt::printf("list %s\n", format(data.senders));
    //     c.adata.senders_head_store(data.senders);
        
    // }

    // fmt::printf("sender out %#x\n", (uintptr) &sender);
    // for (Selector *e = c.adata.senders.head.value; e != nil && intptr(e) > 0; e = e->next) {
    //     if (e == &sender) {
    //         // fmt::printf("SENDER removed %v\n", sender.removed);
    //         panic("!");
    //     }
    // }
    sender.OUT_OF_SCOPE = true;
    LOGX("%d allocated 513 out of scope %#x\n", pthread_self(), (uintptr) &completed);
}

void ChanBase::send_i(this ChanBase &c, void *elem, bool move) {
    c.send_blocking(elem, move);
}

bool ChanBase::send_nonblocking_i(this ChanBase &c, void *elem, bool move) {    
    sync::Lock lock(c.lock);
    Data data;

    if (c.send_nonblocking(elem, move, &data)) {
        return true;
    }

    if (data.senders == SelectorClosed) {
        panic("send on closed channel");
    }

    return false;
}

bool ChanBase::recv_nonblocking(this ChanBase &c, void *out, bool *okp, Data *data_out) {
    if (c.capacity > 0) {
    reload1:
        Data data = c.adata.load();
        do {
        again1:

            LOG("%d %#x recv_nonblocking: got q %v\n", pthread_self(), (uintptr)  &c, data.q);
            if (data.q <= 0) {
                if (data.receivers == SelectorClosed) {
                    if (okp) {
                        *okp = false;
                    }
                    return true;
                }

                if (data_out) *data_out = data;
                return false;
            }

            if (data.q == c.capacity && data.senders != nil && data.senders != SelectorClosed) {
                LOG("%d %#x recv_nonblocking: special case\n", pthread_self(), (uintptr)  &c);

                if (data.senders == SelectorBusy) {
                    goto reload1;
                }

                if (!c.adata.compare_and_swap(&data, {.q = data.q, .receivers = data.receivers, .senders = SelectorBusy})) {
                    LOG("%d %#x recv_nonblocking: compare-and-swap failed\n", pthread_self(), (uintptr)  &c);
                    goto again1;
                }
                data.senders->removed = true;
                // fmt::printf("REMOVING 518 %#x from %#x\n", (uintptr) sender, (uintptr) &c.adata.senders);
                if (data.senders->next != nil) {
                    data.senders->next->prev = nil;
                }
                
                for (Selector::State state = Selector::New; !data.senders->active->compare_and_swap(&state, Selector::Done);) {
                    if (state == Selector::Done) {
                        c.adata.senders_head_store(data.senders->next);
                        goto reload1;
                    }
                    state = Selector::New;
                    // continue if busy
                }

                c.pop(out);
                if (okp) {
                    *okp = true;
                }

                LOG("%d %#x recv_nonblocking: special case push\n", pthread_self(), (uintptr)  &c);
                c.push(data.senders->value, data.senders->move);

                c.adata.senders_head_store(data.senders->next);
                LOG("%d %#x recv_nonblocking: sender popped atomically %#x\n", pthread_self(), (uintptr) &c, (uintptr) data.senders);
                
                LOGX("recv 604 won %#x\n", (uintptr) data.senders->active);
                LOGX("%d about to store 601 %#x\n", pthread_self(), (uintptr) data.senders);
                data.senders->completer->store(data.senders);
                data.senders->completed->notify();
                
                return true;
            }
        } while (!c.adata.compare_and_swap(&data, {.q = data.q-1, .receivers = data.receivers, .senders = data.senders }));
        LOG("%d %#x recv_nonblocking: %v -> %v; actual %v\n", pthread_self(), (uintptr)  &c, data.q, data.q-1, c.adata.q.load());


        c.pop(out);
        
        if (okp) {
            *okp = true;
        }
        return true;
    }

    // zero capacity case
reload2:
    Data data = c.adata.load();
again2:
    if (data.senders == nil) {
        if (data_out) *data_out = data;
        return false;
    }
    if (data.senders == SelectorClosed) {
        if (okp) {
            *okp = false;
        }
        LOG("%d %#x recv_nonblocking (cap=0): closed; early return\n", pthread_self(), (uintptr) &c);
        return true;
    }
    if (data.senders == SelectorBusy) {
        // fmt::printf("busy going back\n");
        goto reload2;
    }

    bool b = c.adata.senders_head_compare_and_swap(&data.senders, SelectorBusy);
    if (!b) {
        LOG("%d %#x recv_nonblocking: compare-and-swap failed\n", pthread_self(), (uintptr)  &c);
        goto again2;
    }
    LOG("%d %#x recv_nonblocking (cap=0): locked\n", pthread_self(), (uintptr) &c);
    data.senders->removed = true;
    // fmt::printf("REMOVING 590 %#x from %#x\n", (uintptr) sender, (uintptr) &c.adata.senders);
    
    if (data.senders->next) {
        // fmt::printf("About to clear sender->next->prev %#x\n", (uintptr) sender->next);
        data.senders->next->prev = nil;
    }

    b = true;
    for (Selector::State state = Selector::New; !data.senders->active->compare_and_swap(&state, Selector::Done);) {
        if (state == Selector::Done) {
            b = false;
            break;
        }
        state = Selector::New;
        // continue if busy
    }

    LOG("%d %#x recv_nonblocking (cap=0): sender popped atomically %#x\n", pthread_self(), (uintptr) &c, (uintptr) data.senders);
    c.adata.senders_head_store(data.senders->next);
    // LOGX("ADDED 657 %#x to %#x\n", (uintptr) &data.senders->next, (uintptr) &c.adata.senders);
    LOG("%d %#x recv_nonblocking (cap=0): unlocked\n", pthread_self(), (uintptr) &c);
    if (!b) {
        goto reload2;
    }
    LOGX("recv 682 won %#x\n", (uintptr) data.senders->active);

    if (out) {
        c.set(out, data.senders->value, data.senders->move);
    }
    if (okp) {
        *okp = true;
    }

    LOG("%d %#x recv_nonblocking (cap=0): about to touch...\n", pthread_self(), (uintptr) &c);
    mem::touch(data.senders);
    LOG("%d %#x recv_nonblocking (cap=0): touched %#x\n", pthread_self(), (uintptr) &c);
    LOGX("%d about to store 673 %#x\n", pthread_self(), (uintptr) data.senders);
    data.senders->completer->store(data.senders);
    // LOGX("About to touch %#x\n", (uintptr) data.senders->completed);
    mem::touch(data.senders->completed);
    data.senders->completed->notify();
    // LOGX("touched %#x\n", (uintptr) data.senders->completed);
    return true;
}

int ChanBase::recv_nonblocking_sel(this ChanBase &c, Selector *receiver) {
    if (c.capacity > 0) {
reload1:
    Data data = c.adata.load();
        
again1:

        LOG("%d %#x recv_nonblocking: got q %v\n", pthread_self(), (uintptr)  &c, data.q);
        if (data.q <= 0) {
            if (data.receivers == SelectorClosed) {
                if (receiver->ok) {
                    *receiver->ok = false;
                }

                for (Selector::State state = Selector::New; !receiver->active->compare_and_swap(&state, Selector::Done);) {
                    if (state == Selector::Done) {
                        receiver->completed->wait();
                        return receiver->completer->load()->id;
                    }
                    state = Selector::New;
                    // continue if busy
                }
                LOGX("recv 725 won %#x\n", (uintptr) receiver->active);
                return receiver->id;
            }
            if (data.receivers == SelectorBusy) {
                goto reload1;
            }
            
            if (!c.adata.compare_and_swap(&data, {.q = data.q, .receivers = SelectorBusy, .senders = data.senders})) {
                goto again1;
            }
            receiver->next = data.receivers;
            if (data.receivers != nil) {
                data.receivers->prev = receiver;
            }
            c.adata.receivers_head_store(receiver);
            LOGX("%d 740\n", pthread_self());
            return -1;
        }

        if (data.q == c.capacity && data.senders != nil && data.senders != SelectorClosed) {
            LOG("%d %#x recv_nonblocking: special case\n", pthread_self(), (uintptr)  &c);

            if (data.senders == SelectorBusy) {
                goto reload1;
            }

            if (!c.adata.compare_and_swap(&data, {.q = data.q, .receivers = data.receivers, .senders = SelectorBusy})) {
                LOG("%d %#x recv_nonblocking: compare-and-swap failed\n", pthread_self(), (uintptr) &c);
                goto again1;
            }

            // lock sender
            for (Selector::State state = Selector::New; !data.senders->active->compare_and_swap(&state, Selector::Busy);) {
                if (state == Selector::Done) {
                    data.senders->removed = true;
                    if (data.senders->next != nil) {
                        data.senders->next->prev = nil;
                    }
                    c.adata.senders_head_store(data.senders->next);
                    LOGX("ADDED 738 %#x to %#x\n", (uintptr) &data.senders->next, (uintptr) &c.adata.senders);
                    goto reload1;
                }
                state = Selector::New;
                // continue if busy
            }

            // lock receiver
            for (Selector::State state = Selector::New; !receiver->active->compare_and_swap(&state, Selector::Done);) {
                if (state == Selector::Done) {
                    // undo
                    data.senders->active->store(Selector::New);
                    c.adata.senders_head_store(data.senders);
                    receiver->completed->wait();
                    LOGX("%d 778\n", pthread_self());
                    return receiver->completer->load()->id;
                }
                state = Selector::New;
                // continue if busy
            }
            LOGX("recv 769 won %#x %#x\n", (uintptr) receiver->active, (uintptr) data.senders->active);

            data.senders->active->store(Selector::Done);
            data.senders->removed = true;
            // fmt::printf("REMOVING 689 %#x\n", (uintptr) sender);
            if (data.senders->next != nil) {
                data.senders->next->prev = nil;
            }

            c.pop(receiver->value);
            if (receiver->ok) {
                *receiver->ok = true;
            }

            LOG("%d %#x recv_nonblocking: special case push\n", pthread_self(), (uintptr)  &c);
            c.push(data.senders->value, data.senders->move);
            
            c.adata.senders_head_store(data.senders->next);
            LOGX("ADDED 763 %#x to %#x\n", (uintptr) &data.senders->next, (uintptr) &c.adata.senders);

            LOG("%d %#x recv_nonblocking: sender popped atomically %#x\n", pthread_self(), (uintptr) &c, (uintptr) data.senders);
            
            LOGX("%d about to store 777 %#x\n", pthread_self(), (uintptr) data.senders);
            data.senders->completer->store(data.senders);
            data.senders->completed->notify();
            
            LOGX("%d 807\n", pthread_self());
            return receiver->id;
        }

        // lock receiver
        for (Selector::State state = Selector::New; !receiver->active->compare_and_swap(&state, Selector::Busy);) {
            if (state == Selector::Done) {
                receiver->completed->wait();
                return receiver->completer->load()->id;
            }
            state = Selector::New;
            // continue if busy
        }

        if (!c.adata.compare_and_swap(&data, {.q = data.q-1, .receivers = data.receivers, .senders = data.senders})) {
            receiver->active->store(Selector::New);
            goto again1;
        }
        LOGX("recv 820 won %#x\n", (uintptr) receiver->active);

        receiver->active->store(Selector::Done);
        c.pop(receiver->value);
        
        if (receiver->ok) {
            *receiver->ok = true;
        }
        
        return receiver->id;
    }

    // zero capacity case
reload2:
    Data data = c.adata.load();
again2:
    if (data.senders == nil) {
        if (data.receivers == SelectorBusy) {
            goto reload2;
        }
        if (!c.adata.compare_and_swap(&data, {.q = data.q, .receivers = SelectorBusy, .senders = data.senders})) {
            goto again2;
        }
        receiver->next = data.receivers;
        if (data.receivers) {
            data.receivers->prev = receiver;
        }
        c.adata.receivers_head_store(receiver);
        return -1;
    }
    if (data.senders == SelectorClosed) {
        if (receiver->ok) {
            *receiver->ok = false;
        }
        LOG("%d %#x recv_nonblocking (cap=0): closed; early return\n", pthread_self(), (uintptr) &c);
        return receiver->id;
    }
    if (data.senders == SelectorBusy) {
        // fmt::printf("busy going back\n");
        goto reload2;
    }

    if (!c.adata.senders_head_compare_and_swap(&data.senders, SelectorBusy)) {
        LOG("%d %#x recv_nonblocking: compare-and-swap failed\n", pthread_self(), (uintptr)  &c);
        goto again2;
    }
    LOG("%d %#x recv_nonblocking (cap=0): locked\n", pthread_self(), (uintptr) &c);
    
    // lock sender; sender -> busy
    for (Selector::State state = Selector::New; !data.senders->active->compare_and_swap(&state, Selector::Busy);) {
        if (state == Selector::Done) {
            data.senders->removed = true;
            if (data.senders->next != nil) {
                data.senders->next->prev = nil;
            }
            c.adata.senders_head_store(data.senders->next);        
            goto reload2;
        }
        state = Selector::New;
        // continue if busy
    }

    // lock receiver; receiver -> done
    for (Selector::State state = Selector::New; !receiver->active->compare_and_swap(&state, Selector::Done);) {
        if (state == Selector::Done) {
            // undo
            data.senders->active->store(Selector::New);
            c.adata.senders_head_store(data.senders);
            receiver->completed->wait();
            return receiver->completer->load()->id;
        }
        state = Selector::New;
        // continue if busy
    }
    LOGX("recv 881 won %#x\n", (uintptr) data.senders->active, (uintptr) receiver->active);
    data.senders->active->store(Selector::Done);
    data.senders->removed = true;
    // fmt::printf("REMOVING 767 %#x\n", (uintptr) sender); 
    if (data.senders->next != nil) {
        data.senders->next->prev = nil;
    }
    c.adata.senders_head_store(data.senders->next);
    LOGX("ADDED 857 %#x to %#x\n", (uintptr) &data.senders->next, (uintptr) &c.adata.senders);

    LOG("%d %#x recv_nonblocking (cap=0): sender popped atomically %#x\n", pthread_self(), (uintptr) &c, (uintptr) data.senders);
    
    if (receiver->value) {
        c.set(receiver->value, data.senders->value, data.senders->move);
    }
    if (receiver->ok) {
        *receiver->ok = true;
    }

    LOG("%d %#x recv_nonblocking (cap=0): about to touch...\n", pthread_self(), (uintptr) &c);
    mem::touch(data.senders);
    LOG("%d %#x recv_nonblocking (cap=0): touched %#x\n", pthread_self(), (uintptr) &c);
    LOGX("%d about to store 877 %#x\n", pthread_self(), (uintptr) data.senders);
    data.senders->completer->store(data.senders);
    data.senders->completed->notify();
    return receiver->id;
}

void ChanBase::recv_blocking(this ChanBase &c, void *out, bool *ok) {
    LOG("%d %#x recv_blocking\n", pthread_self(), (uintptr) &c);

again:
    Data data;
    if (c.recv_nonblocking(out, ok, &data)) {
        LOG("%d %#x recv_blocking: recv nonblocking unlocked succeeded\n", pthread_self(), (uintptr) &c);
        return;
    }
    sync::Lock lock(c.lock);
    LOG("%d %#x recv_blocking: recv nonblocking unlocked failed\n", pthread_self(), (uintptr) &c);

    if (data.receivers == SelectorBusy) {
        goto again;
    }

    bool b = c.adata.compare_and_swap(&data, {.q = data.q, .receivers = SelectorBusy, .senders = data.senders});
    if (!b) {
        LOG("%d %#x recv_blocking: compare_and_swap failed\n", pthread_self(), (uintptr)  &c);
        goto again;
    }

    atomic<Selector::State> active = Selector::New;
    atomic<Selector*> completer = nil;
    Waiter completed;
    LOGX("allocated completed 902 %#x\n", (uintptr) &completed);

    Selector receiver = {
        .value = out,
        .ok = ok,
        .active = &active,
        .completer = &completer,
        .completed = &completed,
        .next = data.receivers,
    };
    if (data.receivers) {
        data.receivers->prev = &receiver;
    }
    c.adata.receivers_head_store(&receiver);
    LOGX("ADDED 830 %#x to %#x\n", (uintptr) &receiver, (uintptr) &c.adata.receivers);
    // fmt::printf("receiver in %#x\n", (uintptr) &receiver);
    
    lock.unlock();
    LOG("%d %#x recv_blocking: about to wait\n", pthread_self(), (uintptr) &c);
    completed.wait();

    lock.relock();
    c.adata.receivers_remove(&receiver);
    LOG("%d %#x recv_blocking: receiver going out of scope: %#x\n", pthread_self(), (uintptr) &c, (uintptr) &receiver);
    // fmt::printf("receiver out %#x\n", (uintptr) &receiver);
    for (Selector *e = c.adata.receivers.head.value; e != nil && intptr(e) > 0; e = e->next) {
        if (e == &receiver) {
            panic("!");
        }
    }
    receiver.OUT_OF_SCOPE = true;
}

void ChanBase::close(this ChanBase &c) {
    LOG("%d %#x close: channel closing...\n", pthread_self(), (uintptr) &c);
    Lock lock(c.lock);


    Data data = c.adata.load();
again:
    if (data.senders == SelectorClosed) {
        panic("close of closed channel");
    }

    if (!c.adata.compare_and_swap(&data, { .q = data.q, .receivers = SelectorClosed, .senders = SelectorClosed})) {
        goto again;
    }

    c.state.store(Closed);

    for (Selector *sender = data.senders; sender != nil; sender = sender->next) {
        bool ok = true;
        for (Selector::State state = Selector::New; !sender->active->compare_and_swap(&state, Selector::Done);) {
            if (state == Selector::Done) {
                ok = false;
                break;
            }
            state = Selector::New;
            // continue if busy
        }
        if (!ok) {
            continue;
        }
        sender->panic = true;
        sender->removed = true;
        LOGX("%d about to store 972 %#x\n", pthread_self(), (uintptr) sender);
        sender->completer->store(sender);
        sender->completed->notify();
    }

    // int i = 0, j =0;
    for (Selector *receiver = data.receivers; receiver != nil; receiver = receiver->next) {
        bool ok = true;
        for (Selector::State state = Selector::New; !receiver->active->compare_and_swap(&state, Selector::Done);) {
            if (state == Selector::Done) {
                ok = false;
                break;
            }
            state = Selector::New;
            // continue if busy
        }
        if (!ok) {
            continue;
        }

        if (receiver->value) {
            c.set(receiver->value, nil, false);
        }
        if (receiver->ok) {
            *receiver->ok = false;
        }

        receiver->removed = true;
        LOGX("%d about to store 999 %#x\n", pthread_self(), (uintptr) receiver);
        receiver->completer->store(receiver);
        receiver->completed->notify();
    }

    LOG("%d %#x close: channel closed\n", pthread_self(), (uintptr) &c);
}

bool ChanBase::closed(this ChanBase const& c) {
    return c.state.load() == Closed;
}

bool ChanBase::is_buffered(this ChanBase const& c) {
    return c.capacity > 0;
}

bool ChanBase::is_empty(this ChanBase const& c) {
    return c.unread() == 0;
}
bool ChanBase::is_full(this ChanBase const& c) {
    // printf("is_full unread %d; capacity %d\n", c.unread, c.capacity);
    return c.unread() >= c.capacity;
}

int ChanBase::length() const {
    return this->unread();
}

bool ChanBase::try_recv(this ChanBase &c, void *out, bool *ok, bool try_locks, bool *lock_fail) {
    Lock lock(c.lock);
    return c.recv_nonblocking(out, ok, nil);
}

int ChanBase::subscribe_recv(this ChanBase &c, internal::Selector &receiver, Lock &lock) {
    return c.recv_nonblocking_sel(&receiver);
}

int ChanBase::subscribe_send(this ChanBase &c, internal::Selector &sender, Lock &lock) {
    return c.send_nonblocking_sel(&sender);
}

void ChanBase::unsubscribe_recv(this ChanBase &c, internal::Selector &receiver, Lock&) {
    c.adata.receivers.remove(&receiver);
}

void ChanBase::unsubscribe_send(this ChanBase &c, internal::Selector &sender, Lock&) {
    c.adata.senders.remove(&sender);
}


bool Recv::poll(bool try_locks, bool *lock_fail) const {
    ChanBase & c = *this->chan;

    return c.try_recv(this->data, this->ok, try_locks, lock_fail);
}

bool Recv::select(bool blocking) const {
    ChanBase & c = *this->chan;
    if (blocking) {
        c.recv_blocking(this->data, this->ok);
        return true;
    }

    sync::Lock lock(c.lock);
    return c.recv_nonblocking(this->data, this->ok, nil);
}

bool Send::poll(bool try_locks, bool *lock_fail) const {
    ChanBase & c = *this->chan;
    // if (c.closed()) {
    //     LOG("%d %#X Send::poll: closed; returning early\n", pthread_self(), (uintptr) &c);
    //     return false;
    // }

    sync::Lock lock(c.lock);
    return c.send_nonblocking(this->data, this->move, nil);
}

bool Send::select(bool blocking) const {
    ChanBase & c = *this->chan;
    if (blocking) {
        c.send_i(this->data, this->move);
        return true;
    }

    return c.send_nonblocking_i(this->data, this->move);
}


int Recv::subscribe(sync::internal::Selector &receiver, Lock &lock) const {
    ChanBase &c = *this->chan;

    receiver.value = this->data;
    receiver.ok = this->ok;
    
    return c.subscribe_recv(receiver, lock);
}

int Send::subscribe(sync::internal::Selector &sender, Lock &lock) const {
    ChanBase &c = *this->chan;

    sender.value = this->data;
    sender.move = this->move;
    
    return c.subscribe_send(sender, lock);
}

void Recv::unsubscribe(sync::internal::Selector &receiver, Lock &lock) const {
    ChanBase &c = *this->chan;

    c.unsubscribe_recv(receiver, lock);
}

void Send::unsubscribe(sync::internal::Selector &receiver, Lock &lock) const {
    ChanBase &c = *this->chan;

    c.unsubscribe_send(receiver, lock);
}

int internal::select_i(arr<OpData> ops, arr<OpData*> ops_ptrs, arr<OpData*> lockfail_ptrs, bool block) {
    // fill ops_ptrs array
    int avail_ops = 0;
    int lockfail_cnt = 0;
    int cnt = 0;
    for (OpData &op : ops) {
        op.selector.id = cnt++;

        // skip nil cases
        if (op.op.chan == nil) {
            continue;
        }

        ops_ptrs[avail_ops++] = &op;
    }

    // LOG("%d select_i: avail_ops %d\n", pthread_self(), avail_ops);
    for (int i = avail_ops; i > 0; i--) {
        int selected_idx = cheaprandn(i);
        OpData *selected_op = ops_ptrs[selected_idx];
        
        bool lockfail = false;
        bool ok = selected_op->op.poll(true, &lockfail);
        if (ok) {
            return int(selected_op - ops.data);
        }

        if (lockfail) {
            lockfail_ptrs[lockfail_cnt++] = selected_op;
        }

        //ops_ptrs[selected_idx] = ops_ptrs[i-1];
        std::swap(ops_ptrs[selected_idx], ops_ptrs[i-1]);
    }

    for (int i = 0; i < lockfail_cnt; i++) {
        OpData *selected_op = lockfail_ptrs[i];

        bool ok = selected_op->op.poll(false, nil);
        if (ok) {
            return int(selected_op - ops.data);
        }
    }    

    if (!block) {
        return -1;
    }
    
    // sort opts_ptr by lock order
    std::sort(ops_ptrs.begin(), ops_ptrs.begin()+avail_ops, [](OpData *d1, OpData *d2) {
        return uintptr(d1->op.chan) < uintptr(d2->op.chan);
    });
    
    atomic<Selector::State> active = Selector::New;
    atomic<Selector*> completer = nil;
    Waiter completed;
    LOGX("%d select_i active 1163 %#x\n", pthread_self(), (uintptr) &active);

    int i = 0;
    // lock in lock order and subscribe
    for (; i < avail_ops; i++) {
        OpData &op = *ops_ptrs[i];

        op.selector.active = &active;
        op.selector.completer = &completer;
        op.selector.completed = &completed;

        if (i == 0 || &op.op.chan->lock != &ops_ptrs[i-1]->op.chan->lock) {
            // skip re-locking the same lock
            op.chanlock.lock(op.op.chan->lock);
        } else {
        }

        LOGX("%d select_i subscribing %#x\n", pthread_self(), (uintptr) &op.selector);
        int result = op.op.subscribe(op.selector, op.chanlock);
        if (result != -1) {
            for (int j = i-1; j >= 0; j--) {
                OpData &op = *ops_ptrs[j];
                LOGX("%d select_i 1205 unsubscribing %#x %d\n", pthread_self(), (uintptr) &op.selector, result);
                op.op.unsubscribe(op.selector, op.chanlock);
            }
            return result;
        }
    }

    // unlock
    for (int j = i-1; j >= 0; j--) {
        OpData &op = *ops_ptrs[j];
        if (op.chanlock.locked) {
            op.chanlock.unlock();
        }
    }

    completed.wait();
    Selector *selected = completer.load();
    LOG("%d select_i completed %#x %d\n", pthread_self(), selected, completed.state.load());

    // unsubscribe
    for (int j = 0; j < i; j++) {
        OpData &op = *ops_ptrs[j];

        if (&op.selector == selected) {
            LOGX("%d select_i 1228 skip %#x\n", pthread_self(), (uintptr) &op.selector);
            continue;
        }
        Lock lock(op.op.chan->lock);
        LOGX("%d select_i 1232 unsubscribing %#x\n", pthread_self(), (uintptr) &op.selector);
        op.op.unsubscribe(op.selector, lock);

        if constexpr (DebugChecks) {
            if (op.op.chan->adata.receivers.contains(&op.selector)) {
                panic("DebugCheck1");
            }
            if (op.op.chan->adata.senders.contains(&op.selector)) {
                panic("DebugCheck2");
            }
        }
    }

    return selected->id;
}

bool internal::select_one(OpData const &op, bool blocking) {
    return op.op.select(blocking);
}
void lib::sync::Chan<void>::push(void *, bool) {
//   int n = unread_v.load();

// try_again:
//   if (n >= capacity) {
//     return false;
//   }

//   bool ok = unread_v.compare_and_swap(&n, n + 1,);
//   if (!ok) {
//     goto try_again;
//   }

//   return true;
}

int lib::sync::Chan<void>::unread() const { 
    return unread_v.load(); 
}

void IntrusiveList::push(Selector *e) {
    // printf("PUSH this(%p) %p\n", this, e);
    LOG("%d %#x IntrusiveList pushing %#x\n", pthread_self(), (uintptr) this, (uintptr) e);
    reload:
    Selector *head;
    {
        sync::Lock lock(mtx);
        head = this->head.load();
    }
    again:
    if (head == SelectorBusy) {
        // fmt::printf("push busy\n");
        goto reload;
    }

    if (head == SelectorClosed) {
        panic("!");
    }

    {
        sync::Lock lock(mtx);
        if (!this->head.compare_and_swap(&head, SelectorBusy)) {
            LOG("IntrusiveList::push compare_and_swap failed\n");
            goto again;
        }
    }

    e->next = head;
    e->prev = nil;

    if (head != nil) {
        // clean up
        // for (Selector *elem = head->prev; elem != nil; elem = elem->prev) {
        //     elem->removed = true;        
        // }
        
        head->prev = e;
    }

    // this->head = e;
    sync::Lock lock(*mtx);
    this->head.store(e);
    // fmt::printf("%d %#x PUSH %#x: list %s\n", pthread_self(), (uintptr) this, (uintptr) e, format(e));
    // dump();
}

Selector *IntrusiveList::pop() {
    // printf("POP this(%p)\n", this);

    // clean up
    // for (Selector *elem = e->prev; elem != nil; elem = elem->prev) {
    //     elem->removed = true;        
    // }

    reload:
    Selector *head;
    {
        sync::Lock lock(mtx);
        head = this->head.load();
    }
    again:
    if (head == SelectorBusy) {
        goto reload;
    }

    if (uintptr(head) == -2) {
        panic("!");
    }

    {
        sync::Lock lock(mtx);
        if (!this->head.compare_and_swap(&head, SelectorBusy)) {
            goto again;
        }
    }

    if (head->next) {
        head->next->prev = nil;
    }

    if (head->prev) {
        panic("!");
        head->prev->next = nil;
    }
    
    head->removed = true;
    // fmt::printf("REMOVING 1283 %#x\n", (uintptr) head);

    sync::Lock lock(*mtx);
    this->head.store(head->next);
    LOG("%d %#x IntrusiveList popped %#x\n", pthread_self(), (uintptr) this, (uintptr) head);
    return head;
}

void IntrusiveList::remove(Selector *e) {
    LOG("%d %#x IntrusiveList removing %#x\n", pthread_self(), (uintptr) this, (uintptr) e);

    // if (e->removed) {
        // LOG("%d %#x IntrusiveList removing %#x: already removed; returning\n", pthread_self(), (uintptr) this, (uintptr) e);
        // return;
    // }

    reload:
    Selector *head;
    {
        sync::Lock lock(mtx);
        head = this->head.load();
    }
    again:
    if (head == SelectorBusy) {
        goto reload;
    }

    if (uintptr(head) == -2) {
        return;
    }

    {
        sync::Lock lock(mtx);
        if (!this->head.compare_and_swap(&head, SelectorBusy)) {
            goto again;
        }
    }
    

    LOG("%d %#x IntrusiveList removing %#x locked\n", pthread_self(), (uintptr) this, (uintptr) e);

    if (e->removed) {
        if (e == head) {
            panic("!");
        }
        sync::Lock lock(*mtx);
        this->head.store(head);
        LOG("%d %#x IntrusiveList removing %#x already removed; unlocked\n", pthread_self(), (uintptr) this, (uintptr) e);
        return;
    }
    e->removed = true;
    // fmt::printf("REMOVING 1333 %#x %d %#x\n", (uintptr) e, pthread_self(), (uintptr) this);

    if constexpr (DebugChecks) {
        bool found = false;
        for (Selector *elem = head; elem != nil; elem = elem->next) {
            if (elem == e) {
                found = true;
                break;
            }
        }
        if (!found) {
            fmt::printf("%d %#x IntrusiveList removing %#x; not found in list %s\n", pthread_self(), (uintptr) this, (uintptr) e, format(head));
            panic("element not found");
        }
    }

    if constexpr (DebugLog) {
        fmt::printf("%d %#x IntrusiveList removing %#x; list %s\n", pthread_self(), (uintptr) this, (uintptr) e, format(head));
        fmt::printf("%d %#x IntrusiveList removing %#x (%s); list %s\n", pthread_self(), (uintptr) this, (uintptr) e, format(e), format(head));
    }

    if (e->prev) {
        if (e->prev->OUT_OF_SCOPE) {
            panic("out of scope");
        }

        e->prev->next= e->next;
    }
    if (e->next) {
        mem::touch(e->next);
        if (e->next->OUT_OF_SCOPE) {
            panic("out of scope");
        }
        e->next->prev = e->prev;
    }

    if (head == e) {
        head = head->next;
    }

    sync::Lock lock(*mtx);
    this->head.store(head);
    LOG("%d %#x IntrusiveList removing %#x unlocked\n", pthread_self(), (uintptr) this, (uintptr) e);

    // if (head == nil) {
    //     return;
    // }

    // if (head == e) {
    //     if (!this->head.compare_and_swap(&head, head->next)) {
    //         LOG("%d %#x IntrusiveList removing at head failed\n", pthread_self(), (uintptr) this);
    //         goto again;
    //     }
    //     LOG("%d %#x IntrusiveList removing at head\n", pthread_self(), (uintptr) this);

    //     if (head->next == nil) {
    //         LOG("%d %#x IntrusiveList list is now empty\n", pthread_self(), (uintptr) this);
    //         // list is now empty
    //         return;
    //     }
    // }

    // // clean up
    // for (Selector *elem = head; elem->prev != nil; ) {
    //     LOG("%d %#x IntrusiveList list cleaning up %#x\n", pthread_self(), (uintptr) this, (uintptr) elem->prev);
    //     Selector *prev = elem->prev;
    //     elem->prev = nil;
    //     elem = prev;
    // }

    // if (e->prev == nil) {
    //     if constexpr (DebugLog) {
    //         fmt::printf("%d %#x IntrusiveList no prev\n", pthread_self(), (uintptr) this);
    //         dump("REMOVE AFTER");
    //     }
    //     return;
    // }

    // LOG("%d %#x IntrusiveList list removing from list %#x\n", pthread_self(), (uintptr) this, (uintptr) e);
    // // remove e from the list
    // e->prev->next = e->next;
    // if (e->next) {
    //     e->next->prev = e->prev;
    // }

    // if constexpr (DebugLog) {
    //     fmt::printf("%d %#x IntrusiveList done\n", pthread_self(), (uintptr) this);
    //     dump("REMOVE AFTER");
    // }
}


void IntrusiveList::dump(str s) {
    Selector *head = this->head.load();
    if (!head) {
        fmt::printf("%d %#x IntrusiveList %s empty\n", pthread_self(), (uintptr) this, s);
        return;
    }
    fmt::printf("%d %#x InstursveList %s contents: %s\n", pthread_self(), (uintptr) this, s, format(head));
    // io::Buffer b;
    // fmt::fprintf(b, "%d %#x InstursveList %s contents: [%#x", pthread_self(), (uintptr) this, s, (uintptr) head);
    // for (Selector *elem = head->next; elem != nil; elem = elem->next) {
    //     fmt::fprintf(b, " -> %#x", (uintptr) elem);
    // }

    // fmt::fprintf(b, "]\n");
    // os::stdout.write(b.str(), error::ignore);
}

bool IntrusiveList::contains(Selector *e) {
    Selector *head = this->head.load();
    while (intptr(head) < 0) {
        return false;
        //head = this->head.load();
    }

    for (Selector *elem = head; elem != nil; ) {

        if (e == elem) {
            return true;
        }

        Selector *next = elem->next;
        while (intptr(next) < 0) {
            return false;
            // next = sync::load(elem->next);
        }

        elem = next;
    }

    return false;
}
bool IntrusiveList::empty() const {
    Selector *head = this->head.load();
    return head == nil;
}

bool IntrusiveList::empty_atomic() const {
    Selector *head = this->head.load();
    return head == nil;
}

ChanBase::Data ChanBase::AtomicData::load() {
    sync::Lock lock(mtx);
    return Data{
        .q = q.load(),
        .receivers = receivers.head.load(),
        .senders = senders.head.load(),
    };
  }
  
bool ChanBase::AtomicData::compare_and_swap(Data *expected, Data newval) {
    sync::Lock lock(mtx);

    // fmt::printf("%d UPDATE CALLED\n", pthread_self());

    if (expected->receivers != receivers.head.load() || expected->senders != senders.head.load()) {
        expected->receivers = receivers.head.load();
        expected->senders = senders.head.load();
        LOG("AtomicData update failed because of unexpected receivers or senders\n");
        return false;
    }

    if (!this->q.compare_and_swap(&expected->q, newval.q)) {
        LOG("AtomicData update failed because of unexpected q\n");
        return false;
    }
    this->receivers.head = newval.receivers;
    this->senders.head = newval.senders;

    // *expected = newval;

    // fmt::printf("%d UPDATE SUCCESS %v %v\n", pthread_self(), this->q.value, expected->q);
    return true;
}