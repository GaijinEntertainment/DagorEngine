//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <string.h>
#include <debug/dag_assert.h>
#include <generic/dag_span.h>
#include <EASTL/type_traits.h>


inline int lup(const char *str, const char *const ar[], int count, int def = -1)
{
  G_ASSERT(str);
  if (str)
    for (int i = 0; i < count; ++i)
      if (strcmp(ar[i], str) == 0)
        return i;
  return def;
}


template <typename T>
inline auto lup(const char *str, const T &ar, int def = -1) -> decltype(eastl::declval<T>().size(), int())
{
  G_ASSERT(str);
  if (str)
    for (int i = 0; i < ar.size(); ++i)
      if (strcmp(ar[i], str) == 0)
        return i;
  return def;
}

inline int lupn(const char *str, int len, const char *const ar[], int count, int def = -1)
{
  G_ASSERT(str);
  if (str)
    for (int i = 0; i < count; ++i)
      if (strncmp(ar[i], str, len) == 0)
        return i;
  return def;
}

template <typename T>
inline int lupn(const char *str, int len, dag::ConstSpan<T> ar, int def = -1)
{
  G_ASSERT(str);
  if (str)
    for (int i = 0; i < ar.size(); ++i)
      if (strncmp(ar[i], str, len) == 0)
        return i;
  return def;
}

template <typename T>
struct Record
{
  const char *str;
  T val;
};

template <typename T>
inline int get_rec_count(const Record<T> *recs)
{
  G_ASSERT(recs);
  int cnt = 0;
  for (const Record<T> *rec = recs; rec->str; ++rec)
    ++cnt;
  return cnt;
}

template <typename T, bool canFail>
static inline int lookup_recf(const Record<T> *recs, const char *str)
{
  G_ASSERT_RETURN(str, canFail ? -1 : 0);
  G_ASSERT(get_rec_count(recs) <= 1024); // sanity check
  int i = 0;
  for (const Record<T> *rec = recs; rec->str; ++rec, ++i)
    if (strcmp(rec->str, str) == 0)
      return i;
  G_ASSERTF(canFail, "could not look up str '%s'", str);
  return canFail ? -1 : 0;
}

template <typename T>
static inline int lookup_rec(const Record<T> *recs, const char *str)
{
  return lookup_recf<T, false>(recs, str);
}

template <typename T, bool canFail>
static inline T get_rec_valuef(const Record<T> *recs, const char *str)
{
  G_ASSERT_RETURN(str, recs[0].val);
  G_ASSERT(get_rec_count(recs) <= 1024); // sanity check
  for (const Record<T> *rec = recs; rec->str; ++rec)
    if (strcmp(rec->str, str) == 0)
      return rec->val;
  G_ASSERTF(canFail, "could not get record val for str '%s'", str);
  return recs[0].val;
}

template <typename T>
static inline T get_rec_value(const Record<T> *recs, const char *str)
{
  return get_rec_valuef<T, false>(recs, str);
}

template <typename T>
static inline T get_rec_value_ex(const Record<T> *recs, const char *str, T default_value)
{
  int idx = lookup_recf<T, true>(recs, str);
  return (idx >= 0) ? recs[idx].val : default_value;
}
