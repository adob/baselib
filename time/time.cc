#include "time.h"

#include <cmath>
#include <time.h>

#include "lib/os.h"


using namespace lib;
using namespace time;

monotime time::mono() {
    struct timespec ts;
    int ret = clock_gettime(CLOCK_BOOTTIME, &ts);
    if (ret) {
        panic(os::from_errno(errno));
    }

    return { ts.tv_sec*1'000'000'000 + ts.tv_nsec };
}

void time::sleep(duration d) {
     struct timespec req = {
        .tv_sec  = d.nsecs / 1'000'000'000,
        .tv_nsec = d.nsecs % 1'000'000'000
     };
     
  retry:
     int r = nanosleep(&req, &req);
     if (r != 0) {
        if (errno == EINTR) {
            goto retry;
        }
        panic("nanosleep failed");
     }
}

float64 duration::seconds() const {
    int64 seconds  = nsecs / second.nsecs;
	int64 nanos    = nsecs % second.nsecs;

	return float64(seconds) + float64(nanos)/1e9;
}

int64 duration::milliseconds() const {
    return nsecs / 1'000'000;
}

duration time::hz(float64 n) {
    return { int64(std::round(1e9 / n)) };
}