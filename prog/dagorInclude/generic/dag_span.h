//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <string.h>
#include <util/dag_stdint.h>

namespace dag
{
template <typename T>
class Span;
template <typename T>
using ConstSpan = dag::Span<T const>;

template <typename T1, typename T2>
struct is_same
{};
template <typename T1>
struct is_same<T1, T1>
{
  static constexpr bool is_true = true;
};
} // namespace dag

#ifdef _DEBUG_TAB_
#include <util/dag_globDef.h>
inline DAGOR_NOINLINE void span_index_assert([[maybe_unused]] const char *name, [[maybe_unused]] int idx, [[maybe_unused]] int dcnt,
  [[maybe_unused]] const void *self, [[maybe_unused]] const void *dptr, [[maybe_unused]] int szt)
{
  G_ASSERT_FAIL("Index out of bounds: %s[%d] dcnt=%d this=0x%p dptr=0x%p sizeof(T)=%d", name, idx, dcnt, self, dptr, szt);
}
#define GENSLICE_VALIDATE_IDX(idx)             \
  if (DAGOR_UNLIKELY((unsigned)idx >= size())) \
    span_index_assert("Span", idx, dcnt, this, dptr, sizeof(T));
#else
#define GENSLICE_VALIDATE_IDX(idx) ((void)0)
#endif

template <typename T>
class dag::Span
{
public:
  typedef T value_type;
  typedef T *iterator;
  typedef const T *const_iterator;
  typedef T &reference;
  typedef const T &const_reference;

public:
  /// Default constructor. Pointer is initialized to NULL and number of elements to 0.
  constexpr Span() = default;

  /// Copy constructor
  constexpr Span(const Span &gs) = default;
  constexpr Span(Span &&gs) = default;

  /// Construct from pointer and number of elements.
  constexpr Span(T *p, intptr_t n)
  {
    dptr = p;
    dcnt = n;
  }
  /// Construct from vector/container (to ConstSpan<T>==Span<T const> only!)
  template <typename V, typename U = T, bool s = dag::is_same<U, typename V::value_type const>::is_true>
  constexpr Span(const V &v) : Span(v.data(), v.size())
  {}

  // operator =
  constexpr Span &operator=(Span &&gs) = default;
  constexpr Span &operator=(const Span &gs) = default;
  template <typename V, typename U = T, bool s = dag::is_same<U, typename V::value_type const>::is_true>
  constexpr Span &operator=(const V &v)
  {
    dptr = v.data();
    dcnt = v.size();
    return *this;
  }

  /// Index operator
  __forceinline constexpr const T &operator[](uint32_t idx) const
  {
    GENSLICE_VALIDATE_IDX(idx);
    return dptr[idx];
  }
  __forceinline constexpr T &operator[](uint32_t idx)
  {
    GENSLICE_VALIDATE_IDX(idx);
    return dptr[idx];
  }

  /// Explicit indexing
  constexpr const T &at(uint32_t idx) const
  {
    GENSLICE_VALIDATE_IDX(idx);
    return dptr[idx];
  }
  constexpr T &at(uint32_t idx)
  {
    GENSLICE_VALIDATE_IDX(idx);
    return dptr[idx];
  }

  constexpr Span subspan(uint32_t ofs, uint32_t cnt)
  {
#ifdef _DEBUG_TAB_
    if (cnt)
    {
      operator[](ofs);
      operator[](ofs + cnt - 1);
    }
#endif
    return Span(data() + ofs, cnt);
  }
  constexpr Span subspan(uint32_t ofs) { return subspan(ofs, size() - ofs); }
  constexpr Span first(uint32_t cnt) { return subspan(0, cnt); }
  constexpr Span last(uint32_t cnt) { return subspan(size() - cnt, cnt); }

  constexpr Span<T const> subspan(uint32_t ofs, uint32_t cnt) const
  {
#ifdef _DEBUG_TAB_
    if (cnt)
    {
      operator[](ofs);
      operator[](ofs + cnt - 1);
    }
#endif
    return Span<T const>(data() + ofs, cnt);
  }
  constexpr Span<T const> subspan(uint32_t ofs) const { return subspan(ofs, size() - ofs); }
  constexpr Span<T const> first(uint32_t cnt) const { return subspan(0, cnt); }
  constexpr Span<T const> last(uint32_t cnt) const { return subspan(size() - cnt, cnt); }

