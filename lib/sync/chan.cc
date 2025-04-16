#include "chan.h"

#include "lib/print.h"
#include "lib/sync/atomic.h"
#include "lib/sync/lock.h"
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
    LOG("%d %#x chan constrcuted with capacity %v\n", pthread_self(), (uintptr) this, capacity);
}

// assumes channel is open
bool ChanBase::send_nonblocking(this ChanBase &c, void *elem, bool move, bool try_locks, bool *lock_fail, sync::Lock &lock) {
    if (c.capacity > 0) {
        Data data = c.adata.load();
        do {
            again:

            LOG("%d %#x send_nonblocking: got q; receivers %#x %v\n", pthread_self(), (uintptr) &c, data.q, (uintptr) data.receivers);
            if (data.q >= c.capacity) {
                return false;
            }

            if (intptr(data.receivers) == -2) {
                // closed
                LOG("%d %#x send_nonblocking: closed\n", pthread_self(), (uintptr) &c);
                return false;
            }

            if (data.q == 0 && data.receivers != nil) {
                LOG("%d %#x send_nonblocking: special case\n", pthread_self(), (uintptr) &c);

                if (intptr(data.receivers) == -1) {
                    data = c.adata.load();
                    goto again; 
                }

                Selector *receiver = data.receivers;
                bool b = c.adata.update(&data, {.q = data.q, .receivers = (Selector*) -1, .senders = data.senders});
                if (!b) {
                    goto again;
                }
                receiver->removed = true;
                bool expected = false;
                bool ok = receiver->active->compare_exchange_strong(expected, true);
                c.adata.receivers.head.store(receiver->next);
                LOG("%d %#x send_nonblocking: receiver popped atomically %#x\n", pthread_self(), (uintptr) &c, (uintptr) receiver);
                if (!ok) {
                    data = c.adata.load();
                    goto again;
                }

                if (receiver->value != nil) {
                    c.set(receiver->value, elem, move);
                }
                if (receiver->ok != nil) {
                    *receiver->ok = true;
                }
                receiver->completed->store(receiver);
                receiver->completed->notify_one();
                return true;
            }
        } while (!c.adata.update(&data, {.q = data.q+1, .receivers = data.receivers, .senders = data.senders }));
        // while(c.adata.update(&data, { .q = data.q+1, .receivers = data.receivers, .senders = data.senders }));
        LOG("%d %#x send_nonblocking_unlocked: q %v -> %v %v\n", pthread_self(), (uintptr) &c, data.q, data.q+1, c.adata.q.load());
        
        c.push(elem, move);
        return true;
    }

    // if (c.adata.receivers.empty_atomic()) {
    //     // LOG("%d %#x send_nonblocking (cap=0): receivers empty; early return\n", pthread_self(), (uintptr) &c);
    //     return false;
    // }
    
    // if (!lock.locked) {
    //     if (try_locks) {
    //         if (!lock.try_lock(c.lock)) {
    //             *lock_fail = true;
    //             return false;
    //         }
    //     } else {
    //         lock.lock(c.lock);
    //     }
    //     // LOG("%d %#x send_nonblocking (cap=0): locked\n", pthread_self(), (uintptr) &c);
    // }

try_again2:
    if (c.adata.receivers.empty()) {
        // LOG("%d %#x send_nonblocking (cap=0): receivers empty; returning\n", pthread_self(), (uintptr) &c);
        return false;
    }

    Selector *receiver = c.adata.receivers.pop();

    bool expected = false;
    if (!receiver->active->compare_exchange_strong(expected, true)) {
        goto try_again2;
    }

    if (receiver->value) {
        c.set(receiver->value, elem, move);
    }
    if (receiver->ok) {
        *receiver->ok = true;
    }

    receiver->completed->store(receiver);
    receiver->completed->notify_one();
    // LOG("%d %#x send_nonblocking (cap=0): notified receiver\n", pthread_self(), (uintptr) &c);
    return true;
}


