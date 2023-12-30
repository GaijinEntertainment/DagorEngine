/*
 * Dagor Engine 4
 * Copyright (C) 2003-2021  Gaijin Entertainment.  All rights reserved
 *
 * (for conditions of distribution and use, see EULA in "prog/eula.txt")
 */

#ifndef _DAGOR_PUBLIC_GENERIC_DAG_ENUMERATE_H_
#define _DAGOR_PUBLIC_GENERIC_DAG_ENUMERATE_H_
#pragma once

#include <EASTL/tuple.h>

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
      constexpr iterator operator++()
      {
        ++it;
        ++counter;
        return *this;
      }

      constexpr bool operator!=(iterator other) { return it != other.it; }

      constexpr auto operator*() { return eastl::tuple<size_t, decltype(*it)>{counter, *it}; }

      IteratorType it = {};
      size_t counter = 0;
    };

    constexpr iterator begin() { return {eastl::begin(container), startIndex}; }

    constexpr iterator end() { return {eastl::end(container)}; }

    T container;
    size_t startIndex;
  };

  return EnumerationHelper{eastl::forward<T>(container), start_index};
}

#endif //_DAGOR_PUBLIC_GENERIC_DAG_ENUMERATE_H_