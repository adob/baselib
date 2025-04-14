#include "2way.h"

#include "lib/print.h"
#include "lib/str.h"
#include "lib/math/math.h"

using namespace lib;
using namespace lib::strings;
using namespace lib::strings::detail;

// returns {index, period} where index is the cut point index of
// the right half and period is the global period of the right half
std::pair<size, size> detail::critical_factorization_fwd(str s) {
    size i = -1;
    size j = 0;     // 0 <= j < len(s) - 1
    size k = 1;
    size p = 1;
    size n = len(s);

    while (j + k < n) {
        byte a = s[j+k];
        byte b = s[i+k];
        if (a < b) {
	        /* Suffix is smaller, period is entire prefix so far.  */
	        j += k;
	        k = 1;
	        p = j - i;
	    } else if (a == b) {
	        /* Advance through repetition of the current period.  */
	        if (k != p) {
	            k++;
            } else {
	            j += p;
	            k = 1;
            }
	    } else /* a > b */ {
	        /* Suffix is larger, start over from current location.  */
	        i = j++;
	        k = p = 1;
	    }
    }
    size max_suffix = i;
    size period = p;

    // Perform reverse lexicographic search.
    i = -1;
    j = 0;
    k = 1;
    p = 1;
    while (j + k < n) {
        byte a = s[j + k];
        byte b = s[i + k];
        if (b < a) {
	        /* Suffix is smaller, period is entire prefix so far.  */
	        j += k;
	        k = 1;
	        p = j - i;
	    } else if (a == b) {
	        /* Advance through repetition of the current period.  */
            if (k != p) {
                k++;
            } else {
                j += p;
	            k = 1;
	        }
	    } else /* a < b */ {
	        /* Suffix is larger, start over from current location.  */
	        i = j++;
	        k = p = 1;
	    }
    }
    size max_suffix_rev = i;

    // Choose the longer suffix. Return the first byte of the right
    // half, rather than the last byte of the left half.
    if (max_suffix_rev + 1 < max_suffix + 1) {
        return {max_suffix + 1, period};
    }
  
    period = p;
    return {max_suffix_rev + 1, period};
}

// returns {index, period} where index is the cut point index of
// the right half and period is the global period of the right half
std::pair<size, size> detail::critical_factorization_rev(str s) {
    size n = len(s);
    size i = n;
    size j = n-1;     // 0 <= j < len(s) - 1
    size k = 1;
    size p = 1;
    size period;

    while (j - k >= 0) {
        byte a = s[j - k];
        byte b = s[i - k];
        if (a < b) {
	        /* Suffix is smaller, period is entire prefix so far.  */
	        j -= k;
	        k = 1;
	        p = i - j;
	    } else if (a == b) {
	        /* Advance through repetition of the current period.  */
	        if (k != p) {
	            k++;
            } else {
	            j -= p;
	            k = 1;
            }
	    } else /* a > b */ {
	        /* Suffix is larger, start over from current location.  */
	        i = j--;
	        k = p = 1;
	    }
    }

    size max_suffix = i;
    period = p;

    // Perform reverse lexicographic search.
    i = n;
    j = n-1;
    k = 1;
    p = 1;
    while (j - k >= 0) {
        byte a = s[j - k];
        byte b = s[i - k];
        if (b < a) {
	        /* Suffix is smaller, period is entire prefix so far.  */
	        j -= k;
	        k = 1;
	        p = i - j;
	    } else if (a == b) {
	        /* Advance through repetition of the current period.  */
            if (k != p) {
                k++;
            } else {
                j -= p;
	            k = 1;
	        }
	    } else /* a < b */ {
	        /* Suffix is larger, start over from current location.  */
	        i = j--;
	        k = p = 1;
	    }
    }
    usize max_suffix_rev = i;

    // Choose the longer suffix. Return the first byte of the right
    // half, rather than the last byte of the left half.
    if (max_suffix_rev + 1 > max_suffix + 1) {
        return {max_suffix, period};
    }
  
    period = p;
    return {max_suffix_rev, period};
}

