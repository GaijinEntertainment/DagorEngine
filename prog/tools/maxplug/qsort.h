// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// T is element, C must have static method int compare(const T&,const T&),
// that can be inline
template <class T, class C>
class Qsort
{
protected:
  char __sort_x[sizeof(T)], __sort_y[sizeof(T)];

public:
  void __sort(T *a, int n)
  {
    int i = 0, j = n - 1;
    memcpy(__sort_x, a + n / 2, sizeof(T));
    do
    {
      while (C::compare(a[i], *(T *)__sort_x) < 0)
        ++i;
      while (C::compare(*(T *)__sort_x, a[j]) < 0)
        --j;
      if (i <= j)
      {
        if (i != j)
        {
          memcpy(__sort_y, a + i, sizeof(T));
          memcpy(a + i, a + j, sizeof(T));
          memcpy(a + j, __sort_y, sizeof(T));
        }
        ++i;
        --j;
      }
    } while (i <= j);
    if (j > 0)
      __sort(a, j + 1);
    if (i < n - 1)
      __sort(a + i, n - i);
  }

  void sort(T *p, int n)
  {
    if (!p || n < 2)
      return;
    __sort(p, n);
  }
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
    int i = 0, j = n - 1;
    T __sort_x = a[n / 2];
    do
    {
      while (C::compare(a[i], __sort_x) < 0)
        ++i;
      while (C::compare(__sort_x, a[j]) < 0)
        --j;
      if (i <= j)
      {
        if (i != j)
        {
          T __sort_y = a[i];
          a[i] = a[j];
          a[j] = __sort_y;
        }
        ++i;
        --j;
      }
    } while (i <= j);
    if (j > 0)
      __sort(a, j + 1);
    if (i < n - 1)
      __sort(a + i, n - i);
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
protected:
  char __sort_x[sizeof(T)], __sort_y[sizeof(T)];
  D *d;

public:
  void __sort(T *a, int n)
  {
    int i = 0, j = n - 1;
    memcpy(__sort_x, a + n / 2, sizeof(T));
    do
    {
      while (C::compare(a[i], *(T *)__sort_x, d) < 0)
        ++i;
      while (C::compare(*(T *)__sort_x, a[j], d) < 0)
        --j;
      if (i <= j)
      {
        if (i != j)
        {
          memcpy(__sort_y, a + i, sizeof(T));
          memcpy(a + i, a + j, sizeof(T));
          memcpy(a + j, __sort_y, sizeof(T));
        }
        ++i;
        --j;
      }
    } while (i <= j);
    if (j > 0)
      __sort(a, j + 1);
    if (i < n - 1)
      __sort(a + i, n - i);
  }

  void sort(T *p, int n, D *da)
  {
    if (!p || n < 2)
      return;
    d = da;
    __sort(p, n);
  }
};

// D as any class
// T is element, C must have static method int compare(const T&,const T&,D&),
// that can be inline
template <class T, class C, class D>
class DataSimpleQsort
{
protected:
  D *d;

public:
  void __sort(T *a, int n)
  {
    int i = 0, j = n - 1;
    T __sort_x = a[n / 2];
    do
    {
      while (C::compare(a[i], __sort_x, d) < 0)
        ++i;
      while (C::compare(__sort_x, a[j], d) < 0)
        --j;
      if (i <= j)
      {
        if (i != j)
        {
          T __sort_y = a[i];
          a[i] = a[j];
          a[j] = __sort_y;
        }
        ++i;
        --j;
      }
    } while (i <= j);
    if (j > 0)
      __sort(a, j + 1);
    if (i < n - 1)
      __sort(a + i, n - i);
  }

  void sort(T *p, int n, D *da)
  {
    if (!p || n < 2)
      return;
    d = da;
    __sort(p, n);
  }
};

template <class C, class D>
class MapQsort
{
public:
  Tab<int> map;
  void sort(int num, D *data)
  {
    // Sort face map using sector info
    nomem(map.resize(num));
    for (int i = 0; i < num; ++i)
      map[i] = i;
    DataSimpleQsort<int, C, D> dqs;
    dqs.sort(&map[0], map.size(), data);
  }

  template <class T>
  void arrange(T *elems)
  {
    // Rearrange elems in the map order
    Tab<T> nelems(tmpmem);
    nomem(nelems.resize(map.size()));
    for (int i = 0; i < map.size(); ++i)
      nelems[i] = elems[map[i]];
    memcpy(elems, &nelems[0], map.size() * sizeof(T));
  }
  template <class T>
  void arrange_big(T *el)
  {
    Tab<int> mp(tmpmem);
    nomem(mp.resize(map.size() * 2));
    memcpy(&mp[0], &map[0], map.size());
    int *rmp = &mp[map.size()];
    for (int i = 0; i < map.size(); ++i)
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
  static inline int compare(const T &a, const T &b) { return -SimpleAscentCompare<T>::compare(a, b, d); }
};
