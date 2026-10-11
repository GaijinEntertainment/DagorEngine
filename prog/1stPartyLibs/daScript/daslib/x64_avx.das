options gen2

module x64_avx shared public

// x86-64 SIMD intrinsics exposed as plain daslang functions, each with a portable fallback body —
// the x64 mirror of aarch64_neon.das (read that header for the recognition contract).
//
// - Every body computes the correct result on ANY target; interp, AOT and non-x64 JIT run it verbatim.
// - On x86-64 the JIT recognizes the call BY NAME (g_x64_intrin_lookup in llvm_jit_intrin.das, gated
//   on the g_target_x64_* cpuid flags) and emits hardware instead. Nothing to annotate or require.
// - Pre-VNNI x86 has no s8xs8 dot, so those paths lower via the exact sign trick
//   dot(w, x) == dot(|w| as u8, sign(w) applied to x): PABSB/PSIGNB + PMADDUBSW + PMADDWD.
// - EXACTNESS PRECONDITION of every sign-trick path: `x` values in [-127, 127], since PSIGNB of -128
//   wraps. The Q8 quantizer guarantees it by construction (d = amax/127); `w` may be any int8.
// - Float lanes match the fallback under strict FP; `options _jit_fast_math` may contract mul+add
//   into an FMA — the laxity every float kernel gets, never on tests.
// - Pointer arguments are passed pre-offset by the caller; stores need no alignment.

//! Signed dot-product (the SDOT shape on x86): 16 int8 from each of `w` and `x`, multiplied
//! elementwise and accumulated in groups of 4 — lane j += sum over e in 0..3 of w[4j+e]*x[4j+e],
//! exact int32, no horizontal reduce. x86-64 lowers it via the header's sign trick (ggml's path).
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

//! 256-bit dot4: 32 int8 per operand (one full Q8_0 block), lane j += dot of quad j PLUS dot of
//! quad j+4 — exactly dot4(dot4(acc, w, x), w + 16, x + 16). On x86-64 the ymm sign-trick chain
//! ends in VEXTRACTI128 + VPADDD, and that high/low fold IS the quad j + quad j+4 grouping.
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

//! Float-accumulating dot32 — the per-block GEMV epilogue fused in: acc + float4(dot32(0, w, x)) * s,
//! lane for lane. On x86-64 the int4 partials convert exactly (each < 2^24) and one broadcast
//! multiply-add replaces the scalar hsum + scale, deferring the reduce to ONE hsum per row.
def dot32_ps(acc : float4; w : int8 const?; x : int8 const?; s : float) : float4 {
    let d = dot32(int4(0, 0, 0, 0), w, x)
    return acc + float4(d) * float4(s, s, s, s)
}

//! 8-wide float-accumulating dot32 — llama.cpp's vec_dot per-block shape. `acc` points at 8 floats
//! (a caller-local float[8]); lane j += float(dot of byte quad j) * s. On x86-64 the 8 int32 quads
//! skip dot32's fold, so LICM keeps the accumulator in a ymm register and the caller hsums per row.
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

//! MXFP4-dequantizing dot32: element j = lut[wn[j] & 15], element 16+j = lut[wn[j] >> 4], the 32
//! dequantized int8 dotted against `x` with dot32's lane grouping — so each nibble byte's lo and hi
//! products land in one lane. On x86-64 both halves dequantize with ONE ymm VPSHUFB; integer math.
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

//! MXFP4 block dequant to memory: dst[j] = lut[wn[j] & 15], dst[16+j] = lut[wn[j] >> 4] — the 32
//! int8 a Q8 expansion of the block would produce (dot32_mx4's dequant half, stored not dotted).
//! On x86-64 it is nibble select + ONE ymm VPSHUFB + one 32-byte store; byte-for-byte identical.
def mx4_dequant32(lut : int8 const?; wn : uint8 const?; var dst : int8?) {
    unsafe {
        for (j in range(16)) {
            let b = int(wn[j])
            dst[j] = lut[b & 15]
            dst[16 + j] = lut[b >> 4]
        }
    }
}

//! dot32 with the VNNI dot step: same signature, same exact int4 result, lane for lane. With
//! 256-bit VNNI (AVX-VNNI, or AVX512-VNNI+VL) the VPMADDUBSW+VPMADDWD pair becomes ONE VPDPBUSD
//! (u8xs8 quad dot into i32, no 16-bit intermediate); sign trick and high/low fold are unchanged.
def dot32_vnni(acc : int4; w : int8 const?; x : int8 const?) : int4 {
    return dot32(acc, w, x)
}

//! dot32_acc8 with the VNNI dot step (see dot32_vnni): the 8 int32 quads come from ONE VPDPBUSD
//! instead of the VPMADDUBSW+VPMADDWD pair, then convert and multiply-accumulate into the caller's
//! 8-float memory accumulator exactly as dot32_acc8 does.
def dot32_acc8_vnni(var acc : float?; w : int8 const?; x : int8 const?; s : float) {
    dot32_acc8(acc, w, x, s)
}

//! dot32_mx4 with the VNNI dot step (see dot32_vnni): the ymm PSHUFB LUT dequant is unchanged;
//! the dequantized block dots against `x` via ONE VPDPBUSD instead of the VPMADDUBSW+VPMADDWD
//! pair. Integer arithmetic throughout — bit-identical to dot32_mx4 and to this fallback.
def dot32_mx4_vnni(acc : int4; lut : int8 const?; wn : uint8 const?; x : int8 const?) : int4 {
    return dot32_mx4(acc, lut, wn, x)
}

//! 512-bit dot32_acc8: TWO consecutive Q8 blocks (64 int8 per operand) into a caller-local
//! float[16] — lane j += float(block-0 quad j) * s0, lane 8+j += float(block-1 quad j) * s1.
//! Caller guarantees two full blocks; an n%64 tail uses dot32_acc8 on the accumulator's low half.
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

//! 512-bit mx4_dequant32: TWO consecutive MXFP4 blocks (32 nibble bytes) expand in one shot —
//! dst[0..31] = block 0's [lo | hi], dst[32..63] = block 1's. On AVX-512BW that is ONE zmm VPSHUFB
//! against the broadcast LUT plus a single 64-byte store; byte-for-byte two mx4_dequant32 calls.
def mx4_dequant64(lut : int8 const?; wn : uint8 const?; var dst : int8?) {
    unsafe {
        mx4_dequant32(lut, wn, dst)
        mx4_dequant32(lut, wn + 16, dst + 32)
    }
}
