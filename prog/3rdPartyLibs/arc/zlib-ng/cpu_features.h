/* cpu_features.h -- CPU architecture feature check
 * Copyright (C) 2017 Hans Kristian Rosbach
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#ifndef CPU_FEATURES_H_
#define CPU_FEATURES_H_

#include "crc32_fold.h"

#if defined(X86_FEATURES)
#  include "arch/x86/x86_features.h"
#  include "fallback_builtins.h"
#elif defined(ARM_FEATURES)
#  include "arch/arm/arm_features.h"
#elif defined(PPC_FEATURES) || defined(POWER_FEATURES)
#  include "arch/power/power_features.h"
#elif defined(S390_FEATURES)
#  include "arch/s390/s390_features.h"
#endif

extern void cpu_check_features(void);

/* adler32 */
typedef uint32_t (*adler32_func)(uint32_t adler, const unsigned char *buf, size_t len);

extern uint32_t adler32_c(uint32_t adler, const unsigned char *buf, size_t len);
#ifdef ARM_NEON_ADLER32
extern uint32_t adler32_neon(uint32_t adler, const unsigned char *buf, size_t len);
#endif
#ifdef PPC_VMX_ADLER32
extern uint32_t adler32_vmx(uint32_t adler, const unsigned char *buf, size_t len);
#endif
#ifdef X86_SSSE3_ADLER32
extern uint32_t adler32_ssse3(uint32_t adler, const unsigned char *buf, size_t len);
#endif
#ifdef X86_AVX2_ADLER32
extern uint32_t adler32_avx2(uint32_t adler, const unsigned char *buf, size_t len);
#endif
#ifdef X86_AVX512_ADLER32
extern uint32_t adler32_avx512(uint32_t adler, const unsigned char *buf, size_t len);
#endif
#ifdef X86_AVX512VNNI_ADLER32
extern uint32_t adler32_avx512_vnni(uint32_t adler, const unsigned char *buf, size_t len);
#endif
#ifdef POWER8_VSX_ADLER32
extern uint32_t adler32_power8(uint32_t adler, const unsigned char* buf, size_t len);
#endif

/* CRC32 folding */
#ifdef X86_PCLMULQDQ_CRC
extern uint32_t crc32_fold_reset_pclmulqdq(crc32_fold *crc);
extern void     crc32_fold_copy_pclmulqdq(crc32_fold *crc, uint8_t *dst, const uint8_t *src, size_t len);
extern void     crc32_fold_pclmulqdq(crc32_fold *crc, const uint8_t *src, size_t len, uint32_t init_crc);
extern uint32_t crc32_fold_final_pclmulqdq(crc32_fold *crc);
extern uint32_t crc32_pclmulqdq(uint32_t crc32, const unsigned char* buf, uint64_t len);
#endif

