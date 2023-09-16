//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_compilerDefs.h>

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
#if __clang_major__ < 12 && defined(__clang__) && defined(__FAST_MATH__)
// unfortunately older clang versions do not work with float_control
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
#else
DAGOR_NOINLINE DAG_FINITE_MATH inline bool check_nan(float a) { return __builtin_isnan(a); }
DAGOR_NOINLINE DAG_FINITE_MATH inline bool check_nan(double a) { return __builtin_isnan(a); }
#endif
DAGOR_NOINLINE DAG_FINITE_MATH inline bool check_finite(float a) { return !__builtin_isinf(a); }
DAGOR_NOINLINE DAG_FINITE_MATH inline bool check_finite(double a) { return !__builtin_isinf(a); }
#undef DAS_FINITE_MATH
#if defined(__clang__) && !defined(__arm64__)
#pragma float_control(pop)
#endif
#else
#include <float.h>
// msvc just does not optimize fast math
__forceinline bool check_finite(float a) { return !isinf(a); }
__forceinline bool check_nan(float a) { return isnan(a); }
__forceinline bool check_finite(double a) { return !isinf(a); }
__forceinline bool check_nan(double a) { return isnan(a); }
#endif
