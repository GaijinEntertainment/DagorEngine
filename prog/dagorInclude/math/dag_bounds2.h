//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point2.h>

#define INLINE __forceinline

class BBox2;
INLINE float non_empty_boxes_not_intersect(const BBox2 &a, const BBox2 &b);

/// @addtogroup math
/// @{

// BBox2 - 2D bounding box //
/**
  2D bounding box
  @sa BBox3 BSphere3 TMatrix TMatrix4 Point3 Point2 Point4
*/
class BBox2
{
public:
  Point2 lim[2];
  INLINE BBox2() { setempty(); }
  BBox2(const Point2 &a, real s) { makebox(a, s); }
  INLINE BBox2(const Point2 &left_top, const Point2 &right_bottom)
  {
    lim[0] = left_top;
    lim[1] = right_bottom;
  }
  INLINE BBox2(real left, real top, real right, real bottom)
  {
    lim[0] = Point2(left, top);
    lim[1] = Point2(right, bottom);
  }

  INLINE void setempty()
  {
    lim[0] = Point2(MAX_REAL / 4, MAX_REAL / 4);
    lim[1] = Point2(MIN_REAL / 4, MIN_REAL / 4);
  }
  INLINE bool isempty() const { return lim[0].x > lim[1].x || lim[0].y > lim[1].y; }
  INLINE void makebox(const Point2 &p, real s)
  {
    Point2 d(s / 2, s / 2);
    lim[0] = p - d;
    lim[1] = p + d;
  }
  INLINE Point2 center() const { return (lim[0] + lim[1]) * 0.5; }
  INLINE Point2 width() const { return lim[1] - lim[0]; }

  INLINE const Point2 &operator[](int i) const { return lim[i]; }
  INLINE Point2 &operator[](int i) { return lim[i]; }

  INLINE float float_is_empty() const { return fsel(lim[1].x - lim[0].x, 0.0f, 1.0f) + fsel(lim[1].y - lim[0].y, 0.0f, 1.0f); }
  INLINE BBox2 &operator+=(const Point2 &p)
  {
    lim[0].x = fsel(lim[0].x - p.x, p.x, lim[0].x);
    lim[1].x = fsel(p.x - lim[1].x, p.x, lim[1].x);
    lim[0].y = fsel(lim[0].y - p.y, p.y, lim[0].y);
    lim[1].y = fsel(p.y - lim[1].y, p.y, lim[1].y);
    return *this;
  }
  INLINE BBox2 &operator+=(const BBox2 &b)
  {
    if (b.isempty())
      return *this;
    lim[0].x = fsel(lim[0].x - b.lim[0].x, b.lim[0].x, lim[0].x);
    lim[1].x = fsel(b.lim[1].x - lim[1].x, b.lim[1].x, lim[1].x);
    lim[0].y = fsel(lim[0].y - b.lim[0].y, b.lim[0].y, lim[0].y);
    lim[1].y = fsel(b.lim[1].y - lim[1].y, b.lim[1].y, lim[1].y);
    return *this;
  }

  INLINE bool operator&(const Point2 &p) const
  {
    if (p.x < lim[0].x)
      return 0;
    if (p.x > lim[1].x)
      return 0;
    if (p.y < lim[0].y)
      return 0;
    if (p.y > lim[1].y)
      return 0;
    return 1;
  }
  INLINE bool operator&(const BBox2 &b) const
  {
    if (b.isempty())
      return 0;
    if (b.lim[0].x > lim[1].x)
      return 0;
    if (b.lim[1].x < lim[0].x)
      return 0;
    if (b.lim[0].y > lim[1].y)
      return 0;
    if (b.lim[1].y < lim[0].y)
      return 0;
    return 1;
  }

  void inflate(float val)
  {
    lim[0].x -= val;
    lim[0].y -= val;
    lim[1].x += val;
    lim[1].y += val;
  }

  INLINE void scale(float val)
  {
    const Point2 c = center();
    lim[0] = (lim[0] - c) * val + c;
    lim[1] = (lim[1] - c) * val + c;
  }

  INLINE real left() const { return lim[0].x; }
  INLINE real right() const { return lim[1].x; }
  INLINE real top() const { return lim[0].y; }
  INLINE real bottom() const { return lim[1].y; }
  INLINE const Point2 &getMin() const { return lim[0]; }
  INLINE const Point2 &getMax() const { return lim[1]; }
  INLINE Point2 size() const { return lim[1] - lim[0]; }

  INLINE const Point2 &leftTop() const { return lim[0]; }
  INLINE Point2 rightTop() const { return Point2(lim[1].x, lim[0].y); }
  INLINE Point2 leftBottom() const { return Point2(lim[0].x, lim[1].y); }
  INLINE const Point2 &rightBottom() const { return lim[1]; }

  template <class T>
  static BBox2 xz(const T &a)
  {
    return BBox2(Point2::xz(a.lim[0]), Point2::xz(a.lim[1]));
  }
  template <class T>
  static BBox2 yz(const T &a)
  {
    return BBox2(Point2::yz(a.lim[0]), Point2::yz(a.lim[1]));
  }
  template <class T>
  static BBox2 xy(const T &a)
  {
    return BBox2(Point2::xy(a.lim[0]), Point2::xy(a.lim[1]));
  }
};

INLINE float non_empty_boxes_not_intersect(const BBox2 &a, const BBox2 &b)
{
  return fsel(a.lim[1].x - b.lim[0].x, 0.0f, 1.0f) + fsel(b.lim[1].x - a.lim[0].x, 0.0f, 1.0f) +
         fsel(a.lim[1].y - b.lim[0].y, 0.0f, 1.0f) + fsel(b.lim[1].y - a.lim[0].y, 0.0f, 1.0f);
}

INLINE float float_non_empty_boxes_not_inclusive(const BBox2 &inner, const BBox2 &outer)
{
  return fsel(inner.lim[0].x - outer.lim[0].x, 0.0f, 1.0f) + fsel(outer.lim[1].x - inner.lim[1].x, 0.0f, 1.0f) +
         fsel(inner.lim[0].y - outer.lim[0].y, 0.0f, 1.0f) + fsel(outer.lim[1].y - inner.lim[1].y, 0.0f, 1.0f);
};

INLINE bool non_empty_boxes_inclusive(const BBox2 &inner, const BBox2 &outer)
{
  return float_non_empty_boxes_not_inclusive(inner, outer) < 1.0f;
};

#undef INLINE

/// @}