size detail::two_way_search_fwd(str haystack, str needle) {
    size j; /* Index into current window of HAYSTACK.  */
    size shift_table[1U << 8]; /* See below.  */
    size haystack_len = len(haystack);
    size needle_len = len(needle);

    /* Factor the needle into two halves, such that the left half is
        smaller than the global period, and the right half is
        periodic (with a period as large as NEEDLE_LEN - suffix).  */
    //size suffix; /* The index of the right half of needle.  */
    auto [suffix, period] = critical_factorization_fwd(needle);
    // print "criticial factorization of %q got suffix %v, period %v" % needle, suffix, period;

    // Populate shift_table.  For each possible byte value c,
    // shift_table[c] is the distance from the last occurrence of c to
    // the end of NEEDLE, or NEEDLE_LEN if c is absent from the NEEDLE.
    // shift_table[NEEDLE[NEEDLE_LEN - 1]] contains the only 0.  */
    for (int i = 0; i < 1U << 8; i++) {
        shift_table[i] = needle_len;
    }
    for (size i = 0; i < needle_len; i++) {
        shift_table[byte(needle[i])] = needle_len - i - 1;
    }

    // Perform the search.  Each iteration compares the right half
    // first.
    if (memcmp(needle.data, needle.data + period, suffix) == 0) {
        // print "periodic case";
        // Entire needle is periodic; a mismatch can only advance by the
        // period, so use memory to avoid rescanning known occurrences
        // of the period.
        size memory = 0;
        size shift;
        j = 0;
        while (j <= haystack_len - needle_len) {
            // Check the last byte first; if it does not match, then
            // shift to the next possible match location.
            shift = shift_table[byte(haystack[j + needle_len - 1])];

            if (0 < shift) {
                if (memory && shift < period) {
                    // Since needle is periodic, but the last period has
                    // a byte out of place, there can be no match until
                    // after the mismatch.
                    shift = needle_len - period;
                }
                memory = 0;
                j += shift;
                continue;
            }

            // Scan for matches in right half.  The last byte has
            // already been matched, by virtue of the shift table.
            size i = math::max(suffix, memory);
            while (i < needle_len - 1 && (needle[i] == haystack[i + j])) {
                i++;
            }
            if (needle_len - 1 <= i) {
                /* Scan for matches in left half.  */
                i = suffix - 1;
                while (memory < i + 1 && (needle[i] == haystack[i + j])) {
                    i--;
                }
                if (i + 1 < memory + 1) {
                    return j;
                }
                /* No match, so remember how many repetitions of period
                on the right half were scanned.  */
                j += period;
                memory = needle_len - period;
            } else {
                j += i - suffix + 1;
                memory = 0;
            }
        }
    } else {
        // print "distinct case";
        // The two halves of needle are distinct; no extra memory is
        // required, and any mismatch results in a maximal shift.
        size shift;
        period = std::max(suffix, needle_len - suffix) + 1;
        j = 0;
        while (j <= haystack_len - needle_len) {
            // Check the last byte first; if it does not match, then
            // shift to the next possible match location.
            shift = shift_table[byte(haystack[j + needle_len - 1])];
            if (0 < shift) {
                j += shift;
                continue;
            }
            // Scan for matches in right half.  The last byte has
            // already been matched, by virtue of the shift table.  */
            usize i = suffix;
            while (i < needle_len - 1 && (needle[i] == haystack[i + j])) {
                i++;
            }
            if (needle_len - 1 <= i) {
                /* Scan for matches in left half.  */
                i = suffix - 1;
                while (i != -1 && ( (needle[i]) ==  (haystack[i + j]))) {
                    i--;
                }
                if (i == -1) {
                    return j;
                }
                j += period;
            } else {
                j += i - suffix + 1;
            }
        }
    }
    return -1;
}

