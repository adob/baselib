#pragma once

#include "lib/strconv/ftoa.h"
#include "lib/types.h"

namespace lib::strconv {
    namespace detail {

        // ryu_ftoa_shortest formats mant*2^exp with prec decimal digits.
        void ryu_ftoa_shortest(DecimalSlice &d, uint64 mant, int exp, FloatInfo &flt) {
            if (mant == 0) {
                d.nd = 0;
                d.dp = 0;
                return;
            }
            // If input is an exact integer with fewer bits than the mantissa,
            // the previous and next integer are not admissible representations.
            if exp <= 0 && bits.TrailingZeros64(mant) >= -exp {
                mant >>= uint(-exp)
                ryuDigits(d, mant, mant, mant, true, false)
                return
            }
            ml, mc, mu, e2 := computeBounds(mant, exp, flt)
            if e2 == 0 {
                ryuDigits(d, ml, mc, mu, true, false)
                return
            }
            // Find 10^q *larger* than 2^-e2
            q := mulByLog2Log10(-e2) + 1

            // We are going to multiply by 10^q using 128-bit arithmetic.
            // The exponent is the same for all 3 numbers.
            var dl, dc, du uint64
            var dl0, dc0, du0 bool
            if flt == &float32info {
                var dl32, dc32, du32 uint32
                dl32, _, dl0 = mult64bitPow10(uint32(ml), e2, q)
                dc32, _, dc0 = mult64bitPow10(uint32(mc), e2, q)
                du32, e2, du0 = mult64bitPow10(uint32(mu), e2, q)
                dl, dc, du = uint64(dl32), uint64(dc32), uint64(du32)
            } else {
                dl, _, dl0 = mult128bitPow10(ml, e2, q)
                dc, _, dc0 = mult128bitPow10(mc, e2, q)
                du, e2, du0 = mult128bitPow10(mu, e2, q)
            }
            if e2 >= 0 {
                panic("not enough significant bits after mult128bitPow10")
            }
            // Is it an exact computation?
            if q > 55 {
                // Large positive powers of ten are not exact
                dl0, dc0, du0 = false, false, false
            }
            if q < 0 && q >= -24 {
                // Division by a power of ten may be exact.
                // (note that 5^25 is a 59-bit number so division by 5^25 is never exact).
                if divisibleByPower5(ml, -q) {
                    dl0 = true
                }
                if divisibleByPower5(mc, -q) {
                    dc0 = true
                }
                if divisibleByPower5(mu, -q) {
                    du0 = true
                }
            }
            // Express the results (dl, dc, du)*2^e2 as integers.
            // Extra bits must be removed and rounding hints computed.
            extra := uint(-e2)
            extraMask := uint64(1<<extra - 1)
            // Now compute the floored, integral base 10 mantissas.
            dl, fracl := dl>>extra, dl&extraMask
            dc, fracc := dc>>extra, dc&extraMask
            du, fracu := du>>extra, du&extraMask
            // Is it allowed to use 'du' as a result?
            // It is always allowed when it is truncated, but also
            // if it is exact and the original binary mantissa is even
            // When disallowed, we can subtract 1.
            uok := !du0 || fracu > 0
            if du0 && fracu == 0 {
                uok = mant&1 == 0
            }
            if !uok {
                du--
            }
            // Is 'dc' the correctly rounded base 10 mantissa?
            // The correct rounding might be dc+1
            cup := false // don't round up.
            if dc0 {
                // If we computed an exact product, the half integer
                // should round to next (even) integer if 'dc' is odd.
                cup = fracc > 1<<(extra-1) ||
                    (fracc == 1<<(extra-1) && dc&1 == 1)
            } else {
                // otherwise, the result is a lower truncation of the ideal
                // result.
                cup = fracc>>(extra-1) == 1
            }
            // Is 'dl' an allowed representation?
            // Only if it is an exact value, and if the original binary mantissa
            // was even.
            lok := dl0 && fracl == 0 && (mant&1 == 0)
            if !lok {
                dl++
            }
            // We need to remember whether the trimmed digits of 'dc' are zero.
            c0 := dc0 && fracc == 0
            // render digits
            ryuDigits(d, dl, dc, du, c0, cup)
            d.dp -= q
        }

        // mulByLog2Log10 returns math.Floor(x * log(2)/log(10)) for an integer x in
        // the range -1600 <= x && x <= +1600.
        //
        // The range restriction lets us work in faster integer arithmetic instead of
        // slower floating point arithmetic. Correctness is verified by unit tests.
        func mulByLog2Log10(x int) int {
            // log(2)/log(10) ≈ 0.30102999566 ≈ 78913 / 2^18
            return (x * 78913) >> 18
        }

        // mulByLog10Log2 returns math.Floor(x * log(10)/log(2)) for an integer x in
        // the range -500 <= x && x <= +500.
        //
        // The range restriction lets us work in faster integer arithmetic instead of
        // slower floating point arithmetic. Correctness is verified by unit tests.
        func mulByLog10Log2(x int) int {
            // log(10)/log(2) ≈ 3.32192809489 ≈ 108853 / 2^15
            return (x * 108853) >> 15
        }
    }
}