//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "dag_IPoint2.h"
#include <limits.h>
#include <math/dag_bounds2.h>


class IBBox2
{
public:
  IPoint2 lim[2];

  IBBox2(const IPoint2 &lim0, const IPoint2 &lim1)
  {
    lim[0] = lim0;
    lim[1] = lim1;
  }
  IBBox2() { setEmpty(); }

  void setEmpty()
  {
    lim[0].x = INT_MAX;
    lim[0].y = INT_MAX;
    lim[1].x = INT_MIN;
    lim[1].y = INT_MIN;
  }

  bool isEmpty() const { return lim[0].x > lim[1].x || lim[0].y > lim[1].y; }

  bool isAreaEmpty() const { return lim[0].x >= lim[1].x || lim[0].y >= lim[1].y; }

  const IPoint2 &operator[](int i) const { return lim[i]; }
  IPoint2 &operator[](int i) { return lim[i]; }

  void add(const IPoint2 &p) { add(p.x, p.y); }
  IBBox2 &operator+=(const IPoint2 &p)
  {
    add(p);
    return *this;
  }
  IBBox2 &operator+=(const IBBox2 &b)
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
    return *this;
  }
  void add(int u, int v)
  {
    if (isEmpty())
    {
      lim[0].x = u;
      lim[0].y = v;
      lim[1].x = u;
      lim[1].y = v;
      return;
    }

    if (u < lim[0].x)
      lim[0].x = u;
    if (v < lim[0].y)
      lim[0].y = v;

    if (u > lim[1].x)
      lim[1].x = u;
    if (v > lim[1].y)
      lim[1].y = v;
  }

  void clip(int &u0, int &v0, int &u1, int &v1) const
  {
    if (u0 < lim[0].x)
      u0 = lim[0].x;
    if (v0 < lim[0].y)
      v0 = lim[0].y;
    if (u1 > lim[1].x)
      u1 = lim[1].x;
    if (v1 > lim[1].y)
      v1 = lim[1].y;
  }
  void clipBox(IBBox2 &b) const { clip(b[0].x, b[0].y, b[1].x, b[1].y); }

  void inflate(int val)
  {
    lim[0].x -= val;
    lim[0].y -= val;
    lim[1].x += val;
    lim[1].y += val;
  }

  bool operator&(const IPoint2 &p) const
  {
    if (p.x < lim[0].x)
      return false;
    if (p.x > lim[1].x)
      return false;
    if (p.y < lim[0].y)
      return false;
    if (p.y > lim[1].y)
      return false;
    return true;
  }
  /// check intersection with box
  bool operator&(const IBBox2 &b) const
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
    return true;
  }

  bool operator==(const IBBox2 &b) const { return b[0] == lim[0] && b[1] == lim[1]; }

  bool operator!=(const IBBox2 &b) const { return b[0] != lim[0] || b[1] != lim[1]; }

  inline IPoint2 width() const { return lim[1] - lim[0]; }

  inline int left() const { return lim[0].x; }
  inline int right() const { return lim[1].x; }
  inline int top() const { return lim[0].y; }
  inline int bottom() const { return lim[1].y; }
  inline const IPoint2 &getMin() const { return lim[0]; }
  inline const IPoint2 &getMax() const { return lim[1]; }
  inline IPoint2 size() const { return lim[1] - lim[0]; }

  inline const IPoint2 &leftTop() const { return lim[0]; }
  inline IPoint2 rightTop() const { return IPoint2(lim[1].x, lim[0].y); }
  inline IPoint2 leftBottom() const { return IPoint2(lim[0].x, lim[1].y); }
  inline const IPoint2 &rightBottom() const { return lim[1]; }
};

inline IBBox2 ibbox2(const BBox2 &p)
{
  return IBBox2(IPoint2((int)floorf(p[0].x), (int)floorf(p[0].y)), IPoint2((int)ceilf(p[1].x), (int)ceilf(p[1].y)));
}
inline BBox2 bbox2(const IBBox2 &p) { return BBox2(Point2((real)p[0].x, (real)p[0].y), Point2((real)p[1].x, (real)p[1].y)); }

// does not check if box is empty. if min of one box == max of other, returns false (boxes intersect, but not overlap)
__forceinline bool unsafe_overlap(const IBBox2 &a, const IBBox2 &b)
{
  if (b.lim[0].x >= a.lim[1].x)
    return false;
  if (b.lim[1].x <= a.lim[0].x)
    return false;
  if (b.lim[0].y >= a.lim[1].y)
    return false;
  if (b.lim[1].y <= a.lim[0].y)
    return false;
  return true;
}
// check if a is completely inside b
__forceinline bool is_box_inside_other(const IBBox2 &a, const IBBox2 &b)
{
  return a[0].x >= b[0].x && a[1].x <= b[1].x && a[0].y >= b[0].y && a[1].y <= b[1].y;
}


inline unsigned squared_int(int i) { return i * i; }

// returns squared distance to integer point from box. if point is inside box it will be 0
inline unsigned sq_distance_ipoint_to_ibox2(const IPoint2 &p, const IBBox2 &box)
{
  if (p.x < box[0].x)
  {
    if (p.y < box[0].y)
      return lengthSq(box[0] - p);
    if (p.y > box[1].y)
      return lengthSq(IPoint2(box[0].x, box[1].y) - p);
    return squared_int(box[0].x - p.x);
  }
  if (p.x > box[1].x)
  {
    if (p.y < box[0].y)
      return lengthSq(IPoint2(box[1].x, box[0].y) - p);
    if (p.y > box[1].y)
      return lengthSq(box[1] - p);
    return squared_int(p.x - box[1].x);
  }
  if (p.y < box[0].y)
    return squared_int(box[0].y - p.y);
  if (p.y > box[1].y)
    return squared_int(p.y - box[1].y);
  return 0; // inside
}
