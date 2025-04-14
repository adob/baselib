#include "go.h"
#include "lock.h"
#include <deque>
#include <utility>

namespace lib::sync {
    struct Gang {
        Mutex mtx;
        std::deque<sync::go> gs;

        template<typename Function, typename... Args>
        void go(Function&& f, Args&&... args) {
            Lock lock(mtx);
            // new sync::go(std::forward<Function>(f), std::forward<Args>(args)...);
            this->gs.emplace_back(std::forward<Function>(f), std::forward<Args>(args)...);
        }

        void join();
        ~Gang();
    } ;
}