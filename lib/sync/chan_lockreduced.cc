#include "chan.h"

#include "lib/print.h"
#include "lib/sync/atomic.h"
#include "lib/sync/lock.h"
#include "lib/types.h"
#include <atomic>
#include <boost/atomic/atomic_ref.hpp>
#include <boost/memory_order.hpp>
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
        // (uintptr) &this->adata.selector, (uintptr) &this->adata.selector);
}

bool ChanBase::send_nonblocking(this ChanBase &c, void *elem, bool move, Data *data_out) {
    if (c.capacity > 0) {
        return c.send_nonblocking_buffered(elem, move, data_out);
    } else {
        return c.send_nonblocking_unbuffered(elem, move, data_out);
    }
}

bool ChanBase::send_nonblocking_buffered(this ChanBase &c, void *elem, bool move, Data *data_out) {
reload1:
    Data data = c.adata.load();
    do {
    again1:

        LOG("%d %#x send_nonblocking: got q %v; receivers %#x\n", pthread_self(), (uintptr) &c, data.q, (uintptr) data.selector);
        if (data.q >= c.capacity) {
            if (data_out) *data_out = data;
            return false;
        }

        if (data.selector == SelectorClosed) {
            // closed
            LOG("%d %#x send_nonblocking: closed\n", pthread_self(), (uintptr) &c);
            if (data_out) *data_out = data;
            return false;
        }

        if (data.q == 0 && data.selector != nil) {
            LOG("%d %#x send_nonblocking: special case\n", pthread_self(), (uintptr) &c);

            if (data.selector == SelectorBusy) {
                goto reload1;
            }

            bool b = c.adata.compare_and_swap(&data, {.q = data.q, .selector = SelectorBusy}); // cleared on line 160
            if (!b) {
                goto again1;
            }
            data.selector->removed = true;
            // fmt::printf("REMOVING 140 %#x from %#x\n", (uintptr) receiver, (uintptr) &c.adata.selector);
            if (data.selector->next != nil) {
                data.selector->next->prev = nil;
            }
            
            b = true;
            for (Selector::State state = Selector::New; !data.selector->active->compare_and_swap(&state, Selector::Done);) {
                if (state == Selector::Done) {
                    b = false;
                    break;
                }
                state = Selector::New;
                // continue if busy
            }

            c.adata.selector_store(data.selector->next);
            LOG("%d %#x send_nonblocking: receiver popped atomically %#x\n", pthread_self(), (uintptr) &c, (uintptr) data.selector);
            if (!b) {
                data = c.adata.load();
                goto again1;
            }
            LOGX("send 165 won %#x\n", (uintptr) data.selector->active);

            if (data.selector->value != nil) {
                c.set(data.selector->value, elem, move);
            }
            if (data.selector->ok != nil) {
                *data.selector->ok = true;
            }
            LOGX("%d about to store 171 %#x\n", pthread_self(), (uintptr) data.selector->completed);
            data.selector->completer->store(data.selector);
            data.selector->completed->notify();
            return true;
        }
    } while (!c.adata.compare_and_swap(&data, {.q = data.q+1, .selector = data.selector}));
    LOG("%d %#x send_nonblocking_unlocked: q %v -> %v %v\n", pthread_self(), (uintptr) &c, data.q, data.q+1, c.adata.load().q);
    
    c.push(elem, move);
    return true;

}

bool ChanBase::send_nonblocking_unbuffered(this ChanBase &c, void *elem, bool move, Data *data_out) {
    // zero capacity case
reload2:
    Data data = c.adata.load();
again2:
    if (data.selector == SelectorBusy) {
        goto reload2;
    }
    if (data.selector == SelectorClosed) {
        if (data_out) *data_out = data;
        return false;
    }
    if (data.q != -1 || data.selector == nil) {  // no receivers
        LOG("%d %#x send_nonblocking (cap=0): receivers empty; returning\n", pthread_self(), (uintptr) &c);
        if (data_out) *data_out = data;
        return false;
    }
    
    if (!c.adata.compare_and_swap(&data, {.q = data.q, .selector = SelectorBusy})) {  // cleared on 223
        LOG("%d %#x send_nonblocking (cap=0): compare-and-swap failed\n", pthread_self(), (uintptr)  &c);
        goto again2;
    }
    data.selector->removed = true;
    // fmt::printf("REMOVING 203 %#x from %#x\n", (uintptr) receiver, (uintptr) &c.adata.selector);
    if (data.selector->next) {
        data.selector->next->prev = nil;
    }
    
    bool b = true;
    for (Selector::State state = Selector::New; !data.selector->active->compare_and_swap(&state, Selector::Done);) {
        if (state == Selector::Done) {
            b = false;
            break;
        }
        state = Selector::New;
        // continue if busy
    }

    c.adata.selector_store(data.selector->next);
    LOG("%d %#x send_nonblocking (cap=0): receiver popped atomically %#x\n", pthread_self(), (uintptr) &c, (uintptr) data.selector);
    if (!b) {
        goto reload2;
    }
    LOGX("send 226 won %#x\n", (uintptr) data.selector->active);

    mem::touch(data.selector);
    if (data.selector->value) {
        c.set(data.selector->value, elem, move);
    }
    if (data.selector->ok) {
        *data.selector->ok = true;
    }
    if (data.selector->OUT_OF_SCOPE) {
        panic("OUT OF SCOPE");
    }
    LOGX("%d about to store 238 %#x\n", pthread_self(), (uintptr) data.selector);
    mem::touch(data.selector);
    data.selector->completer->store(data.selector);
    data.selector->completed->notify();
    LOGX("%d store done 238 %#x\n", pthread_self(), (uintptr) data.selector->completed);
    LOG("%d %#x send_nonblocking (cap=0): notified receiver\n", pthread_self(), (uintptr) &c);
    return true;
}