void ChanBase::send_blocking(this ChanBase &c, void *elem, bool move) {
    LOG("%d %#x send_blocking\n", pthread_self(), (uintptr) &c);
    sync::Lock lock(c.lock);
    bool ok = c.send_nonblocking(elem, move, false, nil, lock);
    if (ok) {
        LOG("%d %#x send_blocking: send_blocking succeeded\n", pthread_self(), (uintptr) &c);
        return;
    }

    LOG("%d %#x send_blocking: send_blocking failed\n", pthread_self(), (uintptr)  &c);

    // if (!lock.locked) {
    //     LOG("%d %#x send_blocking: locked\n", pthread_self(), (uintptr) &c);
    //     lock.lock(c.lock);
    // }

    while (!c.adata.receivers.empty()) {
        Selector *receiver = c.adata.receivers.pop();

        bool expected = false;
        if (!receiver->active->compare_exchange_strong(expected, true)) {
            continue;
        }

        if (receiver->value) {
            c.set(receiver->value, elem, move);
        }

        if (receiver->ok) {
            *receiver->ok = true;
        }
        
        receiver->completed->store(receiver);
        receiver->completed->notify_one();
        return;
    }

    if (c.state.value == Closed) {
        panic("send on closed channel");
    }

    if (c.capacity > 0) {
        int q = c.adata.q.load();
        bool at_cap = false;
        do {
            LOG("%d %#x send_blocking: got q %v\n", pthread_self(), (uintptr) &c, q);
            if (q >= c.capacity) {
                at_cap = true;
                break;
            }

            // if (q == 0) {
            //     // special case; may need to notify receivers
            //     if (!lock.locked) {
            //         lock.lock(c.lock);
            //     }

            //     int old_q = q;
            //     q = c.adata.q.load();
            //     if (q != old_q) {
            //         continue;
            //     }

            //     LOG("%d %#x send_nonblocking: special case locked\n", pthread_self(), (uintptr)  &c);
            //     Selector *receiver = nil;
            //     while (!c.adata.receivers.empty()) {
            //         Selector *t = c.adata.receivers.pop();
            //         t->done = true;

            //         bool expected = false;
            //         bool ok = t->active->compare_exchange_strong(expected, true);
            //         if (ok) {
            //             receiver = t;
            //             break;
            //         }
            //     }

            //     if (receiver == nil) {
            //         continue;
            //     }

            //     if (receiver->value) {
            //         c.set(receiver->value, elem, move);
            //     }
            //     if (receiver->ok) {
            //         *receiver->ok = true;
            //     }
            //     receiver->completed->store(receiver);
            //     receiver->completed->notify_one();
            //     return;
                
            //     LOG("%d %#x send_nonblocking: special case bailing\n", pthread_self(), (uintptr)  &c);
            // }
        } while (!c.adata.q.compare_and_swap(&q, q+1));
        LOG("%d %#x send_blocked: q %v -> %v %v\n", pthread_self(), (uintptr) &c, q, q+1, c.adata.q.load());
        
        if (!at_cap) {
            c.push(elem, move);
            return;
        }
    }

    std::atomic<bool> active = false;
    std::atomic<Selector*> completed = nil;

    Selector sender = {
        .value = elem,
        .move  = move,
        .active = &active,
        .completed = &completed,
    } ;

    c.adata.senders.push(&sender);

    lock.unlock();
    LOG("%d %#x send_blocking: waiting\n", pthread_self(), (uintptr)  &c);
    completed.wait(nil);

    lock.relock();
    c.adata.senders_remove(&sender);

    if (sender.panic) {
        panic("send on closed channel");
    }

    LOG("%d %#x send_blocking: sender going out of scope: %#x\n", pthread_self(), (uintptr) &c, (uintptr) &sender);
}

void ChanBase::send_i(this ChanBase &c, void *elem, bool move) {
    if (c.closed()) {
        panic("send on closed channel");
    }

    c.send_blocking(elem, move);
}

