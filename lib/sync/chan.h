#pragma once

#include <boost/circular_buffer.hpp>
#include "deps/atomic_queue/include/atomic_queue/atomic_queue.h"
#include <atomic>
#include <pthread.h>

#include "lib/base.h"
#include "atomic.h"
#include "lib/io/io_stream.h"
#include "lib/str.h"
#include "mutex.h"
#include "cond.h"
#include "lock.h"

namespace lib::sync {
    using namespace lib;

    template <typename T>
    struct Chan;

    struct SelectOp;
    struct Recv;
    struct Send;

    namespace internal {
        constexpr bool DebugChecks = true;
        constexpr bool DebugLog    = false;

        struct Waiter {
            std::atomic<int> state = 0;

            void notify() {
                state.store(1, std::memory_order::release);
                state.notify_one();
                state.store(2);
            }

            void wait() {
                for (;;) {
                    int s = state.load(std::memory_order::acquire);
                    if (s == 0) {
                        state.wait(0);
                        continue;
                    }
                    if (s == 2) {
                        return;
                    }
                    // spin wait
                }
            }
        } ;
        
        struct Selector {
            void *value = nil;
            bool *ok    = nil;
            bool  move  = false;

            int      id = 0;

            enum State {
                New,
                Busy,
                Done,
            } ;

            atomic<State>      *active = nil;
            atomic<Selector*> *completer = nil;
            Waiter *completed = nil;
            atomic<State>      state = New;
            bool removed = false;

            bool panic = false;

            Selector *prev = nil;
            Selector *next = nil;

            bool OUT_OF_SCOPE = false;
        };

        Selector *const SelectorBusy = (Selector*) -1;
        Selector *const SelectorClosed = (Selector*) -2;        

        struct IntrusiveList {
          void push(Selector *e);

          Selector *pop();

          void remove(Selector *e);

          bool empty() const;

          bool empty_atomic() const;

          Mutex *mtx = nil;
          atomic<Selector*> head = nil;

          void dump(str s="");

          bool contains(Selector *e);
        } ;



        struct Allocator {
            // void *allocate(size n) {}
            // void deallocated(void *p, size n) noexcept {}
        } ;

        template <typename T>
        struct AllocatorT {
            typedef T value_type;
            
            Allocator alloc;

            AllocatorT() = default;
            
            template <typename U>
            constexpr AllocatorT(const AllocatorT<U>&) noexcept {}

            T *allocate(size_t n) {
                fmt::printf("ALLOCATE %v %v\n", sizeof(T), n);
                return (T*) ::malloc(sizeof(T) * n);
            }

            void deallocate(T *p, size n) noexcept {
                fmt::printf("FREE %#X %v\n", (intptr) p, n);
                ::free(p);
            }

            template<class U>
            bool operator==(const AllocatorT <U>&) { return true; }
 
        } ;

        struct ChanBase {
            const int capacity = 0;
            
            struct Data {
                int q = 0;
                Selector *receivers;
                Selector *senders;
            } ;

            struct AtomicData {
                sync::Mutex mtx;

                atomic<int> q = 0;

                internal::IntrusiveList receivers;
                internal::IntrusiveList senders;

                AtomicData() {
                    receivers.mtx = &mtx;
                    senders.mtx = &mtx;
                }

                Data load();
                bool compare_and_swap(Data *expected, Data newval);
                bool receivers_empty_atomic() {
                    sync::Lock lock(mtx);
                    return receivers.empty_atomic();
                }

                bool senders_empty_atomic() {
                    // sync::Lock lock(mtx);
                    return senders.empty_atomic();
                }

                Selector *receivers_pop() {
                    // sync::Lock lock(mtx);
                    return receivers.pop();
                }

                Selector *senders_pop() {
                    // sync::Lock lock(mtx);
                    return senders.pop();
                }

                void receivers_push(Selector *sel) {
                    // sync::Lock lock(mtx);
                    receivers.push(sel);
                }

                void senders_push(Selector *sel) {
                    // sync::Lock lock(mtx);
                    senders.push(sel);
                }

                void receivers_remove(Selector *sel) {
                    // sync::Lock lock(mtx);
                    receivers.remove(sel);
                }

                void senders_remove(Selector *sel) {
                    // sync::Lock lock(mtx);
                    senders.remove(sel);
                }

