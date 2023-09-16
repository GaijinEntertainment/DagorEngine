//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point3.h>
#include <string.h>

#define INLINE __forceinline

/// @addtogroup math
/// @{

// Matrix3 - 3x3 matrix //

/**
  3x3 matrix
  @sa TMatrix TMatrix4 Point3 Point2 Point4
*/
class Matrix3
{
  INLINE void copy(const float *p)
  {
    float *dst = &m[0][0];
    for (int i = 0; i < 9; i++)
      dst[i] = p[i];
  }

public:
  static const Matrix3 IDENT, ZERO;

  union
  {
    real m[3][3];
    Point3 col[3];
  };
  INLINE Matrix3() = default;
  INLINE explicit Matrix3(real a);
  INLINE Matrix3(const float *p) { copy(p); }

  INLINE void identity();
  INLINE void zero();

  INLINE const real *operator[](int i) const { return m[i]; }
  INLINE real *operator[](int i) { return m[i]; }
  INLINE Matrix3 operator-() const;
  INLINE Matrix3 operator+() const { return *this; }

  INLINE Matrix3 operator*(real) const;
  INLINE Matrix3 operator*(const Matrix3 &b) const
  {
    Matrix3 r;

    r.m[0][0] = m[0][0] * b.m[0][0] + m[1][0] * b.m[0][1] + m[2][0] * b.m[0][2];
    r.m[1][0] = m[0][0] * b.m[1][0] + m[1][0] * b.m[1][1] + m[2][0] * b.m[1][2];
    r.m[2][0] = m[0][0] * b.m[2][0] + m[1][0] * b.m[2][1] + m[2][0] * b.m[2][2];

    r.m[0][1] = m[0][1] * b.m[0][0] + m[1][1] * b.m[0][1] + m[2][1] * b.m[0][2];
    r.m[1][1] = m[0][1] * b.m[1][0] + m[1][1] * b.m[1][1] + m[2][1] * b.m[1][2];
    r.m[2][1] = m[0][1] * b.m[2][0] + m[1][1] * b.m[2][1] + m[2][1] * b.m[2][2];

    r.m[0][2] = m[0][2] * b.m[0][0] + m[1][2] * b.m[0][1] + m[2][2] * b.m[0][2];
    r.m[1][2] = m[0][2] * b.m[1][0] + m[1][2] * b.m[1][1] + m[2][2] * b.m[1][2];
    r.m[2][2] = m[0][2] * b.m[2][0] + m[1][2] * b.m[2][1] + m[2][2] * b.m[2][2];

    /*for(int i=0;i<3;++i)
      for(int j=0;j<3;++j) {
        real v=0;
        for(int k=0;k<3;++k) v+=m[k][i]*b.m[j][k];
        r.m[j][i]=v;
      }*/
    return r;
  }
  INLINE Point3 operator*(const Point3 &p) const
  {
    Point3 r;
    r[0] = m[0][0] * p[0] + m[1][0] * p[1] + m[2][0] * p[2];
    r[1] = m[0][1] * p[0] + m[1][1] * p[1] + m[2][1] * p[2];
    r[2] = m[0][2] * p[0] + m[1][2] * p[1] + m[2][2] * p[2];
    return r;
  }
  //==INLINE Quat operator *(const Quat&) const;
  INLINE Matrix3 operator+(const Matrix3 &) const;
  INLINE Matrix3 operator-(const Matrix3 &) const;

  INLINE Matrix3 &operator+=(const Matrix3 &);
  INLINE Matrix3 &operator-=(const Matrix3 &);
  INLINE Matrix3 &operator*=(const Matrix3 &);
  INLINE Matrix3 &operator*=(real);

  INLINE real det() const;

