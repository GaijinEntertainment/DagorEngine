//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>
#include <supp/dag_math.h>
#include <math/dag_bits.h>

// Round up to the next highest power of 2 (for 768 - it will be 1024)
inline int get_bigger_pow2(int a)
{
  int v = a;
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  return v;
}

// Round up to the closest power of 2 (for 768 - it will be 512, for 769 - 1024)
inline int get_closest_pow2(int a)
{
  int v = get_bigger_pow2(a);
  if (v - a < a - (v >> 1))
    return v;
  else
    return (v >> 1);
}

template <class T>
inline bool is_pow_of2(T v)
{
  return v && !(v & (v - 1));
}

__forceinline uint32_t get_log2i(uint32_t v) // branchless version
{
#if HAS_BIT_SCAN_FORWARD
  return v ? __bsr_unsafe(v) : 0;
#else
  uint32_t r; // result of log2(v) will go here
  uint32_t shift;
  r = (v > 0xFFFF) << 4;
  v >>= r;
  shift = (v > 0xFF) << 3;
  v >>= shift;
  r |= shift;
  shift = (v > 0xF) << 2;
  v >>= shift;
  r |= shift;
  shift = (v > 0x3) << 1;
  v >>= shift;
  r |= shift;
  r |= (v >> 1);
  return r;
#endif
}

__forceinline uint32_t get_log2i_unsafe(uint32_t v) // branchless version, assumes v>0, otherwise UNDEFINED
{
#if HAS_BIT_SCAN_FORWARD
  return __bsr_unsafe(v);
#else
  return get_log2i(v);
#endif
}


__forceinline uint16_t get_log2w(uint16_t v) // branchless version
{
#if HAS_BIT_SCAN_FORWARD
  return get_log2i(v);
#else
  uint16_t r; // result of log2(v) will go here
  uint16_t shift;
  r = (v > 0xFF) << 3;
  v >>= r;
  shift = (v > 0xF) << 2;
  v >>= shift;
  r |= shift;
  shift = (v > 0x3) << 1;
  v >>= shift;
  r |= shift;
  r |= (v >> 1);
  return r;
#endif
}

// returns UB for 0, 0 for 1
// 1 for 2, etc
__forceinline uint32_t get_bigger_log2_unsafe(uint32_t v)
{
#if HAS_BIT_SCAN_FORWARD
  uint32_t r = __bsr_unsafe(v);
  return (1 << r) == v ? r : r + 1;
#else
  return get_log2i(get_bigger_pow2(v));
#endif
}


// returns 0 for 0, 0 for 1
// 1 for 2, etc
inline uint32_t get_bigger_log2(uint32_t v) { return v ? get_bigger_log2_unsafe(v) : 0; }

static constexpr uint32_t get_const_log2(uint32_t w) { return w > 1 ? 1 + get_const_log2(w >> 1) : 0; }
static constexpr uint32_t get_const_bigger_log2(uint32_t w)
{
  return (1 << get_const_log2(w)) == w ? get_const_log2(w) : get_const_log2(w) + 1;
}

inline uint32_t get_log2i_of_pow2(uint32_t v) // branchless version
{
#if HAS_BIT_SCAN_FORWARD
  return get_log2i(v);
#else
  unsigned int r = (v & 0xAAAAAAAA) != 0;
  r |= ((v & 0xFFFF0000) != 0) << 4;
  r |= ((v & 0xFF00FF00) != 0) << 3;
  r |= ((v & 0xF0F0F0F0) != 0) << 2;
  r |= ((v & 0xCCCCCCCC) != 0) << 1;
  return r;
#endif
}

inline unsigned int get_log2i_of_pow2w(uint16_t v) // branchless version, this one is still faster than using bsr
{
  unsigned int r = (v & 0xAAAA) != 0;
  r |= ((v & 0xFF00) != 0) << 3;
  r |= ((v & 0xF0F0) != 0) << 2;
  r |= ((v & 0xCCCC) != 0) << 1;
  return r;
}

// returns true if pow2. Zero is pow2
template <typename T>
static constexpr bool is_pow2(T value)
{
  return (value & (value - 1)) == 0;
}
