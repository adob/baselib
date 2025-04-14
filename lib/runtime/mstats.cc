#include "mstats.h"

#include <malloc.h>


using namespace lib;
using namespace runtime;

void runtime::read_mem_stats(MemStats *m) {
    struct mallinfo2 info = mallinfo2();
}
