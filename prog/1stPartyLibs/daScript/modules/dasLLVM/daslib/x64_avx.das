options gen2

module x64_avx shared public

// x86-64 SIMD intrinsics exposed as plain daslang functions, each with a portable fallback body —
// the x64 mirror of aarch64_neon.das (read that header for the recognition contract).
//
// Every function here computes the correct result on ANY target via its daslang body — so the
// interpreter, AOT, and non-x64 JIT builds all just run that body. On an x86-64 JIT target WITH
// AVX2 the LLVM JIT recognizes the call BY NAME (see g_x64_intrin_lookup in llvm_jit_intrin.das,
// gated on g_target_x64_avx2 — cpuid truth, since AVX2 is not universal on x86-64) and emits the
// hardware instruction sequence in place of the call. Nothing to annotate, nothing to require from
// the llvm module — recognition is purely name-based at JIT codegen time, so the fallback runs
// verbatim everywhere the intrinsic isn't emitted (including AVX2-less x64).

//! Signed dot-product (the SDOT shape on x86): load 16 int8 from each of `w` and `x` (16-wide
//! unaligned loads), multiply elementwise, and accumulate consecutive groups of 4 products into the
//! 4 lanes of `acc` — lane j += sum over e in 0..3 of w[4j+e]*x[4j+e], exact int32, with NO
//! horizontal reduce. Pre-VNNI x86 has no s8×s8 dot, so the x86-64 JIT lowers this via the standard
//! exact sign trick — dot(w, x) == dot(|w| as u8, sign(w) applied to x) — as PABSB/PSIGNB +
//! PMADDUBSW + PMADDWD (ggml's pre-VNNI path); on every other target it is the scalar loop below.
//! EXACTNESS PRECONDITION: `x` values must lie in [-127, 127] (PSIGNB of -128 wraps, and a
//! -128×-128 pair sum would saturate PMADDUBSW). The Q8 quantizer guarantees this by construction
//! (d = amax/127) for every activation it emits; `w` may be ANY int8 including -128 (|−128| = 128
//! fits u8, pair sums stay ≤ 32512 < 32767). Pointers are passed pre-offset by the caller.
def dot4(acc : int4; w : int8 const?; x : int8 const?) : int4 {
    var s0 = 0
    var s1 = 0
    var s2 = 0
    var s3 = 0
    unsafe {
        for (e in range(4)) {
            s0 += int(w[e]) * int(x[e])
            s1 += int(w[4 + e]) * int(x[4 + e])
            s2 += int(w[8 + e]) * int(x[8 + e])
            s3 += int(w[12 + e]) * int(x[12 + e])
        }
    }
    return acc + int4(s0, s1, s2, s3)
}

//! 256-bit dot4: load 32 int8 from each of `w` and `x` (one full Q8_0 block) and accumulate into the
//! 4 lanes of `acc` — lane j += dot of quad j PLUS dot of quad j+4, i.e. exactly
//! dot4(dot4(acc, w, x), w + 16, x + 16), lane for lane. On x86-64 the JIT emits the same sign-trick
//! chain as dot4 on 256-bit registers (VPABSB/VPSIGNB + VPMADDUBSW + VPMADDWD, ymm) and folds the
//! v8i32 result to int4 by adding the high 128-bit half onto the low (VEXTRACTI128 + VPADDD) — that
//! fold IS the quad j + quad j+4 grouping, so the fallback below and the hardware path agree exactly.
//! Same EXACTNESS PRECONDITION as dot4: `x` values in [-127, 127] (the Q8 quantizer's range); `w` may
//! be any int8 including -128. Pointers are passed pre-offset by the caller.
def dot32(acc : int4; w : int8 const?; x : int8 const?) : int4 {
    var s0 = 0
    var s1 = 0
    var s2 = 0
    var s3 = 0
    unsafe {
        for (e in range(4)) {
            s0 += int(w[e]) * int(x[e]) + int(w[16 + e]) * int(x[16 + e])
            s1 += int(w[4 + e]) * int(x[4 + e]) + int(w[20 + e]) * int(x[20 + e])
            s2 += int(w[8 + e]) * int(x[8 + e]) + int(w[24 + e]) * int(x[24 + e])
            s3 += int(w[12 + e]) * int(x[12 + e]) + int(w[28 + e]) * int(x[28 + e])
        }
    }
    return acc + int4(s0, s1, s2, s3)
}

