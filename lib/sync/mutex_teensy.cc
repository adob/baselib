module;

#include <TeensyThreads.h>

export module lib.sync.mutex_teensy;

import lib;

export extern "C++"
namespace lib::sync {
    struct Mutex : noncopyable {
        Mutex() = default;
        ~Mutex() = default;

        void lock() {
            mutex.lock();
        }

        bool try_lock() {
            return mutex.try_lock() != 0;
        }

        void unlock() {
            mutex.unlock();
        }

        Threads::Mutex mutex;
    } ;

}