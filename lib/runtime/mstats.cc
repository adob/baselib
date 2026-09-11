#include "mstats.h"

using namespace lib;
using namespace runtime;

void runtime::read_mem_stats(MemStats *m) {
    // TODO: Track cumulative allocation counts and bytes. mallinfo2() exposes
    // allocator state, not these lifetime counters, so it cannot supply them.
    *m = {};
}