//! Float-accumulating dot32 — the per-block GEMV epilogue fused into the intrinsic:
//! acc + float4(dot32(int4(0), w, x)) * s, lane for lane. On x86-64 the JIT emits dot32's exact
//! sign-trick chain and fold, then VCVTDQ2PS on the int4 partials (each bounded by 32*127*128 <
//! 2^24, so the conversion is exact), a broadcast multiply by `s`, and a vector add into `acc` —
//! replacing the kernel-side scalar hsum + scale chain (~10 µops per block) with 3 vector ops and
//! deferring the horizontal reduce to ONE hsum per row. Lane math matches this fallback exactly
//! under strict FP; under `options _jit_fast_math` LLVM may contract the mul+add into an FMA —
//! the same laxity every float kernel gets, never on tests. Same EXACTNESS PRECONDITION as dot32
//! on `x` (values in [-127, 127]); `s` is the folded per-block scale (weight scale × token scale).
def dot32_ps(acc : float4; w : int8 const?; x : int8 const?; s : float) : float4 {
    let d = dot32(int4(0, 0, 0, 0), w, x)
    return acc + float4(d) * float4(s, s, s, s)
}

//! 8-wide float-accumulating dot32 — llama.cpp's vec_dot per-block shape. `acc` points at 8 floats
//! (a caller-local float[8]); lane j accumulates the dot of byte quad j: acc[j] += float(sum over e
//! in 0..3 of w[4j+e]*x[4j+e]) * s. On x86-64 the JIT emits dot32's exact sign-trick chain WITHOUT
//! the high/low fold — the 8 int32 quads convert straight to float (VCVTDQ2PS ymm, exact: each quad
//! bounded < 2^24) and multiply-accumulate into the 8-lane accumulator as one load-op-store that
//! LICM promotes into a ymm register across a block loop. No per-block fold or hsum; the caller does
//! ONE horizontal reduce per row. Lane for lane this fallback under strict FP; `options
//! _jit_fast_math` may contract the mul+add into VFMADD — the same laxity every float kernel gets,
//! never on tests. Same EXACTNESS PRECONDITION as dot32 on `x` (values in [-127, 127]); `s` is the
//! folded per-block scale (weight scale × token scale).
def dot32_acc8(var acc : float?; w : int8 const?; x : int8 const?; s : float) {
    unsafe {
        for (j in range(8)) {
            var q = 0
            for (e in range(4)) {
                q += int(w[j * 4 + e]) * int(x[j * 4 + e])
            }
            acc[j] += float(q) * s
        }
    }
}

