// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>

//
// Array classes
//
// Taken from gfxTools.h 1.2

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) > (b)) ? (b) : (a))
#endif
namespace delaunay
{

template <class T>
class array : public Tab<T>
{
public:
  array() : Tab<T>(delmem) {}
  array(int l) : Tab<T>(delmem) { resize(l); }

  inline T &ref(int i);
  inline T &operator()(int i) { return ref(i); }
  inline int length() { return size(); }
  inline int maxLength() { return size(); }
};

template <class T>
inline T &array<T>::ref(int i)
{
#ifdef SAFETY
  assert(data());
  assert(i >= 0 && i < size());
#endif
  return operator[](i);
}


template <class T>
class array2
{
protected:
  Tab<T> data;
  int w, h;

public:
  array2() : data(delmem) { w = h = 0; }
  array2(int _w, int _h) : data(delmem)
  {
    w = _w;
    h = _h;
    data.resize(w * h);
  }

  inline void init(int _w, int _h)
  {
    data.resize(0);
    w = _w;
    h = _h;
    data.resize(w * h);
  }

  inline T &ref(int i, int j);
  inline T &operator()(int i, int j) { return ref(i, j); }
  inline int width() { return w; }
  inline int height() { return h; }
};

template <class T>
inline T &array2<T>::ref(int i, int j)
{
#ifdef SAFETY
  assert(data.data());
  assert(i >= 0 && i < w);
  assert(j >= 0 && j < h);
#endif
  return data[j * w + i];
}
} // namespace delaunay
