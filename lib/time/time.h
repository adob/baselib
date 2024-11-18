#pragma once

#include <ctime>

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

    enum Month {
        January = 1,
        February,
        March,
        April,
        May,
        June,
        July,
        August,
        September,
        October,
        November,
        December,
    } ;

    struct Location {
        str name;
    } ;

    inline const Location UTC   = {.name = "UTC"};
    inline const Location Local = {.name = "Local"};

    struct monotime : duration {} ;
    struct time {
        uint64          wall = 0;
        int64           ext  = 0;
        //const Location *location = nil;
    } ;

    const duration nanosecond = duration { 1 };
    const duration microsecond = 1'000 * nanosecond;
    const duration millisecond = 1'000 * microsecond;
    const duration second      = 1'000 * millisecond;
    const duration minute      = 60 * second;
    const duration hour        = 60 * minute;

    monotime clock();
    time     now();
    duration hz(float64 n);

    // Unix returns the local Time corresponding to the given Unix time,
    // sec seconds and nsec nanoseconds since January 1, 1970 UTC.
    // It is valid to pass nsec outside the range [0, 999999999].
    // Not all sec values have a corresponding time value. One such
    // value is 1<<63-1 (the largest int64 value).
    time    unix(const struct timespec &walltime);
    time    unix(int64 sec, int32 nsec);
    time    unix(int64 sec, int64 nsec);

    time date(int year, Month month, int day, int hour, int min, int sec, int nsec, const Location &loc);

    void sleep(duration d);

    struct LoopTimer {
        duration delay_duration;
        monotime prev_time;

        explicit LoopTimer(duration d) : delay_duration(d) {}

        void start() {
            prev_time = clock();
        }

        duration delay() {
            if (!prev_time) {
                prev_time = clock();
                return {};
            }

            monotime now = clock();
            duration elapsed = now - prev_time;
            if (elapsed > delay_duration) {
                prev_time = now;
                return elapsed;
            }
            
            duration delay = delay_duration - elapsed;
            sleep(delay);
            prev_time = clock();
            return elapsed;
        }
    } ;
}