bool ChanBase::send_nonblocking_i(this ChanBase &c, void *elem, bool move) {    
    if (c.closed()) {
        panic("send on closed channel");
    }

    sync::Lock lock(c.lock);
    return c.send_nonblocking(elem, move, false, nil, lock);
}

bool ChanBase::recv_nonblocking(this ChanBase &c, void *out, bool *ok, sync::Lock &lock) {
    if (c.capacity > 0) {
        int q = c.adata.q.load();
        do {
            LOG("%d %#x recv_nonblocking: got q %v\n", pthread_self(), (uintptr)  &c, q);
            if (q <= 0) {
                if (c.closed()) {
                    if (ok) {
                        *ok = false;
                    }
                    return true;
                }

                return false;
            }

            if (q == c.capacity) {
                LOG("%d %#x recv_nonblocking: special case\n", pthread_self(), (uintptr)  &c);
                // if (!lock.locked) {
                //     lock.lock(c.lock);
                // }
                if (q != c.adata.q.load()) {
                    continue;
                }
                if (intptr(c.adata.senders.head.load()) == -2) {
                    // closed
                    continue;
                }
                LOG("%d %#x recv_nonblocking: special case locked\n", pthread_self(), (uintptr)  &c);
                Selector *sender = nil;
                while (!c.adata.senders.empty()) {
                    Selector *t = c.adata.senders.pop();

                    bool expected = false;
                    bool ok = t->active->compare_exchange_strong(expected, true);
                    if (ok) {
                        sender = t;
                        break;
                    }
                }

                if (sender == nil) {
                    LOG("%d %#x recv_nonblocking: special case bailing\n", pthread_self(), (uintptr)  &c);
                    continue;
                }

                c.pop(out);
                if (ok) {
                    *ok = true;
                }

                LOG("%d %#x recv_nonblocking: special case push\n", pthread_self(), (uintptr)  &c);
                c.push(sender->value, sender->move);
                sender->completed->store(sender);
                sender->completed->notify_one();
                
                return true;
            }
        } while (!c.adata.q.compare_and_swap(&q, q-1));
        LOG("%d %#x recv_nonblocking: %v -> %v; actual %v\n", pthread_self(), (uintptr)  &c, q, q-1, c.adata.q.load());


        c.pop(out);
        
        if (ok) {
            *ok = true;
        }
        return true;
    }

    // if (!lock.locked) {
    //     // if (c.adata.senders.empty_atomic()) {
    //     //     return false;
    //     // }

    //     LOG("%d %#x recv_nonblocking: acquiring lock...\n", pthread_self(), (uintptr)  &c);
    //     lock.lock(c.lock);
    //     LOG("%d %#x recv_nonblocking: lock acquired\n", pthread_self(), (uintptr)  &c);
    //     // LOG("%d %#x recv_nonblocking (cap=0): locked\n", pthread_self(), (uintptr) &c);
    // }

    // zero capacity case
    if (c.closed()) {
        if (ok) {
            *ok = false;
        }
        // LOG("%d %#x recv_nonblocking (cap=0): closed; early return\n", pthread_self(), (uintptr) &c);
        return true;
    }

try_again:
    if (c.adata.senders.empty()) {
        // LOG("%d %#x recv_nonblocking (cap=0): senders empty; returning\n", pthread_self(), (uintptr) &c);
        return false;
    }

    Selector *sender = c.adata.senders.pop();

    bool expected = false;
    if (!sender->active->compare_exchange_strong(expected, true)) {
        goto try_again;
    }

    if (out) {
        c.set(out, sender->value, sender->move);
    }

    sender->completed->store(sender);
    sender->completed->notify_one();
    if (ok) {
        *ok = true;
    }
    // LOG("%d %#x recv_nonblocking (cap=0): notified sender\n", pthread_self(), (uintptr) &c);
    return true;
}

