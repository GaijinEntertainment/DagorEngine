//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/utility.h>

#ifdef __e2k__
#define CONSTEXPR // lcc 1.27.14 (e2k arch) cannot return non-literal expression as constexpr
#else
#define CONSTEXPR constexpr
#endif

namespace detail
{

template <typename T>
struct EnumerateIterator
{
  using IteratorType = decltype(eastl::begin(eastl::declval<T>()));

  using iterator_category = typename eastl::iterator_traits<IteratorType>::iterator_category;
  using value_type = eastl::pair<size_t, decltype(*eastl::declval<IteratorType>())>;
  using difference_type = typename eastl::iterator_traits<IteratorType>::difference_type;
  using pointer = value_type *;
  using reference = value_type;

  CONSTEXPR EnumerateIterator operator++()
  {
    ++it;
    ++counter;
    return *this;
  }

  CONSTEXPR EnumerateIterator operator--()
    requires(eastl::is_base_of_v<eastl::bidirectional_iterator_tag,
      typename eastl::iterator_traits<eastl::decay_t<IteratorType>>::iterator_category>)
  {
    --it;
    --counter;
    return *this;
  }

  CONSTEXPR bool operator!=(const EnumerateIterator &other) const { return it != other.it; }

  CONSTEXPR value_type operator*() { return eastl::pair<size_t, decltype(*it)>{counter, *it}; }

  IteratorType it;
  size_t counter;
};

} // namespace detail

// simple helper mimicking python's enumerate() function
// useful for keeping a counter when iterating in a range-based for loop
// for(auto [i,v] : enumerate(container)) { ... }
template <typename T>
constexpr auto enumerate(T &&container, size_t start_index = 0)
{
  struct EnumerationHelper
  {
    CONSTEXPR detail::EnumerateIterator<T> begin() { return {eastl::begin(container), startIndex}; }

    CONSTEXPR detail::EnumerateIterator<T> end() { return {eastl::end(container), startIndex + eastl::size(container)}; }

    T container;
    size_t startIndex;
  };

  return EnumerationHelper{eastl::forward<T>(container), start_index};
}
#undef CONSTEXPR
