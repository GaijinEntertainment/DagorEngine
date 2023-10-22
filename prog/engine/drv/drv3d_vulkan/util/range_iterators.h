#pragma once

// allows range based for loops, move to the appropiate location?
template <typename T>
inline T *begin(dag::Span<T> slice)
{
  return slice.data();
}

template <typename T>
inline T *end(dag::Span<T> slice)
{
  return slice.data() + slice.size();
}

template <typename T, size_t N>
inline T *begin(StaticTab<T, N> &slice)
{
  return slice.data();
}

template <typename T, size_t N>
inline T *end(StaticTab<T, N> &slice)
{
  return slice.data() + slice.size();
}

template <typename T, int N>
inline T *begin(carray<T, N> &slice)
{
  return slice.data();
}

template <typename T, int N>
inline T *end(carray<T, N> &slice)
{
  return slice.data() + slice.size();
}

template <typename T, size_t N>
inline const T *begin(const StaticTab<T, N> &slice)
{
  return slice.data();
}

template <typename T, size_t N>
inline const T *end(const StaticTab<T, N> &slice)
{
  return slice.data() + slice.size();
}

template <typename T, int N>
inline const T *begin(const carray<T, N> &slice)
{
  return slice.data();
}

template <typename T, int N>
inline const T *end(const carray<T, N> &slice)
{
  return slice.data() + slice.size();
}