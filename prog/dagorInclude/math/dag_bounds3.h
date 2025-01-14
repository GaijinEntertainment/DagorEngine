//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>
#include <util/dag_globDef.h>

#define INLINE __forceinline

/// @addtogroup math
/// @{


// BBox3 - 3D bounding box //
/**
  3D axis-aligned bounding box
  @sa BSphere3 TMatrix TMatrix4 Point3 Point2 Point4
*/
class BBox3;
INLINE float non_empty_boxes_not_intersect(const BBox3 &a, const BBox3 &b);

class BBox3
{
public:
  /// Minimum (0) and maximum (1) limits for the box.
  Point3 lim[2];

  INLINE BBox3() { setempty(); }
  INLINE BBox3(const Point3 &min, const Point3 &max)
  {
    lim[0] = min;
    lim[1] = max;
  }
  INLINE BBox3(Point3 p, real s) { makecube(p, s); }
  INLINE BBox3(const BSphere3 &s);
  INLINE BBox3 &operator=(const BSphere3 &s);

  INLINE void setempty()
  {
    lim[0] = Point3(MAX_REAL / 4, MAX_REAL / 4, MAX_REAL / 4);
    lim[1] = Point3(MIN_REAL / 4, MIN_REAL / 4, MIN_REAL / 4);
  }
  INLINE bool isempty() const { return lim[0].x > lim[1].x || lim[0].y > lim[1].y || lim[0].z > lim[1].z; }
  INLINE void makecube(const Point3 &p, real s)
  {
    Point3 d(s / 2, s / 2, s / 2);
    lim[0] = p - d;
    lim[1] = p + d;
  }
  INLINE Point3 center() const { return (lim[0] + lim[1]) * 0.5; }
  INLINE Point3 width() const { return lim[1] - lim[0]; }
  INLINE float volume() const { return (lim[1].x - lim[0].x) * (lim[1].y - lim[0].y) * (lim[1].z - lim[0].z); }

  INLINE const Point3 &operator[](int i) const { return lim[i]; }
  INLINE Point3 &operator[](int i) { return lim[i]; }

  INLINE const Point3 &boxMin() const { return lim[0]; }
  INLINE Point3 &boxMin() { return lim[0]; }
  INLINE const Point3 &boxMax() const { return lim[1]; }
  INLINE Point3 &boxMax() { return lim[1]; }

  INLINE bool operator==(const BBox3 &right) const { return lim[0] == right.lim[0] && lim[1] == right.lim[1]; }

  INLINE Point3 point(unsigned int i) const { return Point3(lim[(i & 1)].x, lim[(i & 2) >> 1].y, lim[(i & 4) >> 2].z); }

  INLINE bool operator!=(const BBox3 &right) const { return lim[0] != right.lim[0] || lim[1] != right.lim[1]; }

  INLINE float float_is_empty() const
  {
    return fsel(lim[1].x - lim[0].x, 0.0f, 1.0f) + fsel(lim[1].y - lim[0].y, 0.0f, 1.0f) + fsel(lim[1].z - lim[0].z, 0.0f, 1.0f);
  }
  INLINE BBox3 &operator+=(const Point3 &p)
  {
    lim[0].x = fsel(lim[0].x - p.x, p.x, lim[0].x);
    lim[1].x = fsel(p.x - lim[1].x, p.x, lim[1].x);
    lim[0].y = fsel(lim[0].y - p.y, p.y, lim[0].y);
    lim[1].y = fsel(p.y - lim[1].y, p.y, lim[1].y);
    lim[0].z = fsel(lim[0].z - p.z, p.z, lim[0].z);
    lim[1].z = fsel(p.z - lim[1].z, p.z, lim[1].z);
    return *this;
  }
  INLINE BBox3 &operator+=(const BBox3 &b)
  {
    if (b.isempty())
      return *this;
    lim[0].x = fsel(lim[0].x - b.lim[0].x, b.lim[0].x, lim[0].x);
    lim[1].x = fsel(b.lim[1].x - lim[1].x, b.lim[1].x, lim[1].x);
    lim[0].y = fsel(lim[0].y - b.lim[0].y, b.lim[0].y, lim[0].y);
    lim[1].y = fsel(b.lim[1].y - lim[1].y, b.lim[1].y, lim[1].y);
    lim[0].z = fsel(lim[0].z - b.lim[0].z, b.lim[0].z, lim[0].z);
    lim[1].z = fsel(b.lim[1].z - lim[1].z, b.lim[1].z, lim[1].z);
    return *this;
  }
  INLINE BBox3 &operator+=(const BSphere3 &s);