  INLINE void setcol(int i, const Point3 &v) { col[i] = v; }
  INLINE void setcol(int i, real x, real y, real z)
  {
    m[i][0] = x;
    m[i][1] = y;
    m[i][2] = z;
  }
  INLINE const Point3 &getcol(int i) const { return col[i]; }

#define eqtm(i, j) (m[(i)][(j)] == a.m[(i)][(j)])
  INLINE bool operator==(const Matrix3 &a) const
  {
    return (
      eqtm(0, 0) && eqtm(0, 1) && eqtm(0, 2) && eqtm(1, 0) && eqtm(1, 1) && eqtm(1, 2) && eqtm(2, 0) && eqtm(2, 1) && eqtm(2, 2));
  }
#undef eqtm

#define netm(i, j) (m[(i)][(j)] != a.m[(i)][(j)])
  INLINE bool operator!=(const Matrix3 &a) const
  {
    return (
      netm(0, 0) || netm(0, 1) || netm(0, 2) || netm(1, 0) || netm(1, 1) || netm(1, 2) || netm(2, 0) || netm(2, 1) || netm(2, 2));
  }
#undef netm
  INLINE void rotxTM(real a)
  {
    identity();
    if (a == 0)
      return;
    m[1][1] = m[2][2] = cosf(a);
    m[1][2] = -(m[2][1] = sinf(a));
  }

  INLINE void rotyTM(real a)
  {
    identity();
    if (a == 0)
      return;
    m[2][2] = m[0][0] = cosf(a);
    m[2][0] = -(m[0][2] = sinf(a));
  }

  INLINE void rotzTM(real a)
  {
    identity();
    if (a == 0)
      return;
    m[0][0] = m[1][1] = cosf(a);
    m[0][1] = -(m[1][0] = sinf(a));
  }
};

INLINE Matrix3 operator*(real, const Matrix3 &);
INLINE Matrix3 inverse(const Matrix3 &);

INLINE Matrix3 rotxM3(real a)
{
  Matrix3 m;
  m.rotxTM(a);
  return m;
}
INLINE Matrix3 rotyM3(real a)
{
  Matrix3 m;
  m.rotyTM(a);
  return m;
}
INLINE Matrix3 rotzM3(real a)
{
  Matrix3 m;
  m.rotzTM(a);
  return m;
}

INLINE Matrix3::Matrix3(real a)
{
  memset(m, 0, sizeof(m));
  m[0][0] = m[1][1] = m[2][2] = a;
}

INLINE void Matrix3::zero() { memset(m, 0, sizeof(m)); }

INLINE void Matrix3::identity()
{
  memset(m, 0, sizeof(m));
  m[0][0] = m[1][1] = m[2][2] = 1;
}

INLINE Matrix3 Matrix3::operator-() const
{
  Matrix3 a;
  a.m[0][0] = -m[0][0];
  a.m[0][1] = -m[0][1];
  a.m[0][2] = -m[0][2];
  a.m[1][0] = -m[1][0];
  a.m[1][1] = -m[1][1];
  a.m[1][2] = -m[1][2];
  a.m[2][0] = -m[2][0];
  a.m[2][1] = -m[2][1];
  a.m[2][2] = -m[2][2];
  // for(int i=0;i<3*3;++i) a.m[0][i]=-m[0][i];
  return a;
}

INLINE Matrix3 Matrix3::operator+(const Matrix3 &b) const
{
  Matrix3 r;
  r.m[0][0] = m[0][0] + b.m[0][0];
  r.m[0][1] = m[0][1] + b.m[0][1];
  r.m[0][2] = m[0][2] + b.m[0][2];
  r.m[1][0] = m[1][0] + b.m[1][0];
  r.m[1][1] = m[1][1] + b.m[1][1];
  r.m[1][2] = m[1][2] + b.m[1][2];
  r.m[2][0] = m[2][0] + b.m[2][0];
  r.m[2][1] = m[2][1] + b.m[2][1];
  r.m[2][2] = m[2][2] + b.m[2][2];
  // for(int i=0;i<3*3;++i) r.m[0][i]=m[0][i]+b.m[0][i];
  return r;
}

INLINE Matrix3 Matrix3::operator-(const Matrix3 &b) const
{
  Matrix3 r;
  r.m[0][0] = m[0][0] - b.m[0][0];
  r.m[0][1] = m[0][1] - b.m[0][1];
  r.m[0][2] = m[0][2] - b.m[0][2];
  r.m[1][0] = m[1][0] - b.m[1][0];
  r.m[1][1] = m[1][1] - b.m[1][1];
  r.m[1][2] = m[1][2] - b.m[1][2];
  r.m[2][0] = m[2][0] - b.m[2][0];
  r.m[2][1] = m[2][1] - b.m[2][1];
  r.m[2][2] = m[2][2] - b.m[2][2];
  // for(int i=0;i<3*3;++i) r.m[0][i]=m[0][i]-b.m[0][i];
  return r;
}

