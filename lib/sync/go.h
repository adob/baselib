#pragma once
#include <exception>
#include <functional>
#include <future>
#include <pthread.h>
#include <thread>

#include "lib/debug.h"
#include "lib/debug/debug.h"
#include "lib/exceptions.h"
#include "lib/os.h"
#include "lib/os/error.h"

namespace lib::sync {

    //void *thread_func(void *arg);

    struct Thread {
        std::thread thread;

        template<typename Function, typename... Args>
        Thread(Function&& f, Args&&... args) : thread(std::move(f), std::move(args)...) {}

        void join() {
            this->thread.join();
        }

        ~Thread() {
            this->thread.detach();
        }
    } ;

    namespace exceptions {
        struct GoExit : Exception {
            virtual void fmt(io::Reader &out, error err) const override;
          } ;
    }
    

    struct go {
        template <typename Function>
        struct Wrapper {
            Function f;
            
            template <typename ...Args>
            void operator()(Args && ...args)  { 
                try {
                    f(std::forward<Args>(args)...);
                } catch (sync::exceptions::GoExit const&) {
                    // do nothing
                }
            }
        } ;

        std::thread thread;
        bool active = false;

        go() {}

        template<typename Function, typename... Args>
        go(Function&& f, Args&&... args) : 
            thread(Wrapper<Function>{std::forward<Function>(f)}, std::forward<Args>(args)...), active(true) {        
        }

        void join() {
            if (!this->active) {
                return;
            }
            this->active = false;
            thread.join();
        }

        void detach() {
            this->active = false;
            thread.detach();
        }

        go& operator=(go &&other) {
            if (this->active) {
                panic("assignment to active go");
            }
            this->active = other.active;
            this->thread = std::move(other.thread);
            other.active = false;
            return *this;
        }

        ~go() {
            if (this->active) {
                thread.join();
            }
        }
    } ;



    // goexit terminates the goroutine that calls it.
    // Thros GoExit exception.
    void goexit();
} 