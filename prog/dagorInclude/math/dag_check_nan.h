//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_bitwise_cast.h>
#include <stdint.h>

// Must stay correct in TUs built with -ffast-math / -fp:fast, where isnan,
// isfinite and fpclassify get folded to constants under the compiler's no-NaN
// promise. We test the IEEE-754 exponent/mantissa bits as integers, but first
// read the value through a volatile: with -ffinite-math-only the compiler
// otherwise assumes a clean float (a parameter, a division result) is finite
// and folds the bit test away before bit_cast can hide it. The volatile load
// forces an opaque value from memory. No per-compiler pragmas needed.
__forceinline bool check_nan(float a)
{
  volatile float v = a;
  // The (float) cast forces that load on clang (else it reuses the reg, store dies).
  return (dag::bit_cast<uint32_t>((float)v) & 0x7FFFFFFFu) > 0x7F800000u;
}
__forceinline bool check_nan(double a)
{
  volatile double v = a;
  return (dag::bit_cast<uint64_t>((double)v) & 0x7FFFFFFFFFFFFFFFull) > 0x7FF0000000000000ull;
}
__forceinline bool check_finite(float a)
{
  volatile float v = a;
  return (dag::bit_cast<uint32_t>((float)v) & 0x7FFFFFFFu) < 0x7F800000u;
}
__forceinline bool check_finite(double a)
{
  volatile double v = a;
  return (dag::bit_cast<uint64_t>((double)v) & 0x7FFFFFFFFFFFFFFFull) < 0x7FF0000000000000ull;
}

void verify_nan_finite_checks();
