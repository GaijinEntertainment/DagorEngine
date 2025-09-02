//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#if _TARGET_SIMD_SSE
#if defined(_MSC_VER) && !defined(__clang__)
extern "C" void _mm_prefetch(char const *, int);
#pragma intrinsic(_mm_prefetch)
#else
#include <xmmintrin.h>
#endif

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define PREFETCH_DATA(ofs, base) (void)(ofs)
#endif
#endif

#ifndef PREFETCH_DATA
#define PREFETCH_DATA(ofs, base) (void)_mm_prefetch(((char *)base) + ofs, _MM_HINT_NTA)
#endif

#if defined(__GNUC__)
#if defined(__e2k__)
#define HAVE_INTRISTIC_RDTSC 1
#elif defined(__clang__)
#define HAVE_INTRISTIC_RDTSC __has_builtin(__builtin_ia32_rdtsc)
#elif defined(__GNUC__)
#define HAVE_INTRISTIC_RDTSC (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5))
#else
#define HAVE_INTRISTIC_RDTSC 0
#endif

#if HAVE_INTRISTIC_RDTSC
#include <x86intrin.h>
#else
static __inline__ unsigned long long __rdtsc()
{
  unsigned hi, lo;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
}
#endif
#undef HAVE_INTRISTIC_RDTSC

#elif _MSC_VER <= 1310
static inline unsigned __int64 __rdtsc()
{
  unsigned __int64 t1;
  __asm rdtsc;
  __asm mov dword ptr t1, eax;
  __asm mov dword ptr t1 + 4, edx;
  return t1;
}
#endif

#elif _TARGET_APPLE && _TARGET_SIMD_SSE
#include <xmmintrin.h>
#define PREFETCH_DATA(ofs, base) (void)_mm_prefetch(((char *)base) + ofs, _MM_HINT_NTA)

static __inline__ unsigned long long __rdtsc()
{
  unsigned hi, lo;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
}

#elif _TARGET_PC_MACOSX && _TARGET_SIMD_NEON
#define PREFETCH_DATA(ofs, base) ((void)0)

static __inline__ unsigned long long __rdtsc()
{
  uint64_t ticks;
  asm volatile("isb" : : : "memory");
  asm volatile("mrs %0, cntvct_el0" : "=r"(ticks));
  return ticks;
}

#elif _TARGET_APPLE | _TARGET_ANDROID | _TARGET_C3
#define PREFETCH_DATA(ofs, base) ((void)0)

static __inline__ unsigned long long __rdtsc() { return 0; }

#elif _TARGET_SIMD_NEON
#define PREFETCH_DATA(ofs, base) ((void)0)

#else

#error Unrecognized target
#endif