  //
  // STL compatibility
  //

  constexpr iterator begin() const { return dptr; }
  constexpr const_iterator cbegin() const { return dptr; }

  constexpr iterator end() const { return dptr + dcnt; }
  constexpr const_iterator cend() const { return dptr + dcnt; }

  constexpr reference front()
  {
    GENSLICE_VALIDATE_IDX(0);
    return *dptr;
  }
  constexpr const_reference front() const
  {
    GENSLICE_VALIDATE_IDX(0);
    return *dptr;
  }
  constexpr reference back()
  {
    GENSLICE_VALIDATE_IDX(0);
    return dptr[dcnt - 1];
  }
  constexpr const_reference back() const
  {
    GENSLICE_VALIDATE_IDX(0);
    return dptr[dcnt - 1];
  }

  constexpr const T *data() const { return dptr; }
  constexpr T *data() { return dptr; }
  constexpr uint32_t size() const { return (uint32_t)dcnt; }
  constexpr uint32_t capacity() const { return (uint32_t)dcnt; }
  constexpr bool empty() const { return dcnt == 0; }

  constexpr void set(Span<T> s)
  {
    Span<T>::dptr = s.data();
    Span<T>::dcnt = s.size();
  }
  constexpr void set(T *p, intptr_t c)
  {
    Span<T>::dptr = p;
    Span<T>::dcnt = c;
  }
  constexpr void reset()
  {
    Span<T>::dptr = NULL;
    Span<T>::dcnt = 0;
  }

protected:
  T *dptr = nullptr;
  intptr_t dcnt = 0;
};

template <class T>
inline constexpr dag::Span<T> make_span(T *p, intptr_t n)
{
  return dag::Span<T>(p, n);
}

template <class T>
inline constexpr dag::ConstSpan<T> make_span_const(const T *p, intptr_t n)
{
  return dag::ConstSpan<T>(p, n);
}

template <class V, typename T = typename V::value_type>
inline constexpr dag::Span<T> make_span(V &v)
{
  return make_span<T>(v.data(), v.size());
}
template <class V, typename T = typename V::value_type>
inline constexpr dag::ConstSpan<T> make_span_const(const V &v)
{
  return make_span_const(v.data(), v.size());
}

template <class T, size_t N>
inline constexpr dag::Span<T> make_span(T (&array)[N])
{
  return dag::Span<T>(array, N);
}
template <class T, size_t N>
inline constexpr dag::ConstSpan<T> make_span_const(const T (&array)[N])
{
  return dag::ConstSpan<T>(array, N);
}

#ifdef _DEBUG_TAB_
#undef GENSLICE_VALIDATE_IDX
#endif

template <typename V, typename T = typename V::value_type>
inline constexpr uint32_t elem_size(const V &)
{
  return (uint32_t)sizeof(T);
}
template <typename V, typename T = typename V::value_type>
inline constexpr uint32_t data_size(const V &v)
{
  return v.size() * (uint32_t)sizeof(T);
}

template <typename V>
inline constexpr const auto *safe_at(const V &v, uint32_t idx)
{
  return idx < v.size() ? v.data() + idx : nullptr;
}
template <typename V>
inline constexpr auto *safe_at(V &v, uint32_t idx)
{
  return idx < v.size() ? v.data() + idx : nullptr;
}

template <class V, class T1, typename U = typename V::value_type>
inline constexpr int find_value_idx(const V &v, T1 const &val)
{
  for (const U *p = v.cbegin(), *e = v.cend(); p < e; p++)
    if (*p == val)
      return (intptr_t)(p - v.cbegin());
  return -1;
}

// @NOTE: can't be made constexpr, since we don't have constexpr cstring yet
template <class V, typename U = typename V::value_type>
inline void mem_set_0(V &v)
{
  memset(v.data(), 0, data_size(v));
}
template <class V, typename U = typename V::value_type>
inline void mem_set_ff(V &v)
{
  memset(v.data(), 0xFF, data_size(v));
}

template <class V, typename U = typename V::value_type>
inline void mem_copy_to(const V &v, void *mem_dest)
{
  memcpy(mem_dest, v.data(), data_size(v));
}
template <class V, typename U = typename V::value_type>
inline void mem_copy_from(V &v, const void *mem_src)
{
  memcpy(v.data(), mem_src, data_size(v));
}
template <class V, typename U = typename V::value_type>
inline bool mem_eq(const V &v, const void *mem_cmp)
{
  return memcmp(v.data(), mem_cmp, data_size(v)) == 0;
}