void ChanBase::recv_blocking(this ChanBase &c, void *out, bool *ok) {
    LOG("%d %#x recv_blocking\n", pthread_self(), (uintptr) &c);

    sync::Lock lock(c.lock);
    if (c.recv_nonblocking(out, ok, lock)) {
        LOG("%d %#x recv_blocking: recv nonblocking unlocked succeeded\n", pthread_self(), (uintptr) &c);
        return;
    }
    LOG("%d %#x recv_blocking: recv nonblocking unlocked failed\n", pthread_self(), (uintptr) &c);

    // locking lock here
    // if (!lock.locked) {
    //     LOG("%d %#x recv_blocking: locked\n", pthread_self(), (uintptr) &c, c.adata.senders.empty());
    //     lock.lock(c.lock);
    // }

    int q = c.adata.q.load();
    do {
        LOG("%d %#x recv_blocking 1: got q %v\n", pthread_self(), (uintptr)  &c, q);
        if (q < 1) {
            break;
        }

        if (q == c.capacity) {
            LOG("%d %#x recv_blocking: special case\n", pthread_self(), (uintptr)  &c);
            // already locked
            // if (!lock.locked) {
            //     lock.lock(c.lock);
            // }
            // if (q != c.adata.q.load()) {
            //     continue;
            // }
            LOG("%d %#x recv_blocking: special case locked\n", pthread_self(), (uintptr)  &c);
            Selector *sender = nil;
            while (!c.adata.senders.empty()) {
                Selector *t = c.adata.senders.pop();

                bool expected = false;
                bool ok = t->active->compare_exchange_strong(expected, true);
                if (ok) {
                    sender = t;
                    break;
                }
            }

            if (sender == nil) {
                LOG("%d %#x recv_blocking: special case bailing\n", pthread_self(), (uintptr)  &c);
                continue;
            }

            c.pop(out);
            if (ok) {
                *ok = true;
            }

            LOG("%d %#x recv_blocking: special case push\n", pthread_self(), (uintptr)  &c);
            c.push(sender->value, sender->move);
            sender->completed->store(sender);
            sender->completed->notify_one();
            
            return;
        }
        LOG("%d %#x recv_blocking 1: q %v -> %v\n", pthread_self(), (uintptr)  &c, q, q-1);
    } while (!c.adata.q.compare_and_swap(&q, q-1));
    LOG("%d %#x recv_blocking 1: q %v %v\n", pthread_self(), (uintptr)  &c, q, c.adata.q.load());
    if (q > 0) {
        c.pop(out);
        if (ok) {
            *ok = true;
        }
        return;
    }

    if (c.state.value == Closed) {
        if (ok) {
            *ok = false;
        }
        return;
    }
    
    // can read from buffer
    while (!c.adata.senders.empty()) {
        Selector *sender = c.adata.senders.pop();
    
        bool expected = false;
        if (!sender->active->compare_exchange_strong(expected, true)) {
            continue;
        }
    
        if (out) {
            c.set(out, sender->value, sender->move);
        }
    
        sender->completed->store(sender);
        sender->completed->notify_one();
        if (ok) {
            *ok = true;
        }
        return;
    }

    std::atomic<bool> active = false;
    std::atomic<Selector*> completed = nil;

    Selector receiver = {
        .value = out,
        .ok = ok,
        .active = &active,
        .completed = &completed,
    };

    c.adata.receivers.push(&receiver);

    lock.unlock();
    LOG("%d %#x recv_blocking: about to wait\n", pthread_self(), (uintptr) &c);
    completed.wait(nil);

    lock.relock();
    c.adata.receivers_remove(&receiver);

    LOG("%d %#x recv_blocking: wait done\n", pthread_self(), (uintptr) &c);
    LOG("%d %#x recv_blocking: receiver going out of scope: %#x\n", pthread_self(), (uintptr) &c, (uintptr) &receiver);
}

