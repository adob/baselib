#include "go.h"

using namespace lib;
using namespace sync;

// void *sync::thread_func(void *arg) {
//     std::function<void()> *func = static_cast<std::function<void()>*>(arg);

//     (*func)();
// }

void sync::goexit() {
#ifdef __cpp_exceptions
    throw sync::exceptions::GoExit();
#else
    panic("unimplemented");
#endif
}

void sync::exceptions::GoExit::fmt(io::Reader &out, error err) const {
    out.write("sync::goexit() called", err);
}
