import lib.runtime.debug;

#include <sys/sysinfo.h>

import lib.sync.once;

using namespace lib;
using namespace lib::runtime;

static int ncpu = -1;
static sync::Once config;

int runtime::num_cpu() {
    config.run([&] {
        ncpu = get_nprocs();
    });
    
    return ncpu;
}