void ChanBase::close(this ChanBase &c) {
    LOG("%d %#x close: channel closing...\n", pthread_self(), (uintptr) &c);
    Lock lock(c.lock);

    if (c.closed()) {
        panic("close of closed channel");
    }

    c.state.store(Closed);
    Selector *next;

    Selector *sender = c.adata.senders.head.load();
    for (;;) {
        if (intptr(sender) == -1) {
            sender = c.adata.senders.head.load();
            continue;
        }

        if (c.adata.senders.head.compare_and_swap(&sender, (Selector *) -2)) {
            break;
        }
    }
    
    Selector *receiver = c.adata.receivers.head.load();
    for (;;) {
        if (intptr(receiver) == -1) {
            receiver = c.adata.receivers.head.load();
            continue;
        }

        if (c.adata.receivers.head.compare_and_swap(&receiver, (Selector *) -2)) {
            break;
        }
    }

    for (; sender != nil; sender = next) {
        next = sender->next;

        bool expected = false;
        bool ok = sender->active->compare_exchange_strong(expected, true);
        if (!ok) {
            continue;
        }
        sender->panic = true;
        sender->removed = true;
        sender->completed->store(sender);
        sender->completed->notify_one();
    }

    // int i = 0, j =0;
    for (; receiver != nil; receiver = next) {
        // i++;
        next = receiver->next;
        
        bool expected = false;
        bool ok = receiver->active->compare_exchange_strong(expected, true);
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
        receiver->completed->store(receiver);
        receiver->completed->notify_one();
        // j++;
    }

    
    LOG("%d %#x close: channel closed\n", pthread_self(), (uintptr) &c);
    // fmt::printf("CLOSE %v %v\n", i, j);
    // if (sync::load(c.adata.receivers.head) != nil || sync::load(c.adata.senders.head) != nil) {
    //     panic("state");
    // }
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
    return c.recv_nonblocking(out, ok, lock);
}

bool ChanBase::subscribe_recv(this ChanBase &c, internal::Selector &receiver, Lock &lock) {
    if (c.recv_nonblocking(receiver.value, receiver.ok, lock)) {
        return true;
    }

    c.adata.receivers.push(&receiver);
    return false;
}

bool ChanBase::subscribe_send(this ChanBase &c, internal::Selector &sender, Lock &lock) {
    if (c.send_nonblocking(sender.value, sender.move, false, nil, lock)) {
        return true;
    }

    c.adata.senders.push(&sender);
    return false;
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
    return c.recv_nonblocking(this->data, this->ok, lock);
}

bool Send::poll(bool try_locks, bool *lock_fail) const {
    ChanBase & c = *this->chan;
    if (c.closed()) {
        LOG("%d %#X Send::poll: closed; returning early\n", pthread_self(), (uintptr) &c);
        return false;
    }

    sync::Lock lock(c.lock);
    return c.send_nonblocking(this->data, this->move, try_locks, lock_fail, lock);
}

bool Send::select(bool blocking) const {
    ChanBase & c = *this->chan;
    if (blocking) {
        c.send_i(this->data, this->move);
        return true;
    }

    return c.send_nonblocking_i(this->data, this->move);
}


bool Recv::subscribe(sync::internal::Selector &receiver, Lock &lock) const {
    ChanBase &c = *this->chan;

    receiver.value = this->data;
    receiver.ok = this->ok;
    
    return c.subscribe_recv(receiver, lock);
}