int ChanBase::send_sel(this ChanBase &c, Selector *sender) {
    if (c.capacity > 0) {
        return c.send_sel_buffered(sender);
    } else {
        return c.send_sel_unbuffered(sender);;
    }
}

int ChanBase::send_sel_buffered(this ChanBase &c, Selector *sender) {
reload1:
    Data data = c.adata.load();
    
again1:
    LOG("%d %#x send_nonblocking: got q %v; receivers %#x\n", pthread_self(), (uintptr) &c, data.q, (uintptr) data.selector);
    if (data.selector == SelectorClosed) {
        // closed
        LOG("%d %#x send_nonblocking: closed\n", pthread_self(), (uintptr) &c);
        sender->removed = true;
        return -1;
    }

    if (data.q >= c.capacity) {
        if (data.selector == SelectorBusy) {
            goto reload1;
        }
        if (!c.adata.compare_and_swap(&data, {.q = data.q, .selector = SelectorBusy})) {  // cleareed on 275
            goto again1;
        }
        sender->next = data.selector;
        if (data.selector != nil) {
            data.selector->prev = sender;
        }
        c.adata.selector_store(sender);
        LOGX("ADDED 269 %#x to %#x\n", (uintptr) &sender, (uintptr) &c.adata.selector);
        return -1;
    }

    if (data.q == 0 && data.selector != nil) {
        LOG("%d %#x send_nonblocking: special case\n", pthread_self(), (uintptr) &c);

        if (data.selector == SelectorBusy) {
            goto reload1;
        }

        if (!c.adata.compare_and_swap(&data, {.q = data.q, .selector = SelectorBusy})) {  // cleared on 298, 310, 326
            goto again1;
        }

        // lock receiver
        for (Selector::State state = Selector::New; !data.selector->active->compare_and_swap(&state, Selector::Busy);) {
            if (state == Selector::Done) {
                data.selector->removed = true;
                if (data.selector->next != nil) {
                    data.selector->next->prev = nil;
                }
                c.adata.selector_store(data.selector->next);
                goto reload1;
            }
            state = Selector::New;
            // continue if busy
        }

        // lock sender
        for (Selector::State state = Selector::New; !sender->active->compare_and_swap(&state, Selector::Done);) {
            if (state == Selector::Done) {
                // undo
                data.selector->active->store(Selector::New);
                c.adata.selector_store(data.selector);

                sender->completed->wait();
                return sender->completer->load()->id;
            }
            state = Selector::New;
            // continue if busy
        }
        LOGX("send 320 won %#x %#x\n", (uintptr) data.selector, (uintptr) sender->active);

        data.selector->active->store(Selector::Done);
        data.selector->removed = true;
        // fmt::printf("REMOVING 287 %#x\n", (uintptr) receiver);
        if (data.selector->next != nil) {
            data.selector->next->prev = nil;
        }
        c.adata.selector_store(data.selector->next);

        LOG("%d %#x send_nonblocking: receiver popped atomically %#x\n", pthread_self(), (uintptr) &c, (uintptr) data.selector);
        if (data.selector->value != nil) {
            c.set(data.selector->value, sender->value, sender->move);
        }
        if (data.selector->ok != nil) {
            *data.selector->ok = true;
        }
        LOGX("%d about to store 329 %#x\n", pthread_self(), (uintptr) data.selector);
        data.selector->completer->store(data.selector);
        data.selector->completed->notify();

        return sender->id;
    }

    // lock sender
    for (Selector::State state = Selector::New; !sender->active->compare_and_swap(&state, Selector::Busy);) {
        if (state == Selector::Done) {
            // undo
            // data.selector->active->store(Selector::New);
            // c.adata.receivers_head_store(data.selector);

            sender->completed->wait();
            return sender->completer->load()->id;
        }
        state = Selector::New;
        // continue if busy
    }
    if (!c.adata.compare_and_swap(&data, {.q = data.q+1, .selector = data.selector})) {
        sender->active->store(Selector::New);
        goto again1;
    }
    LOGX("send 355 won %#x\n", (uintptr) sender->active);

    sender->active->store(Selector::Done);
    c.push(sender->value, sender->move);
    return sender->id;
}