                void receivers_clear() {
                    sync::Lock lock(mtx);
                    receivers.head = nil;
                }

                void senders_clear() {
                    sync::Lock lock(mtx);
                    senders.head = nil;
                }

                bool senders_head_compare_and_swap(Selector **expected, Selector *newval) {
                    sync::Lock lock(mtx);
                    return senders.head.compare_and_swap(expected, newval);
                }

                bool receivers_head_compare_and_swap(Selector **expected, Selector *newval) {
                    sync::Lock lock(mtx);
                    return receivers.head.compare_and_swap(expected, newval);
                }

                Selector *senders_head_load() {
                    sync::Lock lock(mtx);
                    return senders.head.load();
                }

                Selector *receivers_head_load() {
                    sync::Lock lock(mtx);
                    return receivers.head.load();
                }

                void senders_head_store(Selector *v) {
                    sync::Lock lock(mtx);
                    if (DebugLog) {
                        fmt::printf("%d %#x sender added %#x\n", pthread_self(), (uintptr) &receivers, (uintptr) v);
                    }
                    senders.head.store(v);
                }

                void receivers_head_store(Selector *v) {
                    sync::Lock lock(mtx);
                    if (DebugLog) {
                        fmt::printf("%d %#x receiver added %#x\n", pthread_self(), (uintptr) &receivers, (uintptr) v);
                    }
                    receivers.head.store(v);
                }

            } adata;
        
            enum State : byte {
                Open,
                Closed
            };

            atomic<State> state = atomic<State>(Open);
            
            Mutex    lock;

            ChanBase(int capacity);

            void close(this ChanBase &c);

            bool closed(this ChanBase const& c);
            
            bool is_buffered(this ChanBase const& c);
            bool is_empty(this ChanBase const& c);
            bool is_full(this ChanBase const& c);
            int length() const;

        protected:
            // virtual void buffer_push(void *elem, bool move) = 0;
            virtual void push(void *data, bool move) = 0; 
            virtual void pop(void *out) = 0;
            virtual void set(void *dest, void *val, bool move) = 0;
            // virtual void buffer_pop(void *out) = 0;
            virtual int unread() const = 0;

            void send_i(this ChanBase &c, void *elem, bool move);
            bool send_nonblocking_i(this ChanBase &c, void *elem, bool move);
            bool send_nonblocking(this ChanBase &c, void *elem, bool move, Data *data_out);
            int send_nonblocking_sel(this ChanBase &c, Selector *receiver);
            // bool send_nonblocking_locked(this ChanBase &c, void *elem, bool move, sync::Lock &lock);

            void send_blocking(this ChanBase &c, void *elem, bool move);

            // bool recv_nonblocking_locked(this ChanBase &c, void *out, bool *ok, Lock&);
            bool recv_nonblocking(this ChanBase &c, void *out, bool *ok, Data *data_out);
            int recv_nonblocking_sel(this ChanBase &c, Selector *receiver);
            void recv_blocking(this ChanBase &c, void *out, bool *ok);

            bool try_recv(this ChanBase &c, void *out, bool *ok, bool try_locks, bool *lock_fail);
            // bool try_send(this ChanBase &c, void *out, bool move, bool try_locks, bool *lock_fail);

            // bool can_recv(this ChanBase &c, Selector **selout, bool try_locks, bool *lock_fail, sync::Lock sendlock, sync::Lock &);
            // bool can_send(this ChanBase &c, Selector **selout, bool try_locks, bool *lock_fail, sync::Lock recvlock, sync::Lock &);

            // void recv_now(this ChanBase &c, void *out, bool *ok, Selector *sel, sync::Lock sendlock, sync::Lock &chanlock);
            // void send_now(this ChanBase &c, void *out, bool move, Selector *sel, sync::Lock sendlock, sync::Lock &chanlock);

            int subscribe_recv(this ChanBase &c, internal::Selector &receiver, Lock&);
            int subscribe_send(this ChanBase &c, internal::Selector &sender, Lock&);

            void unsubscribe_recv(this ChanBase &c, internal::Selector &receiver, Lock&);
            void unsubscribe_send(this ChanBase &c, internal::Selector &sender, Lock&);

            void send(ChanBase &c, void *elem, void(*)(ChanBase &c, void *elem));

