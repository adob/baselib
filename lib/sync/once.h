#pragma once

#include <atomic>

#include "lib/sync/lock.h"
#include "lib/sync/mutex.h"
#include "lib/types.h"
#include "lib/utils.h"

namespace lib::sync {

    // Once is an object that will perform exactly one action.
    struct Once : noncopyable {
        std::atomic<bool> done = false;
        Mutex             m;


        // run calls the function f if and only if do is being called for the
        // first time for this instance of [Once]. In other words, given
        //
        //	Once once
        //
        // if once.run(f) is called multiple times, only the first call will invoke f,
        // even if f has a different value in each invocation. A new instance of
        // Once is required for each function to execute.
        //
        // run is intended for initialization that must be run exactly once. Since f
        // is niladic, it may be necessary to use a function literal to capture the
        // arguments to a function to be invoked by run:
        //
        //	config.once.run([&] { config.init(filename) })
        //
        // Because no call to Do returns until the one call to f returns, if f causes
        // Do to be called, it will deadlock.
        //
        // If f throws an exception, run considers it to have returned; future calls 
        // of run return without calling f.
        void run(this Once &o, auto f) {
            if (o.done.load()) {
                return;
            }
            
            sync::Lock lock (o.m);
            if (o.done.load()) {
                return;
            }

        #ifdef __cpp_exceptions
            try {
                f();
            } catch (...) {
                o.done.store(true);
                throw;
            }
        #else
            f();
        #endif

            o.done.store(true);
        }
    };
}