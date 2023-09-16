//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_Point4.h>

#define INLINE __forceinline

/// @addtogroup math
/// @{

// TMatrix4 - Transformation matrix //
/**
  4x4 Transformation matrix
  @sa TMatrix TMatrix4D Point3 Point2 Point4
*/
class TMatrix4D
{
public:
  union
  {
    struct
    {
      double _11, _12, _13, _14;
      double _21, _22, _23, _24;
      double _31, _32, _33, _34;
      double _41, _42, _43, _44;
    };
    double m[4][4];
  };

  INLINE TMatrix4D() = default;
  INLINE TMatrix4D(const TMatrix &tm)
  {
    m[0][0] = tm.m[0][0], m[0][1] = tm.m[0][1], m[0][2] = tm.m[0][2], m[0][3] = 0;
    m[1][0] = tm.m[1][0], m[1][1] = tm.m[1][1], m[1][2] = tm.m[1][2], m[1][3] = 0;
    m[2][0] = tm.m[2][0], m[2][1] = tm.m[2][1], m[2][2] = tm.m[2][2], m[2][3] = 0;
    m[3][0] = tm.m[3][0], m[3][1] = tm.m[3][1], m[3][2] = tm.m[3][2], m[3][3] = 1;
  }
  INLINE TMatrix4D(const TMatrix4 &tm)
  {
    m[0][0] = tm.m[0][0], m[0][1] = tm.m[0][1], m[0][2] = tm.m[0][2], m[0][3] = tm.m[0][3];
    m[1][0] = tm.m[1][0], m[1][1] = tm.m[1][1], m[1][2] = tm.m[1][2], m[1][3] = tm.m[1][3];
    m[2][0] = tm.m[2][0], m[2][1] = tm.m[2][1], m[2][2] = tm.m[2][2], m[2][3] = tm.m[2][3];
    m[3][0] = tm.m[3][0], m[3][1] = tm.m[3][1], m[3][2] = tm.m[3][2], m[3][3] = tm.m[3][3];
  }
  INLINE TMatrix4D(double _m00, double _m01, double _m02, double _m03, double _m10, double _m11, double _m12, double _m13, double _m20,
    double _m21, double _m22, double _m23, double _m30, double _m31, double _m32, double _m33)
  {
    m[0][0] = _m00;
    m[0][1] = _m01;
    m[0][2] = _m02;
    m[0][3] = _m03;
    m[1][0] = _m10;
    m[1][1] = _m11;
    m[1][2] = _m12;
    m[1][3] = _m13;
    m[2][0] = _m20;
    m[2][1] = _m21;
    m[2][2] = _m22;
    m[2][3] = _m23;
    m[3][0] = _m30;
    m[3][1] = _m31;
    m[3][2] = _m32;
    m[3][3] = _m33;
  }

  explicit INLINE operator TMatrix() const
  {
    TMatrix tm;
    tm.m[0][0] = m[0][0];
    tm.m[0][1] = m[0][1];
    tm.m[0][2] = m[0][2];
    tm.m[1][0] = m[1][0];
    tm.m[1][1] = m[1][1];
    tm.m[1][2] = m[1][2];
    tm.m[2][0] = m[2][0];
    tm.m[2][1] = m[2][1];
    tm.m[2][2] = m[2][2];
    tm.m[3][0] = m[3][0];
    tm.m[3][1] = m[3][1];
    tm.m[3][2] = m[3][2];
    return tm;
  }
  explicit INLINE operator TMatrix4() const
  {
    TMatrix4 tm;
    tm.m[0][0] = m[0][0];
    tm.m[0][1] = m[0][1];
    tm.m[0][2] = m[0][2];
    tm.m[0][3] = m[0][3];
    tm.m[1][0] = m[1][0];
    tm.m[1][1] = m[1][1];
    tm.m[1][2] = m[1][2];
    tm.m[1][3] = m[1][3];
    tm.m[2][0] = m[2][0];
    tm.m[2][1] = m[2][1];
    tm.m[2][2] = m[2][2];
    tm.m[2][3] = m[2][3];
    tm.m[3][0] = m[3][0];
    tm.m[3][1] = m[3][1];
    tm.m[3][2] = m[3][2];
    tm.m[3][3] = m[3][3];
    return tm;
  }