            // bool notify_receiver(this ChanBase &c, void *elem, bool move, Lock&);
            // void notify_sender(this ChanBase &c, sync::Lock&);

            friend Recv;
            friend Send;
        };
    }
    
    //using namespace internal;

    // https://github.com/golang/go/blob/master/src/runtime/chan.go
    // https://medium.com/womenintechnology/exploring-the-internals-of-channels-in-go-f01ac6e884dc
    // https://github.com/tylertreat/chan/blob/master/src/chan.h

    template <typename T>
    struct Chan : internal::ChanBase {
        // boost::circular_buffer<T> buffer;
        // https://github.com/max0x7ba/atomic_queue?tab=readme-ov-file
        atomic_queue::AtomicQueueB2<T/*, internal::AllocatorT<T>*/> buffer;

        Chan(int capacity = 0) : ChanBase(capacity), buffer(capacity) {
            //printf("BUFFER SIZE %lu\n", sizeof(buffer));
            //printf("capacity %u\n", buffer.capacity());
        }

        void send(this Chan &c, T &&elem) {
            c.send_i(&elem, true);
        }

        void send(this Chan &c, T const& elem) {
            if constexpr (internal::DebugLog) {
                fmt::printf("%d %#x send %v\n", pthread_self(), (uintptr) &c, elem);
            }
            c.send_i((void*) &elem, false);
        }

        T recv(this Chan &c, bool *ok = nil) {
            T t = {};
            c.recv_blocking(&t, ok);
            return t;
        }

        bool recv_nonblocking(this Chan &c, T *out=nil, bool *ok = nil) {
            return c.recv_nonblocking(out, nil, ok);
        }

        protected:
        // void buffer_push(void *elem, bool move) override {
        //     // printf("buffer push\n");
        //     // this->unread++;
        //     if (move) {
        //         this->buffer.try_push(std::move(*((T*) elem)));
        //     } else {
        //         this->buffer.try_push(*((const T*) elem));
        //     }
        // }

        void push(void *elem, bool move) override {
            if (move) {
                this->buffer.push(std::move(*((T*) elem)));
            } else {
                this->buffer.push(*((const T*) elem));
            }

            if constexpr (internal::DebugLog) {
                fmt::printf("%d %#x pushed %v; size %v\n", pthread_self(), (uintptr) this, (T*) elem, buffer.was_size());
            }
                
        }

        void pop(void *out) override {
            if constexpr (internal::DebugLog) {
                fmt::printf("%d %#x about to pop...\n", pthread_self(), (uintptr) this);
            }
            if (out) {
                *((T*) out) = buffer.pop();
            } else {
                buffer.pop();
            }
            if constexpr (internal::DebugLog) {
                fmt::printf("%d %#x popped %v; size %v\n", pthread_self(), (uintptr) this, (T*) out, buffer.was_size());
            }
        }

        // void buffer_pop(void *out) override {
        //     // printf("buffer pop\n");
        //     // this->unread--;
        //     if (out) {
        //         *((T*) out) = this->buffer.pop();
        //         // *((T*) out) = std::move(this->buffer[this->unread]);
        //     } else {
        //         this->buffer.pop();
        //     }
        // }

        int unread() const override {
            return buffer.was_size();
        }

        void set(void *dest, void *val, bool move) override {
            if constexpr (internal::DebugLog) {
                fmt::printf("%d %#x set %v\n", pthread_self(), (uintptr) this, (T*) val);
            }
            if (val == nil) {
                *((T*) dest) = {};
            } else if (move) {
                *((T*) dest) = std::move(*(T*) val);
            } else {
                *((T*) dest) = *(const T*) val;
            }
        }
    } ;

    template <>
    struct Chan<void> : internal::ChanBase {
        Chan(int capacity = 0) : ChanBase(capacity) {}
        atomic<int> unread_v = atomic<int>(0);

        void send(this Chan &c) {
            c.send_i(0, false);
        }

        bool recv(this Chan &c, bool *ok = nil) {
            bool b;
            if (!ok) {
                ok = &b;
            }

            c.recv_blocking(0, ok);

            return *ok;
        }

        bool recv_nonblocking(this Chan &c) {
            sync::Lock lock;
            return c.ChanBase::recv_nonblocking(nil, nil, nil);
        }

