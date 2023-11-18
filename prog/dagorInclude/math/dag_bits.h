//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#if _TARGET_PC_WIN | _TARGET_XBOX
#ifdef __cplusplus
extern "C"
{
#endif
  unsigned int __popcnt(unsigned int);
  unsigned char _BitScanForward(unsigned long *_Index, unsigned long _Mask);
  unsigned char _BitScanReverse(unsigned long *_Index, unsigned long _Mask);
#ifdef __cplusplus
}
#endif
#pragma intrinsic(__popcnt)
#pragma intrinsic(_BitScanForward)
#pragma intrinsic(_BitScanReverse)
#endif

#define POW2_ALIGN(v, a)        (((v) + ((a)-1)) & ~((a)-1))
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

#if defined(__GNUC__) || defined(__clang__)
inline int __popcount(unsigned v) { return __builtin_popcount(v); }
#else
inline int __popcount(unsigned v)
{
#if defined(_MSC_VER) && _TARGET_64BIT
  return __popcnt(v);
#else // sw implementation
  v = v - ((v >> 1) & 0x55555555);                       // reuse input as temporary
  v = (v & 0x33333333) + ((v >> 2) & 0x33333333);        // temp
  return ((v + (v >> 4) & 0xF0F0F0F) * 0x1010101) >> 24; // count
#endif
}
#endif


#if _TARGET_SIMD_SSE || defined(__GNUC__)
#define HAS_BIT_SCAN_FORWARD 1
#if defined(__GNUC__)

inline unsigned __bsf(int v) { return v ? __builtin_ctz(v) : 32; }

inline unsigned __bsf_unsafe(int v) // undefined for 0
{
  return __builtin_ctz(v);
}

inline int __bit_scan_forward(unsigned int &index, unsigned int val)
{
  if (!val)
    return 0;
  index = __builtin_ctz(val);
  return 1;
}

inline unsigned __bsr(int v) { return v ? 31 - __builtin_clz(v) : 32; }

inline unsigned __bsr_unsafe(int v) // undefined for 0
{
  return 31 - __builtin_clz(v);
}

inline int __bit_scan_reverse(unsigned int &index, unsigned int val)
{
  if (!val)
    return 0;
  index = 31 - __builtin_clz(val);
  return 1;
}

#elif _TARGET_PC_WIN | _TARGET_XBOX

inline unsigned __bsf(int v)
{
  unsigned long r;
  return _BitScanForward(&r, v) ? r : 32;
}

inline unsigned __bsf_unsafe(int v) // undefined for 0
{
  unsigned long r;
  _BitScanForward(&r, v);
  return r;
}

inline int __bit_scan_forward(unsigned long &index, unsigned int val) { return _BitScanForward(&index, val); }

inline unsigned __bsr(int v)
{
  unsigned long r;
  return _BitScanReverse(&r, v) ? r : 32;
}

inline unsigned __bsr_unsafe(int v) // undefined for 0
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
