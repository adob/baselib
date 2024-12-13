#include "time.h"

#include <cmath>
#include <ctime>
#include <time.h>

#include "lib/os.h"
#include "lib/os/error.h"


using namespace lib;

const int 
    SecondsPerMinute   = 60,
	SecondsPerHour     = 60 * SecondsPerMinute,
	SecondsPerDay      = 24 * SecondsPerHour,
	SecondsPerWeek     = 7 * SecondsPerDay,
	DaysPer400Years    = 365*400 + 97,
	DaysPer100Years    = 365*100 + 24,
	DaysPer4Years      = 365*4 + 1;

// The unsigned zero year for internal calculations.
// Must be 1 mod 400, and times before it will not compute correctly,
// but otherwise can be changed at will.
const int64 AbsoluteZeroYear = -292277022399;


const int64 WallToInternal = int64(1884*365 + 1884/4 - 1884/100 + 1884/400) * SecondsPerDay;
const int64 UnixToInternal = int64(1969*365 + 1969/4 - 1969/100 + 1969/400) * SecondsPerDay;

// The year of the zero Time.
// Assumed by the unixToInternal computation below.
const int64 InternalYear = 1;

// Offsets to convert between internal and absolute or Unix times.
const int64 AbsoluteToInternal = int64((AbsoluteZeroYear - InternalYear) * 365.2425 * SecondsPerDay);
const int64 InternalToUnix = -UnixToInternal;

const uint64 HasMonotonic = uint64(1) << 63;
const int64  MaxWall      = WallToInternal + ((uint64(1)<<33) - 1); // year 2157
const int64  MinWall      = UnixToInternal;               // year 1885
const uint64 NsecMask     = (int64(1)<<30) - 1;
const uint   NsecShift    = 30;

const arr<int32> DaysBefore = {{
	0,
	31,
	31 + 28,
	31 + 28 + 31,
	31 + 28 + 31 + 30,
	31 + 28 + 31 + 30 + 31,
	31 + 28 + 31 + 30 + 31 + 30,
	31 + 28 + 31 + 30 + 31 + 30 + 31,
	31 + 28 + 31 + 30 + 31 + 30 + 31 + 31,
	31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
	31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
	31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30,
	31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30 + 31,
}};

