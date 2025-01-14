//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>


#define INLINE inline


class Plane3
{
public:
  Point3 n;
  real d;


  INLINE Plane3() : d(0.f) {}

  INLINE Plane3(real A, real B, real C, real D) { set(A, B, C, D); }

  INLINE Plane3(const Point3 &normal, real D) { set(normal, D); }

  INLINE Plane3(const Point3 &normal, const Point3 &pt) { set(normal, pt); }

  INLINE Plane3(const Point3 &p0, const Point3 &p1, const Point3 &p2) { set(p0, p1, p2); }


  INLINE const Point3 &getNormal() const { return n; }


  INLINE void set(real A, real B, real C, real D)
  {
    n.x = A;
    n.y = B;
    n.z = C;
    d = D;
  }

  INLINE void set(const Point3 &normal, real D)
  {
    n = normal;
    d = D;
  }

  INLINE void normalize()
  {
    float lg = n.length();
    d /= lg;
    n /= lg;
  }
  INLINE void setNormalized(real A, real B, real C, real D)
  {
    set(A, B, C, D);
    normalize();
  }

  INLINE void set(const Point3 &normal, const Point3 &pt)
  {
    n = normal;
    d = -(n * pt);
  }

  INLINE void set(const Point3 &p0, const Point3 &p1, const Point3 &p2)
  {
    n = (p1 - p0) % (p2 - p0);
    d = -(n * p0);
  }


  INLINE void setNormalized(const Point3 &normal, const Point3 &pt)
  {
    n = ::normalize(normal);
    d = -(n * pt);
  }

  INLINE void setNormalized(const Point3 &p0, const Point3 &p1, const Point3 &p2)
  {
    n = ::normalize((p1 - p0) % (p2 - p0));
    d = -(n * p0);
  }


  INLINE void flip()
  {
    n = -n;
    d = -d;
  }


  INLINE real distance(const Point3 &pt) const { return pt * n + d; }


  INLINE Point3 project(const Point3 &pt) const
  {
    real n2 = lengthSq(n);
    if (n2 == 0)
      return pt;

    return pt - n * distance(pt) / n2;
  }


  INLINE Point3 calcLineIntersectionPoint(const Point3 &p0, const Point3 &p1) const
  {
    Point3 dir = p1 - p0;

    real den = n * dir;
    if (den == 0)
      return p0; // line is parallel to the plane

    return ((n * p0 + d) / (-den)) * dir + p0;
  }


  INLINE bool operator==(const Plane3 &p) const { return n == p.n && d == p.d; }

  INLINE bool operator!=(const Plane3 &p) const { return n != p.n || d != p.d; }
};


#undef INLINE
