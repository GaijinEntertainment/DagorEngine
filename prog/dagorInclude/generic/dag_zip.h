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

// simple helper mimicking python's zip() function
// can be used to iterate through multiple containers of the same size
// for(auto [a,b,c] : zip(containerA, containerB, containerC)) { ... }

namespace detail
{
template <typename... TupleTypes, size_t... Indices>
constexpr bool any_equal(eastl::index_sequence<Indices...>, const eastl::tuple<TupleTypes...> &left,
  const eastl::tuple<TupleTypes...> &right)
{
  return ((eastl::get<Indices>(left) == eastl::get<Indices>(right)) || ...);
}
}; // namespace detail

template <typename... T>
constexpr auto zip(T &&...containers)
{
  struct ZipHelper
  {
    using IteratorTypes = eastl::tuple<decltype(eastl::begin(eastl::declval<T>()))...>;
    using Indices = eastl::make_index_sequence<sizeof...(T)>;

    struct iterator
    {
      CONSTEXPR iterator operator++()
      {
        it = eastl::apply([](auto &&...args) { return eastl::make_tuple(++args...); }, it);
        return *this;
      }

      CONSTEXPR bool operator!=(iterator other) { return !detail::any_equal(Indices(), it, other.it); }

      CONSTEXPR auto operator*()
      {
        return eastl::apply([](auto &&...args) { return eastl::forward_as_tuple(*args...); }, it);
      }

      IteratorTypes it;
    };

    CONSTEXPR iterator begin()
    {
      return {eastl::apply([](auto &&...args) { return eastl::make_tuple(eastl::begin(args)...); }, containers)};
    }

    CONSTEXPR iterator end()
    {
      return {eastl::apply([](auto &&...args) { return eastl::make_tuple(eastl::end(args)...); }, containers)};
    }

    eastl::tuple<T...> containers;
  };

  return ZipHelper{eastl::forward_as_tuple(containers...)};
}
#undef CONSTEXPR
