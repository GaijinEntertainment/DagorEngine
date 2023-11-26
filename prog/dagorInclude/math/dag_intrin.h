//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#if (defined(_MSC_VER) && !defined(__clang__)) || defined(__BMI__)
#include <immintrin.h>
#endif

inline unsigned __ctz_unsafe(unsigned long long value)
{
#if defined(__clang__) || defined(__GNUC__)
  return __builtin_ctzll(value); // tzcnt or rep bsf or rbit + clz
#elif _TARGET_64BIT && defined(_MSC_VER)
  return _tzcnt_u64(value);               // tzcnt -> rep bsf
#endif

  unsigned n = 1; //-V779
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

  unsigned n = 1; //-V779
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

inline unsigned __ctz(unsigned long long value) { return value ? __ctz_unsafe(value) : 64; }

inline unsigned __ctz(long long value) { return __ctz((unsigned long long)value); }

inline unsigned __ctz(unsigned int value) { return value ? __ctz_unsafe(value) : 32; }

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
  return __builtin_clzll(value); // bsr
#endif
#elif defined(__GNUC__)
  return __builtin_clzll(value); // bsr or lznct if -mabm
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

  unsigned n = 0; //-V779
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
  return __builtin_clz(value);   // bsr
#endif
#elif defined(__GNUC__)
  return __builtin_clz(value); // bsr or lznct if -mabm
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

  unsigned n = 0; //-V779
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

inline unsigned __clz(unsigned long long value) { return value ? __clz_unsafe(value) : 64; }

inline unsigned __clz(long long value) { return __clz((unsigned long long)value); }

inline unsigned __clz(unsigned int value) { return value ? __clz_unsafe(value) : 32; }

inline unsigned __clz(int value) { return __clz((unsigned int)value); }

inline unsigned __clz(unsigned long value)
{
  if constexpr (sizeof(value) == 8)
    return __clz((unsigned long long)value);
  else
    return __clz((unsigned int)value);
}

inline unsigned __clz(long value) { return __clz((unsigned long)value); }

inline unsigned __blsr(unsigned long long value)
{
#if defined(__BMI__) && _TARGET_64BIT
  return _blsr_u64(value);
#else
  return value & (value - 1);
#endif
}

inline unsigned __blsr(long long value) { return __blsr((unsigned long long)value); }

inline unsigned __blsr(unsigned int value)
{
#if defined(__BMI__)
  return _blsr_u32(value);
#else
  return value & (value - 1);
#endif
}

inline unsigned __blsr(int value) { return __blsr((unsigned int)value); }

inline unsigned __blsr(unsigned long value)
{
  if constexpr (sizeof(value) == 8)
    return __blsr((unsigned long long)value);
  else
    return __blsr((unsigned int)value);
}

inline unsigned __blsr(long value) { return __blsr((unsigned long)value); }
