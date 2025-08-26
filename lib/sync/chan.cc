#include "chan.h"

#include "lib/print.h"
#include <atomic>
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


ChanBase::ChanBase(int capacity) : capacity(capacity) {}

bool ChanBase::send_nonblocking(this ChanBase &c, void *elem, bool move, sync::Lock &lock) {
    try_again:

    // block until there is a no receiver
    if (c.receivers.empty()) {

        if (!c.is_full()) {
            c.buffer_push(elem, move);
            return true;
        }
        
        return false;
    }

    Selector *receiver = c.receivers.pop();
    receiver->done = true;

    bool expected = false;
    bool ok = receiver->active->compare_exchange_strong(expected, true);
    if (!ok) {
        goto try_again;
    }

    if (receiver->value) {
        c.set(receiver->value, elem, move);
    }
    if (receiver->ok) {
        *receiver->ok = true;
    }

    receiver->completer->store(receiver);
    receiver->completed->notify();
    lock.unlock();
    return true;
}

void ChanBase::send_blocking(this ChanBase &c, void *elem, bool move, sync::Lock &lock) {
    bool ok = c.send_nonblocking(elem, move, lock);
    if (ok) {
        return;
    }

    std::atomic<bool> active = false;
    Waiter completed;
    atomic<Selector*> completer = nil;

    Selector sender = {
        .value = elem,
        .move  = move,
        .active = &active,
        .completed = &completed,
        .completer = &completer,
    } ;

    c.senders.push(&sender);

    lock.unlock();
    completed.wait();

    if (sender.panic) {
        panic("send on closed channel");
    }
}

void ChanBase::send_i(this ChanBase &c, void *elem, bool move) {
    Lock lock(c.lock);

    if (c.closed()) {
        panic("send on closed channel");
    }

    c.send_blocking(elem, move, lock);
}

bool ChanBase::recv_nonblocking(this ChanBase &c, void *out, bool *ok_ptr, Lock &lock) {
    // printf("recv_nonblocking\n");
    if (!c.is_empty()) {
        c.buffer_pop(out);
        // printf("recv_nonblocking buffer_pop\n");
        if (ok_ptr) {
            *ok_ptr = true;
        }

        // signal senders if present
        try_again1:
        if (!c.senders.empty()) {
            Selector *sender = c.senders.pop();
            sender->done = true;

            bool expected = false;
            bool ok = sender->active->compare_exchange_strong(expected, true);
            if (!ok) {
                goto try_again1;
            }

            c.buffer_push(sender->value, sender->move);
            sender->completer->store(sender);
            sender->completed->notify();

            
            lock.unlock();
        }
        return true;
    }

    if (c.closed()) {
        if (ok_ptr) {
            *ok_ptr = false;
        }
        return true;
    }

    try_again2:
    // block if there any no senders
    // print "recv_i senders.empty %v" % c.senders.empty();
    if (c.senders.empty()) {
        return false;
    }

    Selector *sender = c.senders.pop();
    sender->done = true;

    bool expected = false;
    bool ok = sender->active->compare_exchange_strong(expected, true);
    // printf("recv_nonblocking popped %d\n", ok);
    if (!ok) {
        goto try_again2;
    }

    if (out) {
        c.set(out, sender->value, sender->move);
    }    
    
    if (ok_ptr) {
        *ok_ptr = true;
    }

    sender->completer->store(sender);
    sender->completed->notify();

    lock.unlock();
    return true;
}

void ChanBase::recv_blocking(this ChanBase &c, void *out, bool *ok, Lock &lock) {
    if (c.recv_nonblocking(out, ok, lock)) {
        return;
    }

    std::atomic<bool> active = false;
    Waiter completed;
    atomic<Selector*> completer = nil;

    Selector receiver = {
        .value = out,
        .ok = ok,
        .active = &active,
        .completed = &completed,
        .completer = &completer,
    };

    c.receivers.push(&receiver);

    lock.unlock();
    completed.wait();
}

void ChanBase::recv_i(this ChanBase &c, void *out, bool *ok) {
    // printf("recv_i\n");
    Lock lock(c.lock);

    c.recv_blocking(out, ok, lock);
}

void ChanBase::close(this ChanBase &c) {
    Lock lock(c.lock);

    if (c.closed()) {
        panic("close of closed channel");
    }

    c.state = Closed;
    Selector *next;

    for (Selector *sender = c.senders.head; sender != nil; sender = next) {
        next = sender->next;

        bool expected = false;
        bool ok = sender->active->compare_exchange_strong(expected, true);
        if (!ok) {
            continue;
        }
        sender->panic = true;
        sender->done = true;
        sender->completer->store(sender);
        sender->completed->notify();
    }

    for (Selector *receiver = c.receivers.head; receiver != nil; receiver = next) {
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

        receiver->done = true;
        receiver->completer->store(receiver);
        receiver->completed->notify();
    }

    c.senders.head = nil;
    c.receivers.head = nil;
}

bool ChanBase::closed(this ChanBase const& c) {
    return c.state == Closed;
}