/* memory chunking */
extern uint32_t chunksize_c(void);
extern uint8_t* chunkcopy_c(uint8_t *out, uint8_t const *from, unsigned len);
extern uint8_t* chunkcopy_safe_c(uint8_t *out, uint8_t const *from, unsigned len, uint8_t *safe);
extern uint8_t* chunkunroll_c(uint8_t *out, unsigned *dist, unsigned *len);
extern uint8_t* chunkmemset_c(uint8_t *out, unsigned dist, unsigned len);
extern uint8_t* chunkmemset_safe_c(uint8_t *out, unsigned dist, unsigned len, unsigned left);
#ifdef X86_SSE2_CHUNKSET
extern uint32_t chunksize_sse2(void);
extern uint8_t* chunkcopy_sse2(uint8_t *out, uint8_t const *from, unsigned len);
extern uint8_t* chunkcopy_safe_sse2(uint8_t *out, uint8_t const *from, unsigned len, uint8_t *safe);
extern uint8_t* chunkunroll_sse2(uint8_t *out, unsigned *dist, unsigned *len);
extern uint8_t* chunkmemset_sse2(uint8_t *out, unsigned dist, unsigned len);
extern uint8_t* chunkmemset_safe_sse2(uint8_t *out, unsigned dist, unsigned len, unsigned left);
#endif
#ifdef X86_AVX_CHUNKSET
extern uint32_t chunksize_avx(void);
extern uint8_t* chunkcopy_avx(uint8_t *out, uint8_t const *from, unsigned len);
extern uint8_t* chunkcopy_safe_avx(uint8_t *out, uint8_t const *from, unsigned len, uint8_t *safe);
extern uint8_t* chunkunroll_avx(uint8_t *out, unsigned *dist, unsigned *len);
extern uint8_t* chunkmemset_avx(uint8_t *out, unsigned dist, unsigned len);
extern uint8_t* chunkmemset_safe_avx(uint8_t *out, unsigned dist, unsigned len, unsigned left);
#endif
#ifdef ARM_NEON_CHUNKSET
extern uint32_t chunksize_neon(void);
extern uint8_t* chunkcopy_neon(uint8_t *out, uint8_t const *from, unsigned len);
extern uint8_t* chunkcopy_safe_neon(uint8_t *out, uint8_t const *from, unsigned len, uint8_t *safe);
extern uint8_t* chunkunroll_neon(uint8_t *out, unsigned *dist, unsigned *len);
extern uint8_t* chunkmemset_neon(uint8_t *out, unsigned dist, unsigned len);
extern uint8_t* chunkmemset_safe_neon(uint8_t *out, unsigned dist, unsigned len, unsigned left);
#endif
#ifdef POWER8_VSX_CHUNKSET
extern uint32_t chunksize_power8(void);
extern uint8_t* chunkcopy_power8(uint8_t *out, uint8_t const *from, unsigned len);
extern uint8_t* chunkcopy_safe_power8(uint8_t *out, uint8_t const *from, unsigned len, uint8_t *safe);
extern uint8_t* chunkunroll_power8(uint8_t *out, unsigned *dist, unsigned *len);
extern uint8_t* chunkmemset_power8(uint8_t *out, unsigned dist, unsigned len);
extern uint8_t* chunkmemset_safe_power8(uint8_t *out, unsigned dist, unsigned len, unsigned left);
#endif

/* CRC32 */
typedef uint32_t (*crc32_func)(uint32_t crc32, const unsigned char * buf, uint64_t len);

extern uint32_t crc32_byfour(uint32_t crc, const unsigned char *buf, uint64_t len);
#ifdef ARM_ACLE_CRC_HASH
extern uint32_t crc32_acle(uint32_t crc, const unsigned char *buf, uint64_t len);
#elif defined(POWER8_VSX_CRC32)
extern uint32_t crc32_power8(uint32_t crc, const unsigned char *buf, uint64_t len);
#elif defined(S390_CRC32_VX)
extern uint32_t s390_crc32_vx(uint32_t crc, const unsigned char *buf, uint64_t len);
#endif

/* compare256 */
typedef uint32_t (*compare256_func)(const uint8_t *src0, const uint8_t *src1);

extern uint32_t compare256_c(const uint8_t *src0, const uint8_t *src1);
#ifdef UNALIGNED_OK
extern uint32_t compare256_unaligned_16(const uint8_t *src0, const uint8_t *src1);
#ifdef HAVE_BUILTIN_CTZ
extern uint32_t compare256_unaligned_32(const uint8_t *src0, const uint8_t *src1);
#endif
#if defined(UNALIGNED64_OK) && defined(HAVE_BUILTIN_CTZLL)
extern uint32_t compare256_unaligned_64(const uint8_t *src0, const uint8_t *src1);
#endif
#endif
#if defined(X86_SSE2) && defined(HAVE_BUILTIN_CTZ)
extern uint32_t compare256_sse2(const uint8_t *src0, const uint8_t *src1);
#endif
#if defined(X86_AVX2) && defined(HAVE_BUILTIN_CTZ)
extern uint32_t compare256_avx2(const uint8_t *src0, const uint8_t *src1);
#endif