  INLINE double &operator()(int iRow, int iColumn) { return m[iRow][iColumn]; }
  INLINE const double &operator()(int iRow, int iColumn) const { return m[iRow][iColumn]; }

  INLINE void identity();
  INLINE void zero();

  INLINE const double *operator[](int i) const { return m[i]; }
  INLINE double *operator[](int i) { return m[i]; }
  INLINE TMatrix4D operator-() const;
  INLINE TMatrix4D operator+() const { return *this; }

  INLINE TMatrix4D operator*(double) const;
  INLINE TMatrix4D operator*(const TMatrix4D &b) const
  {
    TMatrix4D r(*this);
    r *= b;
    return r;
  }
  INLINE TMatrix4D operator+(const TMatrix4D &) const;
  INLINE TMatrix4D operator-(const TMatrix4D &) const;

  INLINE TMatrix4D &operator+=(const TMatrix4D &);
  INLINE TMatrix4D &operator-=(const TMatrix4D &);
  INLINE TMatrix4D &operator*=(const TMatrix4D &);
  INLINE TMatrix4D &operator*=(double);

  INLINE void setcol(int i, const Point4 &v)
  {
    m[0][i] = v[0];
    m[1][i] = v[1];
    m[2][i] = v[2];
    m[3][i] = v[3];
  }
  INLINE void setcol(int i, double x, double y, double z, double w)
  {
    m[0][i] = x;
    m[1][i] = y;
    m[2][i] = z;
    ;
    m[3][i] = w;
  }
  INLINE Point4 getcol(int i) const { return Point4(m[0][i], m[1][i], m[2][i], m[3][i]); }

  INLINE void setrow(int i, const Point4 &v)
  {
    m[i][0] = v[0];
    m[i][1] = v[1];
    m[i][2] = v[2];
    m[i][3] = v[3];
  }
  INLINE void setrow(int i, double x, double y, double z, double w)
  {
    m[i][0] = x;
    m[i][1] = y;
    m[i][2] = z;
    ;
    m[i][3] = w;
  }
  INLINE Point4 getrow(int i) const { return Point4(m[i][0], m[i][1], m[i][2], m[i][3]); }

  INLINE TMatrix4D transpose() const
  {
    TMatrix4D r;
    for (int i = 0; i < 4; ++i)
      for (int j = 0; j < 4; ++j)
        r.m[i][j] = m[j][i];
    return r;
  }
  INLINE DPoint3 rotate43(const DPoint3 &p) const
  {
    DPoint3 r;
    r[0] = (m[0][0] * p[0] + m[1][0] * p[1] + m[2][0] * p[2]);
    r[1] = (m[0][1] * p[0] + m[1][1] * p[1] + m[2][1] * p[2]);
    r[2] = (m[0][2] * p[0] + m[1][2] * p[1] + m[2][2] * p[2]);
    return r;
  }
  INLINE DPoint3 transform43(const DPoint3 &p) const
  {
    DPoint3 r;
    r[0] = (m[0][0] * p[0] + m[1][0] * p[1] + m[2][0] * p[2]) + m[3][0];
    r[1] = (m[0][1] * p[0] + m[1][1] * p[1] + m[2][1] * p[2]) + m[3][1];
    r[2] = (m[0][2] * p[0] + m[1][2] * p[1] + m[2][2] * p[2]) + m[3][2];
    return r;
  }
};

extern bool inverse44(const TMatrix4D &in, TMatrix4D &result, double &det); //<returns false if det() < 0

INLINE TMatrix4D operator*(double, const TMatrix4D &);

INLINE void TMatrix4D::zero() { memset(m, 0, sizeof(m)); }

INLINE void TMatrix4D::identity()
{
  memset(m, 0, sizeof(m));
  _11 = _22 = _33 = _44 = 1;
}

INLINE TMatrix4D TMatrix4D::operator-() const
{
  TMatrix4D a;
  for (int j = 0; j < 4; ++j)
    for (int i = 0; i < 4; ++i)
      a.m[j][i] = -m[j][i];
  return a;
}

