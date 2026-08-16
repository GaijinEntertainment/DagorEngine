options gen2

require dastest/testing_boost
require ../../modules/dasLLVM/daslib/f16_cvt.das

// f16_cvt intrinsic emission: under -jit on aarch64 / x64+F16C the module's f16_to_f32 /
// f32_to_f16 lower to hardware half converts (fpext/fptrunc through half), while the ref_*
// copies below — same bit-twiddle, different module, so the name-keyed lookup never matches —
// compile as-is; integer ops are exact in every tier, so hardware-vs-reference isolates the
// intrinsic. Interp / no-F16C lanes run the identical body on both sides (trivially green).
// NaN inputs compare by predicate only: hardware quietens sNaN / keeps payloads where the
// reference flattens f32->f16 NaN to sign|0x7E00 — both stay NaN, payloads are not contractual.

def ref_f16_to_f32(h : uint) : float {
    let exp = (h >> 10u) & 0x1Fu
    let mant = h & 0x3FFu
    let neg = (h & 0x8000u) != 0u
    if (exp == 0u) {
        let v = float(mant) * 5.9604644775390625e-8
        return neg ? -v : v
    }
    let sign = (h & 0x8000u) << 16u
    var bits : uint
    if (exp == 0x1Fu) {
        bits = sign | 0x7F800000u | (mant << 13u)
    } else {
        bits = sign | ((exp + 112u) << 23u) | (mant << 13u)
    }
    return unsafe(reinterpret<float>(bits))
}

def ref_f32_to_f16(f : float) : uint {
    let bits = unsafe(reinterpret<uint>(f))
    let sign = (bits >> 16u) & 0x8000u
    let absb = bits & 0x7FFFFFFFu
    if (absb > 0x7F800000u) {
        return sign | 0x7E00u
    }
    if (absb >= 0x47800000u) {
        return sign | 0x7C00u
    }
    if (absb >= 0x38800000u) {
        let v = absb - 0x38000000u
        return sign | ((v + 0xFFFu + ((v >> 13u) & 1u)) >> 13u)
    }
    let e = absb >> 23u
    let m = (absb & 0x7FFFFFu) | 0x800000u
    if (e < 102u || (e == 102u && m == 0x800000u)) {
        return sign
    }
    let sh = 126u - e
    let base = m >> sh
    let rem = m & ((1u << sh) - 1u)
    let half = 1u << (sh - 1u)
    let up = rem > half || (rem == half && (base & 1u) == 1u)
    return sign | (up ? base + 1u : base)
}

def is_f16_nan(h : uint) : bool => ((h >> 10u) & 0x1Fu) == 0x1Fu && (h & 0x3FFu) != 0u

def is_f32_nan_bits(v : float) : bool {
    let bits = unsafe(reinterpret<uint>(v))
    return (bits & 0x7F800000u) == 0x7F800000u && (bits & 0x007FFFFFu) != 0u
}

def f32_bits(v : float) : uint => unsafe(reinterpret<uint>(v))

[test]
def test_f16_to_f32_exhaustive(t : T?) {
    t |> run("all 65536 f16 patterns: hardware == reference bitwise (NaNs predicate-only)") @(t : T?) {
        var bad = 0
        var bad_nan = 0
        for (h in range(65536)) {
            let hu = uint(h)
            let hw = f16_to_f32(hu)
            if (is_f16_nan(hu)) {
                if (!is_f32_nan_bits(hw)) {
                    bad_nan++
                }
            } elif (f32_bits(hw) != f32_bits(ref_f16_to_f32(hu))) {
                bad++
            }
        }
        t |> equal(bad, 0, "non-nan f16->f32 is bit-exact vs the reference")
        t |> equal(bad_nan, 0, "every f16 nan pattern converts to an f32 nan")
    }
}

[test]
def test_f32_to_f16_sweep(t : T?) {
    t |> run("65536 upper halves x round/sticky low patterns: hardware == reference") @(t : T?) {
        // hi exhausts sign+exponent+kept-mantissa bits; lo exercises the round bit (0x1000),
        // sticky combinations, and the tie cases of the RNE decision at the normal-range
        // rounding position (bit 12) and across the moving subnormal shift.
        let lows = fixed_array(0x0000u, 0x1000u, 0x1001u, 0x0FFFu, 0x2000u, 0x8000u, 0xFFFFu)
        var bad = 0
        var bad_nan = 0
        for (hi in range(65536)) {
            for (lo in lows) {
                let bits = (uint(hi) << 16u) | lo
                let f = unsafe(reinterpret<float>(bits))
                let hw = f32_to_f16(f)
                if (is_f32_nan_bits(f)) {
                    if (!is_f16_nan(hw)) {
                        bad_nan++
                    }
                } elif (hw != ref_f32_to_f16(f)) {
                    bad++
                }
            }
        }
        t |> equal(bad, 0, "non-nan f32->f16 matches the reference RNE bit-twiddle")
        t |> equal(bad_nan, 0, "every f32 nan input converts to an f16 nan")
    }
}

[test]
def test_f16_round_trip(t : T?) {
    t |> run("f32_to_f16(f16_to_f32(h)) == h for every non-nan pattern") @(t : T?) {
        var bad = 0
        for (h in range(65536)) {
            let hu = uint(h)
            if (is_f16_nan(hu)) {
                continue
            }
            if (f32_to_f16(f16_to_f32(hu)) != hu) {
                bad++
            }
        }
        t |> equal(bad, 0, "all non-nan f16 patterns round-trip")
    }
}

[test]
def test_f32_to_f16_boundaries(t : T?) {
    t |> run("round-to-nearest-even boundaries match numpy float16") @(t : T?) {
        t |> equal(f32_to_f16(0.1), 0x2e66u)
        t |> equal(f32_to_f16(-1.5), 0xbe00u)
        t |> equal(f32_to_f16(65504.0), 0x7bffu)   // f16 max
        t |> equal(f32_to_f16(65519.9), 0x7bffu)   // just under the overflow tie
        t |> equal(f32_to_f16(65520.0), 0x7c00u)   // IEEE overflow to inf
        t |> equal(f32_to_f16(2.9802322387695312e-8), 0x0u)  // exactly 2^-25: tie to even 0
        t |> equal(f32_to_f16(2.9805302e-8), 0x1u)           // a hair above: min subnormal
        t |> equal(f32_to_f16(10.0), 0x4900u)
        t |> equal(f32_bits(f16_to_f32(0x8000u)), 0x80000000u)  // -0 survives f16->f32
        t |> equal(f32_to_f16(unsafe(reinterpret<float>(0x80000000u))), 0x8000u)  // and back
        t |> equal(f32_to_f16(unsafe(reinterpret<float>(0x7F800000u))), 0x7C00u)  // +inf
        t |> equal(f32_bits(f16_to_f32(0xFC00u)), 0xFF800000u)                    // -inf
        let h = f32_to_f16(unsafe(reinterpret<float>(0x7FC00001u)))  // quiet nan in -> nan out
        t |> success(is_f16_nan(h), "nan input yields an f16 nan")
    }
}