#ifdef DEFLATE_H_
/* insert_string */
extern void insert_string_c(deflate_state *const s, const uint32_t str, uint32_t count);
#ifdef X86_SSE42_CRC_HASH
extern void insert_string_sse4(deflate_state *const s, const uint32_t str, uint32_t count);
#elif defined(ARM_ACLE_CRC_HASH)
extern void insert_string_acle(deflate_state *const s, const uint32_t str, uint32_t count);
#endif

/* longest_match */
extern uint32_t longest_match_c(deflate_state *const s, Pos cur_match);
#ifdef UNALIGNED_OK
extern uint32_t longest_match_unaligned_16(deflate_state *const s, Pos cur_match);
#ifdef HAVE_BUILTIN_CTZ
extern uint32_t longest_match_unaligned_32(deflate_state *const s, Pos cur_match);
#endif
#if defined(UNALIGNED64_OK) && defined(HAVE_BUILTIN_CTZLL)
extern uint32_t longest_match_unaligned_64(deflate_state *const s, Pos cur_match);
#endif
#endif
#if defined(X86_SSE2) && defined(HAVE_BUILTIN_CTZ)
extern uint32_t longest_match_sse2(deflate_state *const s, Pos cur_match);
#endif
#if defined(X86_AVX2) && defined(HAVE_BUILTIN_CTZ)
extern uint32_t longest_match_avx2(deflate_state *const s, Pos cur_match);
#endif

/* longest_match_slow */
extern uint32_t longest_match_slow_c(deflate_state *const s, Pos cur_match);
#ifdef UNALIGNED_OK
extern uint32_t longest_match_slow_unaligned_16(deflate_state *const s, Pos cur_match);
extern uint32_t longest_match_slow_unaligned_32(deflate_state *const s, Pos cur_match);
#ifdef UNALIGNED64_OK
extern uint32_t longest_match_slow_unaligned_64(deflate_state *const s, Pos cur_match);
#endif
#endif
#if defined(X86_SSE2) && defined(HAVE_BUILTIN_CTZ)
extern uint32_t longest_match_slow_sse2(deflate_state *const s, Pos cur_match);
#endif
#if defined(X86_AVX2) && defined(HAVE_BUILTIN_CTZ)
extern uint32_t longest_match_slow_avx2(deflate_state *const s, Pos cur_match);
#endif

/* quick_insert_string */
extern Pos quick_insert_string_c(deflate_state *const s, const uint32_t str);
#ifdef X86_SSE42_CRC_HASH
extern Pos quick_insert_string_sse4(deflate_state *const s, const uint32_t str);
#elif defined(ARM_ACLE_CRC_HASH)
extern Pos quick_insert_string_acle(deflate_state *const s, const uint32_t str);
#endif

/* slide_hash */
typedef void (*slide_hash_func)(deflate_state *s);

#ifdef X86_SSE2
extern void slide_hash_sse2(deflate_state *s);
#elif defined(ARM_NEON_SLIDEHASH)
extern void slide_hash_neon(deflate_state *s);
#endif
#if defined(PPC_VMX_SLIDEHASH)
extern void slide_hash_vmx(deflate_state *s);
#endif
#if defined(POWER8_VSX_SLIDEHASH)
extern void slide_hash_power8(deflate_state *s);
#endif
#ifdef X86_AVX2
extern void slide_hash_avx2(deflate_state *s);
#endif

/* update_hash */
extern uint32_t update_hash_c(deflate_state *const s, uint32_t h, uint32_t val);
#ifdef X86_SSE42_CRC_HASH
extern uint32_t update_hash_sse4(deflate_state *const s, uint32_t h, uint32_t val);
#elif defined(ARM_ACLE_CRC_HASH)
extern uint32_t update_hash_acle(deflate_state *const s, uint32_t h, uint32_t val);
#endif
#endif

#endif
