// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <util/dag_tabHlp.h>
#include <util/dag_stdint.h>
#include <debug/dag_fatal.h>

static void *simplesearch(const void *k, const void *ptr, int n, int w)
{
  char *p = (char *)ptr;
  for (int i = 0; i < n; ++i, p += w)
    if (memcmp(k, p, w) == 0)
      return p;
  return NULL;
}

void *dag_bin_search(const void *k, const void *ptr, intptr_t n, int w, int (*f)(const void *, const void *))
{
  if (!ptr || !k || n <= 0)
    return NULL;
  else if (!f)
    return simplesearch(k, ptr, n, w);
  int a = 0, b = n - 1;
  const char *p = (const char *)ptr;
  int v = f(k, p);
  if (v == 0)
    return (void *)p;
  else if (v < 0)
    return NULL;
  v = f(k, p + b * w);
  if (v == 0)
    return (void *)(p + b * w);
  else if (v > 0)
    return NULL;
  while (a <= b)
  {
    int c = (a + b) / 2;
    if (c == a)
    {
      if (f(k, p + a * w) == 0)
        return (void *)(p + a * w);
      return NULL;
    }
    v = f(k, p + c * w);
    if (v == 0)
      return (void *)(p + c * w);
    else if (v < 0)
      b = c;
    else
      a = c;
  }
  return NULL;
}

template <class T>
static void __sort(char *a, int n, int w, int (*f)(const void *, const void *))
{
  if (n <= 1)
    return;

  int l = 1, r = n;
  T::swap(a + 0 * w, a + (n >> 1) * w, w);

  const void *pivot = a;

  while (l < r)
    if (f(a + l * w, pivot) <= 0)
      ++l;
    else
    {
      for (--r; l < r && f(a + r * w, pivot) >= 0; --r)
        ;

      T::swap(a + l * w, a + r * w, w);
    }

  --l;
  T::swap(a + 0 * w, a + l * w, w);

  if (l <= 1)
    __sort<T>(a + r * w, n - r, w, f);
  else if (n - r <= 1)
    __sort<T>(a, l, w, f);
  else
  {
    __sort<T>(a, l, w, f);
    __sort<T>(a + r * w, n - r, w, f);
  }
}

namespace tabsorted
{
template <typename T>
struct SwapX
{
  static inline void swap(void *l, void *r, int w)
  {
    T *ql = (T *)l, *qr = (T *)r, tmp;
    for (int i = w / sizeof(T); i > 0; ql++, qr++, i--)
    {
      tmp = *ql;
      *ql = *qr;
      *qr = tmp;
    }
  }
};
template <typename T>
struct Swap
{
  static inline void swap(void *l, void *r, int)
  {
    T *ql = (T *)l, *qr = (T *)r, tmp;
    tmp = *ql;
    *ql = *qr;
    *qr = tmp;
  }
};
} // namespace tabsorted

extern "C" void dag_qsort(void *p, intptr_t n, int w, int (*f)(const void *, const void *))
{
  if (!p || n < 2 || !w || !f)
    return;

  if (w == 4)
    __sort<tabsorted::Swap<uint32_t>>((char *)p, n, w, f);
  else if ((w & 0x7) == 0)
    __sort<tabsorted::SwapX<uint64_t>>((char *)p, n, w, f);
  else if ((w & 0x3) == 0)
    __sort<tabsorted::SwapX<uint32_t>>((char *)p, n, w, f);
  else if (w == 2)
    __sort<tabsorted::Swap<uint16_t>>((char *)p, n, w, f);
  else if (w == 1)
    __sort<tabsorted::Swap<uint8_t>>((char *)p, n, w, f);
  else
    DAG_FATAL("dag_qsort not implemented for w=%d", w);
}

#define EXPORT_PULL dll_pull_baseutil_tabSorted
#include <supp/exportPull.h>
