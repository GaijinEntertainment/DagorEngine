#ifndef FALLBACK_BUILTINS_H
#define FALLBACK_BUILTINS_H

#if defined(_MSC_VER) && !defined(__clang__)
#if defined(_M_IX86) || defined(_M_AMD64) || defined(_M_IA64) ||  defined(_M_ARM) || defined(_M_ARM64)

#include <intrin.h>
#ifdef X86_FEATURES
#  include "arch/x86/x86_features.h"
#endif

/* This is not a general purpose replacement for __builtin_ctz. The function expects that value is != 0
 * Because of that assumption trailing_zero is not initialized and the return value of _BitScanForward is not checked
 */
static __forceinline unsigned long __builtin_ctz(uint32_t value) {
#ifdef X86_FEATURES
#  ifndef X86_NOCHECK_TZCNT
    if (x86_cpu_has_tzcnt)
#  endif
        return _tzcnt_u32(value);
#endif
    unsigned long trailing_zero;
    _BitScanForward(&trailing_zero, value);
    return trailing_zero;
}
#define HAVE_BUILTIN_CTZ

#ifdef _M_AMD64
/* This is not a general purpose replacement for __builtin_ctzll. The function expects that value is != 0
 * Because of that assumption trailing_zero is not initialized and the return value of _BitScanForward64 is not checked
 */
static __forceinline unsigned long long __builtin_ctzll(uint64_t value) {
#ifdef X86_FEATURES
#  ifndef X86_NOCHECK_TZCNT
    if (x86_cpu_has_tzcnt)
#  endif
        return _tzcnt_u64(value);
#endif
    unsigned long trailing_zero;
    _BitScanForward64(&trailing_zero, value);
    return trailing_zero;
}
#define HAVE_BUILTIN_CTZLL
#endif // Microsoft AMD64

#endif // Microsoft AMD64/IA64/x86/ARM/ARM64 test
#endif // _MSC_VER & !clang

/* Unfortunately GCC didn't support these things until version 10 */
#ifdef __AVX2__
#include <immintrin.h>

#if (!defined(__clang__) && defined(__GNUC__) && __GNUC__ < 10 && !defined(__e2k__))
static inline __m256i _mm256_zextsi128_si256(__m128i a) {
    __m128i r;
    __asm__ volatile ("vmovdqa %1,%0" : "=x" (r) : "x" (a));
    return _mm256_castsi128_si256(r);
}

#ifdef __AVX512F__
static inline __m512i _mm512_zextsi128_si512(__m128i a) {
    __m128i r;
    __asm__ volatile ("vmovdqa %1,%0" : "=x" (r) : "x" (a));
    return _mm512_castsi128_si512(r);
}
#endif // __AVX512F__
#endif // gcc version 10 test

#endif // __AVX2__

#if defined(ARM_NEON_ADLER32) && !defined(__aarch64__)
/* Compatibility shim for the _high family of functions */
#define vmull_high_u8(a, b) vmull_u8(vget_high_u8(a), vget_high_u8(b))
#define vmlal_high_u8(a, b, c) vmlal_u8(a, vget_high_u8(b), vget_high_u8(c))
#define vmlal_high_u16(a, b, c) vmlal_u16(a, vget_high_u16(b), vget_high_u16(c))
#define vaddw_high_u8(a, b) vaddw_u8(a, vget_high_u8(b))
#endif

#ifdef ARM_NEON_SLIDEHASH

#define vqsubq_u16_x4_x1(out, a, b) do { \
    out.val[0] = vqsubq_u16(a.val[0], b); \
    out.val[1] = vqsubq_u16(a.val[1], b); \
    out.val[2] = vqsubq_u16(a.val[2], b); \
    out.val[3] = vqsubq_u16(a.val[3], b); \
} while (0)

/* Have to check for hard float ABI on GCC/clang, but not
 * on MSVC (we don't compile for the soft float ABI on windows)
 */
#if !defined(ARM_NEON_HASLD4) && (defined(__ARM_FP) || defined(_MSC_VER))

#if defined(_MSC_VER) && (defined(_M_ARM64) || defined(_M_ARM64EC)) && !defined(__clang__)
#  include <arm64_neon.h>
#else
#  include <arm_neon.h>
#endif

static inline uint16x8x4_t vld1q_u16_x4(uint16_t *a) {
    uint16x8x4_t ret = (uint16x8x4_t) {{
                          vld1q_u16(a),
                          vld1q_u16(a+8),
                          vld1q_u16(a+16),
                          vld1q_u16(a+24)}};
    return ret;
}

static inline uint8x16x4_t vld1q_u8_x4(uint8_t *a) {
    uint8x16x4_t ret = (uint8x16x4_t) {{
                          vld1q_u8(a),
                          vld1q_u8(a+16),
                          vld1q_u8(a+32),
                          vld1q_u8(a+48)}};
    return ret;
}

static inline void vst1q_u16_x4(uint16_t *p, uint16x8x4_t a) {
    vst1q_u16(p, a.val[0]);
    vst1q_u16(p + 8, a.val[1]);
    vst1q_u16(p + 16, a.val[2]);
    vst1q_u16(p + 24, a.val[3]);
}
#endif // HASLD4 check and hard float
#endif // ARM_NEON_SLIDEHASH

#endif // include guard FALLBACK_BUILTINS_H