template <class T>
inline void mem_set_0(dag::Span<T> v)
{
  memset(v.data(), 0, data_size(v));
}
template <class T>
inline void mem_set_ff(dag::Span<T> v)
{
  memset(v.data(), 0xFF, data_size(v));
}
template <class T>
inline void mem_copy_from(dag::Span<T> v, const void *mem_src)
{
  memcpy(v.data(), mem_src, data_size(v));
}

template <class V, typename = typename V::allocator_type>
inline constexpr void clear_and_shrink(V &v)
{
  v = V(v.get_allocator());
}

template <class V, typename T = typename V::value_type>
inline constexpr void clear_and_resize(V &v, uint32_t sz)
{
  if (sz == v.size())
    return;
  v.clear();
  v.resize(sz);
}

template <class V, typename T = typename V::value_type>
inline constexpr void erase_items(V &v, uint32_t at, uint32_t n)
{
  if (n == 1) // This branch typically should be optimized away
    v.erase(v.begin() + at);
  else
    v.erase(v.begin() + at, v.begin() + at + n);
}
template <class V, typename T = typename V::value_type>
inline constexpr void erase_items_fast(V &v, uint32_t at, uint32_t n)
{
  if (n == 1) // This branch typically should be optimized away
    v.erase_unsorted(v.begin() + at);
  else
    v.erase(v.begin() + at, v.begin() + at + n);
}
template <class V, typename T = typename V::value_type>
inline constexpr void safe_erase_items(V &v, int32_t at, int32_t n)
{
  if (at < 0)
  {
    n += at;
    at = 0;
  }
  if (n <= 0)
    return;
  if (at + n > v.size())
    n = v.size() - at;
  if (n <= 0)
    return;
  v.erase(v.begin() + at, v.begin() + at + n);
}
template <class V, typename T = typename V::value_type, int TSZ = sizeof(*(T) nullptr)>
inline constexpr void erase_ptr_items(V &v, uint32_t at, uint32_t n)
{
#ifdef _DEBUG_TAB_
  if (n)
    make_span_const(v).operator[](at), make_span_const(v).operator[](at + n - 1);
#endif
  for (uint32_t i = at; i < at + n; i++)
    delete v.data()[i];
  v.erase(v.begin() + at, v.begin() + at + n);
}

template <class V, typename T = typename V::value_type, int TSZ = sizeof(*(T) nullptr)>
inline constexpr void clear_all_ptr_items(V &v)
{
  erase_ptr_items(v, 0, v.size());
}
template <class V, typename T = typename V::value_type, int TSZ = sizeof(*(T) nullptr)>
inline constexpr void clear_all_ptr_items_and_shrink(V &v)
{
  erase_ptr_items(v, 0, v.size());
  v.shrink_to_fit();
}

template <class V, typename T1, typename U = typename V::value_type>
inline constexpr bool erase_item_by_value(V &v, T1 const &val)
{
  intptr_t idx = find_value_idx(v, val);
  if (idx < 0)
    return false;
  v.erase(v.begin() + idx);
  return true;
}

template <class V, typename T = typename V::value_type>
inline constexpr uint32_t insert_items(V &v, uint32_t at, uint32_t n)
{
  v.insert_default(v.begin() + at, n);
  return at;
}
template <class V, typename T = typename V::value_type>
inline constexpr uint32_t insert_items(V &v, uint32_t at, uint32_t n, const T *p)
{
  if (p)
    v.insert(v.begin() + at, p, p + n);
  else
    return insert_items(v, at, n);
  return at;
}
template <class V, typename T, typename U = typename V::value_type>
inline constexpr uint32_t insert_item_at(V &v, uint32_t at, const T &p)
{
  v.insert(v.begin() + at, p);
  return at;
}

template <class V, typename T = typename V::value_type>
inline constexpr uint32_t append_items(V &v, uint32_t n)
{
  auto at = v.size();
  v.append_default(n);
  return at;
}
template <class V, typename T = typename V::value_type>
inline constexpr uint32_t append_items(V &v, uint32_t n, const T *p)
{
  return insert_items(v, v.size(), n, p);
}

template <class V>
inline constexpr void reserve_and_resize(V &v, typename V::size_type sz)
{
  v.reserve(sz);
  v.resize(sz);
}
