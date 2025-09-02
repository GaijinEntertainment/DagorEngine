//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <cstdint>
#include <EASTL/type_traits.h>

namespace dag
{

template <class T>
constexpr T *align_up(T *d, size_t alignment)
{
  alignment--;
  return (T *)((uintptr_t(d) + alignment) & ~alignment); //-V558
}

template <class T>
  requires eastl::is_integral_v<T>
constexpr T align_up(T d, size_t alignment)
{
  alignment--;
  return (d + alignment) & ~(T(alignment));
}

template <class TypeToAlign, class T>
  requires eastl::is_integral_v<T>
constexpr T align_up(T d)
{
  return align_up(d, alignof(TypeToAlign));
}

template <class T>
constexpr T *align_down(T *d, size_t alignment)
{
  alignment--;
  return (T *)((uintptr_t(d)) & ~alignment); //-V558
}

template <class T>
  requires eastl::is_integral_v<T>
constexpr T align_down(T d, size_t alignment)
{
  alignment--;
  return d & ~T(alignment);
}

template <class TypeToAlign, class T>
  requires eastl::is_integral_v<T>
constexpr T align_down(T d)
{
  return align_down(d, alignof(TypeToAlign));
}

} // namespace dag
