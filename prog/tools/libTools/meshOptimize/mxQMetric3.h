#ifndef MXQMETRIC3_INCLUDED // -*- C++ -*-
#define MXQMETRIC3_INCLUDED
#if !defined(__GNUC__)
#pragma once
#endif

/************************************************************************

  3D Quadric Error Metric

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.

  $Id$

 ************************************************************************/
#include <math/dag_math3d.h>

typedef Point3 Vec3;
typedef Matrix3 Mat3;
typedef TMatrix4 Mat4;
typedef Point4 Vec4;

class MxQuadric3
{
private:
  enum
  {
    X = 0,
    Y = 1,
    Z = 2
  };
  double a2, ab, ac, ad;
  double b2, bc, bd;
  double c2, cd;
  double d2;

  double r;

  void init(double a, double b, double c, double d, double area);
  void init(const Mat4 &Q, double area);

public:
  MxQuadric3() { clear(); }
  MxQuadric3(double a, double b, double c, double d, double area = 1.0) { init(a, b, c, d, area); }
  MxQuadric3(const float *n, double d, double area = 1.0) { init(n[X], n[Y], n[Z], d, area); }
  MxQuadric3(const double *n, double d, double area = 1.0) { init(n[X], n[Y], n[Z], d, area); }
  MxQuadric3(const MxQuadric3 &Q) { *this = Q; }

  Mat3 tensor() const;
  Vec3 vector() const { return Vec3(ad, bd, cd); }
  double offset() const { return d2; }
  double area() const { return r; }
  Mat4 homogeneous() const;

  void set_coefficients(const double *);
  void set_area(double a) { r = a; }
  void point_constraint(const float *);

  void clear(double val = 0.0) { a2 = ab = ac = ad = b2 = bc = bd = c2 = cd = d2 = r = val; }
  MxQuadric3 &operator=(const MxQuadric3 &Q);
  MxQuadric3 &operator+=(const MxQuadric3 &Q);
  MxQuadric3 &operator-=(const MxQuadric3 &Q);
  MxQuadric3 &operator*=(double s);
  MxQuadric3 &transform(const Mat4 &P);

  double evaluate(double x, double y, double z) const;
  double evaluate(const double *v) const { return evaluate(v[X], v[Y], v[Z]); }
  double evaluate(const float *v) const { return evaluate(v[X], v[Y], v[Z]); }

  double operator()(double x, double y, double z) const { return evaluate(x, y, z); }
  double operator()(const double *v) const { return evaluate(v[X], v[Y], v[Z]); }
  double operator()(const float *v) const { return evaluate(v[X], v[Y], v[Z]); }
  double operator()(const Vec3 &v) const { return evaluate(v[X], v[Y], v[Z]); }

  bool optimize(Vec3 &v) const;
  bool optimize(float *x, float *y, float *z) const;

  bool optimize(float &v, const Vec3 &v1, const Vec3 &v2) const;
  bool optimize(Vec3 &v, const Vec3 &v1, const Vec3 &v2, const Vec3 &v3) const;
};

inline MxQuadric3 operator+(const MxQuadric3 &a, const MxQuadric3 &b)
{
  MxQuadric3 r = a;
  r += b;
  return r;
}

// MXQMETRIC3_INCLUDED
#endif
