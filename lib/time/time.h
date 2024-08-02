#pragma once

#include "lib/base.h"

namespace lib::time {
    using namespace lib;

    struct duration {
        int64 nsecs = 0;

        explicit operator bool() const {
            return nsecs != 0;
        }

        constexpr duration operator-(duration other) const {
            return { nsecs - other.nsecs };
        }

        constexpr duration operator*(int n) const {
            return { nsecs * n };
        }
        
        constexpr duration operator+(duration other) const {
            return { nsecs + other.nsecs };
        }

        constexpr friend duration operator*(int n, duration d) {
            return d*n;
        }

        float64 seconds() const;
        int64 milliseconds() const;

        constexpr auto operator <=> (duration other) const { return nsecs <=> other.nsecs; }
    } ;


    struct monotime : duration {} ;

    const duration nanosecond = duration { 1 };
    const duration microsecond = 1'000 * nanosecond;
    const duration millisecond = 1'000 * microsecond;
    const duration second      = 1'000 * millisecond;
    const duration minute      = 60 * second;
    const duration hour        = 60 * minute;

    monotime mono();
    duration hz(float64 n);

    void sleep(duration d);

    struct LoopTimer {
        time::duration delay_duration;
        time::monotime prev_time;

        explicit LoopTimer(time::duration d) : delay_duration(d) {}

        void start() {
            prev_time = time::mono();
        }

        time::duration delay() {
            if (!prev_time) {
                prev_time = time::mono();
                return {};
            }

            time::monotime now = time::mono();
            time::duration elapsed = now - prev_time;
            if (elapsed > delay_duration) {
                prev_time = now;
                return elapsed;
            }
            
            time::duration delay = delay_duration - elapsed;
            time::sleep(delay);
            prev_time = time::mono();
            return elapsed;
        }
    } ;
}
