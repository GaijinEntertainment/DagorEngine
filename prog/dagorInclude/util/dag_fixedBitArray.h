//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

/// @addtogroup common
/// @{

/** @file
  #FixedBitarray class
*/


#include <generic/dag_carray.h>

template <unsigned N>
class FixedBitArray : carray<size_t, ((N + sizeof(size_t) * 8 - 1) / (sizeof(size_t) * 8))>
{
  static const unsigned int static_bits_size = N;
  static const unsigned int bitsShift = (sizeof(size_t) == 4 ? 5 : 6);
  static const size_t bits = sizeof(size_t) * 8;
  static const size_t bitsMask = bits - 1;
  static const int elements_size = ((N + sizeof(size_t) * 8 - 1) / (sizeof(size_t) * 8));

public:
  /// Returns number of elements.
  static int size() { return N; }

  FixedBitArray() { memset(this, 0, sizeof(*this)); }

  __forceinline void set(int i)
  {
    G_STATIC_ASSERT(sizeof(size_t) == 4 || sizeof(size_t) == 8); // 32 or 64 bit platform only
    (carray<size_t, elements_size>::operator[]((int)(i >> bitsShift))) |= (size_t(1UL) << (i & bitsMask));
  }
  __forceinline void reset(int i)
  {

    (carray<size_t, elements_size>::operator[]((int)(i >> bitsShift))) &= ~(size_t(1UL) << (i & bitsMask));
  }
  __forceinline bool get(int i) const
  {
    return (bool)((carray<size_t, elements_size>::operator[]((int)(i >> bitsShift))) & ((size_t(1UL) << (i & bitsMask))));
  }
  __forceinline bool operator[](size_t i) const { return get(i); }
};

///@}