int ChanBase::send_sel_unbuffered(this ChanBase &c, Selector *sender) {
    // zero capacity case
reload2:
    Data data = c.adata.load();
again2:
    if (data.selector == SelectorBusy) {
        goto reload2;
    }
    if (data.selector == SelectorClosed) {
        sender->removed = true;
        return -1;
    }
    if (data.q != -1 || data.selector == nil) {  // no receivers
        LOG("%d %#x send_nonblocking (cap=0): receivers empty; returning\n", pthread_self(), (uintptr) &c);
        if (!c.adata.compare_and_swap(&data, {.q = data.q, .selector = SelectorBusy})) {  // cleared on 379
            goto again2;
        }
        sender->next = data.selector;
        if (data.selector != nil) {
            data.selector->prev = sender;
        }
        // c.adata.senders_head_store(sender);
        c.adata.store({.q = 1, .selector = sender});
        LOGX("ADDED 361%#x to %#x\n", (uintptr) &sender, (uintptr) &c.adata.selector);
        return -1;
    }

    if (!c.adata.compare_and_swap(&data, {.q = data.q, .selector = SelectorBusy})) {  // cleared on  407, 419, 435
        LOG("%d %#x send_nonblocking (cap=0): compare-and-swap failed\n", pthread_self(), (uintptr)  &c);
        goto again2;
    }

    // lock receiver then sender (order matters)
    
    // lock receiver; receiver -> busy
    for (Selector::State state = Selector::New; !data.selector->active->compare_and_swap(&state, Selector::Busy);) {
        if (state == Selector::Done) {
            data.selector->removed = true;
            if (data.selector->next != nil) {
                data.selector->next->prev = nil;
            }
            c.adata.selector_store(data.selector->next);        
            goto reload2;
        }
        state = Selector::New;
        // continue if busy
    }

    // lock sender; sender -> done
    for (Selector::State state = Selector::New; !sender->active->compare_and_swap(&state, Selector::Done);) {
        if (state == Selector::Done) {
            // undo
            data.selector->active->store(Selector::New);
            c.adata.selector_store(data.selector);
            
            sender->completed->wait();
            return sender->completer->load()->id;
        }
        state = Selector::New;
        // continue if busy
    }
    LOGX("send 411 won %#x %#x\n", (uintptr) data.selector->active, (uintptr) sender->active);

    data.selector->active->store(Selector::Done);
    // fmt::printf("REMOVING 356 %#x\n", (uintptr) receiver);

    data.selector->removed = true;
    if (data.selector->next != nil) {
        data.selector->next->prev = nil;
    }
    c.adata.selector_store(data.selector->next);
    
    LOG("%d %#x send_nonblocking (cap=0): receiver popped atomically %#x\n", pthread_self(), (uintptr) &c, (uintptr) data.selector);

    if (data.selector->value) {
        c.set(data.selector->value, sender->value, sender->move);
    }
    if (data.selector->ok) {
        *data.selector->ok = true;
    }
    LOGX("%d about to store 419 %#x\n", pthread_self(), (uintptr) data.selector);
    data.selector->completer->store(data.selector);
    data.selector->completed->notify();
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

    if (data.selector == SelectorClosed) {
        panic("send on closed channel");
    }

    if (data.selector == SelectorBusy) {
        goto again;
    }

    bool b = c.adata.compare_and_swap(&data, {.q = data.q, .selector = SelectorBusy});  // cleared on 504, 506
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
        .next = data.selector,
    } ;
    if (data.selector != nil) {
        if (data.selector->removed) {
            fmt::printf("WAS REMOVED %#x\n", (uintptr) data.selector);
            panic("removed");
        }

        data.selector->prev = &sender;
    }

    if (c.capacity > 0) {
        c.adata.selector_store(&sender);
    } else {
        c.adata.store({.q = 1, .selector = &sender});
    }
    
    // LOGX("ADDED 428 %#x to %#x\n", (uintptr) &sender, (uintptr) &c.adata.selector);
    // fmt::printf("sender in %#x\n", (uintptr) &sender);

    // lock.unlock();
    LOG("%d %#x send_blocking: waiting\n", pthread_self(), (uintptr)  &c);
    completed.wait();

    // lock.relock();
    c.adata.remove(&sender);

    if (sender.panic) {
        panic("send on closed channel");
    }

    LOG("%d %#x send_blocking: sender going out of scope: %#x\n", pthread_self(), (uintptr) &c, (uintptr) &sender);
    // if constexpr (DebugChecks) {
    //     Data data;
    //     for (;;) {
    //         data = c.adata.load();
    //         while (data.selector == SelectorBusy) {
    //             data = c.adata.load();
    //         }
    //         bool ok = c.adata.senders_head_compare_and_swap(&data.selector, SelectorBusy);
    //         if (ok) {
    //             break;
    //         }
    //     }

    //     fmt::printf("list %s\n", format(data.selector));
    //     c.adata.senders_head_store(data.selector);
        
    // }

    // fmt::printf("sender out %#x\n", (uintptr) &sender);
    // for (Selector *e = c.adata.selector.head.value; e != nil && intptr(e) > 0; e = e->next) {
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
    // sync::Lock lock(c.lock);
    Data data;

    if (c.send_nonblocking(elem, move, &data)) {
        return true;
    }

    if (data.selector == SelectorClosed) {
        panic("send on closed channel");
    }

    return false;
}

bool ChanBase::recv_nonblocking(this ChanBase &c, void *out, bool *okp, Data *data_out) {
    if (c.capacity > 0) {
        return c.recv_nonblocking_buffered(out, okp, data_out);
    } else {
        return c.recv_nonblocking_unbuffered(out, okp, data_out);
    }
}

