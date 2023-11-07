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
#if _TARGET_64BIT
  unsigned char _BitScanForward64(unsigned long *_Index, unsigned long long _Mask);
#endif
#if !defined(__clang__)
  extern unsigned int _lzcnt_u32(unsigned int);
  extern unsigned int _tzcnt_u32(unsigned int);
#if _TARGET_64BIT
  extern unsigned long long _lzcnt_u64(unsigned long long);
  extern unsigned long long _tzcnt_u64(unsigned long long);
#endif
#endif
#ifdef __cplusplus
}
#endif
#pragma intrinsic(__popcnt)
#pragma intrinsic(_BitScanForward)
#pragma intrinsic(_BitScanReverse)
#if _TARGET_64BIT
#pragma intrinsic(_BitScanForward64)
#endif
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

inline unsigned __ctz_unsafe(unsigned long long value)
{
#if defined(__clang__) || defined(__GNUC__)
  return __builtin_ctzll(value); // tzcnt or rep bsf or rbit + clz
#elif _TARGET_64BIT && defined(_MSC_VER)
  return _tzcnt_u64(value);               // tzcnt -> rep bsf
#endif

  unsigned n = 1;
  // clang-format off
  if((value & 0xFFFFFFFF) == 0) { n += 32; value >>= 32; }
  if((value & 0x0000FFFF) == 0) { n += 16; value >>= 16; }
  if((value & 0x000000FF) == 0) { n +=  8; value >>=  8; }
  if((value & 0x0000000F) == 0) { n +=  4; value >>=  4; }
  if((value & 0x00000003) == 0) { n +=  2; value >>=  2; }
  // clang-format on
  return (n - ((unsigned)value & 1));
}

inline unsigned __ctz_unsafe(long long value) { return __ctz_unsafe((unsigned long long)value); }

inline unsigned __ctz_unsafe(unsigned int value)
{
#if defined(__clang__) || defined(__GNUC__)
  return __builtin_ctz(value); // tzcnt or rep bsf
#elif defined(_MSC_VER)
  return _tzcnt_u32(value);               // tzcnt -> rep bsf
#endif

  unsigned n = 1;
  // clang-format off
  if((value & 0x0000FFFF) == 0) { n += 16; value >>= 16; }
  if((value & 0x000000FF) == 0) { n +=  8; value >>=  8; }
  if((value & 0x0000000F) == 0) { n +=  4; value >>=  4; }
  if((value & 0x00000003) == 0) { n +=  2; value >>=  2; }
  // clang-format on
  return (n - ((unsigned)value & 1));
}

inline unsigned __ctz_unsafe(int value) { return __ctz_unsafe((unsigned int)value); }

inline unsigned __ctz_unsafe(unsigned long value)
{
  if constexpr (sizeof(value) == 8)
    return __ctz_unsafe((unsigned long long)value);
  else
    return __ctz_unsafe((unsigned int)value);
}

inline unsigned __ctz_unsafe(long value) { return __ctz_unsafe((unsigned long)value); }

inline unsigned __ctz(unsigned long long value)
{
#if defined(__arm64__) || defined(__ARM_ARCH) // Apple silicon or ARMv7 and above
  return __builtin_ctzll(value);              //  rbit  + clz
#else
#if _TARGET_64BIT
#if defined(__clang__)
  return __builtin_ia32_tzcnt_u64(value); // bsf or tzct if -mbmi
#elif defined(__GNUC__)
#ifdef __BMI__
  return __builtin_ctzll(value);            // tzcnt
#else
  return value ? __builtin_ctzll(value) : 64; // bsf
#endif
#elif defined(_MSC_VER)
#if defined(__AVX2__)
  return _tzcnt_u64(value); // tzcnt
#else
  unsigned long r;
  _BitScanForward64(&r, value); // bsf
  return value ? r : 64;
#endif
#endif
#endif
#endif

  if (value)
  {
    unsigned n = 1;
    // clang-format off
    if((value & 0xFFFFFFFF) == 0) { n += 32; value >>= 32; }
    if((value & 0x0000FFFF) == 0) { n += 16; value >>= 16; }
    if((value & 0x000000FF) == 0) { n +=  8; value >>=  8; }
    if((value & 0x0000000F) == 0) { n +=  4; value >>=  4; }
    if((value & 0x00000003) == 0) { n +=  2; value >>=  2; }
    // clang-format on
    return (n - ((unsigned)value & 1));
  }

  return 64;
}

