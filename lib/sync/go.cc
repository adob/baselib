#include "go.h"

using namespace lib;
using namespace sync;

// void *sync::thread_func(void *arg) {
//     std::function<void()> *func = static_cast<std::function<void()>*>(arg);

//     (*func)();
// }

void sync::goexit() {
    throw sync::exceptions::GoExit();
}

void sync::exceptions::GoExit::fmt(io::Reader &out, error err) const {
    out.write("sync::goexit() called", err);
}