bool ChanBase::recv_nonblocking_buffered(this ChanBase &c, void *out, bool *okp, Data *data_out) {
reload1:
    Data data = c.adata.load();
    do {
    again1:

        LOG("%d %#x recv_nonblocking: got q %v\n", pthread_self(), (uintptr)  &c, data.q);
        if (data.q <= 0) {
            if (data.selector == SelectorClosed) {
                if (okp) {
                    *okp = false;
                }
                return true;
            }

            if (data_out) *data_out = data;
            return false;
        }

        if (data.q == c.capacity && data.selector != nil && data.selector != SelectorClosed) {
            LOG("%d %#x recv_nonblocking: special case\n", pthread_self(), (uintptr)  &c);

            if (data.selector == SelectorBusy) {
                goto reload1;
            }

            if (!c.adata.compare_and_swap(&data, {.q = data.q, .selector = SelectorBusy})) {   // cleared on  611, 626
                LOG("%d %#x recv_nonblocking: compare-and-swap failed\n", pthread_self(), (uintptr)  &c);
                goto again1;
            }
            data.selector->removed = true;
            // fmt::printf("REMOVING 518 %#x from %#x\n", (uintptr) sender, (uintptr) &c.adata.selector);
            if (data.selector->next != nil) {
                data.selector->next->prev = nil;
            }
            
            for (Selector::State state = Selector::New; !data.selector->active->compare_and_swap(&state, Selector::Done);) {
                if (state == Selector::Done) {
                    c.adata.selector_store(data.selector->next);
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
            c.push(data.selector->value, data.selector->move);

            c.adata.selector_store(data.selector->next);
            LOG("%d %#x recv_nonblocking: sender popped atomically %#x\n", pthread_self(), (uintptr) &c, (uintptr) data.selector);
            
            LOGX("recv 604 won %#x\n", (uintptr) data.selector->active);
            LOGX("%d about to store 601 %#x\n", pthread_self(), (uintptr) data.selector);
            data.selector->completer->store(data.selector);
            data.selector->completed->notify();
            
            return true;
        }
    } while (!c.adata.compare_and_swap(&data, {.q = data.q-1, .selector = data.selector}));
    LOG("%d %#x recv_nonblocking: %v -> %v; actual %v\n", pthread_self(), (uintptr)  &c, data.q, data.q-1, c.adata.load().q);


    c.pop(out);
    
    if (okp) {
        *okp = true;
    }
    return true;
    
}

bool ChanBase::recv_nonblocking_unbuffered(this ChanBase &c, void *out, bool *okp, Data *data_out) {
    // zero capacity case
reload2:
    Data data = c.adata.load();
again2:
    if (data.selector == SelectorBusy) {
        // fmt::printf("busy going back\n");
        goto reload2;
    }
    if (data.selector == SelectorClosed) {
        if (okp) {
            *okp = false;
        }
        LOG("%d %#x recv_nonblocking (cap=0): closed; early return\n", pthread_self(), (uintptr) &c);
        return true;
    }
    if (data.q != 1 || data.selector == nil) {  // no senders
        if (data_out) *data_out = data;
        return false;
    }

    if (!c.adata.compare_and_swap(&data, {.q = data.q, .selector = SelectorBusy})) {  // cleared on 692
        LOG("%d %#x recv_nonblocking: compare-and-swap failed\n", pthread_self(), (uintptr)  &c);
        goto again2;
    }
    LOG("%d %#x recv_nonblocking (cap=0): locked\n", pthread_self(), (uintptr) &c);
    data.selector->removed = true;
    
    if (data.selector->next) {
        data.selector->next->prev = nil;
    }

    bool b = true;
    for (Selector::State state = Selector::New; !data.selector->active->compare_and_swap(&state, Selector::Done);) {
        if (state == Selector::Done) {
            b = false;
            break;
        }
        state = Selector::New;
        // continue if busy
    }

    LOG("%d %#x recv_nonblocking (cap=0): sender popped atomically %#x\n", pthread_self(), (uintptr) &c, (uintptr) data.selector);
    c.adata.selector_store(data.selector->next);
    // c.adata.store({.q = data.q, .selector = data.selector->next});
    // LOGX("ADDED 657 %#x to %#x\n", (uintptr) &data.selector->next, (uintptr) &c.adata.selector);
    LOG("%d %#x recv_nonblocking (cap=0): unlocked\n", pthread_self(), (uintptr) &c);
    if (!b) {
        goto reload2;
    }
    LOGX("recv 682 won %#x\n", (uintptr) data.selector->active);

    if (out) {
        c.set(out, data.selector->value, data.selector->move);
    }
    if (okp) {
        *okp = true;
    }

    LOG("%d %#x recv_nonblocking (cap=0): about to touch...\n", pthread_self(), (uintptr) &c);
    mem::touch(data.selector);
    LOG("%d %#x recv_nonblocking (cap=0): touched %#x\n", pthread_self(), (uintptr) &c);
    LOGX("%d about to store 673 %#x\n", pthread_self(), (uintptr) data.selector);
    data.selector->completer->store(data.selector);
    mem::touch(data.selector->completed);
    data.selector->completed->notify();
    return true;
}

int ChanBase::recv_sel(this ChanBase &c, Selector *receiver) {
    if (c.capacity > 0) {
        return c.recv_sel_buffered(receiver);
    } else {
        return c.recv_sel_unbuffered(receiver);
    }
}

int ChanBase::recv_sel_buffered(this ChanBase &c, Selector *receiver) {
reload1:
    Data data = c.adata.load(); 
again1:
    LOG("%d %#x recv_nonblocking: got q %v\n", pthread_self(), (uintptr)  &c, data.q);
    if (data.q <= 0) {
        if (data.selector == SelectorClosed) {
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
        if (data.selector == SelectorBusy) {
            goto reload1;
        }
        
        if (!c.adata.compare_and_swap(&data, {.q = data.q, .selector = SelectorBusy})) {  // cleared on 756
            goto again1;
        }
        receiver->next = data.selector;
        if (data.selector != nil) {
            data.selector->prev = receiver;
        }
        c.adata.selector_store(receiver);
        LOGX("%d 740\n", pthread_self());
        return -1;
    }

    if (data.q == c.capacity && data.selector != nil && data.selector != SelectorClosed) {
        LOG("%d %#x recv_nonblocking: special case\n", pthread_self(), (uintptr)  &c);

        if (data.selector == SelectorBusy) {
            goto reload1;
        }

        if (!c.adata.compare_and_swap(&data, {.q = data.q, .selector = SelectorBusy})) {  // clared on 777, 796, 822
            LOG("%d %#x recv_nonblocking: compare-and-swap failed\n", pthread_self(), (uintptr) &c);
            goto again1;
        }

        // lock receiver then sender (order matters)
        // lock receiver; receiver -> busy
        for (Selector::State state = Selector::New; !receiver->active->compare_and_swap(&state, Selector::Busy);) {
            if (state == Selector::Done) {
                c.adata.selector_store(data.selector);
                receiver->completed->wait();
                return receiver->completer->load()->id;
            }
            state = Selector::New;
            // continue if busy
        }

        // lock sender; sender -> done
        for (Selector::State state = Selector::New; !data.selector->active->compare_and_swap(&state, Selector::Done);) {
            if (state == Selector::Done) {
                // undo
                receiver->active->store(Selector::New);

                // next sender
                data.selector->removed = true;
                if (data.selector->next != nil) {
                    data.selector->next->prev = nil;
                }
                c.adata.selector_store(data.selector->next);      

                // TODO go to next sender instead  
                goto reload1;
            }

            state = Selector::New;
            // continue if busy
        }
        LOGX("recv 769 won %#x %#x\n", (uintptr) receiver->active, (uintptr) data.selector->active);

        receiver->active->store(Selector::Done);
        data.selector->removed = true;
        // fmt::printf("REMOVING 689 %#x\n", (uintptr) sender);
        if (data.selector->next != nil) {
            data.selector->next->prev = nil;
        }

        c.pop(receiver->value);
        if (receiver->ok) {
            *receiver->ok = true;
        }

        LOG("%d %#x recv_nonblocking: special case push\n", pthread_self(), (uintptr)  &c);
        c.push(data.selector->value, data.selector->move);
        
        c.adata.selector_store(data.selector->next);
        LOGX("ADDED 763 %#x to %#x\n", (uintptr) &data.selector->next, (uintptr) &c.adata.selector);

        LOG("%d %#x recv_nonblocking: sender popped atomically %#x\n", pthread_self(), (uintptr) &c, (uintptr) data.selector);
        
        LOGX("%d about to store 777 %#x\n", pthread_self(), (uintptr) data.selector);
        data.selector->completer->store(data.selector);
        data.selector->completed->notify();
        
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

    if (!c.adata.compare_and_swap(&data, {.q = data.q-1, .selector = data.selector})) {
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

int ChanBase::recv_sel_unbuffered(this ChanBase &c, Selector *receiver) {
    // zero capacity case
reload2:
    Data data = c.adata.load();
again2:
    // if (data.q == -1 && data.selector != nil)
    if (data.selector == SelectorBusy) {
        // fmt::printf("busy going back\n");
        goto reload2;
    }
    if (data.selector == SelectorClosed) {
        if (receiver->ok) {
            *receiver->ok = false;
        }
        LOG("%d %#x recv_nonblocking (cap=0): closed; early return\n", pthread_self(), (uintptr) &c);
        return receiver->id;
    }
    if (data.q != 1 || data.selector == nil) { // no senders

        if (!c.adata.compare_and_swap(&data, {.q = data.q, .selector = SelectorBusy})) {  // cleared on 886
            goto again2;
        }
        receiver->next = data.selector;
        if (data.selector) {
            data.selector->prev = receiver;
        }
        // c.adata.receivers_head_store(receiver);
        c.adata.store({.q = -1, .selector = receiver});
        return -1;
    }

    if (!c.adata.compare_and_swap(&data, {.q = data.q, .selector = SelectorBusy})) {  // cleard on 900, 919, 935
        LOG("%d %#x recv_nonblocking: compare-and-swap failed\n", pthread_self(), (uintptr)  &c);
        goto again2;
    }
    LOG("%d %#x recv_nonblocking (cap=0): locked\n", pthread_self(), (uintptr) &c);

    // lock receiver then sender (order matters)
    // lock receiver; receiver -> busy
    for (Selector::State state = Selector::New; !receiver->active->compare_and_swap(&state, Selector::Busy);) {
        if (state == Selector::Done) {
            c.adata.selector_store(data.selector);
            receiver->completed->wait();
            return receiver->completer->load()->id;
        }
        state = Selector::New;
        // continue if busy
    }

    // lock sender; sender -> done
    for (Selector::State state = Selector::New; !data.selector->active->compare_and_swap(&state, Selector::Done);) {
        if (state == Selector::Done) {
            // undo
            receiver->active->store(Selector::New);

            // next sender
            data.selector->removed = true;
            if (data.selector->next != nil) {
                data.selector->next->prev = nil;
            }
            c.adata.selector_store(data.selector->next);      

            // TODO go to next sender instead  
            goto reload2;
        }

        state = Selector::New;
        // continue if busy
    }

    receiver->active->store(Selector::Done);
    data.selector->removed = true;
    // fmt::printf("REMOVING 767 %#x\n", (uintptr) sender); 
    if (data.selector->next != nil) {
        data.selector->next->prev = nil;
    }
    c.adata.selector_store(data.selector->next);
    // c.adata.store({.q = data.q, .selector = data.selector->next});
    LOGX("ADDED 857 %#x to %#x\n", (uintptr) &data.selector->next, (uintptr) &c.adata.selector);

    LOG("%d %#x recv_nonblocking (cap=0): sender popped atomically %#x\n", pthread_self(), (uintptr) &c, (uintptr) data.selector);
    
    if (receiver->value) {
        c.set(receiver->value, data.selector->value, data.selector->move);
    }
    if (receiver->ok) {
        *receiver->ok = true;
    }

    LOG("%d %#x recv_nonblocking (cap=0): about to touch...\n", pthread_self(), (uintptr) &c);
    mem::touch(data.selector);
    LOG("%d %#x recv_nonblocking (cap=0): touched %#x\n", pthread_self(), (uintptr) &c);
    LOGX("%d about to store 877 %#x\n", pthread_self(), (uintptr) data.selector);
    data.selector->completer->store(data.selector);
    data.selector->completed->notify();
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
    // sync::Lock lock(c.lock);
    LOG("%d %#x recv_blocking: recv nonblocking unlocked failed\n", pthread_self(), (uintptr) &c);

    if (data.selector == SelectorBusy) {
        goto again;
    }

    bool b = c.adata.compare_and_swap(&data, {.q = data.q, .selector = SelectorBusy}); // cleared on 996, 993
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
        .next = data.selector,
    };
    if (data.selector) {
        data.selector->prev = &receiver;
    }
    
    if (c.capacity > 0)  {
        c.adata.selector_store(&receiver);
    } else {
        c.adata.store({.q = -1, .selector = &receiver});
    }
    
    LOGX("ADDED 830 %#x to %#x\n", (uintptr) &receiver, (uintptr) &c.adata.selector);
    
    LOG("%d %#x recv_blocking: about to wait\n", pthread_self(), (uintptr) &c);
    completed.wait();

    
    c.adata.remove(&receiver);
    LOG("%d %#x recv_blocking: receiver going out of scope: %#x\n", pthread_self(), (uintptr) &c, (uintptr) &receiver);
    receiver.OUT_OF_SCOPE = true;
}

void ChanBase::close(this ChanBase &c) {
    LOG("%d %#x close: channel closing...\n", pthread_self(), (uintptr) &c);

reload:
    Data data = c.adata.load();
again:
    if (data.selector == SelectorClosed) {
        panic("close of closed channel");
    }

    if (data.selector == SelectorBusy) {
        goto reload;
    }

    if (!c.adata.compare_and_swap(&data, { .q = data.q, .selector = SelectorClosed})) {
        goto again;
    }

    c.state.store(Closed);

    if ((c.capacity > 0 && data.q == c.capacity) || (c.capacity == 0 && data.q == 1)) {
        for (Selector *sender = data.selector; sender != nil; sender = sender->next) {
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
    }

    if ((c.capacity > 0 && data.q == 0) || (c.capacity == 0 && data.q == -1)) {
        for (Selector *receiver = data.selector; receiver != nil; receiver = receiver->next) {
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

bool ChanBase::try_recv(this ChanBase &c, void *out, bool *ok) {
    return c.recv_nonblocking(out, ok, nil);
}

int ChanBase::subscribe_recv(this ChanBase &c, internal::Selector &receiver) {
    return c.recv_sel(&receiver);
}

int ChanBase::subscribe_send(this ChanBase &c, internal::Selector &sender) {
    return c.send_sel(&sender);
}

void ChanBase::unsubscribe_recv(this ChanBase &c, internal::Selector &receiver) {
    c.adata.remove(&receiver);
}

void ChanBase::unsubscribe_send(this ChanBase &c, internal::Selector &sender) {
    c.adata.remove(&sender);
}


bool Recv::poll() const {
    ChanBase & c = *this->chan;

    return c.try_recv(this->data, this->ok);
}

bool Recv::select(bool blocking) const {
    ChanBase & c = *this->chan;
    if (blocking) {
        c.recv_blocking(this->data, this->ok);
        return true;
    }

    // sync::Lock lock(c.lock);
    return c.recv_nonblocking(this->data, this->ok, nil);
}

bool Send::poll() const {
    ChanBase & c = *this->chan;
    // if (c.closed()) {
    //     LOG("%d %#X Send::poll: closed; returning early\n", pthread_self(), (uintptr) &c);
    //     return false;
    // }

    // sync::Lock lock(c.lock);
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


int Recv::subscribe(sync::internal::Selector &receiver) const {
    ChanBase &c = *this->chan;

    receiver.value = this->data;
    receiver.ok = this->ok;
    
    return c.subscribe_recv(receiver);
}

int Send::subscribe(sync::internal::Selector &sender) const {
    ChanBase &c = *this->chan;

    sender.value = this->data;
    sender.move = this->move;
    
    return c.subscribe_send(sender);
}

void Recv::unsubscribe(sync::internal::Selector &receiver) const {
    ChanBase &c = *this->chan;

    c.unsubscribe_recv(receiver);
}

void Send::unsubscribe(sync::internal::Selector &receiver) const {
    ChanBase &c = *this->chan;

    c.unsubscribe_send(receiver);
}

int internal::select_i(arr<OpData> ops, arr<OpData*> ops_ptrs, bool block) {
    // if (!block) {
    //     int avail_ops = 0;
    //     int cnt = 0;
    //     for (OpData &op : ops) {
    //         op.selector.id = cnt++;

    //         // skip nil cases
    //         if (op.op.chan == nil) {
    //             continue;
    //         }

    //         ops_ptrs[avail_ops++] = &op;
    //     }

    //     for (int i = avail_ops; i > 0; i--) {
    //         int selected_idx = cheaprandn(i);
    //         OpData *selected_op = ops_ptrs[selected_idx];
            
    //         bool ok = selected_op->op.poll();
    //         if (ok) {
    //             return int(selected_op - ops.data);
    //         }

    //         std::swap(ops_ptrs[selected_idx], ops_ptrs[i-1]);
    //     }
    //     return -1;
    // } else {
    //     int avail_ops = 0;
    //     int cnt = 0;
    //     for (OpData &op : ops) {
    //         op.selector.id = cnt++;

    //         // skip nil cases
    //         if (op.op.chan == nil) {
    //             continue;
    //         }

    //         ops_ptrs[avail_ops++] = &op;
    //     }

    //     atomic<Selector::State> active = Selector::New;
    //     atomic<Selector*> completer = nil;
    //     Waiter completed;
    //     LOGX("%d select_i active 1163 %#x\n", pthread_self(), (uintptr) &active);
    
    //     int i = 0;
    //     for (; i < avail_ops; i++) {
    //         OpData &op = *ops_ptrs[i];
    
    //         op.selector.active = &active;
    //         op.selector.completer = &completer;
    //         op.selector.completed = &completed;
    
    //         LOGX("%d select_i subscribing %#x\n", pthread_self(), (uintptr) &op.selector);
    //         int result = op.op.subscribe(op.selector);
    //         if (result != -1) {
    //             for (int j = i-1; j >= 0; j--) {
    //                 OpData &op = *ops_ptrs[j];
    //                 LOGX("%d select_i 1205 unsubscribing %#x %d\n", pthread_self(), (uintptr) &op.selector, result);
    //                 op.op.unsubscribe(op.selector);
    //             }
    //             return result;
    //         }
    //     }
    
    //     completed.wait();
    //     Selector *selected = completer.load();
    //     LOG("%d select_i completed %#x %d\n", pthread_self(), selected, completed.state.load());
    
    //     // unsubscribe
    //     for (int j = 0; j < i; j++) {
    //         OpData &op = *ops_ptrs[j];
    
    //         if (&op.selector == selected) {
    //             LOGX("%d select_i 1228 skip %#x\n", pthread_self(), (uintptr) &op.selector);
    //             continue;
    //         }
    //         LOGX("%d select_i 1232 unsubscribing %#x\n", pthread_self(), (uintptr) &op.selector);
    //         op.op.unsubscribe(op.selector);
    //     }
    
    //     return selected->id;
    // }
    // return -1;
    
    int avail_ops = 0;
    int cnt = 0;
    for (OpData &op : ops) {
        op.selector.id = cnt++;

        // skip nil cases
        if (op.op.chan == nil) {
            continue;
        }

        ops_ptrs[avail_ops++] = &op;
    }

    for (int i = avail_ops; i > 0; i--) {
        int selected_idx = cheaprandn(i);
        OpData *selected_op = ops_ptrs[selected_idx];
        
        bool ok = selected_op->op.poll();
        if (ok) {
            return int(selected_op - ops.data);
        }

        std::swap(ops_ptrs[selected_idx], ops_ptrs[i-1]);
    }

    if (!block) {
        return -1;
    }
    
    atomic<Selector::State> active = Selector::New;
    atomic<Selector*> completer = nil;
    Waiter completed;
    LOGX("%d select_i active 1163 %#x\n", pthread_self(), (uintptr) &active);

    int i = 0;
    for (; i < avail_ops; i++) {
        OpData &op = *ops_ptrs[i];

        op.selector.active = &active;
        op.selector.completer = &completer;
        op.selector.completed = &completed;

        LOGX("%d select_i subscribing %#x\n", pthread_self(), (uintptr) &op.selector);
        int result = op.op.subscribe(op.selector);
        if (result != -1) {
            for (int j = i-1; j >= 0; j--) {
                OpData &op = *ops_ptrs[j];
                LOGX("%d select_i 1205 unsubscribing %#x %d\n", pthread_self(), (uintptr) &op.selector, result);
                op.op.unsubscribe(op.selector);
            }
            return result;
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
        LOGX("%d select_i 1232 unsubscribing %#x\n", pthread_self(), (uintptr) &op.selector);
        op.op.unsubscribe(op.selector);
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

// void IntrusiveList::push(Selector *e) {
//     // printf("PUSH this(%p) %p\n", this, e);
//     LOG("%d %#x IntrusiveList pushing %#x\n", pthread_self(), (uintptr) this, (uintptr) e);
//     reload:
//     Selector *head;
//     {
//         sync::Lock lock(mtx);
//         head = this->head.load();
//     }
//     again:
//     if (head == SelectorBusy) {
//         // fmt::printf("push busy\n");
//         goto reload;
//     }

//     if (head == SelectorClosed) {
//         panic("!");
//     }

//     {
//         sync::Lock lock(mtx);
//         if (!this->head.compare_and_swap(&head, SelectorBusy)) {
//             LOG("IntrusiveList::push compare_and_swap failed\n");
//             goto again;
//         }
//     }

//     e->next = head;
//     e->prev = nil;

//     if (head != nil) {
//         // clean up
//         // for (Selector *elem = head->prev; elem != nil; elem = elem->prev) {
//         //     elem->removed = true;        
//         // }
        
//         head->prev = e;
//     }

//     // this->head = e;
//     sync::Lock lock(*mtx);
//     this->head.store(e);
//     // fmt::printf("%d %#x PUSH %#x: list %s\n", pthread_self(), (uintptr) this, (uintptr) e, format(e));
//     // dump();
// }

// Selector *IntrusiveList::pop() {
//     // printf("POP this(%p)\n", this);

//     // clean up
//     // for (Selector *elem = e->prev; elem != nil; elem = elem->prev) {
//     //     elem->removed = true;        
//     // }

//     reload:
//     Selector *head;
//     {
//         sync::Lock lock(mtx);
//         head = this->head.load();
//     }
//     again:
//     if (head == SelectorBusy) {
//         goto reload;
//     }

//     if (uintptr(head) == -2) {
//         panic("!");
//     }

//     {
//         sync::Lock lock(mtx);
//         if (!this->head.compare_and_swap(&head, SelectorBusy)) {
//             goto again;
//         }
//     }

//     if (head->next) {
//         head->next->prev = nil;
//     }

//     if (head->prev) {
//         panic("!");
//         head->prev->next = nil;
//     }
    
//     head->removed = true;
//     // fmt::printf("REMOVING 1283 %#x\n", (uintptr) head);

//     sync::Lock lock(*mtx);
//     this->head.store(head->next);
//     LOG("%d %#x IntrusiveList popped %#x\n", pthread_self(), (uintptr) this, (uintptr) head);
//     return head;
// }

// void IntrusiveList::remove(Selector *e) {
//     LOG("%d %#x IntrusiveList removing %#x\n", pthread_self(), (uintptr) this, (uintptr) e);

    

//     reload:
//     Selector *head;
//     {
//         sync::Lock lock(mtx);
//         head = this->head.load();
//     }
//     again:
//     if (head == SelectorBusy) {
//         goto reload;
//     }

//     if (uintptr(head) == -2) {
//         return;
//     }

//     {
//         sync::Lock lock(mtx);
//         if (!this->head.compare_and_swap(&head, SelectorBusy)) {
//             goto again;
//         }
//     }
    

//     LOG("%d %#x IntrusiveList removing %#x locked\n", pthread_self(), (uintptr) this, (uintptr) e);

//     if (e->removed) {
//         if (e == head) {
//             panic("!");
//         }
//         sync::Lock lock(*mtx);
//         this->head.store(head);
//         LOG("%d %#x IntrusiveList removing %#x already removed; unlocked\n", pthread_self(), (uintptr) this, (uintptr) e);
//         return;
//     }
//     e->removed = true;
//     // fmt::printf("REMOVING 1333 %#x %d %#x\n", (uintptr) e, pthread_self(), (uintptr) this);

//     if constexpr (DebugChecks) {
//         bool found = false;
//         for (Selector *elem = head; elem != nil; elem = elem->next) {
//             if (elem == e) {
//                 found = true;
//                 break;
//             }
//         }
//         if (!found) {
//             fmt::printf("%d %#x IntrusiveList removing %#x; not found in list %s\n", pthread_self(), (uintptr) this, (uintptr) e, format(head));
//             panic("element not found");
//         }
//     }

//     if constexpr (DebugLog) {
//         fmt::printf("%d %#x IntrusiveList removing %#x; list %s\n", pthread_self(), (uintptr) this, (uintptr) e, format(head));
//         fmt::printf("%d %#x IntrusiveList removing %#x (%s); list %s\n", pthread_self(), (uintptr) this, (uintptr) e, format(e), format(head));
//     }

//     if (e->prev) {
//         if (e->prev->OUT_OF_SCOPE) {
//             panic("out of scope");
//         }

//         e->prev->next= e->next;
//     }
//     if (e->next) {
//         mem::touch(e->next);
//         if (e->next->OUT_OF_SCOPE) {
//             panic("out of scope");
//         }
//         e->next->prev = e->prev;
//     }

//     if (head == e) {
//         head = head->next;
//     }

//     sync::Lock lock(*mtx);
//     this->head.store(head);
//     LOG("%d %#x IntrusiveList removing %#x unlocked\n", pthread_self(), (uintptr) this, (uintptr) e);
// }


// void IntrusiveList::dump(str s) {
//     Selector *head = this->head.load();
//     if (!head) {
//         fmt::printf("%d %#x IntrusiveList %s empty\n", pthread_self(), (uintptr) this, s);
//         return;
//     }
//     fmt::printf("%d %#x InstursveList %s contents: %s\n", pthread_self(), (uintptr) this, s, format(head));
//     // io::Buffer b;
//     // fmt::fprintf(b, "%d %#x InstursveList %s contents: [%#x", pthread_self(), (uintptr) this, s, (uintptr) head);
//     // for (Selector *elem = head->next; elem != nil; elem = elem->next) {
//     //     fmt::fprintf(b, " -> %#x", (uintptr) elem);
//     // }

//     // fmt::fprintf(b, "]\n");
//     // os::stdout.write(b.str(), error::ignore);
// }

// bool IntrusiveList::contains(Selector *e) {
//     Selector *head = this->head.load();
//     while (intptr(head) < 0) {
//         return false;
//         //head = this->head.load();
//     }

//     for (Selector *elem = head; elem != nil; ) {

//         if (e == elem) {
//             return true;
//         }

//         Selector *next = elem->next;
//         while (intptr(next) < 0) {
//             return false;
//             // next = sync::load(elem->next);
//         }

//         elem = next;
//     }

//     return false;
// }
// bool IntrusiveList::empty() const {
//     Selector *head = this->head.load();
//     return head == nil;
// }

// bool IntrusiveList::empty_atomic() const {
//     Selector *head = this->head.load();
//     return head == nil;
// }

ChanBase::Data ChanBase::AtomicData::load() {
    // sync::Lock lock(mtx);

    boost::atomic_ref<Data> aref(this->raw);
    static_assert(boost::atomic_ref<Data>::is_always_lock_free);

    return aref.load(boost::memory_order::seq_cst);
    
  }
  
bool ChanBase::AtomicData::compare_and_swap(Data *expected, Data newval) {
    // sync::Lock lock(mtx);
    
    boost::atomic_ref<Data> aref(this->raw);
    static_assert(boost::atomic_ref<Data>::is_always_lock_free);

    return aref.compare_exchange_strong(*expected, newval, boost::memory_order::release, boost::memory_order::acquire);
}
void ChanBase::AtomicData::store(Data data) {
    // sync::Lock lock(mtx);

    boost::atomic_ref<Data> aref(this->raw);
    static_assert(boost::atomic_ref<Data>::is_always_lock_free);

    if (intptr(data.selector) < 0) {
        panic("!");
    }

    return aref.store(data, boost::memory_order::release);
}

void ChanBase::AtomicData::remove(Selector *e) {
reload:
    Selector *head = this->selector_load();
again:
    if (head == SelectorBusy) {
        goto reload;
    }

    if (head == SelectorClosed) {
        return;
    }

    if (!this->selector_compare_and_swap(&head, SelectorBusy)) { // cleared on  1570, 1615
        goto again;
    }
    

    LOG("%d %#x AtomicData removing %#x locked\n", pthread_self(), (uintptr) this, (uintptr) e);

    if (e->removed) {
        if (e == head) {
            panic("!");
        }
        LOG("%d %#x AtomicData removing %#x already removed; unlocked\n", pthread_self(), (uintptr) this, (uintptr) e);
        this->selector_store(head);
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
            fmt::printf("%d %#x AtomicData removing %#x; not found in list %s\n", pthread_self(), (uintptr) this, (uintptr) e, format(head));
            panic("element not found");
        }
    }

    if constexpr (DebugLog) {
        fmt::printf("%d %#x AtomicData removing %#x; list %s\n", pthread_self(), (uintptr) this, (uintptr) e, format(head));
        fmt::printf("%d %#x AtomicData removing %#x (%s); list %s\n", pthread_self(), (uintptr) this, (uintptr) e, format(e), format(head));
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


    this->selector_store(head);
    LOG("%d %#x AtomicData removing %#x unlocked\n", pthread_self(), (uintptr) this, (uintptr) e);
}
Selector * ChanBase::AtomicData::selector_load() {
    // sync::Lock lock(mtx);

    return sync::load(this->raw.selector);
}

void ChanBase::AtomicData::selector_store(Selector *v) {
    // sync::Lock lock(mtx);
    if (intptr(v) < 0) {
        panic("!");
    }
    sync::store(this->raw.selector, v);
}

bool ChanBase::AtomicData::selector_compare_and_swap(Selector **oldval, Selector *newval) {
    // sync::Lock lock(mtx);
    return sync::compare_and_swap(this->raw.selector, oldval, newval);
}
