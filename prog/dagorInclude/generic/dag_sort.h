//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stlqsort.h>
#include <debug/dag_assert.h>

namespace dag
{

template <typename T>
struct OpaqueData
{
  alignas(T) char data[sizeof(T)];
};

template <typename T, typename C>
struct ValidateSortComparator
{
  const C &cmp;
  ValidateSortComparator(const C &c) : cmp(c) {}
  bool operator()(const T &a, const T &b) const
  {
    if (cmp(a, b))
    {
      // If a < b then !(b < a)
      if constexpr (eastl::is_arithmetic_v<T> || eastl::is_pointer_v<T>)
        G_ASSERTF(!cmp(b, a), "Invalid sort comparator: %@ %@", a, b);
      else
        G_ASSERTF(!cmp(b, a), "Invalid sort comparator: %p %p", &a, &b);
      return true;
    }
    return false;
  }
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
#ifdef _DEBUG_TAB_
  stlsort::sort((D *)s, (D *)e, ValidateSortComparator<D, S>(S((typename SortFuncPredicate<D>::typed_cmp_func_t)f)));
#else
  stlsort::sort((D *)s, (D *)e, S((typename SortFuncPredicate<D>::typed_cmp_func_t)f));
#endif
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
#ifdef _DEBUG_TAB_
  stlsort::sort(s, e, ValidateSortComparator<T, SortPredicate<T, Predicate>>(SortPredicate<T, Predicate>(compare)));
#else
  stlsort::sort(s, e, SortPredicate<T, Predicate>(compare));
#endif
}

} // namespace dag

template <class V, typename T = typename V::value_type>
inline void sort(V &v, uint32_t at, uint32_t n, typename dag::SortFuncPredicate<T>::typed_cmp_func_t cmp)
{
  T *p = v.data() + at;
  if constexpr (eastl::is_scalar_v<T>)
#ifdef _DEBUG_TAB_
    return stlsort::sort(p, p + n, dag::ValidateSortComparator<T, dag::SortFuncPredicate<T>>(dag::SortFuncPredicate<T>(cmp)));
#else
    return stlsort::sort(p, p + n, dag::SortFuncPredicate<T>(cmp));
#endif

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
#ifdef _DEBUG_TAB_
  stlsort::sort(v.data() + at, v.data() + at + n, dag::ValidateSortComparator<T, Compare>(c));
#else
  stlsort::sort(v.data() + at, v.data() + at + n, c);
#endif
}
template <class V, class Compare, typename T = typename V::value_type>
inline void fast_sort(V &v, const Compare &c)
{
#ifdef _DEBUG_TAB_
  stlsort::sort(v.data(), v.data() + v.size(), dag::ValidateSortComparator<T, Compare>(c));
#else
  stlsort::sort(v.data(), v.data() + v.size(), c);
#endif
}