INLINE Matrix3 &Matrix3::operator+=(const Matrix3 &a)
{
  m[0][0] += a.m[0][0];
  m[0][1] += a.m[0][1];
  m[0][2] += a.m[0][2];
  m[1][0] += a.m[1][0];
  m[1][1] += a.m[1][1];
  m[1][2] += a.m[1][2];
  m[2][0] += a.m[2][0];
  m[2][1] += a.m[2][1];
  m[2][2] += a.m[2][2];
  // for(int i=0;i<3*3;++i) m[0][i]+=a.m[0][i];
  return *this;
}

INLINE Matrix3 &Matrix3::operator-=(const Matrix3 &a)
{
  m[0][0] -= a.m[0][0];
  m[0][1] -= a.m[0][1];
  m[0][2] -= a.m[0][2];
  m[1][0] -= a.m[1][0];
  m[1][1] -= a.m[1][1];
  m[1][2] -= a.m[1][2];
  m[2][0] -= a.m[2][0];
  m[2][1] -= a.m[2][1];
  m[2][2] -= a.m[2][2];
  // for(int i=0;i<3*3;++i) m[0][i]-=a.m[0][i];
  return *this;
}


INLINE Matrix3 Matrix3::operator*(real f) const
{
  Matrix3 a;
  a.m[0][0] = m[0][0] * f;
  a.m[0][1] = m[0][1] * f;
  a.m[0][2] = m[0][2] * f;
  a.m[1][0] = m[1][0] * f;
  a.m[1][1] = m[1][1] * f;
  a.m[1][2] = m[1][2] * f;
  a.m[2][0] = m[2][0] * f;
  a.m[2][1] = m[2][1] * f;
  a.m[2][2] = m[2][2] * f;
  // for(int i=0;i<3*3;++i) a.m[0][i]=m[0][i]*f;
  return a;
}

INLINE Matrix3 operator*(real f, const Matrix3 &a) { return a * f; }

INLINE Matrix3 &Matrix3::operator*=(real f)
{
  m[0][0] *= f;
  m[0][1] *= f;
  m[0][2] *= f;
  m[1][0] *= f;
  m[1][1] *= f;
  m[1][2] *= f;
  m[2][0] *= f;
  m[2][1] *= f;
  m[2][2] *= f;
  // for(int i=0;i<3*3;++i) m[0][i]*=f;
  return *this;
}

INLINE Matrix3 &Matrix3::operator*=(const Matrix3 &b)
{
  Matrix3 r;

  r.m[0][0] = m[0][0] * b.m[0][0] + m[1][0] * b.m[0][1] + m[2][0] * b.m[0][2];
  r.m[1][0] = m[0][0] * b.m[1][0] + m[1][0] * b.m[1][1] + m[2][0] * b.m[1][2];
  r.m[2][0] = m[0][0] * b.m[2][0] + m[1][0] * b.m[2][1] + m[2][0] * b.m[2][2];

  r.m[0][1] = m[0][1] * b.m[0][0] + m[1][1] * b.m[0][1] + m[2][1] * b.m[0][2];
  r.m[1][1] = m[0][1] * b.m[1][0] + m[1][1] * b.m[1][1] + m[2][1] * b.m[1][2];
  r.m[2][1] = m[0][1] * b.m[2][0] + m[1][1] * b.m[2][1] + m[2][1] * b.m[2][2];

  r.m[0][2] = m[0][2] * b.m[0][0] + m[1][2] * b.m[0][1] + m[2][2] * b.m[0][2];
  r.m[1][2] = m[0][2] * b.m[1][0] + m[1][2] * b.m[1][1] + m[2][2] * b.m[1][2];
  r.m[2][2] = m[0][2] * b.m[2][0] + m[1][2] * b.m[2][1] + m[2][2] * b.m[2][2];
  /*
    for(int i=0;i<3;++i)
      for(int j=0;j<3;++j) {
        real v=0;
        for(int k=0;k<3;++k) v+=m[k][i]*b.m[j][k];
        r.m[j][i]=v;
      }
  */
  copy(&r.m[0][0]);
  return *this;
}

