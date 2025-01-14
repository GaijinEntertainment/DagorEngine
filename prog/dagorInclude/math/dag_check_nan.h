//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_compilerDefs.h>
#include <cstring>
#include <cstdint>
#include <cmath>

#if defined(__GNUC__) || defined(__clang__)
#if !defined(__clang__)
#define DAG_FINITE_MATH __attribute__((optimize("no-finite-math-only")))
#elif defined(__SSE__)
#define DAG_FINITE_MATH __vectorcall
#else
#define DAG_FINITE_MATH
#endif
#if defined(__clang__) && !defined(__arm64__)
#pragma float_control(push)
#pragma float_control(precise, on)
#endif
#if (__clang_major__ < 12 || (__clang_major__ >= 17 && __clang_major__ <= 19) || defined(__APPLE__) || _TARGET_ANDROID) && \
  defined(__clang__) && defined(__FAST_MATH__)
// unfortunately older clang versions do not work with float_control, and in clang 17-19.1 it's broken
__forceinline DAG_FINITE_MATH bool check_nan(float a)
{
  volatile float b = a;
  return b != a;
}
__forceinline DAG_FINITE_MATH bool check_nan(double a)
{
  volatile double b = a;
  return b != a;
}
__forceinline DAG_FINITE_MATH bool check_finite(float a)
{
  uint32_t i;
  memcpy(&i, &a, sizeof(a));
  i &= ~(1 << 31);
  memcpy(&a, &i, sizeof(a));
  static volatile float inf = INFINITY;
  return a != inf && !check_nan(a);
}
__forceinline DAG_FINITE_MATH bool check_finite(double a)
{
  uint64_t i;
  memcpy(&i, &a, sizeof(a));
  i &= ~(1ull << 63);
  memcpy(&a, &i, sizeof(a));
  static volatile double inf = INFINITY;
  return a != inf && !check_nan(a);
}
#else
DAGOR_NOINLINE DAG_FINITE_MATH inline bool check_nan(float a) { return __builtin_isnan(a); }
DAGOR_NOINLINE DAG_FINITE_MATH inline bool check_nan(double a) { return __builtin_isnan(a); }
DAGOR_NOINLINE DAG_FINITE_MATH inline bool check_finite(float a) { return __builtin_isfinite(a) && !__builtin_isnan(a); }
DAGOR_NOINLINE DAG_FINITE_MATH inline bool check_finite(double a) { return __builtin_isfinite(a) && !__builtin_isnan(a); }
#endif
#undef DAG_FINITE_MATH
#if defined(__clang__) && !defined(__arm64__)
#pragma float_control(pop)
#endif
#else
#include <float.h>
// msvc just does not optimize fast math
__forceinline bool check_finite(float a) { return isfinite(a); }
__forceinline bool check_nan(float a) { return isnan(a); }
__forceinline bool check_finite(double a) { return isfinite(a); }
__forceinline bool check_nan(double a) { return isnan(a); }
#endif

void verify_nan_finite_checks();