//! MXFP4-dequantizing dot32: one MXFP4 block's 16 packed nibble bytes expand through a 16-entry
//! signed LUT — element j (j in 0..15) = lut[wn[j] & 15], element 16+j = lut[wn[j] >> 4] — and the
//! 32 dequantized int8 dot against 32 int8 of `x` into the 4 lanes of `acc` with dot32's exact
//! lane grouping (lane j += dot of element quad j PLUS quad j+4, which pairs each nibble byte's lo
//! and hi products in one lane). On x86-64 the JIT loads the 16 nibble bytes once, selects lo/hi
//! nibbles (VPAND / VPSRLW+VPAND), dequantizes both halves with ONE ymm VPSHUFB of the LUT
//! broadcast to both 128-bit lanes (indices are 0..15 with bit 7 clear, so PSHUFB's zeroing path
//! never fires and its per-lane semantics match the [lo | hi] layout), then runs dot32's exact
//! sign-trick chain and fold on the result. Integer arithmetic throughout, so hardware and this
//! fallback agree bit for bit. Saturation is structurally impossible (|lut| <= 12, the doubled
//! e2m1 table, bounds PMADDUBSW pair sums by 12*128*2 = 3072 << 32767), but the sign trick's
//! EXACTNESS PRECONDITION on `x` is unchanged from dot32: values in [-127, 127] — PSIGNB of -128
//! wraps to -128 when the LUT weight is negative (off by 2*|lut|*128 per such pair). The Q8
//! quantizer guarantees the range by construction (d = amax/127), exactly as on the Q8 path.
//! Pointers are passed pre-offset by the caller.
def dot32_mx4(acc : int4; lut : int8 const?; wn : uint8 const?; x : int8 const?) : int4 {
    var s0 = 0
    var s1 = 0
    var s2 = 0
    var s3 = 0
    unsafe {
        for (e in range(4)) {
            s0 += int(lut[int(wn[e]) & 15]) * int(x[e]) + int(lut[int(wn[e]) >> 4]) * int(x[16 + e])
            s1 += int(lut[int(wn[4 + e]) & 15]) * int(x[4 + e]) + int(lut[int(wn[4 + e]) >> 4]) * int(x[20 + e])
            s2 += int(lut[int(wn[8 + e]) & 15]) * int(x[8 + e]) + int(lut[int(wn[8 + e]) >> 4]) * int(x[24 + e])
            s3 += int(lut[int(wn[12 + e]) & 15]) * int(x[12 + e]) + int(lut[int(wn[12 + e]) >> 4]) * int(x[28 + e])
        }
    }
    return acc + int4(s0, s1, s2, s3)
}

//! MXFP4 block dequant to memory: one block's 16 packed nibble bytes expand through the 16-entry
//! signed LUT — dst[j] = lut[wn[j] & 15], dst[16+j] = lut[wn[j] >> 4] — writing the 32 int8 a Q8
//! expansion of the block would produce (dot32_mx4's dequant half, stored instead of dotted). On
//! x86-64 the JIT emits the identical in-register sequence (nibble select + ONE ymm VPSHUFB of the
//! LUT broadcast) followed by a single 32-byte store — ~8x fewer ops than this scalar loop, which
//! is the interp / AOT / non-x64 fallback. Byte-for-byte identical either way (pure integer LUT).
//! Pointers are passed pre-offset by the caller; dst needs no alignment.
def mx4_dequant32(lut : int8 const?; wn : uint8 const?; var dst : int8?) {
    unsafe {
        for (j in range(16)) {
            let b = int(wn[j])
            dst[j] = lut[b & 15]
            dst[16 + j] = lut[b >> 4]
        }
    }
}

// ===== AVX kernel matrix tiers (VNNI / AVX-512) =====
// Recognition is per-tier: each name below sits in its own JIT lookup table gated on the matching
// g_target_x64_* flag (cpuid truth — see llvm_jit_common.das), so on a box without the tier the
// call compiles its daslang body instead. Every body here DELEGATES to the established intrinsic
// of the same math (dot32 / dot32_acc8 / mx4_dequant32) — exactness by construction, and on an
// AVX2-only box the delegate is itself recognized, so a matrix kernel degrades to the proven ymm
// sign-trick path rather than to scalar code.

//! dot32 with the VNNI dot step: same signature, same exact int4 result, lane for lane. On x86-64
//! with 256-bit VNNI (AVX-VNNI, or AVX512-VNNI+VL) the JIT replaces dot32's VPMADDUBSW+VPMADDWD
//! pair with ONE VPDPBUSD (u8×s8 quad dot into i32 — no 16-bit intermediate at all, so PMADDUBSW's
//! saturation headroom analysis becomes moot); the VPABSB/VPSIGNB sign trick and the high/low fold
//! are unchanged. EXACTNESS PRECONDITION stays dot32's: `x` values in [-127, 127] (PSIGNB of -128
//! wraps); the Q8 quantizer guarantees the range.
def dot32_vnni(acc : int4; w : int8 const?; x : int8 const?) : int4 {
    return dot32(acc, w, x)
}

