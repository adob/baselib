module;

#include <TeensyThreads.h>

export module lib.sync.rwmutex_teensy;

import lib;

export extern "C++"
namespace lib::sync {
    // A writer holds turnstile while waiting for and using resource, preventing
    // later readers from starving it. The first reader acquires resource and
    // the last reader releases it, allowing readers to run concurrently.
    struct RWMutex : noncopyable {
        RWMutex() = default;
        ~RWMutex() = default;

        void r_lock() {
            turnstile.lock();
            readers_mutex.lock();
            if (readers++ == 0) {
                resource.lock();
            }
            readers_mutex.unlock();
            turnstile.unlock();
        }

        bool try_r_lock() {
            if (!turnstile.try_lock()) {
                return false;
            }
            if (!readers_mutex.try_lock()) {
                turnstile.unlock();
                return false;
            }
            if (readers == 0 && !resource.try_lock()) {
                readers_mutex.unlock();
                turnstile.unlock();
                return false;
            }
            ++readers;
            readers_mutex.unlock();
            turnstile.unlock();
            return true;
        }

        void r_unlock() {
            readers_mutex.lock();
            if (--readers == 0) {
                resource.unlock();
            }
            readers_mutex.unlock();
        }

        void lock() {
            turnstile.lock();
            resource.lock();
        }

        bool try_lock() {
            if (!turnstile.try_lock()) {
                return false;
            }
            if (!resource.try_lock()) {
                turnstile.unlock();
                return false;
            }
            return true;
        }

        void unlock() {
            resource.unlock();
            turnstile.unlock();
        }

        unsigned int readers = 0;
        Threads::Mutex turnstile;
        Threads::Mutex readers_mutex;
        Threads::Mutex resource;
    } ;
}
