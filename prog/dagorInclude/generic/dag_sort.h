//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_stlqsort.h>

namespace dag
{

template <typename T>
struct OpaqueData
{
  alignas(T) char data[sizeof(T)];
};

template <typename T>
struct SortFuncPredicate
{
  typedef int (*typed_cmp_func_t)(const T *, const T *);
  typed_cmp_func_t func;
  SortFuncPredicate(typed_cmp_func_t f) : func(f) {}
  bool operator()(const T &a, const T &b) const { return (*func)(&a, &b) < 0; }
};

template <typename T, class D, class S = SortFuncPredicate<D>, typename CMP = typename SortFuncPredicate<T>::typed_cmp_func_t>
static inline void sortOpaque(T *s, T *e, CMP f)
{
  stlsort::sort((D *)s, (D *)e, S((typename SortFuncPredicate<D>::typed_cmp_func_t)f));
}

template <typename T, class Predicate>
struct SortPredicate
{
  const Predicate &predicate;
  SortPredicate(const Predicate &p) : predicate(p) {}
  bool operator()(const T &a, const T &b) const { return predicate.order(a, b) < 0; }
};
template <typename T, class Predicate>
inline void sort(T *s, T *e, const Predicate &compare)
{
  stlsort::sort(s, e, SortPredicate<T, Predicate>(compare));
}

} // namespace dag

template <class V, typename T = typename V::value_type>
inline void sort(V &v, uint32_t at, uint32_t n, typename dag::SortFuncPredicate<T>::typed_cmp_func_t cmp)
{
  T *p = v.data() + at;
  switch (sizeof(T))
  {
    case 4: return dag::sortOpaque<T, int32_t>(p, p + n, cmp);
    case 8: return dag::sortOpaque<T, int64_t>(p, p + n, cmp);
    case 2: return dag::sortOpaque<T, int16_t>(p, p + n, cmp);
    case 1: return dag::sortOpaque<T, int8_t>(p, p + n, cmp);
    default: break;
  }
  return dag::sortOpaque<T, typename dag::OpaqueData<T>>(p, p + n, cmp);
}

template <class V, typename T = typename V::value_type>
inline void sort(V &v, typename dag::SortFuncPredicate<T>::typed_cmp_func_t cmp)
{
  sort<V, T>(v, 0u, v.size(), cmp);
}

template <class V, class Predicate, typename T = typename V::value_type>
inline void sort(V &v, uint32_t at, uint32_t n, const Predicate &c)
{
  dag::sort(v.data() + at, v.data() + at + n, c);
}
template <class V, class Predicate, typename T = typename V::value_type>
inline void sort(V &v, const Predicate &c)
{
  dag::sort(v.data(), v.data() + v.size(), c);
}

template <class V, typename T = typename V::value_type, class Predicate = typename V::Predicate>
inline void sort(V &v, uint32_t at, uint32_t n)
{
  dag::sort(v.data() + at, v.data() + at + n, Predicate());
}
template <class V, typename T = typename V::value_type, class Predicate = typename V::Predicate>
inline void sort(V &v)
{
  dag::sort(v.data(), v.data() + v.size(), Predicate());
}

template <class V, class Compare, typename T = typename V::value_type>
inline void fast_sort(V &v, uint32_t at, uint32_t n, const Compare &c)
{
  stlsort::sort(v.data() + at, v.data() + at + n, c);
}
template <class V, class Compare, typename T = typename V::value_type>
inline void fast_sort(V &v, const Compare &c)
{
  stlsort::sort(v.data(), v.data() + v.size(), c);
}