//! dot32_acc8 with the VNNI dot step (see dot32_vnni): the 8 int32 quads come from ONE VPDPBUSD
//! instead of the VPMADDUBSW+VPMADDWD pair, then convert and multiply-accumulate into the caller's
//! 8-float memory accumulator exactly as dot32_acc8 does. Same exactness precondition and the same
//! _jit_fast_math FMA-contraction laxity.
def dot32_acc8_vnni(var acc : float?; w : int8 const?; x : int8 const?; s : float) {
    dot32_acc8(acc, w, x, s)
}

//! dot32_mx4 with the VNNI dot step (see dot32_vnni): the ymm PSHUFB LUT dequant is unchanged;
//! the dequantized block dots against `x` via ONE VPDPBUSD instead of the VPMADDUBSW+VPMADDWD
//! pair. Integer arithmetic throughout — bit-identical to dot32_mx4 and to this fallback.
def dot32_mx4_vnni(acc : int4; lut : int8 const?; wn : uint8 const?; x : int8 const?) : int4 {
    return dot32_mx4(acc, lut, wn, x)
}

//! 512-bit dot32_acc8: TWO consecutive Q8 blocks (64 int8 per operand) per call, accumulating into
//! a caller-local float[16] — lane j (j in 0..7) += float(dot of block-0 quad j) * s0, lane 8+j +=
//! float(dot of block-1 quad j) * s1. On x86-64 with AVX-512BW the JIT emits the sign trick on zmm
//! (VPABSB; VPSIGNB has no 512-bit form, so sign application is compare+negate+select — same
//! values, w==0 lanes stay un-zeroed but multiply against |w|=0 anyway) + VPMADDUBSW/VPMADDWD zmm,
//! then VCVTDQ2PS and one multiply-accumulate against the [s0 x8 | s1 x8] scale vector, promoted
//! into a zmm register across a block loop by LICM. Same exactness precondition on `x` as dot32;
//! same _jit_fast_math contraction laxity. Caller guarantees TWO full blocks (n%64 tails use
//! dot32_acc8 on the same accumulator's low half).
def dot64_acc16(var acc : float?; w : int8 const?; x : int8 const?; s0, s1 : float) {
    unsafe {
        dot32_acc8(acc, w, x, s0)
        dot32_acc8(acc + 8, w + 32, x + 32, s1)
    }
}

//! dot64_acc16 with the VNNI dot step: the zmm VPMADDUBSW+VPMADDWD pair becomes ONE VPDPBUSD
//! (AVX512-VNNI), everything else as dot64_acc16 — including this fallback, which delegates to it
//! (so on an AVX512BW-without-VNNI box the body itself JITs to the zmm multiply-add chain).
def dot64_acc16_vnni(var acc : float?; w : int8 const?; x : int8 const?; s0, s1 : float) {
    dot64_acc16(acc, w, x, s0, s1)
}

//! 512-bit mx4_dequant32: TWO consecutive MXFP4 blocks (32 nibble bytes) expand through the LUT in
//! one shot — dst[0..31] = block 0's [lo | hi], dst[32..63] = block 1's. On x86-64 with AVX-512BW
//! the JIT selects both blocks' nibbles, concatenates the four 16-byte index groups to one zmm,
//! and looks them up with ONE VPSHUFB zmm against the LUT broadcast to all four 128-bit lanes,
//! then a single 64-byte store. Byte-for-byte identical to two mx4_dequant32 calls (pure integer
//! LUT). Pointers pre-offset by the caller; dst needs no alignment.
def mx4_dequant64(lut : int8 const?; wn : uint8 const?; var dst : int8?) {
    unsafe {
        mx4_dequant32(lut, wn, dst)
        mx4_dequant32(lut, wn + 16, dst + 32)
    }
}
