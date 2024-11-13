// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/bitset.h>
#include <EASTL/span.h>
#include <math/dag_bits.h>
#include <util/dag_globDef.h>


template <size_t N>
inline eastl::bitset<N> &or_bit(eastl::bitset<N> &s, size_t i, bool b = true)
{
  return s.set(i, s.test(i) || b);
}

template <size_t N>
inline char *append_literal(char *at, char *ed, const char (&lit)[N])
{
  auto d = static_cast<size_t>(ed - at);
  auto c = min(d, N - 1);
  memcpy(at, lit, c);
  return at + c;
}

template <size_t N>
inline char *append_or_mask_value_name(char *beg, char *at, char *ed, const char (&name)[N])
{
  if (beg != at)
  {
    at = append_literal(at, ed, " | ");
  }
  return append_literal(at, ed, name);
}

template <typename T>
inline T align_value(T value, T alignment)
{
  return (value + alignment - 1) & ~(alignment - 1);
}

template <size_t N>
inline eastl::span<const char> string_literal_span(const char (&sl)[N])
{
  return {sl, N - 1};
}

template <size_t N>
inline eastl::span<const wchar_t> string_literal_span(const wchar_t (&sl)[N])
{
  return {sl, N - 1};
}

// Applies function object 'f' to each bit index of each set bit in bit mask 'm'.
template <typename F>
inline void for_each_set_bit(uint32_t m, F f)
{
  while (0 != m)
  {
    uint32_t i = __bsf_unsafe(m);
    m ^= 1u << i;
    f(i);
  }
}