  /// check intersection with point
  INLINE bool operator&(const Point3 &p) const
  {
    if (p.x < lim[0].x)
      return 0;
    if (p.x > lim[1].x)
      return 0;
    if (p.y < lim[0].y)
      return 0;
    if (p.y > lim[1].y)
      return 0;
    if (p.z < lim[0].z)
      return 0;
    if (p.z > lim[1].z)
      return 0;
    return 1;
  }

  /// check intersection with box
  INLINE bool operator&(const BBox3 &b) const
  {
    if (b.lim[0].x > b.lim[1].x || lim[0].x > lim[1].x)
      return false;
    return non_empty_intersect(b);
  }
  INLINE bool non_empty_intersect(const BBox3 &b) const
  {
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
  INLINE void scale(float val)
  {
    const Point3 c = center();
    lim[0] = (lim[0] - c) * val + c;
    lim[1] = (lim[1] - c) * val + c;
  }

  INLINE void inflate(float val)
  {
    lim[0].x -= val;
    lim[0].y -= val;
    lim[0].z -= val;
    lim[1].x += val;
    lim[1].y += val;
    lim[1].z += val;
  }

  INLINE void inflateXZ(float val)
  {
    lim[0].x -= val;
    lim[0].z -= val;
    lim[1].x += val;
    lim[1].z += val;
  }

  INLINE BBox3 getIntersection(const BBox3 &right) const
  {
    if (!operator&(right))
      return BBox3();

    BBox3 result;
    result.lim[1][0] = min(lim[1][0], right.lim[1][0]);
    result.lim[0][0] = max(lim[0][0], right.lim[0][0]);
    result.lim[1][1] = min(lim[1][1], right.lim[1][1]);
    result.lim[0][1] = max(lim[0][1], right.lim[0][1]);
    result.lim[1][2] = min(lim[1][2], right.lim[1][2]);
    result.lim[0][2] = max(lim[0][2], right.lim[0][2]);
    return result;
  }

  static const BBox3 IDENT;
};

INLINE float non_empty_boxes_not_intersect(const BBox3 &a, const BBox3 &b)
{
  return fsel(a.lim[1].x - b.lim[0].x, 0.0f, 1.0f) + fsel(b.lim[1].x - a.lim[0].x, 0.0f, 1.0f) +
         fsel(a.lim[1].y - b.lim[0].y, 0.0f, 1.0f) + fsel(b.lim[1].y - a.lim[0].y, 0.0f, 1.0f) +
         fsel(a.lim[1].z - b.lim[0].z, 0.0f, 1.0f) + fsel(b.lim[1].z - a.lim[0].z, 0.0f, 1.0f);
}

INLINE float float_non_empty_boxes_not_inclusive(const BBox3 &inner, const BBox3 &outer)
{
  return fsel(inner.lim[0].x - outer.lim[0].x, 0.0f, 1.0f) + fsel(outer.lim[1].x - inner.lim[1].x, 0.0f, 1.0f) +
         fsel(inner.lim[0].y - outer.lim[0].y, 0.0f, 1.0f) + fsel(outer.lim[1].y - inner.lim[1].y, 0.0f, 1.0f) +
         fsel(inner.lim[0].z - outer.lim[0].z, 0.0f, 1.0f) + fsel(outer.lim[1].z - inner.lim[1].z, 0.0f, 1.0f);
};

INLINE bool non_empty_boxes_inclusive(const BBox3 &inner, const BBox3 &outer)
{
  return float_non_empty_boxes_not_inclusive(inner, outer) < 1.0f;
};


// BSphere3 - 3D bounding sphere (uses only fast routines) //
/**
  3D bounding sphere (uses only fast routines)
  @sa BBox3 TMatrix TMatrix4 Point3 Point2 Point4
*/
class BSphere3
{
public:
  Point3 c;
  real r, r2;
  INLINE BSphere3() { setempty(); }
  INLINE BSphere3(const Point3 &p, real s)
  {
    c = p;
    r = s;
    r2 = r * r;
  }
  INLINE BSphere3 &operator=(const BBox3 &a)
  {
    if (a.lim[1].x < a.lim[0].x)
    {
      setempty();
      return *this;
    }
    r = length(a.lim[1] - a.lim[0]) * (0.5f * 1.01f);
    c = (a.lim[1] + a.lim[0]) * 0.5f;
    r2 = r * r;
    return *this;
  }

