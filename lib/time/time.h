#pragma once

#include <ctime>

#include "lib/base.h"
#include "lib/types.h"

#ifdef unix
#undef unix
#define unix unix
#endif

namespace lib::time {
    using namespace lib;

    struct duration : numeric {
        int64 nsecs = 0;

        constexpr duration(int64 nsecs=0) : nsecs(nsecs) {}

        float64 seconds() const;
        int64 milliseconds() const;
        int64 nanoseconds() const { return this->nsecs; };
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

    struct monotime : duration {
      duration sub(monotime other);
    } ;

    struct time {
        // nanoseconds since boot
        int64 nsecs = 0;
        // uint64          wall = 0;
        // int64           ext  = 0;
        //const Location *location = nil;

        // Unix returns t as a Unix time, the number of seconds elapsed
        // since January 1, 1970 UTC. The result does not depend on the
        // location associated with t.
        // Unix-like operating systems often record time as a 32-bit
        // count of seconds, but since the method here returns a 64-bit
        // value it is valid for billions of years into the past or future.
        int64 unix() const;

        // unix_nano returns t as a Unix time, the number of nanoseconds elapsed
        // since January 1, 1970 UTC. The result is undefined if the Unix time
        // in nanoseconds cannot be represented by an int64 (a date before the year
        // 1678 or after 2262). Note that this means the result of calling unix_nano
        // on the zero Time is undefined. The result does not depend on the
        // location associated with t.
        int64 unix_nano() const;

        // Sub returns the duration t-u. If the result exceeds the maximum (or minimum)
        // value that can be stored in a [Duration], the maximum (or minimum) duration
        // will be returned.
        // To compute t-d for a duration d, use t.Add(-d)
        duration sub(time u) const;
    } ;

    const duration nanosecond = duration { 1 };
    const duration microsecond = 1'000 * nanosecond;
    const duration millisecond = 1'000 * microsecond;
    const duration second      = 1'000 * millisecond;
    const duration minute      = 60 * second;
    const duration hour        = 60 * minute;

    monotime clock();
    time     now();
    
    // since returns the time elapsed since t.
    // It is shorthand for time.Now().Sub(t).
    duration since(time t);

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