inline unsigned __ctz(long long value) { return __ctz((unsigned long long)value); }

inline unsigned __ctz(unsigned int value)
{
#if defined(__arm64__) || defined(__ARM_ARCH) // Apple silicon or ARMv7 and above
  return __builtin_ctz(value);                // rbit + clz
#else
#if _TARGET_64BIT
#if defined(__clang__)
  return __builtin_ia32_tzcnt_u32(value); // bsf or tzct if -mbmi
#elif defined(__GNUC__)
#ifdef __BMI__
  return __builtin_ctz(value);              // tzcnt
#else
  return value ? __builtin_ctz(value) : 32;   // bsf
#endif
#elif defined(_MSC_VER)
#if defined(__AVX2__)
  return _tzcnt_u32(value); // tzcnt
#else
  unsigned long r;
  _BitScanForward(&r, value); // bsf
  return value ? r : 32;
#endif
#endif
#endif
#endif

  if (value)
  {
    unsigned n = 1;
    // clang-format off
    if((value & 0x0000FFFF) == 0) { n += 16; value >>= 16; }
    if((value & 0x000000FF) == 0) { n +=  8; value >>=  8; }
    if((value & 0x0000000F) == 0) { n +=  4; value >>=  4; }
    if((value & 0x00000003) == 0) { n +=  2; value >>=  2; }
    // clang-format on
    return (n - ((unsigned)value & 1));
  }

  return 32;
}

inline unsigned __ctz(int value) { return __ctz((unsigned int)value); }

inline unsigned __ctz(unsigned long value)
{
  if constexpr (sizeof(value) == 8)
    return __ctz((unsigned long long)value);
  else
    return __ctz((unsigned int)value);
}

inline unsigned __ctz(long value) { return __ctz((unsigned long)value); }

inline unsigned __clz_unsafe(unsigned long long value)
{
#if defined(__arm64__) || defined(__aarch64__) // Apple silicon or ARMv8
  return __builtin_clzll(value);               // clz
#else
#if _TARGET_64BIT
#if defined(__clang__)
#ifdef __LZCNT__
  return __builtin_ia32_lzcnt_u64(value); // lzcnt
#else
  return __builtin_clzll(value);              // bsr
#endif
#elif defined(__GNUC__)
  return __builtin_clzll(value);            // bsr or lznct if -mabm
#elif defined(_MSC_VER)
#if defined(__AVX2__)
  return _lzcnt_u64(value); // lzcnt
#else
  unsigned long r;
  _BitScanForward64(&r, value); // bsr
  return r ^ 63;
#endif
#endif
#endif
#endif

  unsigned n = 0;
  // clang-format off
  if(value & (0xFFFFFFFF00000000ull)) { n += 32; value >>= 32; }
  if(value & 0xFFFF0000)              { n += 16; value >>= 16; }
  if(value & 0xFFFFFF00)              { n +=  8; value >>=  8; }
  if(value & 0xFFFFFFF0)              { n +=  4; value >>=  4; }
  if(value & 0xFFFFFFFC)              { n +=  2; value >>=  2; }
  if(value & 0xFFFFFFFE)              { n +=  1;               }
  // clang-format on

  return n;
}

inline unsigned __clz_unsafe(long long value) { return __clz_unsafe((unsigned long long)value); }

inline unsigned __clz_unsafe(unsigned int value)
{
#if defined(__ARM_ARCH)
  return __builtin_clz(value); // clz
#else
#if defined(__clang__)
#ifdef __LZCNT__
  return __builtin_ia32_lzcnt_u32(value); // lzcnt
#else
  return __builtin_clz(value);              // bsr
#endif
#elif defined(__GNUC__)
  return __builtin_clz(value);                           // bsr or lznct if -mabm
#elif defined(_MSC_VER)
#if defined(__AVX2__)
  return _lzcnt_u32(value); // lzcnt
#else
  unsigned long r;
  _BitScanForward(&r, value); // bsr
  return r ^ 31;
#endif
#endif
#endif

  unsigned n = 0;
  // clang-format off
  if(value & 0xFFFF0000)              { n += 16; value >>= 16; }
  if(value & 0xFFFFFF00)              { n +=  8; value >>=  8; }
  if(value & 0xFFFFFFF0)              { n +=  4; value >>=  4; }
  if(value & 0xFFFFFFFC)              { n +=  2; value >>=  2; }
  if(value & 0xFFFFFFFE)              { n +=  1;               }
  // clang-format on

  return n;
}

