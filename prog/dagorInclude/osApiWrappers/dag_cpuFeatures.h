//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

/*
Available bools for fast check (values following from compilation options will be constexpr):
  cpu_feature_sse41
  cpu_feature_sse42
  cpu_feature_popcnt
  cpu_feature_fma
  cpu_feature_avx
  cpu_feature_avx2
  cpu_feature_fast_256bit_avx

Some AMD CPU's with AVX support have not native 256-bit calculations. They are implemented by using
of two 128-bit execution units. Algorithms with 256-bit registers using on that processors gives
about 0.8-1.0x speed difference againt 128-bit analogue, but CPU's with native 256-bit are about 1.6x
times faster in same case.
XBox One and PS4 (AMD Jaguar family) have slow 256-bit avx.
*/

#include <supp/dag_define_COREIMP.h>

// Values got from CPUID flags, compilation options are ignored
KRNLIMP extern bool cpu_feature_sse41_checked;
KRNLIMP extern bool cpu_feature_sse42_checked;
KRNLIMP extern bool cpu_feature_popcnt_checked;
KRNLIMP extern bool cpu_feature_fma_checked;
KRNLIMP extern bool cpu_feature_avx_checked;
KRNLIMP extern bool cpu_feature_avx2_checked;
KRNLIMP extern bool cpu_feature_fast_256bit_avx_checked;

#if (__cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) // for inline bool
// MSVC defines only __AVX__ and __AVX2__

#if _TARGET_SIMD_SSE >= 4 || defined(__SSE4_1__) || defined(__AVX__)
constexpr bool cpu_feature_sse41 = true;
#elif !_TARGET_SIMD_SSE
constexpr bool cpu_feature_sse41 = false;
#else
inline bool &cpu_feature_sse41 = cpu_feature_sse41_checked;
#endif

#if _TARGET_SIMD_SSE >= 4 || defined(__SSE4_2__) || defined(__AVX__)
constexpr bool cpu_feature_sse42 = true;
#elif !_TARGET_SIMD_SSE
constexpr bool cpu_feature_sse42 = false;
#else
inline bool &cpu_feature_sse42 = cpu_feature_sse42_checked;
#endif

#if _TARGET_SIMD_SSE >= 4 || defined(__SSE4_1__) || defined(__AVX__)
constexpr bool cpu_feature_popcnt = true;
#elif !_TARGET_SIMD_SSE
constexpr bool cpu_feature_popcnt = false;
#else
inline bool &cpu_feature_popcnt = cpu_feature_popcnt_checked;
#endif

#if defined(__AVX2__)
constexpr bool cpu_feature_fma = true;
#elif !_TARGET_SIMD_SSE
constexpr bool cpu_feature_fma = false;
#else
inline bool &cpu_feature_fma = cpu_feature_fma_checked;
#endif

#ifdef __AVX__
constexpr bool cpu_feature_avx = true;
#elif !_TARGET_SIMD_SSE
constexpr bool cpu_feature_avx = false;
#else
inline bool &cpu_feature_avx = cpu_feature_avx_checked;
#endif // __AVX__

#ifdef __AVX2__
constexpr bool cpu_feature_avx2 = true;
#elif !_TARGET_SIMD_SSE
constexpr bool cpu_feature_avx2 = false;
#else
inline bool &cpu_feature_avx2 = cpu_feature_avx2_checked;
#endif // __AVX2__

#if defined _TARGET_C1 || defined _TARGET_XBOXONE || !defined(_TARGET_64BIT)
constexpr bool cpu_feature_fast_256bit_avx = false; // AMD Jaguar has no benefits from using 256-bit regs (which is used in prevgen
                                                    // consoles - ps4, xb1)
#elif defined __AVX__
constexpr bool cpu_feature_fast_256bit_avx = true;
#else
inline bool &cpu_feature_fast_256bit_avx = cpu_feature_fast_256bit_avx_checked;
#endif // __AVX__

#endif // __cplusplus >= 201703L

#include <supp/dag_undef_COREIMP.h>
