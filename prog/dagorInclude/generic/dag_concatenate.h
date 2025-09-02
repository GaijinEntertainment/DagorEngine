//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/array.h>

namespace backport
{
template <typename InputIterator, typename Size, typename OutputIterator>
constexpr inline OutputIterator copy_n(InputIterator first, Size n, OutputIterator result)
{
  for (; n > 0; --n)
    *result++ = *first++;
  return result;
}
} // namespace backport

template <typename T, size_t... N>
constexpr auto concatenate(const eastl::array<T, N> &...arrays)
{
  eastl::array<T, (N + ...)> result;
  size_t index = 0;

  ((backport::copy_n(arrays.begin(), N, result.begin() + index), index += N), ...);

  return result;
}

template <typename T, size_t... N>
constexpr auto concatenate(const T (&...arrays)[N])
{
  eastl::array<T, (N + ...)> result;
  size_t index = 0;

  ((backport::copy_n(eastl::begin(arrays), N, result.begin() + index), index += N), ...);

  return result;
}