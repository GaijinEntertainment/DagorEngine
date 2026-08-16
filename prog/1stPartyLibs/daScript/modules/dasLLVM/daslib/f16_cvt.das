options gen2

module f16_cvt shared public

// IEEE-754 binary16 <-> binary32 conversion, exposed as plain daslang functions with exact
// bit-twiddle bodies. On a JIT target with hardware half support (any aarch64 — scalar fcvt is
// ARMv8.0 baseline — or x86-64 with F16C) the LLVM JIT recognizes the calls BY NAME (see the
// f16_cvt table in llvm_jit_intrin.das) and emits half-precision convert IR in place of the call
// (fpext/fptrunc through the half type -> fcvtl/fcvtn / vcvtph2ps/vcvtps2ph in vectorized loops).
// Everywhere else — interpreter, AOT, non-F16C x86 — the daslang body below runs verbatim.
//
// The one observable hardware/software divergence is NaN handling: hardware fpext quietens a
// signaling NaN (sets the quiet bit, keeps the payload) where the f16_to_f32 body preserves the
// bits exactly, and hardware fptrunc keeps a truncated NaN payload where f32_to_f16 flattens every
// NaN to sign|0x7E00. Both stay NaN, so consumers must not depend on NaN payload bits.

//! Convert an IEEE-754 binary16 (held in the low 16 bits) to fp32.
def f16_to_f32(h : uint) : float {
    let exp = (h >> 10u) & 0x1Fu
    let mant = h & 0x3FFu
    let neg = (h & 0x8000u) != 0u
    if (exp == 0u) {  // zero or subnormal: value = mant * 2^-24, exactly representable in fp32
        let v = float(mant) * 5.9604644775390625e-8
        return neg ? -v : v
    }
    let sign = (h & 0x8000u) << 16u
    var bits : uint
    if (exp == 0x1Fu) {  // inf / nan
        bits = sign | 0x7F800000u | (mant << 13u)
    } else {  // normal: rebias exponent 15 -> 127 (+112), widen mantissa 10 -> 23 bits
        bits = sign | ((exp + 112u) << 23u) | (mant << 13u)
    }
    return unsafe(reinterpret<float>(bits))
}

//! Convert fp32 to IEEE-754 binary16 bits, round-to-nearest-even (matches ARM fcvt / x86
//! F16C hardware conversion — what ggml's FP32_TO_FP16 compiles to on both oracles).
def f32_to_f16(f : float) : uint {
    let bits = unsafe(reinterpret<uint>(f))
    let sign = (bits >> 16u) & 0x8000u
    let absb = bits & 0x7FFFFFFFu
    if (absb > 0x7F800000u) {          // nan -> quiet nan
        return sign | 0x7E00u
    }
    if (absb >= 0x47800000u) {         // inf, or >= 65536 (overflows past f16 max)
        return sign | 0x7C00u
    }
    if (absb >= 0x38800000u) {         // normal f16 range (>= 2^-14): rebias, RNE off 13 bits
        let v = absb - 0x38000000u     // exponent field now f16-biased, aligned at bit 23
        return sign | ((v + 0xFFFu + ((v >> 13u) & 1u)) >> 13u)  // mantissa carry -> inf is correct IEEE overflow
    }
    let e = absb >> 23u                // subnormal or zero: h = round(m·2^(e-126)) in 2^-24 units
    let m = (absb & 0x7FFFFFu) | 0x800000u
    if (e < 102u || (e == 102u && m == 0x800000u)) {  // below 2^-25, or exactly 2^-25 (tie to even 0)
        return sign
    }
    let sh = 126u - e                  // e in [102,112] -> sh in [14,24]
    let base = m >> sh
    let rem = m & ((1u << sh) - 1u)
    let half = 1u << (sh - 1u)
    let up = rem > half || (rem == half && (base & 1u) == 1u)
    return sign | (up ? base + 1u : base)  // carry into 0x0400 = smallest normal, also correct
}
