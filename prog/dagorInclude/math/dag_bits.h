//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/type_traits.h> // eastl::is_integral_v

#if _TARGET_PC_WIN | _TARGET_XBOX
#ifdef __cplusplus
extern "C"
{
#endif
  unsigned int __popcnt(unsigned int);
#if _TARGET_64BIT
  unsigned __int64 __popcnt64(unsigned __int64);
#endif
  unsigned char _BitScanForward(unsigned long *_Index, unsigned long _Mask);
  unsigned char _BitScanReverse(unsigned long *_Index, unsigned long _Mask);
#ifdef __cplusplus
}
#endif
#pragma intrinsic(__popcnt)
#if _TARGET_64BIT
#pragma intrinsic(__popcnt64)
#endif
#pragma intrinsic(_BitScanForward)
#pragma intrinsic(_BitScanReverse)
#endif

#define POW2_ALIGN(v, a)        (((v) + ((a) - 1)) & ~((a) - 1))
#define POW2_ALIGN_PTR(v, a, t) ((t *)(POW2_ALIGN((uintptr_t)v, (uintptr_t)a)))

inline uint32_t reverse_bits32(uint32_t bits)
{
  bits = (bits << 16) | (bits >> 16);
  bits = ((bits & 0x00ff00ffU) << 8) | ((bits & 0xff00ff00U) >> 8);
  bits = ((bits & 0x0f0f0f0fU) << 4) | ((bits & 0xf0f0f0f0U) >> 4);
  bits = ((bits & 0x33333333U) << 2) | ((bits & 0xccccccccU) >> 2);
  bits = ((bits & 0x55555555U) << 1) | ((bits & 0xaaaaaaaaU) >> 1);
  return bits;
}

namespace dag
{

template <class T>
  requires eastl::is_integral_v<T>
constexpr int popcount(T x) noexcept
{
  if (!__builtin_is_constant_evaluated())
  {
#if defined(__GNUC__) || defined(__clang__)
    if constexpr (sizeof(x) == sizeof(uint64_t))
      return __builtin_popcountll(x);
    else
      return __builtin_popcount(x);
#elif defined(_MSC_VER) && _TARGET_64BIT
    if constexpr (sizeof(x) == sizeof(uint64_t))
      return static_cast<int>(__popcnt64(x));
    else
      return static_cast<int>(__popcnt(x));
#endif
  }

  if constexpr (sizeof(x) == sizeof(uint64_t))
  {
    auto i = static_cast<uint64_t>(x);
    i = i - ((i >> 1) & 0x5555555555555555ULL);
    i = (i & 0x3333333333333333ULL) + ((i >> 2) & 0x3333333333333333ULL);
    return (((i + (i >> 4)) & 0x0F0F0F0F0F0F0F0FULL) * 0x0101010101010101ULL) >> 56;
  }
  else
  {
    auto i = static_cast<uint32_t>(x);
    i = i - ((i >> 1) & 0x55555555U);
    i = (i & 0x33333333U) + ((i >> 2) & 0x33333333U);
    return (((i + (i >> 4)) & 0x0F0F0F0FU) * 0x01010101U) >> 24;
  }
}

} // namespace dag

inline int __popcount(unsigned v) { return dag::popcount(v); } // bw compat

#if _TARGET_SIMD_SSE || defined(__GNUC__) || (_TARGET_SIMD_NEON && defined(_MSC_VER))
#define HAS_BIT_SCAN_FORWARD 1
#if defined(__GNUC__) || defined(__clang__)

inline unsigned __bsf(unsigned v) { return v ? __builtin_ctz(v) : 32; }

inline unsigned __bsf_unsafe(unsigned v) // undefined for 0
{
  return __builtin_ctz(v);
}

inline int __bit_scan_forward(unsigned long &index, unsigned int val)
{
  if (!val)
    return 0;
  index = __builtin_ctz(val);
  return 1;
}

inline unsigned __bsr(unsigned v) { return v ? 31 - __builtin_clz(v) : 32; }

inline unsigned __bsr_unsafe(unsigned v) // undefined for 0
{
  return 31 - __builtin_clz(v);
}

inline int __bit_scan_reverse(unsigned long &index, unsigned int val)
{
  if (!val)
    return 0;
  index = 31 - __builtin_clz(val);
  return 1;
}

#elif _TARGET_PC_WIN | _TARGET_XBOX

inline unsigned __bsf(unsigned v)
{
  unsigned long r;
  return _BitScanForward(&r, v) ? r : 32;
}

inline unsigned __bsf_unsafe(unsigned v) // undefined for 0
{
  unsigned long r;
  _BitScanForward(&r, v);
  return r;
}

inline int __bit_scan_forward(unsigned long &index, unsigned int val) { return _BitScanForward(&index, val); }

inline unsigned __bsr(unsigned v)
{
  unsigned long r;
  return _BitScanReverse(&r, v) ? r : 32;
}

inline unsigned __bsr_unsafe(unsigned v) // undefined for 0
{
  unsigned long r;
  _BitScanReverse(&r, v);
  return r;
}

inline int __bit_scan_reverse(unsigned long &index, unsigned int val) { return _BitScanReverse(&index, val); }
#endif

#else
#define HAS_BIT_SCAN_FORWARD 0
#endif
