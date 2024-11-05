// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace eastl
{
template <typename R>
class function;
template <typename _Tp, typename _Dp>
class unique_ptr;
template <typename _Tp>
struct default_delete;
}; // namespace eastl

namespace da_profiler
{
using eastl::default_delete;
using eastl::function;
using eastl::unique_ptr;
template <typename T>
inline void move_swap(T &a, T &b)
{
  T temp((T &&) a);
  a = (T &&) b;
  b = (T &&) temp;
}
/*
  template< class T > struct remove_reference      { typedef T type; };
  template< class T > struct remove_reference<T&>  { typedef T type; };
  template< class T > struct remove_reference<T&&> { typedef T type; };
  template< class T > constexpr std::remove_reference_t<T>&& move( T&& t ) noexcept
    {return static_cast<typename std::remove_reference<T>::type&&>(t);}*/
} // namespace da_profiler