/// this operator was not used, not sure what it really does ;-)
/*
INLINE Quat Matrix3::operator *(const Quat &q) const {
  Quat r;
  for(int i=0;i<3;++i) {
    real v=0;
    for(int j=0;j<3;++j) v+=m[j][i]*q[j];
    r[i]=v;
  }
  r.w=q.w;
  return r;
}
*/

INLINE real Matrix3::det() const
{
  return m[0][0] * m[1][1] * m[2][2] + m[0][1] * m[1][2] * m[2][0] + m[1][0] * m[2][1] * m[0][2] - m[0][2] * m[1][1] * m[2][0] -
         m[0][1] * m[1][0] * m[2][2] - m[1][2] * m[2][1] * m[0][0];
}

INLINE Matrix3 inverse(const Matrix3 &a)
{
  Matrix3 r;
  real inv_d = 1.0f / a.det();
  r.m[0][0] = (a.m[1][1] * a.m[2][2] - a.m[2][1] * a.m[1][2]) * inv_d;
  r.m[0][1] = (-a.m[0][1] * a.m[2][2] + a.m[2][1] * a.m[0][2]) * inv_d;
  r.m[0][2] = (a.m[0][1] * a.m[1][2] - a.m[1][1] * a.m[0][2]) * inv_d;
  r.m[1][0] = (-a.m[1][0] * a.m[2][2] + a.m[2][0] * a.m[1][2]) * inv_d;
  r.m[1][1] = (a.m[0][0] * a.m[2][2] - a.m[2][0] * a.m[0][2]) * inv_d;
  r.m[1][2] = (-a.m[0][0] * a.m[1][2] + a.m[1][0] * a.m[0][2]) * inv_d;
  r.m[2][0] = (a.m[1][0] * a.m[2][1] - a.m[2][0] * a.m[1][1]) * inv_d;
  r.m[2][1] = (-a.m[0][0] * a.m[2][1] + a.m[2][0] * a.m[0][1]) * inv_d;
  r.m[2][2] = (a.m[0][0] * a.m[1][1] - a.m[1][0] * a.m[0][1]) * inv_d;
  return r;
}

INLINE Matrix3 transpose(const Matrix3 &a)
{
  Matrix3 r;
  r.m[0][0] = a.m[0][0];
  r.m[0][1] = a.m[1][0];
  r.m[0][2] = a.m[2][0];
  r.m[1][0] = a.m[0][1];
  r.m[1][1] = a.m[1][1];
  r.m[1][2] = a.m[2][1];
  r.m[2][0] = a.m[0][2];
  r.m[2][1] = a.m[1][2];
  r.m[2][2] = a.m[2][2];
  return r;
}

/// make matrix that rotates around specified axis
INLINE Matrix3 makeM3(const Point3 &axis, real ang)
{
  Matrix3 m;
  real s, c;
  real xx, yy, zz, xy, yz, zx, xs, ys, zs, one_c;

  Point3 a = axis;
  c = length(a);
  if (c == 0)
  {
    m.identity();
    return m;
  }

  a /= c;
  s = sinf(ang);
  c = cosf(ang);

  xx = a.x * a.x;
  yy = a.y * a.y;
  zz = a.z * a.z;
  xy = a.x * a.y;
  yz = a.y * a.z;
  zx = a.z * a.x;
  xs = a.x * s;
  ys = a.y * s;
  zs = a.z * s;
  one_c = 1 - c;

  m.m[0][0] = one_c * xx + c;
  m.m[1][0] = one_c * xy - zs;
  m.m[2][0] = one_c * zx + ys;

  m.m[0][1] = one_c * xy + zs;
  m.m[1][1] = one_c * yy + c;
  m.m[2][1] = one_c * yz - xs;

  m.m[0][2] = one_c * zx - ys;
  m.m[1][2] = one_c * yz + xs;
  m.m[2][2] = one_c * zz + c;

  return m;
}

#undef INLINE

/// @}