  INLINE void setempty()
  {
    c.zero();
    r = -1;
    r2 = -1;
  }
  INLINE bool isempty() const { return r < 0; }

  INLINE BSphere3 &operator+=(const Point3 &p)
  {
    Point3 cd = p - c;
    real rd = length(cd);
    if (r >= rd)
      return *this;
    if (isempty())
      return *this = BSphere3(p, 0.f);
    float rad = (rd + r) / 2.f;
    c = c + cd * ((rad - r) / rd);
    r = rad;
    r2 = rad * rad;
    return *this;
  }
  INLINE BSphere3 &operator+=(const BSphere3 &b)
  {
    Point3 cd = b.c - c;
    real rd = length(cd);
    if (b.isempty() || r >= rd + b.r)
      return *this;
    if (isempty() || b.r >= rd + r)
      return *this = b;
    float rad = (rd + r + b.r) / 2.f;
    c = c + cd * ((rad - r) / rd);
    r = rad;
    r2 = rad * rad;
    return *this;
  }

  INLINE BSphere3 &operator+=(const BBox3 &b)
  {
    if (b.isempty())
      return *this;
    if (isempty())
    {
      c = b.center();
      r2 = lengthSq(b.width()) * 0.25;
      r = sqrtf(r2) * 1.01f;
      return *this;
    }
    Point3 mind = abs(b[0] - c);
    Point3 maxd = abs(b[1] - c);
    Point3 p = Point3(b[mind.x < maxd.x ? 1 : 0].x, b[mind.y < maxd.y ? 1 : 0].y, b[mind.z < maxd.z ? 1 : 0].z);
    *this += p;
    return *this;
  }

  INLINE bool operator&(const Point3 &p) const
  {
    if (r < 0)
      return 0;
    return (lengthSq(p - c) <= r2);
  }
  INLINE bool operator&(const BSphere3 &b) const
  {
    if (r < 0 || b.r < 0)
      return 0;
    real rd = r + b.r;
    return lengthSq(c - b.c) < (rd * rd);
  }

  INLINE bool operator&(const BBox3 &b) const
  {

    float dmin = 0;
    for (int i = 0; i < 3; i++)
    {
      if (c[i] < b.lim[0][i])
        dmin += sqr(c[i] - b.lim[0][i]);
      else if (c[i] > b.lim[1][i])
        dmin += sqr(c[i] - b.lim[1][i]);
    }
    return dmin <= r2;
  }
};

INLINE BBox3::BBox3(const BSphere3 &s)
{
  if (s.r >= 0)
  {
    makecube(s.c, s.r * 2.0f);
  }
  else
    setempty();
}

INLINE BBox3 &BBox3::operator=(const BSphere3 &s)
{
  if (s.r >= 0)
  {
    makecube(s.c, s.r * 2.0f);
  }
  else
    setempty();
  return *this;
}

INLINE BBox3 &BBox3::operator+=(const BSphere3 &s)
{
  if (!s.isempty())
  {
    BBox3 bb;
    bb.makecube(s.c, s.r * 2.0f);
    *this += bb;
  }
  return *this;
}

#undef INLINE

/// @}
