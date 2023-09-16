//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include "dag_IPoint3.h"
#include <limits.h>
#include <math/dag_bounds3.h>


class IBBox3
{
public:
  IPoint3 lim[2];

  IBBox3() { setEmpty(); }
  IBBox3(const IPoint3 &lim0, const IPoint3 &lim1)
  {
    lim[0] = lim0;
    lim[1] = lim1;
  }

  void setEmpty()
  {
    lim[0].x = INT_MAX;
    lim[0].y = INT_MAX;
    lim[0].z = INT_MAX;
    lim[1].x = INT_MIN;
    lim[1].y = INT_MIN;
    lim[1].z = INT_MIN;
  }

  bool isEmpty() const { return lim[0].x > lim[1].x || lim[0].y > lim[1].y || lim[0].z > lim[1].z; }

  bool isVolumeEmpty() const { return lim[0].x >= lim[1].x || lim[0].y >= lim[1].y || lim[0].z >= lim[1].z; }

  const IPoint3 &operator[](int i) const { return lim[i]; }
  IPoint3 &operator[](int i) { return lim[i]; }

  void add(const IPoint3 &p) { add(p.x, p.y, p.z); }
  IBBox3 &operator+=(const IPoint3 &p)
  {
    add(p);
    return *this;
  }
  IBBox3 &operator+=(const IBBox3 &b)
  {
    if (b.isEmpty())
      return *this;
    if (b.lim[0].x < lim[0].x)
      lim[0].x = b.lim[0].x;
    if (b.lim[1].x > lim[1].x)
      lim[1].x = b.lim[1].x;
    if (b.lim[0].y < lim[0].y)
      lim[0].y = b.lim[0].y;
    if (b.lim[1].y > lim[1].y)
      lim[1].y = b.lim[1].y;
    if (b.lim[0].z < lim[0].z)
      lim[0].z = b.lim[0].z;
    if (b.lim[1].z > lim[1].z)
      lim[1].z = b.lim[1].z;
    return *this;
  }
  void add(int u, int v, int w)
  {
    if (isEmpty())
    {
      lim[0].x = u;
      lim[0].y = v;
      lim[0].z = w;
      lim[1].x = u;
      lim[1].y = v;
      lim[1].z = w;
      return;
    }

    if (u < lim[0].x)
      lim[0].x = u;
    if (v < lim[0].y)
      lim[0].y = v;
    if (w < lim[0].z)
      lim[0].z = w;

    if (u > lim[1].x)
      lim[1].x = u;
    if (v > lim[1].y)
      lim[1].y = v;
    if (w > lim[1].z)
      lim[1].z = w;
  }

  void clip(int &u0, int &v0, int &w0, int &u1, int &v1, int &w1) const
  {
    if (u0 < lim[0].x)
      u0 = lim[0].x;
    if (v0 < lim[0].y)
      v0 = lim[0].y;
    if (w0 < lim[0].z)
      w0 = lim[0].z;
    if (u1 > lim[1].x)
      u1 = lim[1].x;
    if (v1 > lim[1].y)
      v1 = lim[1].y;
    if (w1 > lim[1].z)
      w1 = lim[1].z;
  }

  void clipBox(IBBox3 &b) const { clip(b[0].x, b[0].y, b[0].z, b[1].x, b[1].y, b[1].z); }

  void inflate(int val)
  {
    lim[0].x -= val;
    lim[0].y -= val;
    lim[0].z -= val;
    lim[1].x += val;
    lim[1].y += val;
    lim[1].z += val;
  }

  bool operator&(const IPoint3 &p) const
  {
    if (p.x < lim[0].x)
      return false;
    if (p.x > lim[1].x)
      return false;
    if (p.y < lim[0].y)
      return false;
    if (p.y > lim[1].y)
      return false;
    if (p.z < lim[0].z)
      return false;
    if (p.z > lim[1].z)
      return false;
    return true;
  }
  /// check intersection with box
  bool operator&(const IBBox3 &b) const
  {
    if (b.isEmpty())
      return false;
    if (b.lim[0].x > lim[1].x)
      return false;
    if (b.lim[1].x < lim[0].x)
      return false;
    if (b.lim[0].y > lim[1].y)
      return false;
    if (b.lim[1].y < lim[0].y)
      return false;
    if (b.lim[0].z > lim[1].z)
      return false;
    if (b.lim[1].z < lim[0].z)
      return false;
    return true;
  }

  bool operator==(const IBBox3 &b) const { return b[0] == lim[0] && b[1] == lim[1]; }

  bool operator!=(const IBBox3 &b) const { return b[0] != lim[0] || b[1] != lim[1]; }

  inline IPoint3 width() const { return lim[1] - lim[0]; }
};


inline IBBox3 ibbox3(const BBox3 &p)
{
  return IBBox3(IPoint3((int)floorf(p[0].x), (int)floorf(p[0].y), (int)floorf(p[0].z)),
    IPoint3((int)ceilf(p[1].x), (int)ceilf(p[1].y), (int)ceilf(p[1].z)));
}
inline BBox3 bbox3(const IBBox3 &p)
{
  return BBox3(Point3((real)p[0].x, (real)p[0].y, (real)p[0].z), Point3((real)p[1].x, (real)p[1].y, (real)p[1].z));
}
