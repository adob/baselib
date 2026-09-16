module;

#include <TeensyThreads.h>

export module lib.sync.cond_teensy;

import lib;
import lib.sync.mutex;

export extern "C++"
namespace lib::sync {
    struct Cond : noncopyable {
        Cond() = default;
        ~Cond() = default;

        void wait(Mutex& mutex) {
            Waiter waiter;
            waiter.gate.lock();

            state.lock();
            waiter.next = waiters;
            waiters = &waiter;
            state.unlock();

            mutex.unlock();
            waiter.gate.lock();
            waiter.gate.unlock();
            mutex.lock();
        }

        void signal() {
            state.lock();
            Waiter* waiter = waiters;
            if (waiter != nullptr) {
                waiters = waiter->next;
            }
            state.unlock();

            if (waiter != nullptr) {
                waiter->gate.unlock();
            }
        }

        void broadcast() {
            state.lock();
            Waiter* waiter = waiters;
            waiters = nullptr;
            state.unlock();

            while (waiter != nullptr) {
                Waiter* next = waiter->next;
                waiter->gate.unlock();
                waiter = next;
            }
        }

    private:
        struct Waiter {
            Threads::Mutex gate;
            Waiter* next = nullptr;
        };

        Threads::Mutex state;
        Waiter* waiters = nullptr;
    };
}