bool Send::subscribe(sync::internal::Selector &sender, Lock &lock) const {
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
    
    std::atomic<bool> active = false;
    std::atomic<Selector*> completed = nil;

    int i = 0;
    // lock in lock order and subscribe
    for (; i < avail_ops; i++) {
        OpData &op = *ops_ptrs[i];

        op.selector.active = &active;
        op.selector.completed = &completed;

        if (i == 0 || &op.op.chan->lock != &ops_ptrs[i-1]->op.chan->lock) {
            // skip re-locking the same lock
            LOG("LOCKING IN ORDER %#x\n", (uintptr) op.op.chan);
            op.chanlock.lock(op.op.chan->lock);
        } else {
            LOG("LOCKING IN ORDER SKIP %#x\n", (uintptr) op.op.chan);
        }

        if (op.op.subscribe(op.selector, op.chanlock)) {
            for (int j = i-1; j >= 0; j--) {
                OpData &op = *ops_ptrs[j];
                op.op.unsubscribe(op.selector, op.chanlock);
            }
            return int(&op - ops.data);
        }
    }

    // unlock
    for (int j = i-1; j >= 0; j--) {
        OpData &op = *ops_ptrs[j];
        if (op.chanlock.locked) {
            op.chanlock.unlock();
        }
    }

    completed.wait(nil);

    Selector *selected = completed.load();

    // unsubscribe
    for (int j = 0; j < i; j++) {
        OpData &op = *ops_ptrs[j];

        if (&op.selector == selected) {
            continue;
        }
        Lock lock(op.op.chan->lock);
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
    reload:
    Selector *head = this->head.load();
    again:
    if (uintptr(head) == -1) {
        goto reload;
    }

    if (uintptr(head) == -2) {
        panic("!");
    }

    if (!this->head.compare_and_swap(&head, (Selector *) -1)) {
        goto again;
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
    this->head.store(e);
    // dump();
}

Selector *IntrusiveList::pop() {
    // printf("POP this(%p)\n", this);

    // clean up
    // for (Selector *elem = e->prev; elem != nil; elem = elem->prev) {
    //     elem->removed = true;        
    // }

    reload:
    Selector *head = this->head.load();
    again:
    if (uintptr(head) == -1) {
        goto reload;
    }

    if (uintptr(head) == -2) {
        panic("!");
    }

    if (!this->head.compare_and_swap(&head, (Selector *) -1)) {
        goto again;
    }

    if (head->next) {
        head->next->prev = nil;
    }

    if (head->prev) {
        panic("!");
        head->prev->next = nil;
    }
    
    head->removed = true;

    this->head.store(head->next);
    return head;
}

void IntrusiveList::remove(Selector *e) {
    if (e->removed) {
        return;
    }
    e->removed = true;

    reload:
    Selector *head = this->head.load();
    again:
    if (uintptr(head) == -1) {
        goto reload;
    }

    if (uintptr(head) == -2) {
        return;
    }

    if (!this->head.compare_and_swap(&head, (Selector *) -1)) {
        goto again;
    }

    if constexpr (DebugLog) {
        fmt::printf("%d %#x IntrusiveList removing %#x; list %s\n", pthread_self(), (uintptr) this, (uintptr) e, format(head));
        fmt::printf("%d %#x IntrusiveList removing %#x (%s); list %s\n", pthread_self(), (uintptr) this, (uintptr) e, format(e), format(head));
    }

    if (e->prev) {
        e->prev->next= e->next;
    }
    if (e->next) {
        e->next->prev = e->prev;
    }

    if (head == e) {
        head = head->next;
    }

    this->head.store(head);

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
    for (Selector *elem = head; elem != nil; elem = elem->next) {
        if (e == elem) {
        return true;
        }
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
  
  bool ChanBase::AtomicData::update(Data *expected, Data newval) {
      sync::Lock lock(mtx);
  
      // fmt::printf("%d UPDATE CALLED\n", pthread_self());
  
      if (expected->receivers != receivers.head.load() || expected->senders != senders.head.load()) {
          expected->receivers = receivers.head.load();
          expected->senders = senders.head.load();
          return false;
      }
  
      if (!this->q.compare_and_swap(&expected->q, newval.q)) {
          return false;
      }
      this->receivers.head = newval.receivers;
      this->senders.head = newval.senders;
  
      *expected = newval;
  
      // fmt::printf("%d UPDATE SUCCESS %v %v\n", pthread_self(), this->q.value, expected->q);
      return true;
  }