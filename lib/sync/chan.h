#pragma once

#include <boost/circular_buffer.hpp>
#include <atomic>

#include "lib/base.h"
#include "lib/sync/atomic.h"
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

    constexpr bool DebugChecks = true;

    namespace internal {
        // template <typename T>
        // struct Queue {

        // } ;

        template <typename T>
        struct IntrusiveList {
            struct Element {
                T *prev = nil;
                T *next = nil;
            } ;

            void push(T *e) {
                // printf("PUSH this(%p) %p\n", this, e);
                e->next = this->head;
                e->prev = nil;

                if (this->head) {
                    this->head->prev = e;
                }

                this->head = e;
                // dump();
            }

            T *pop() {
                // printf("POP this(%p)\n", this);
                T *e = this->head;
                this->head = e->next;

                if (e->next) {
                    e->next->prev = nil; 
                }
                // dump();
                return e;
            }

            void remove(T *e) {
                // printf("REMOVE this(%p) %p\n", this, e);
                if constexpr (DebugChecks) {
                    bool found = false;
                    for (T *elem = this->head; elem != nil; elem = elem->next) {
                        if (e == elem) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        printf("PANIC!!! %p\n", this);
                        dump();

                        panic("element not found");
                    }
                }
                if (!e->prev) {
                    this->head = e->next;
                } else {
                    e->prev->next = e->next;
                }

                if (e->next) {
                    e->next->prev = e->prev;
                }
            }

            bool empty() const {
                return this->head == nil;
            }

            T *head = nil;

            void dump() {
                //flockfile(stdout);
                if (!this->head) {
                    printf("empty list this(%p)\n", this);
                    return;
                }
                printf("DUMP this(%p) [%p", this, this->head);
                for (T *elem = this->head->next; elem != nil; elem = elem->next) {
                    printf(" -> %p", elem);
                }

                printf("]\n");
                //funlockfile(stdout);
            }

            bool contains(T *e) {
                for (T *elem = this->head; elem != nil; elem = elem->next) {
                    if (e == elem) {
                        return true;
                    }
                }

                return false;
            }
        } ;

        struct Waiter {
            std::atomic<int> state = 0;

            void notify();

            void wait();
        } ;

        struct Selector : IntrusiveList<Selector>::Element {
            // sync::Mutex *mtx = nil;
            // sync::Cond  *cond = nil;

            void    *value = nil;
            bool    *ok = nil;
            bool    move = false;

            int      id = 0;

            std::atomic<bool>      *active = nil;
            Waiter *completed;
            atomic<Selector*> *completer = nil;
            bool done = false;

            bool panic = false;
        };

        struct ChanBase {
            int       unread = 0;
            const int       capacity = 0;
            //void       *receiver = nil;

            internal::IntrusiveList<internal::Selector>  receivers;
            internal::IntrusiveList<internal::Selector>    senders;
        
            enum State : byte {
                Open,
                //CanRecv,
                Closed
            } state = Open;
            
            Mutex    lock;
            //Cond     r_cond;
            //Cond     send_cond;

            ChanBase(int capacity);

            void close(this ChanBase &c);

            bool closed(this ChanBase const& c);
            
            bool is_buffered(this ChanBase const& c);
            bool is_empty(this ChanBase const& c);
            bool is_full(this ChanBase const& c);
            int length() const;

        protected:
            virtual void buffer_push(void *elem, bool move) = 0;
            virtual void set(void *dest, void *val, bool move) = 0;
            virtual void buffer_pop(void *out) = 0;

            void send_i(this ChanBase &c, void *elem, bool move);
            bool send_nonblocking(this ChanBase &c, void *elem, bool move, Lock&);
            void send_blocking(this ChanBase &c, void *elem, bool move, Lock&);

            void recv_i(this ChanBase &c, void *out, bool *closed);
            bool recv_nonblocking(this ChanBase &c, void *out, bool *closed, Lock&);
            void recv_blocking(this ChanBase &c, void *out, bool *closed, Lock&);

            bool try_recv(this ChanBase &c, void *out, bool *ok, bool try_locks, bool *lock_fail);
            bool try_send(this ChanBase &c, void *out, bool move, bool try_locks, bool *lock_fail);

            // bool can_recv(this ChanBase &c, Selector **selout, bool try_locks, bool *lock_fail, sync::Lock sendlock, sync::Lock &);
            // bool can_send(this ChanBase &c, Selector **selout, bool try_locks, bool *lock_fail, sync::Lock recvlock, sync::Lock &);

            // void recv_now(this ChanBase &c, void *out, bool *ok, Selector *sel, sync::Lock sendlock, sync::Lock &chanlock);
            // void send_now(this ChanBase &c, void *out, bool move, Selector *sel, sync::Lock sendlock, sync::Lock &chanlock);

            bool subscribe_recv(this ChanBase &c, internal::Selector &receiver, Lock&);
            bool subscribe_send(this ChanBase &c, internal::Selector &sender, Lock&);

            void unsubscribe_recv(this ChanBase &c, internal::Selector &receiver, Lock&);
            void unsubscribe_send(this ChanBase &c, internal::Selector &sender, Lock&);

            void send(ChanBase &c, void *elem, void(*)(ChanBase &c, void *elem));

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
        boost::circular_buffer<T> buffer;

        Chan(int capacity = 0) : ChanBase(capacity), buffer(capacity) {}


        void send(this Chan &c, T &&elem) {
            c.send_i(&elem, true);
        }

        void send(this Chan &c, T const& elem) {
            c.send_i((void*) &elem, false);
        }

        T recv(this Chan &c, bool *ok = nil) {
            T t = {};
            c.recv_i(&t, ok);
            return t;
        }

        protected:
        void buffer_push(void *elem, bool move) override {
            // printf("buffer push\n");
            this->unread++;
            if (move) {
                this->buffer.push_front(std::move(*((T*) elem)));
            } else {
                this->buffer.push_front(*((const T*) elem));
            }
        }

        void buffer_pop(void *out) override {
            // printf("buffer pop\n");
            this->unread--;
            if (out) {
                *((T*) out) = std::move(this->buffer[this->unread]);
            }
        }

        void set(void *dest, void *val, bool move) override {
            if (val == nil) {
                *((T*) dest) = T{};
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

        void send(this Chan &c) {
            c.send_i(0, false);
        }

        bool recv(this Chan &c, bool *ok = nil) {
            bool b;
            if (!ok) {
                ok = &b;
            }

            c.recv_i(0, ok);

            return *ok;
        }

        protected:
        void buffer_push(void*, bool) override {}

        void buffer_pop(void *) override {}

        void set(void *, void *, bool) override {}
    } ;

    struct SelectOp {
        internal::ChanBase *chan;
        void *data;
        
        virtual bool poll(bool try_locks, bool *lock_fail) const = 0;

        virtual bool subscribe(sync::internal::Selector &receiver, Lock&) const = 0;
        virtual void unsubscribe(sync::internal::Selector &receiver, Lock&) const = 0;
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

        bool subscribe(sync::internal::Selector &receiver, Lock&) const override;
        void unsubscribe(sync::internal::Selector &receiver, Lock&) const override;
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

        bool subscribe(sync::internal::Selector &receiver, Lock&) const override;
        void unsubscribe(sync::internal::Selector &receive, Lock&) const override;
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

        int select_i(arr<OpData> ops, arr<OpData*> ops_ptrs, arr<OpData*> lockfail_ptrs, bool blocking, bool debug=false);
    }

    template <typename... Args>
    int select(Args&&... ops) {
        std::array<internal::OpData, sizeof...(Args)> ops_data = {ops...};
        std::array<internal::OpData*, sizeof...(Args)> ops_ptrs = {};
        std::array<internal::OpData*, sizeof...(Args)> ops_ptrs2 = {};

        return internal::select_i(arr(ops_data.data(), ops_data.size()), arr(ops_ptrs.data(), ops_ptrs.size()), arr(ops_ptrs2.data(), ops_ptrs2.size()), true);

    }

    template <typename... Args>
    int select_debug(Args&&... ops) {
        std::array<internal::OpData, sizeof...(Args)> ops_data = {ops...};
        std::array<internal::OpData*, sizeof...(Args)> ops_ptrs = {};
        std::array<internal::OpData*, sizeof...(Args)> ops_ptrs2 = {};

        return internal::select_i(arr(ops_data.data(), ops_data.size()), arr(ops_ptrs.data(), ops_ptrs.size()), arr(ops_ptrs2.data(), ops_ptrs2.size()), true, true);
    }

    template <typename... Args>
    int poll(Args&&... ops) {
        std::array<internal::OpData, sizeof...(Args)> ops_data = {ops...};
        std::array<internal::OpData*, sizeof...(Args)> ops_ptrs = {};
        std::array<internal::OpData*, sizeof...(Args)> ops_ptrs2 = {};

        return internal::select_i(arr(ops_data.data(), ops_data.size()), arr(ops_ptrs.data(), ops_ptrs.size()), arr(ops_ptrs2.data(), ops_ptrs2.size()), false);
    }



   
}