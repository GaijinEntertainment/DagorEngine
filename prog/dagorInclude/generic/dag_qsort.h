//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_smallTab.h>
#include <generic/dag_tab.h>

// T is element, C must have static method int compare(const T&,const T&),
// that can be inline
template <class T, class C>
class Qsort
{
public:
  void __sort(T *a, int n)
  {
    if (n <= 1)
      return;

#define SWAP(l, r)                 \
  memcpy(tmp, a + l, sizeof(T));   \
  memcpy(a + l, a + r, sizeof(T)); \
  memcpy(a + r, tmp, sizeof(T));

    int l = 1, r = n;
    SWAP(0, (n >> 1))

    memcpy(pivot, a, sizeof(T));

    while (l < r)
    {
      if (C::compare(a[l], *(T *)pivot) <= 0)
        ++l;
      else
      {
        for (--r; l < r && C::compare(a[r], *(T *)pivot) >= 0; --r)
          ;

        SWAP(l, r)
      }
    }

    --l;
    SWAP(0, l)

    if (l <= 1)
      __sort(a + r, n - r);
    else if (n - r <= 1)
      __sort(a, l);
    else
    {
      __sort(a, l);
      __sort(a + r, n - r);
    }

//__sort(a, l);
//__sort(a + r, n - r);
#undef SWAP
  }

  void sort(T *p, int n)
  {
    if (!p || n < 2)
      return;
    __sort(p, n);
  }

protected:
  char pivot[sizeof(T)], tmp[sizeof(T)];
};

// T is element, C must have static method int compare(const T&,const T&),
// that can be inline
// SimpleQsort should be used only if T is small type that have def constructor and operator =
template <class T, class C>
class SimpleQsort
{
public:
  static void __sort(T *a, int n)
  {
    if (n <= 1)
      return;

#define SWAP(l, r)      \
  {                     \
    const T tmp = a[l]; \
    a[l] = a[r];        \
    a[r] = tmp;         \
  }

    int l = 1, r = n;
    SWAP(0, (n >> 1))

    const T pivot = a[0];

    while (l < r)
    {
      if (C::compare(a[l], pivot) <= 0)
        ++l;
      else
      {
        for (--r; l < r && C::compare(a[r], pivot) >= 0; --r)
          ;

        SWAP(l, r)
      }
    }

    --l;
    SWAP(0, l)

    if (l <= 1)
      __sort(a + r, n - r);
    else if (n - r <= 1)
      __sort(a, l);
    else
    {
      __sort(a, l);
      __sort(a + r, n - r);
    }

#undef SWAP
  }

  static void sort(T *p, int n)
  {
    if (!p || n < 2)
      return;
    __sort(p, n);
  }
};

// D as any class
// T is element, C must have static method int compare(const T&,const T&,D&),
// that can be inline
template <class T, class C, class D>
class DataQsort
{
public:
  void __sort(T *a, int n)
  {
    if (n <= 1)
      return;

#define SWAP(l, r)                 \
  memcpy(tmp, a + l, sizeof(T));   \
  memcpy(a + l, a + r, sizeof(T)); \
  memcpy(a + r, tmp, sizeof(T));

    int l = 1, r = n;
    SWAP(0, (n >> 1))

    memcpy(pivot, a, sizeof(T));

    while (l < r)
    {
      if (C::compare(a[l], *(T *)pivot, d) <= 0)
        ++l;
      else
      {
        for (--r; l < r && C::compare(a[r], *(T *)pivot, d) >= 0; --r)
          ;

        SWAP(l, r)
      }
    }

    --l;
    SWAP(0, l)

    if (l <= 1)
      __sort(a + r, n - r);
    else if (n - r <= 1)
      __sort(a, l);
    else
    {
      __sort(a, l);
      __sort(a + r, n - r);
    }

//    __sort(a + r, n - r);
//    __sort(a, l);
#undef SWAP
  }

  void sort(T *p, int n, D *da)
  {
    if (!p || n < 2)
      return;
    d = da;
    __sort(p, n);
  }

protected:
  char pivot[sizeof(T)], tmp[sizeof(T)];
  D *d;
};

