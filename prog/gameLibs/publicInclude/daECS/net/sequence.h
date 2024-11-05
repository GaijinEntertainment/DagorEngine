//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>
#include <EASTL/numeric_limits.h>
#include <EASTL/functional.h>

namespace net
{

typedef uint16_t sequence_t;

template <typename T>
inline EA_CPP14_CONSTEXPR bool is_seq_gt(T s1, T s2) // (s1 > s2)?
{
  const T halfMax = eastl::numeric_limits<T>::max() / 2;
  return ((s1 > s2) && (s1 - s2 <= halfMax)) || ((s2 > s1) && (s2 - s1 > halfMax));
}

template <typename T>
inline EA_CPP14_CONSTEXPR bool is_seq_lt(T s1, T s2) // (s1 < s2)?
{
  return is_seq_gt(s2, s1);
}

template <typename T>
inline T seq_diff(T big, T small)
{
  if (big >= small)
    return big - small;
  else // overflow
    return (eastl::numeric_limits<sequence_t>::max() - small) + big + 1;
}

struct LessSeq : public eastl::binary_function<sequence_t, sequence_t, bool>
{
  EA_CPP14_CONSTEXPR bool operator()(sequence_t a, sequence_t b) const { return is_seq_lt(a, b); }
};


} // namespace net
