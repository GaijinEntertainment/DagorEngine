//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/type_traits.h>
#include <EASTL/functional.h>

template <typename T, int view_count, int item_count = 1>
struct ViewDependentResource
{
  T values[view_count][item_count];
  int currentView = 0;

  constexpr static int viewCount = view_count;
  constexpr static int itemCount = item_count;
  constexpr static bool isArray = item_count > 1;

  template <typename... Args>
  explicit ViewDependentResource(Args &&...args) : values{eastl::forward<Args>(args)...}
  {}

  template <bool _isArray = isArray>
  eastl::enable_if_t<_isArray, T &> operator[](int itemIndex)
  {
    return values[currentView][itemIndex];
  }

  template <bool _isArray = isArray>
  eastl::enable_if_t<!_isArray, T &> operator[](int viewIndex)
  {
    return values[viewIndex][0];
  }

  template <bool _isArray = isArray>
  operator eastl::enable_if_t<!_isArray, const T &>() const
  {
    return values[currentView][0];
  }

  template <bool _isArray = isArray>
  eastl::enable_if_t<!_isArray, ViewDependentResource &> operator=(const T &value)
  {
    values[currentView][0] = value;
    return *this;
  }

  template <bool _isArray = isArray>
  eastl::enable_if_t<!_isArray, ViewDependentResource &> operator=(T &&value)
  {
    values[currentView][0] = eastl::move(value);
    return *this;
  }

  template <bool _isArray = isArray>
  eastl::enable_if_t<!_isArray, bool> operator==(const T &value)
  {
    return values[currentView][0] == value;
  }

  template <bool _isArray = isArray>
  eastl::enable_if_t<!_isArray, bool> operator!=(const T &value)
  {
    return values[currentView][0] != value;
  }

  template <bool _isArray = isArray>
  eastl::enable_if_t<!_isArray, bool> operator<(const T &value)
  {
    return values[currentView][0] < value;
  }

  template <bool _isArray = isArray>
  eastl::enable_if_t<!_isArray, bool> operator>(const T &value)
  {
    return values[currentView][0] > value;
  }

  template <bool _isArray = isArray>
  eastl::enable_if_t<!_isArray, bool> operator<=(const T &value)
  {
    return values[currentView][0] <= value;
  }

  template <bool _isArray = isArray>
  eastl::enable_if_t<!_isArray, bool> operator>=(const T &value)
  {
    return values[currentView][0] >= value;
  }

  template <bool _isArray = isArray>
  eastl::enable_if_t<!_isArray, bool> operator!()
  {
    return !values[currentView][0];
  }

  template <bool _isArray = isArray>
  eastl::enable_if_t<!_isArray, T> operator++(int)
  {
    return values[currentView][0]++;
  }

  template <bool _isArray = isArray>
  eastl::enable_if_t<!_isArray, T> operator++()
  {
    return ++values[currentView][0];
  }

  template <bool _isArray = isArray>
  eastl::enable_if_t<!_isArray, T> operator%(const T &value)
  {
    return values[currentView][0] % value;
  }

  template <bool _isArray = isArray>
  eastl::enable_if_t<!_isArray, T> operator*(const T &value)
  {
    return values[currentView][0] * value;
  }

  template <bool _isArray = isArray>
  eastl::enable_if_t<!_isArray, T> operator/(const T &value)
  {
    return values[currentView][0] / value;
  }

  template <bool _isArray = isArray>
  eastl::enable_if_t<!_isArray, T> operator+(const T &value)
  {
    return values[currentView][0] + value;
  }

  template <bool _isArray = isArray>
  eastl::enable_if_t<!_isArray, T> operator-(const T &value)
  {
    return values[currentView][0] - value;
  }

  template <bool _isArray = isArray>
  eastl::enable_if_t<!_isArray, T> operator&(const T &value)
  {
    return values[currentView][0] & value;
  }

  template <bool _isArray = isArray>
  eastl::enable_if_t<!_isArray, T *> operator->()
  {
    return &values[currentView][0];
  }

  template <bool _isArray = isArray>
  eastl::enable_if_t<!_isArray, const T *> operator->() const
  {
    return &values[currentView][0];
  }

  template <bool _isArray = isArray>
  eastl::enable_if_t<!_isArray, T &> current()
  {
    return values[currentView][0];
  }

  template <bool _isArray = isArray>
  eastl::enable_if_t<!_isArray, const T &> current() const
  {
    return values[currentView][0];
  }

  template <typename Func>
  void forEach(Func &&func)
  {
    forTheFirstN(viewCount, eastl::forward<Func>(func));
  }

  template <typename Func>
  auto forTheFirstN(int n, Func &&func) -> eastl::enable_if_t<eastl::is_invocable_v<Func, T &>>
  {
    for (int viewIndex = 0; viewIndex < n; ++viewIndex)           //-V1008
      for (int itemIndex = 0; itemIndex < itemCount; ++itemIndex) //-V1008
        eastl::forward<Func>(func)(values[viewIndex][itemIndex]);
  }

  template <typename Func>
  auto forTheFirstN(int n, Func &&func) -> eastl::enable_if_t<eastl::is_invocable_v<Func, T &, int>>
  {
    for (int viewIndex = 0; viewIndex < n; ++viewIndex)           //-V1008
      for (int itemIndex = 0; itemIndex < itemCount; ++itemIndex) //-V1008
        eastl::forward<Func>(func)(values[viewIndex][itemIndex], viewIndex);
  }

  template <typename Func>
  auto forTheFirstN(int n, Func &&func) -> eastl::enable_if_t<eastl::is_invocable_v<Func, T &, int, int>>
  {
    for (int viewIndex = 0; viewIndex < n; ++viewIndex)           //-V1008
      for (int itemIndex = 0; itemIndex < itemCount; ++itemIndex) //-V1008
        eastl::forward<Func>(func)(values[viewIndex][itemIndex], viewIndex, itemIndex);
  }

  void setCurrentView(int view) { currentView = view; }
};

template <typename T, int view_count, int item_count>
inline T operator-(T a, const ViewDependentResource<T, view_count, item_count> &b)
{
  return a - b.current();
}
