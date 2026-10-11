// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

/// @addtogroup common
/// @{

/** @file
  common classes and functions for 2D, 3D and 4D math
*/

#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <max.h>
#define INLINE __forceinline
typedef float real;
#define MAX_REAL FLT_MAX
#define MIN_REAL (-FLT_MAX)
#define REAL_EPS FLT_EPSILON
__forceinline real rabs(real a) { return fabsf(a); }

class Matrix33
{
public:
  real m[3][3];
  INLINE Matrix33() {}

  INLINE Point3 operator*(const Point3 &p) const
  {
    Point3 r;
    r[0] = m[0][0] * p[0] + m[1][0] * p[1] + m[2][0] * p[2];
    r[1] = m[0][1] * p[0] + m[1][1] * p[1] + m[2][1] * p[2];
    r[2] = m[0][2] * p[0] + m[1][2] * p[1] + m[2][2] * p[2];
    return r;
  }

  INLINE real det() const;

  INLINE void setcol(int i, const Point3 &v)
  {
    m[i][0] = v[0];
    m[i][1] = v[1];
    m[i][2] = v[2];
  }
  INLINE Point3 getcol(int i) const { return Point3(m[i][0], m[i][1], m[i][2]); }
};

INLINE real Matrix33::det() const
{
  return m[0][0] * m[1][1] * m[2][2] + m[0][1] * m[1][2] * m[2][0] + m[1][0] * m[2][1] * m[0][2] - m[0][2] * m[1][1] * m[2][0] -
         m[0][1] * m[1][0] * m[2][2] - m[1][2] * m[2][1] * m[0][0];
}

INLINE Matrix33 inverse(const Matrix33 &a)
{
  Matrix33 r;
  real d = a.det();
  r.m[0][0] = (a.m[1][1] * a.m[2][2] - a.m[2][1] * a.m[1][2]) / d;
  r.m[0][1] = (-a.m[0][1] * a.m[2][2] + a.m[2][1] * a.m[0][2]) / d;
  r.m[0][2] = (a.m[0][1] * a.m[1][2] - a.m[1][1] * a.m[0][2]) / d;
  r.m[1][0] = (-a.m[1][0] * a.m[2][2] + a.m[2][0] * a.m[1][2]) / d;
  r.m[1][1] = (a.m[0][0] * a.m[2][2] - a.m[2][0] * a.m[0][2]) / d;
  r.m[1][2] = (-a.m[0][0] * a.m[1][2] + a.m[1][0] * a.m[0][2]) / d;
  r.m[2][0] = (a.m[1][0] * a.m[2][1] - a.m[2][0] * a.m[1][1]) / d;
  r.m[2][1] = (-a.m[0][0] * a.m[2][1] + a.m[2][0] * a.m[0][1]) / d;
  r.m[2][2] = (a.m[0][0] * a.m[1][1] - a.m[1][0] * a.m[0][1]) / d;
  return r;
}

INLINE unsigned real2uchar(float p)
{
  float _n = p + 1.0f;
  unsigned i = *(unsigned *)&_n;
  if (i >= 0x40000000)
    i = 0xFF;
  else if (i <= 0x3F800000)
    i = 0;
  else
    i = (i >> 15) & 0xFF;
  return i;
}


/**
  4x3 transformation matrix.
  last column (3rd, counting from zero) is translation
  @sa Matrix3 TMatrix4 Point3 Point2 Point4
*/
class TMatrix
{
public:
  real m[4][3];

  /// identity constant matrix
  static TMatrix IDENT;

  INLINE TMatrix(real);

  INLINE Point3 getcol(int i) const { return Point3(m[i][0], m[i][1], m[i][2]); }
};

INLINE TMatrix::TMatrix(real a)
{
  memset(m, 0, sizeof(m));
  m[0][0] = m[1][1] = m[2][2] = a;
}

class BBox2
{
public:
  Point2 lim[2];
  INLINE BBox2() { setempty(); }

  INLINE void setempty()
  {
    lim[0] = Point2(MAX_REAL, MAX_REAL);
    lim[1] = Point2(MIN_REAL, MIN_REAL);
  }

  INLINE Point2 &operator[](int i) { return lim[i]; }

  INLINE BBox2 &operator+=(const Point2 &p)
  {
    if (p.x < lim[0].x)
      lim[0].x = p.x;
    if (p.x > lim[1].x)
      lim[1].x = p.x;
    if (p.y < lim[0].y)
      lim[0].y = p.y;
    if (p.y > lim[1].y)
      lim[1].y = p.y;
    return *this;
  }
};

INLINE real lengthSq(const Point3 &a) { return LengthSquared(a); }

class BBox3
{
public:
  Point3 lim[2];
  INLINE BBox3() { setempty(); }

  INLINE void setempty()
  {
    lim[0] = Point3(MAX_REAL, MAX_REAL, MAX_REAL);
    lim[1] = Point3(MIN_REAL, MIN_REAL, MIN_REAL);
  }
  INLINE Point3 center() const { return (lim[0] + lim[1]) * 0.5f; }
  INLINE Point3 width() const { return lim[1] - lim[0]; }

  INLINE BBox3 &operator+=(const Point3 &p)
  {
    if (p.x < lim[0].x)
      lim[0].x = p.x;
    if (p.x > lim[1].x)
      lim[1].x = p.x;
    if (p.y < lim[0].y)
      lim[0].y = p.y;
    if (p.y > lim[1].y)
      lim[1].y = p.y;
    if (p.z < lim[0].z)
      lim[0].z = p.z;
    if (p.z > lim[1].z)
      lim[1].z = p.z;
    return *this;
  }
};

class StaticMeshRayTracer
{
public:
  virtual ~StaticMeshRayTracer() = default;
  //! Tests ray hit to object and returns parameters of hit (if happen)
  virtual bool traceray(const Point3 &p, const Point3 &dir, real &t, int &fi, Point3 &bc) = 0;

  //! trace backfaces
  virtual bool traceray_back(const Point3 &p, const Point3 &dir, real &t, int &fi, Point3 &bc) = 0;
  //! Builds (or rebuilds) object for given mesh
  virtual void build(Mesh &m) = 0;
};