// D as any class
// T is element, C must have static method int compare(const T&,const T&,D&),
// that can be inline
template <class T, class C, class D>
class DataSimpleQsort
{
public:
  void __sort(T *a, int n)
  {
    if (n <= 1)
      return;

#define SWAP(l, r)      \
  {                     \
    const T tmp = a[l]; \
    a[l] = a[r];        \
    a[r] = tmp;         \
  }

    int l = 1, r = n;
    SWAP(0, (n >> 1))

    const T pivot = a[0];

    while (l < r)
    {
      if (C::compare(a[l], pivot, d) <= 0)
        ++l;
      else
      {
        for (--r; l < r && C::compare(a[r], pivot, d) >= 0; --r)
          ;

        SWAP(l, r)
      }
    }

    --l;
    SWAP(0, l)

    if (l <= 1)
      __sort(a + r, n - r);
    else if (n - r <= 1)
      __sort(a, l);
    else
    {
      __sort(a, l);
      __sort(a + r, n - r);
    }

//    __sort(a + r, n - r);
//    __sort(a, l);
#undef SWAP
  }

  void sort(T *p, int n, D *da)
  {
    if (!p || n < 2)
      return;
    d = da;
    __sort(p, n);
  }

protected:
  D *d;
};

// T is element, C must have static method int compare(const T&,const T&,D&),
// that can be inline
template <class T, class CD>
class DataMemberQsort
{
public:
  void __sort(T *a, int n)
  {
    if (n <= 1)
      return;

#define SWAP(l, r)      \
  {                     \
    const T tmp = a[l]; \
    a[l] = a[r];        \
    a[r] = tmp;         \
  }

    int l = 1, r = n;
    SWAP(0, (n >> 1))

    const T pivot = a[0];

    while (l < r)
    {
      if (d->compare(a[l], pivot) <= 0)
        ++l;
      else
      {
        for (--r; l < r && d->compare(a[r], pivot) >= 0; --r)
          ;

        SWAP(l, r)
      }
    }

    --l;
    SWAP(0, l)

    if (l <= 1)
      __sort(a + r, n - r);
    else if (n - r <= 1)
      __sort(a, l);
    else
    {
      __sort(a, l);
      __sort(a + r, n - r);
    }

//__sort(a, l);
//__sort(a + r, n - r);
#undef SWAP
  }

  void sort(T *p, int n, CD *da)
  {
    if (!p || n < 2)
      return;
    d = da;
    __sort(p, n);
  }

protected:
  CD *d;
};

template <class C, class D>
class MapQsort
{
public:
  MapQsort(IMemAlloc *mem = tmpmem_ptr()) : map(mem) {}

  void sort(int num, D *data)
  {

    // Sort face map using sector info
    map.resize(num);
    for (int i = 0; i < num; ++i)
      map[i] = i;

    DataSimpleQsort<int, C, D> dqs;
    dqs.sort(map.data(), map.size(), data);
  }

  template <class T>
  void arrange(T *elems)
  {

    // Rearrange elems in the map order
    SmallTab<T, TmpmemAlloc> nelems;
    clear_and_resize(nelems, map.size());
    for (int i = 0; i < map.size(); ++i)
      nelems[i] = elems[map[i]];
    mem_copy_to(nelems, elems);
  }

  template <class T>
  void arrange_big(T *el)
  {
    SmallTab<int, TmpmemAlloc> mp;
    clear_and_resize(mp, map.size() * 2);
    mem_copy_to(map, mp.data());

    int *rmp = &mp[map.size()], i;
    for (i = 0; i < map.size(); ++i)
      rmp[mp[i]] = i;

    char buf[sizeof(T)];
    for (i = 0; i < map.size(); ++i)
    {
      int j = mp[i];
      if (j == i)
        continue;

      int k = rmp[i];
      mp[k] = j;
      rmp[j] = k;
      memcpy(buf, el + i, sizeof(T));
      memcpy(el + i, el + j, sizeof(T));
      memcpy(el + j, buf, sizeof(T));
    }
  }

public:
  Tab<int> map;
};

template <class T>
class MapSimpleAscentCompare
{
public:
  static inline int compare(const int &a, const int &b, T *d)
  {
    if (d[a] < d[b])
      return -1;
    else if (d[a] > d[b])
      return 1;
    else
      return 0;
  }
};

template <class T>
class SimpleAscentCompare
{
public:
  static inline int compare(const T &a, const T &b)
  {
    if (a < b)
      return -1;
    else if (a > b)
      return 1;
    else
      return 0;
  }
};

template <class T>
class MapSimpleDescentCompare
{
public:
  static inline int compare(const int &a, const int &b, T *d) { return -MapSimpleAscentCompare<T>::compare(a, b, d); }
};

template <class T>
class SimpleDescentCompare
{
public:
  static inline int compare(const T &a, const T &b) { return -SimpleAscentCompare<T>::compare(a, b); }
};
