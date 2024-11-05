//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <memory/dag_memBase.h>

#include <supp/dag_define_KRNLIMP.h>

#ifdef __cplusplus
extern "C"
{
#endif

  typedef int (*cmp_func_t)(const void *, const void *);

  /// quick sorting, see standard C qsort()
  KRNLIMP void dag_qsort(void *p, intptr_t n, int w, cmp_func_t f);

  /// binary search, see standard C bsearch()
  KRNLIMP void *dag_bin_search(const void *key, const void *p, intptr_t n, int w, cmp_func_t f);

#ifdef __cplusplus
}

template <class T, class Predicate>
inline const T *data_bin_search(const T &k, const T *p, intptr_t n, const Predicate &predicate)
{
  (void)(predicate); // to make vc happy
  if (!p)
    return NULL;
  if (n <= 4)
  {
    for (int i = 0; i < n; ++i, ++p)
    {
      if (predicate.order(k, *p) == 0)
        return p;
    }
    return NULL;
  }
  int a = 0, b = n - 1;
  int v = predicate.order(k, *p);
  if (!v)
    return p;
  else if (v < 0)
    return NULL;
  v = predicate.order(k, p[b]);
  if (v == 0)
    return p + b;
  else if (v > 0)
    return NULL;
  while (a <= b)
  {
    int c = (a + b) / 2;
    if (c == a)
    {
      if (predicate.order(k, p[a]) == 0)
        return p + a;
      return NULL;
    }
    v = predicate.order(k, p[c]);
    if (v == 0)
      return p + c;
    else if (v < 0)
      b = c;
    else
      a = c;
  }
  return NULL;
}

template <class T, class Predicate>
inline intptr_t tab_sorted_insert(T *&data, uint32_t &total, uint32_t &num, IMemAlloc *mem, const T &val, int step,
  const Predicate &predicate)
{
  (void)(predicate); // to make vc happy
  if (!data || num <= 0)
    return dag_tab_insert(data, total, num, mem, num, 1, sizeof(T), &val, step);
  int a = 0, b = num - 1;
  if (predicate.order(val, *data) <= 0)
    return dag_tab_insert(data, total, num, mem, 0, 1, sizeof(T), &val, step);
  if (predicate.order(data[b], val) <= 0)
    return dag_tab_insert(data, total, num, mem, num, 1, sizeof(T), &val, step);

  while (a < b)
  {
    int c = (a + b) / 2;
    if (c == a)
      return dag_tab_insert(data, total, num, mem, a + 1, 1, sizeof(T), &val, step);
    int v = predicate.order(val, data[c]);
    if (v == 0)
      return dag_tab_insert(data, total, num, mem, c, 1, sizeof(T), &val, step);
    else if (v < 0)
      b = c;
    else
      a = c;
  }
  return dag_tab_insert(data, total, num, mem, a + 1, 1, sizeof(T), &val, step);
}


KRNLIMP void *dag_tab_insert2(void *ptr, uint32_t &total, uint32_t &used, IMemAlloc *mem, intptr_t at, intptr_t n, intptr_t sz,
  intptr_t step, uint32_t &out_idx);


template <typename T>
intptr_t dag_tab_insert(T *&ptr, uint32_t &total, uint32_t &used, IMemAlloc *mem, intptr_t at, intptr_t n, intptr_t sz, const T *p,
  intptr_t step)
{
  uint32_t out_idx;
  ptr = (T *)dag_tab_insert2(ptr, total, used, mem, at, n, sz, step, out_idx);
  if (p)
    for (T *dest = ptr + out_idx, *dest_e = dest + n; dest < dest_e; dest++, p++)
      new (dest, _NEW_INPLACE) T(*p);
  return out_idx;
}

#endif

#include <supp/dag_undef_KRNLIMP.h>