INLINE TMatrix4D TMatrix4D::operator+(const TMatrix4D &b) const
{
  TMatrix4D r;
  for (int j = 0; j < 4; ++j)
    for (int i = 0; i < 4; ++i)
      r.m[j][i] = m[j][i] + b.m[j][i];
  return r;
}

INLINE TMatrix4D TMatrix4D::operator-(const TMatrix4D &b) const
{
  TMatrix4D r;
  for (int j = 0; j < 4; ++j)
    for (int i = 0; i < 4; ++i)
      r.m[j][i] = m[j][i] - b.m[j][i];
  return r;
}

INLINE TMatrix4D &TMatrix4D::operator+=(const TMatrix4D &a)
{
  for (int j = 0; j < 4; ++j)
    for (int i = 0; i < 4; ++i)
      m[j][i] += a.m[j][i];
  return *this;
}

INLINE TMatrix4D &TMatrix4D::operator-=(const TMatrix4D &a)
{
  for (int j = 0; j < 4; ++j)
    for (int i = 0; i < 4; ++i)
      m[j][i] -= a.m[j][i];
  return *this;
}


INLINE TMatrix4D TMatrix4D::operator*(double f) const
{
  TMatrix4D a;
  for (int j = 0; j < 4; ++j)
    for (int i = 0; i < 4; ++i)
      a.m[j][i] = m[j][i] * f;
  return a;
}

INLINE TMatrix4D operator*(double f, const TMatrix4D &a) { return a * f; }

INLINE TMatrix4D &TMatrix4D::operator*=(double f)
{
  for (int j = 0; j < 4; ++j)
    for (int i = 0; i < 4; ++i)
      m[j][i] *= f;
  return *this;
}

INLINE TMatrix4D &TMatrix4D::operator*=(const TMatrix4D &b)
{
  TMatrix4D r;
  for (int i = 0; i < 4; ++i)
  {
#define MUL_COLUMN(j) r.m[i][j] = m[i][0] * b.m[0][j] + m[i][1] * b.m[1][j] + m[i][2] * b.m[2][j] + m[i][3] * b.m[3][j]
    MUL_COLUMN(0);
    MUL_COLUMN(1);
    MUL_COLUMN(2);
    MUL_COLUMN(3);
#undef MUL_COLUMN
  }
  *this = r;
  return *this;
}

INLINE TMatrix4D dmatrix_perspective_reverse(double wk, double hk, double zn, double zf)
{
  return TMatrix4D(wk, 0, 0, 0, 0, hk, 0, 0, 0, 0, -zn / (zf - zn), 1, 0, 0, zn * zf / (zf - zn), 0);
}

INLINE TMatrix4D dmatrix_perspective_forward(double wk, double hk, double z_near, double z_far)
{
  double q = z_far / (z_far - z_near);
  return TMatrix4D(wk, 0, 0, 0, 0, hk, 0, 0, 0, 0, q, 1, 0, 0, -q * z_near, 0);
}

INLINE TMatrix4D dmatrix_perspective(double wk, double hk, double z_near, double z_far)
{
  return dmatrix_perspective_reverse(wk, hk, z_near, z_far);
}


INLINE TMatrix4D orthonormalized_inverse(const TMatrix4D &a)
{
  TMatrix4D r;
  r.m[0][0] = a.m[0][0];
  r.m[0][1] = a.m[1][0];
  r.m[0][2] = a.m[2][0];
  r.m[1][0] = a.m[0][1];
  r.m[1][1] = a.m[1][1];
  r.m[1][2] = a.m[2][1];
  r.m[2][0] = a.m[0][2];
  r.m[2][1] = a.m[1][2];
  r.m[2][2] = a.m[2][2];

  r.m[3][0] = -(r.m[0][0] * a[3][0] + r.m[1][0] * a[3][1] + r.m[2][0] * a[3][2]);
  r.m[3][1] = -(r.m[0][1] * a[3][0] + r.m[1][1] * a[3][1] + r.m[2][1] * a[3][2]);
  r.m[3][2] = -(r.m[0][2] * a[3][0] + r.m[1][2] * a[3][1] + r.m[2][2] * a[3][2]);

  r.m[0][3] = 0.0;
  r.m[1][3] = 0.0;
  r.m[2][3] = 0.0;
  r.m[3][3] = 1.0;

  return r;
}


#undef INLINE
/// @}
