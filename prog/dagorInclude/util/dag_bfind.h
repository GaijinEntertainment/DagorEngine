//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_span.h>

//! finds index of element by key in sorted table using binary search algorithm
//! and T::cmpGt() and T::cmpEq() comparators;
//! returns index>=0 when element found, or -(insertPos+1) when element not found
//! NOTE: there must be only one instance of each key in table
//! NOTE: table assumed to be non-empty
template <class T, class T2>
inline int bfind_idx(dag::ConstSpan<T> table, T2 key)
{
  int lo = 0, hi = table.size() - 1, m = 0;
  const T *__restrict e = table.data();

  // check boundaries
  if (T::cmpGt(e, key))
    return -1;
  if (T::cmpEq(e, key))
    return 0;

  e = &table[hi];
  if (T::cmpEq(e, key))
    return hi;
  if (!T::cmpGt(e, key))
    return -(hi + 1 + 1);

  // binary search
  while (lo < hi)
  {
    m = (lo + hi) >> 1;
    e = &table[m];

    if (T::cmpEq(e, key))
      return m;
    if (m == lo)
      return -(hi + 1);

    if (T::cmpGt(e, key))
      hi = m;
    else
      lo = m;
  }
  return -(hi + 1);
}

//! the same as bfind_idx(), but supports multiple instances of key in table;
//! returns index of first instance of key
template <class T, class T2>
inline int bfind_rep_idx(dag::ConstSpan<T> table, T2 key)
{
  int lo = 0, hi = table.size() - 1, m = 0;
  const T *__restrict e = table.data();

  // check boundaries
  if (T::cmpGt(e, key))
    return -1;
  if (T::cmpEq(e, key))
    return 0;

  e = &table[hi];
  if (T::cmpEq(e, key))
  {
    while (hi > 0 && T::cmpEq(e - 1, key))
    {
      e--;
      hi--;
    }
    return hi;
  }
  if (!T::cmpGt(e, key))
    return -(hi + 1 + 1);

  // binary search
  while (lo < hi)
  {
    m = (lo + hi) >> 1;
    e = &table[m];

    if (T::cmpEq(e, key))
    {
      while (T::cmpEq(e - 1, key))
      {
        e--;
        m--;
        if (!m)
          break;
      }
      return m;
    }
    if (m == lo)
      return -(hi + 1);

    if (T::cmpGt(e, key))
      hi = m;
    else
      lo = m;
  }
  return -(hi + 1);
}

//! the same as bfind_idx(), but allow empty table
template <class T, class T2>
inline int bfind_idx_safe(dag::ConstSpan<T> table, T2 key)
{
  if (!table.size())
    return -1;
  return bfind_idx(table, key);
}

//! the same as bfind_rep_idx(), but allow empty table
template <class T, class T2>
inline int bfind_rep_idx_safe(dag::ConstSpan<T> table, T2 key)
{
  if (!table.size())
    return -1;
  return bfind_rep_idx(table, key);
}

// templates specialized for vectors (via conversion to dag::ConstSpan)
template <class V, class T2, typename U = typename V::value_type>
inline int bfind_idx(const V &v, T2 key)
{
  return bfind_idx<U>(make_span_const(v), key);
}
template <class V, class T2, typename U = typename V::value_type>
inline int bfind_rep_idx(const V &v, T2 key)
{
  return bfind_rep_idx<U>(make_span_const(v), key);
}
template <class V, class T2, typename U = typename V::value_type>
inline int bfind_idx_safe(const V &v, T2 key)
{
  return bfind_idx_safe<U>(make_span_const(v), key);
}
template <class V, class T2, typename U = typename V::value_type>
inline int bfind_rep_idx_safe(const V &v, T2 key)
{
  return bfind_rep_idx_safe<U>(make_span_const(v), key);
}

//! return uint16_t data corresponding to uint16_t key (uses binary search in packed map (HIGH16:LOW16)=data:key)
static inline int bfind_packed_uint16_x2(dag::ConstSpan<uint32_t> map, int key, int def_val = -1)
{
  int lo = 0, hi = map.size() - 1, m;
  int cmp;

  // check bounds first
  if (hi < 0)
    return def_val;

  cmp = key - (map[0] & 0xFFFF);
  if (cmp < 0)
    return def_val;
  if (cmp == 0)
    return map[0] >> 16;

  if (hi != 0)
  {
    cmp = key - (map[hi] & 0xFFFF);
    if (cmp == 0)
      return map[hi] >> 16;
  }
  if (cmp > 0)
    return def_val;

  // binary search
  while (lo < hi)
  {
    m = (lo + hi) >> 1;
    cmp = key - (map[m] & 0xFFFF);

    if (cmp == 0)
      return map[m] >> 16;
    if (m == lo)
      return def_val;

    if (cmp < 0)
      hi = m;
    else
      lo = m;
  }
  return def_val;
}

//! same as bfind_packed_uint16_x2, but faster in case of map.size()<=4
static inline int bfind_packed_uint16_x2_fast(dag::ConstSpan<uint32_t> map, int key)
{
  switch (4 - map.size())
  {
    case 0:
      if ((map[3] & 0xFFFF) == key)
        return map[3] >> 16;
    case 1:
      if ((map[2] & 0xFFFF) == key)
        return map[2] >> 16;
    case 2:
      if ((map[1] & 0xFFFF) == key)
        return map[1] >> 16;
    case 3:
      if ((map[0] & 0xFFFF) == key)
        return map[0] >> 16;
    case 4: return -1;
  }
  return bfind_packed_uint16_x2(map, key);
}