        protected:
        // void buffer_push(void*, bool) override {}

        void push(void *, bool) override;
        // bool try_

        // void buffer_pop(void *) override {}

        int unread() const override;

        void set(void *, void *, bool) override {}
    } ;

    struct SelectOp {
        internal::ChanBase *chan;
        void *data;
        
        virtual bool poll(bool try_locks, bool *lock_fail) const = 0;

        virtual int subscribe(sync::internal::Selector &receiver, Lock&) const = 0;
        virtual void unsubscribe(sync::internal::Selector &receiver, Lock&) const = 0;
        
        virtual bool select(bool blocking) const = 0;
    } ;  

    struct Recv : SelectOp {
        bool *ok;

        template <typename T>
        Recv(Chan<T> &chan, T *data = nil, bool *ok = nil)  {
            init(&chan, data, ok);
        }

        template <typename T>
        Recv(Chan<T> *chan, T *data = nil, bool *ok = nil)  {

            init(chan, data, ok);
        }

        void init(internal::ChanBase *chan, void *data, bool *ok) {
            this->chan = chan;
            this->data = data;
            // this->mtx = chan.lock;
            this->ok = ok;
        }

        bool poll(bool try_locks, bool *lock_fail) const override;

        int subscribe(sync::internal::Selector &receiver, Lock&) const override;
        void unsubscribe(sync::internal::Selector &receiver, Lock&) const override;

        bool select(bool blocking) const override;
    } ;
    
    struct Send : SelectOp {
        bool move = false;

        template <typename T>
        Send(sync::Chan<T> &chan, T const &data)  : move(false) {
            init(&chan, (void*) &data);
        }

        template <typename T>
        Send(sync::Chan<T> &chan, T &&data)  : move(true) {
            init(&chan, &data);
        }

        template <typename T>
        Send(sync::Chan<T> *chan, T const &data)  : move(false) {
            init(chan, (void*) &data);
        }

        template <typename T>
        Send(sync::Chan<T> *chan, T &&data)  : move(true) {
            init(chan, &data);
        }
    
        void init(internal::ChanBase *chan, void *data) {
            this->chan = chan;
            this->data = data;
            // this->mtx = &chan.lock;
        }

        bool poll(bool try_locks, bool *lock_fail) const override;

        int subscribe(sync::internal::Selector &receiver, Lock&) const override;
        void unsubscribe(sync::internal::Selector &receive, Lock&) const override;

        bool select(bool blocking) const override;
    } ;


    namespace internal {
        struct OpData {
            sync::Lock chanlock;
            sync::Lock selector_lock;

            SelectOp const& op;

            Selector selector;
            Selector *ready_selector = nil;
            

            OpData(SelectOp const& op) : op(op) {}
        } ;

        int select_i(arr<OpData> ops, arr<OpData*> ops_ptrs, arr<OpData*> lockfail_ptrs, bool blocking);
        bool select_one(OpData const &op, bool blocking);
    }

    template <typename... Args>
    int select(Args&&... ops) {
        if constexpr (sizeof...(ops) == 1) {
            internal::select_one(ops..., true);
            return 0;
        }
        std::array<internal::OpData, sizeof...(Args)> ops_data = {ops...};
        std::array<internal::OpData*, sizeof...(Args)> ops_ptrs = {};
        std::array<internal::OpData*, sizeof...(Args)> ops_ptrs2 = {};

        return internal::select_i(arr(ops_data.data(), ops_data.size()), arr(ops_ptrs.data(), ops_ptrs.size()), arr(ops_ptrs2.data(), ops_ptrs2.size()), true);

    }

    template <typename... Args>
    int poll(Args&&... ops) {
        if constexpr (sizeof...(ops) == 1) {
            bool b = internal::select_one(ops..., false);
            return b ? 0 : -1;
        }
        std::array<internal::OpData, sizeof...(Args)> ops_data = {ops...};
        std::array<internal::OpData*, sizeof...(Args)> ops_ptrs = {};
        std::array<internal::OpData*, sizeof...(Args)> ops_ptrs2 = {};

        return internal::select_i(arr(ops_data.data(), ops_data.size()), arr(ops_ptrs.data(), ops_ptrs.size()), arr(ops_ptrs2.data(), ops_ptrs2.size()), false);
    }



   
}