inline unsigned __clz_unsafe(int value) { return __clz_unsafe((unsigned int)value); }

inline unsigned __clz_unsafe(unsigned long value)
{
  if constexpr (sizeof(value) == 8)
    return __clz_unsafe((unsigned long long)value);
  else
    return __clz_unsafe((unsigned int)value);
}

inline unsigned __clz_unsafe(long value) { return __clz_unsafe((unsigned long)value); }

inline unsigned __clz(unsigned long long value)
{
#if defined(__arm64__) || defined(__aarch64__) // Apple silicon or ARMv8
  return __builtin_clzll(value);               // clz
#else
#if _TARGET_64BIT
#if defined(__clang__)
#ifdef __LZCNT__
  return __builtin_ia32_lzcnt_u64(value); // lzcnt
#else
  return value ? __builtin_clzll(value) : 64; // bsr
#endif
#elif defined(__GNUC__)
#ifdef __LZCNT__
  return __builtin_clzll(value);            // lzcnt
#else
  return value ? __builtin_clzll(value) : 64; // bsr
#endif
#elif defined(_MSC_VER)
#if defined(__AVX2__)
  return _lzcnt_u64(value); // lzcnt
#else
  unsigned long r;
  _BitScanForward64(&r, value); // bsr
  return value ? r ^ 63 : 64;
#endif
#endif
#endif
#endif

  if (value)
  {
    unsigned n = 0;
    // clang-format off
    if(value & (0xFFFFFFFF00000000ull)) { n += 32; value >>= 32; }
    if(value & 0xFFFF0000)              { n += 16; value >>= 16; }
    if(value & 0xFFFFFF00)              { n +=  8; value >>=  8; }
    if(value & 0xFFFFFFF0)              { n +=  4; value >>=  4; }
    if(value & 0xFFFFFFFC)              { n +=  2; value >>=  2; }
    if(value & 0xFFFFFFFE)              { n +=  1;               }
    // clang-format on

    return n;
  }

  return 64;
}

inline unsigned __clz(long long value) { return __clz((unsigned long long)value); }

inline unsigned __clz(unsigned int value)
{
#if defined(__ARM_ARCH)
  return __builtin_clz(value); // clz
#else
#if defined(__clang__)
#ifdef __LZCNT__
  return __builtin_ia32_lzcnt_u32(value); // lzcnt
#else
  return value ? __builtin_clz(value) : 32; // bsr
#endif
#elif defined(__GNUC__)
#ifdef __LZCNT__
  return __builtin_clz(value);                           // lzcnt
#else
  return value ? __builtin_clz(value) : 32; // bsr
#endif
#elif defined(_MSC_VER)
#if defined(__AVX2__)
  return _lzcnt_u32(value); // lzcnt
#else
  unsigned long r;
  _BitScanForward(&r, value); // bsr
  return value ? r ^ 31 : 32;
#endif
#endif
#endif

  if (value)
  {
    unsigned n = 0;
    // clang-format off
    if(value & 0xFFFF0000)              { n += 16; value >>= 16; }
    if(value & 0xFFFFFF00)              { n +=  8; value >>=  8; }
    if(value & 0xFFFFFFF0)              { n +=  4; value >>=  4; }
    if(value & 0xFFFFFFFC)              { n +=  2; value >>=  2; }
    if(value & 0xFFFFFFFE)              { n +=  1;               }
    // clang-format on

    return n;
  }

  return 32;
}

inline unsigned __clz(int value) { return __clz((unsigned int)value); }

inline unsigned __clz(unsigned long value)
{
  if constexpr (sizeof(value) == 8)
    return __clz((unsigned long long)value);
  else
    return __clz((unsigned int)value);
}

inline unsigned __clz(long value) { return __clz((unsigned long)value); }