size detail::two_way_search_rev(str haystack, str needle) {
    size j; /* Index into current window of HAYSTACK.  */
    size shift_table[1U << 8]; /* See below.  */
    size haystack_len = len(haystack);
    size needle_len = len(needle);

    /* Factor the needle into two halves, such that the left half is
        smaller than the global period, and the right half is
        periodic (with a period as large as NEEDLE_LEN - suffix).  */
    //size suffix; /* The index of the right half of needle.  */
    auto [suffix, period] = critical_factorization_rev(needle);
    // print "criticial factorization of %q got suffix %v, period %v" % needle, suffix, period;

    // Populate shift_table.  For each possible byte value c,
    // shift_table[c] is the distance from the last occurrence of c to
    // the end of NEEDLE, or NEEDLE_LEN if c is absent from the NEEDLE.
    // shift_table[NEEDLE[NEEDLE_LEN - 1]] contains the only 0.  */
    for (int i = 0; i < 1U << 8; i++) {
        shift_table[i] = needle_len;
    }
    for (size i = needle_len - 1; i >= 0; i--) {
        shift_table[byte(needle[i])] = i;
    }

    // Perform the search.  Each iteration compares the right half
    // first.
    // if (memcmp(needle.data + needle.len - 1, needle.data + needle.len - 1 - period, suffix) == 0) {
    size prefix = needle_len - suffix;
    if (needle[len(needle) - prefix, len(needle)] == needle[len(needle) - period - prefix, len(needle) - period]) {
        // print "periodic case";
        // Entire needle is periodic; a mismatch can only advance by the
        // period, so use memory to avoid rescanning known occurrences
        // of the period.
        size memory = 0;
        size shift;
        j = haystack_len - 1;
        while (j >= needle_len - 1) {
            // Check the last byte first; if it does not match, then
            // shift to the next possible match location. 
            // print "j %v; c %#U; shift %v" % j, haystack[j - needle_len + 1], shift_table[byte(haystack[j - needle_len + 1])];
            shift = shift_table[byte(haystack[j - needle_len + 1])];
            // print "shift is", shift;
            if (shift > 0) {
                if (memory && shift < period) {
                    // Since needle is periodic, but the last period has
                    // a byte out of place, there can be no match until
                    // after the mismatch.
                    shift = needle_len - period;
                }
                memory = 0;
                j -= shift;
                // print "L324: shift j by %v to %v" % -shift, j;
                continue;
            }

            // Scan for matches in second half.  The last byte has
            // already been matched, by virtue of the shift table.
            size i = math::min(suffix - 1, needle_len - 1 - memory);
            size k = prefix;
            // print "last byte matched; i %v; j %v; k %v" % i, j, k;
            while (i > 0 && (needle[i] == haystack[j - k])) {
                // print "loop k %v" % k;
                i--;
                k++;
            }
            // print "i is", i;
            if (i == 0) {
                // print "match in second half";
                // Scan for matches in first half.
                i = suffix;
                size k = needle_len - suffix - 1;
                size limit = needle_len - memory ;
                // print "i %v; k %v; limit %v, memory %v" % i, k, limit, memory;
                while (i < limit && needle[i] == haystack[j - k]) {
                    i++;
                    k--;
                }
                if (i >= limit) {
                    return j - needle_len + 1;
                }
                // No match, so remember how many repetitions of period
                // on the second half were scanned.
                j -= period;
                memory = needle_len - period;
                // print "memory is", memory;
            } else {
                // right match failed
                // print "right match failed; i %v; prefix %v" % i, prefix;
                j -= needle_len - i - prefix;
                memory = 0;
            }
        }
    } else {
        // print "distinct case";
        // The two halves of needle are distinct; no extra memory is
        // required, and any mismatch results in a maximal shift.
        size shift;
        period = std::max(prefix, needle_len - prefix) + 1;
        j = haystack_len - 1;
        while (j >= needle_len - 1) {
            // Check the last byte first; if it does not match, then
            // shift to the next possible match location.
            // print "j %v; c %#U; shift %v" % j, haystack[j - needle_len + 1], shift_table[byte(haystack[j - needle_len + 1])];
            shift = shift_table[byte(haystack[j - needle_len + 1])];
            // print "shift is", shift;
            if (shift > 0) {
                j -= shift;
                continue;
            }
            // Scan for matches in second half.  The last byte has
            // already been matched, by virtue of the shift table.  */
            size i = suffix - 1;
            size k = prefix;
            while (i > 0 && (needle[i] == haystack[j - k])) {
                i--;
                k++;
            }
            if (i == 0) {
                // print "match in second half";
                // Scan for matches in first half.
                i = suffix;
                size k = needle_len - suffix - 1;
                size limit = needle_len;
                // print "i %v; k %v; limit %v" % i, k, limit;
                while (i < limit && needle[i] == haystack[j - k]) {
                    // print "loop";
                    i++;
                    k--;
                }
                if (i >= limit) {
                    // print "match in first half";
                    return j - needle_len + 1;
                }
                // print "no match in first half";
                j -= period;
            } else {
                // panic("unimplemented");
                // j -= i - suffix + 1;
                j -= suffix - i;
            }
        }
    }
    return -1;
}