time::monotime time::clock() {
    struct timespec ts;
    int ret = clock_gettime(CLOCK_BOOTTIME, &ts);
    if (ret) {
        panic(os::Errno(errno));
    }

    return { ts.tv_sec*1'000'000'000 + ts.tv_nsec };
}

time::time time::now() {
    struct timespec boottime;
    int ret = clock_gettime(CLOCK_BOOTTIME, &boottime);
    if (ret) {
        panic(os::Errno(errno));
    }

    int64 mono = boottime.tv_sec*1'000'000'000 + boottime.tv_nsec;

    struct timespec walltime;
    ret = clock_gettime(CLOCK_REALTIME, &walltime);
    if (ret) {
        panic(os::Errno(errno));
    }

    int64 sec = walltime.tv_sec;
    int32 nsec = int32(walltime.tv_nsec);

    //mono -= start_time;
    sec += UnixToInternal - MinWall;
    if (uint64(sec) >> 33 != 0) {
        // Seconds field overflowed the 33 bits available when
		// storing a monotonic time. This will be true after
		// March 16, 2157.
		return time{uint64(nsec), sec + MinWall};
    }

    return {HasMonotonic | uint64(sec) << NsecShift | uint64(nsec), mono, /*&Local*/};
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

float64 time::duration::seconds() const {
    int64 seconds  = nsecs / second.nsecs;
	int64 nanos    = nsecs % second.nsecs;

	return float64(seconds) + float64(nanos)/1e9;
}

int64 time::duration::milliseconds() const {
    return nsecs / 1'000'000;
}

time::duration time::hz(float64 n) {
    return { int64(std::round(1e9 / n)) };
}

static auto fn() {
    struct {} ret;
    return ret;
}

// norm returns nhi, nlo such that
//
//	hi * base + lo == nhi * base + nlo
//	0 <= nlo < base
static std::tuple<int, int> norm(int hi, int lo, int base) {
	if (lo < 0) {
		int n = (-lo-1)/base + 1;
		hi -= n;
		lo += n * base;
	}
	if (lo >= base) {
		int n = lo / base;
		hi += n;
		lo -= n * base;
	}
	return {hi, lo};
}

static bool is_leap(int year){
	return year%4 == 0 && (year%100 != 0 || year%400 == 0);
}

// daysSinceEpoch takes a year and returns the number of days from
// the absolute epoch to the start of that year.
// This is basically (year - zeroYear) * 365, but accounting for leap days.
static uint64 days_since_epoch(int year) {
	uint64 y = uint64(int64(year) - AbsoluteZeroYear);

	// Add in days from 400-year cycles.
	auto n = y / 400;
	y -= 400 * n;
	auto d = DaysPer400Years * n;

	// Add in 100-year cycles.
	n = y / 100;
	y -= 100 * n;
	d += DaysPer100Years * n;

	// Add in 4-year cycles.
	n = y / 4;
	y -= 4 * n;
	d += DaysPer4Years * n;

	// Add in non-leap years.
	n = y;
	d += 365 * n;

	return d;
}


time::time time::date(int year, Month month, int day, int hour, int min, int sec, int nsec, const Location &loc) {
    // Normalize month, overflowing into year.
	int m = int(month) - 1;
	std::tie(year, m) = norm(year, m, 12);
	month = Month(m + 1);

	// Normalize nsec, sec, min, hour, overflowing into day.
	std::tie(sec, nsec) = norm(sec, nsec, 1e9);
	std::tie(min, sec) = norm(min, sec, 60);
	std::tie(hour, min) = norm(hour, min, 60);
	std::tie(day, hour) = norm(day, hour, 24);

	// Compute days since the absolute epoch.
	uint64 d = days_since_epoch(year);

	// Add in days before this month.
	d += uint64(DaysBefore[month-1]);
	if (is_leap(year) && month >= March) {
		d++; // February 29
	}

	// Add in days before today.
	d += uint64(day - 1);

	// Add in time elapsed today.
	uint64 abs = d * SecondsPerDay;
	abs += uint64(hour*SecondsPerHour + min*SecondsPerMinute + sec);

	auto unix = int64(abs) + (AbsoluteToInternal + InternalToUnix);

    panic("unimplemented");
    return {};
	// Look for zone offset for expected time, so we can adjust to UTC.
	// The lookup function expects UTC, so first we pass unix in the
	// hope that it will not be too close to a zone transition,
	// and then adjust if it is.
	// _, offset, start, end, _ := loc.lookup(unix)
	// if offset != 0 {
	// 	utc := unix - int64(offset)
	// 	// If utc is valid for the time zone we found, then we have the right offset.
	// 	// If not, we get the correct offset by looking up utc in the location.
	// 	if utc < start || utc >= end {
	// 		_, offset, _, _, _ = loc.lookup(utc)
	// 	}
	// 	unix -= int64(offset)
	// }

	// t := unixTime(unix, int32(nsec))
	// t.setLoc(loc)
	// return t
}

time::time time::unix(const struct timespec& walltime) {
    int64 sec = walltime.tv_sec;
    int32 nsec = int32(walltime.tv_nsec);

	return time {uint64(nsec), sec + UnixToInternal, /*Local*/};
}

time::time time::unix(int64 sec, int32 nsec) {
	if (nsec < 0 || nsec >= 1'000'000'000) {
		int32 n = nsec / 1'000'000'000;
		sec += n;
		nsec -= n * 1'000'000'000;
		if (nsec < 0) {
			nsec += 1'000'000'000;
			sec--;
		}
	}

	return time {uint64(nsec), sec + UnixToInternal, /*Local*/};
}

time::time unix(int64 sec, int64 nsec) {
	if (nsec < 0 || nsec >= 1'000'000'000) {
		int64 n = nsec / 1'000'000'000;
		sec += n;
		nsec -= n * 1'000'000'000;
		if (nsec < 0) {
			nsec += 1'000'000'000;
			sec--;
		}
	}

	return unix(sec, nsec);
}