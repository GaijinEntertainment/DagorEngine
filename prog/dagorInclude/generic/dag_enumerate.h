//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/tuple.h>

#ifdef __e2k__
#define CONSTEXPR // lcc 1.27.14 (e2k arch) cannot return non-literal expression as constexpr
#else
#define CONSTEXPR constexpr
#endif

// simple helper mimicking python's enumerate() function
// useful for keeping a counter when iterating in a range-based for loop
// for(auto [i,v] : enumerate(container)) { ... }
template <typename T>
constexpr auto enumerate(T &&container, size_t start_index = 0)
{
  struct EnumerationHelper
  {
    using IteratorType = decltype(eastl::begin(eastl::declval<T>()));

    struct iterator
    {
      CONSTEXPR iterator operator++()
      {
        ++it;
        ++counter;
        return *this;
      }

      CONSTEXPR bool operator!=(iterator other) { return it != other.it; }

      CONSTEXPR auto operator*() { return eastl::tuple<size_t, decltype(*it)>{counter, *it}; }

      IteratorType it;
      size_t counter;
    };

    CONSTEXPR iterator begin() { return {eastl::begin(container), startIndex}; }

    CONSTEXPR iterator end() { return {eastl::end(container)}; }

    T container;
    size_t startIndex;
  };

  return EnumerationHelper{eastl::forward<T>(container), start_index};
}
#undef CONSTEXPR
