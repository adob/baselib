#include "debug.h"

#include <sys/sysinfo.h>

#include "lib/sync/once.h"

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