bool ChanBase::is_buffered(this ChanBase const& c) {
    return c.capacity > 0;
}

bool ChanBase::is_empty(this ChanBase const& c) {
    return c.unread == 0;
}
bool ChanBase::is_full(this ChanBase const& c) {
    // printf("is_full unread %d; capacity %d\n", c.unread, c.capacity);
    return c.unread >= c.capacity;
}

int ChanBase::length() const {
    return this->unread;
}

bool ChanBase::try_recv(this ChanBase &c, void *out, bool *ok, bool try_locks, bool *lock_fail) {
    Lock lock;
    if (try_locks) {
        if (!lock.try_lock(c.lock)) {
            *lock_fail = true;
            return false;
        }
    } else {
        lock.lock(c.lock);
    }

    return c.recv_nonblocking(out, ok, lock);
}

bool ChanBase::try_send(this ChanBase &c, void *out, bool move, bool try_locks, bool *lock_fail) {
    Lock lock;
    if (try_locks) {
        if (!lock.try_lock(c.lock)) {
            *lock_fail = true;
            return false;
        }
    } else {
        lock.lock(c.lock);
    }

    return c.send_nonblocking(out, move, lock);
}


bool ChanBase::subscribe_recv(this ChanBase &c, internal::Selector &receiver, Lock &lock) {
    if (c.recv_nonblocking(receiver.value, receiver.ok, lock)) {
        return true;
    }

    c.receivers.push(&receiver);
    return false;
}

bool ChanBase::subscribe_send(this ChanBase &c, internal::Selector &sender, Lock &lock) {
    if (c.send_nonblocking(sender.value, sender.move, lock)) {
        return true;
    }

    c.senders.push(&sender);
    return false;
}

void ChanBase::unsubscribe_recv(this ChanBase &c, internal::Selector &receiver, Lock&) {
    if (receiver.done) {
        return;
    }

    c.receivers.remove(&receiver);
}

void ChanBase::unsubscribe_send(this ChanBase &c, internal::Selector &sender, Lock&) {
    if (sender.done) {
        return;
    }
    
    c.senders.remove(&sender);
}


bool Recv::poll(bool try_locks, bool *lock_fail) const {
    ChanBase & c = *this->chan;

    return c.try_recv(this->data, this->ok, try_locks, lock_fail);
}



bool Send::poll(bool try_locks, bool *lock_fail) const {
    ChanBase & c = *this->chan;

    return c.try_send(this->data, this->move, try_locks, lock_fail);
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

int internal::select_i(arr<OpData> ops, arr<OpData*> ops_ptrs, arr<OpData*> lockfail_ptrs, bool block, bool debug) {
    debug = false;

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
    // print "ops_ptrs", ops_ptrs;
    // sort opts_ptr by lock order
    std::sort(ops_ptrs.begin(), ops_ptrs.begin()+avail_ops, [](OpData *d1, OpData *d2) {
        return uintptr(d1->op.chan) < uintptr(d2->op.chan);
    });
    // print "ops_ptrs", ops_ptrs;
    
    std::atomic<bool> active = false;
    Waiter completed;
    atomic<Selector*> completer = nil;

    int i = 0;
    // lock in lock order and subscribe
    for (; i < avail_ops; i++) {
        OpData &op = *ops_ptrs[i];

        op.selector.active = &active;
        op.selector.completed = &completed;
        op.selector.completer = &completer;

        // skip re-locking the same lock
        if (i == 0 || &op.op.chan->lock != &ops_ptrs[i-1]->op.chan->lock) {
            op.chanlock.lock(op.op.chan->lock);
        }

        // printf("loop i %d; %p\n", i, &op.selector);
        if (op.op.subscribe(op.selector, op.chanlock)) {
            for (int j = i-1; j >= 0; j--) {
                OpData &op = *ops_ptrs[j];
                // printf("UNSUBSCRIBE j %d; %p\n", j, &op.selector);
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

    completed.wait();

    Selector *selected = completer.load();

    // unsubscribe
    for (int j = 0; j < i; j++) {
        OpData &op = *ops_ptrs[j];

        if (&op.selector == selected) {
            continue;
        }
        Lock lock(op.op.chan->lock);
        op.op.unsubscribe(op.selector, lock);

        if constexpr (DebugChecks) {
            if (op.op.chan->receivers.contains(&op.selector)) {
                panic("DebugCheck1");
            }
            if (op.op.chan->senders.contains(&op.selector)) {
                panic("DebugCheck2");
            }
        }
    }

    return selected->id;
}
void lib::sync::internal::Waiter::notify() {
    // printf("NOTIFY %p\n", this);
    state.store(1, std::memory_order::release);
    state.notify_one();
    state.store(2);
}

void lib::sync::internal::Waiter::wait() {
    // printf("WAITING START %p\n", this);
    for (;;) {
        int s = state.load(std::memory_order::acquire);
        if (s == 0) {
            state.wait(0);
            continue;
        }
        if (s == 2) {
            // printf("WAITING DONE %p\n", this);
            return;
        }
        // spin wait
    }
}
