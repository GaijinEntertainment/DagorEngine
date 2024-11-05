//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>
#include <math/dag_Point2.h>
#include <math/dag_Quat.h>
#include <math/dag_Matrix3.h>
#include <string.h>
#include <math/dag_mathBase.h>

#define INLINE __forceinline

/// @addtogroup math
/// @{

// TMatrix - Transformation matrix
/**
  4x3 transformation matrix.
  last column (3rd, counting from zero) is translation
  @sa Matrix3 TMatrix4 Point3 Point2 Point4
*/
class TMatrix
{
public:
  union
  {
    real m[4][3];
    real array[12];
    Point3 col[4];
  };

  /// identity and zero constant matrices
  static const TMatrix IDENT, ZERO;

  INLINE TMatrix() = default;
  INLINE explicit TMatrix(real);

  INLINE void identity();
  INLINE void zero();

  INLINE const real *operator[](int i) const { return m[i]; }
  INLINE real *operator[](int i) { return m[i]; }
  INLINE TMatrix operator-() const;
  INLINE TMatrix operator+() const { return *this; }

  INLINE TMatrix operator*(real) const;
  /// multiply matrices
  INLINE TMatrix operator*(const TMatrix &b) const
  {
    TMatrix r;

    r.m[0][0] = m[0][0] * b.m[0][0] + m[1][0] * b.m[0][1] + m[2][0] * b.m[0][2];
    r.m[1][0] = m[0][0] * b.m[1][0] + m[1][0] * b.m[1][1] + m[2][0] * b.m[1][2];
    r.m[2][0] = m[0][0] * b.m[2][0] + m[1][0] * b.m[2][1] + m[2][0] * b.m[2][2];
    r.m[3][0] = m[3][0] + m[0][0] * b.m[3][0] + m[1][0] * b.m[3][1] + m[2][0] * b.m[3][2];

    r.m[0][1] = m[0][1] * b.m[0][0] + m[1][1] * b.m[0][1] + m[2][1] * b.m[0][2];
    r.m[1][1] = m[0][1] * b.m[1][0] + m[1][1] * b.m[1][1] + m[2][1] * b.m[1][2];
    r.m[2][1] = m[0][1] * b.m[2][0] + m[1][1] * b.m[2][1] + m[2][1] * b.m[2][2];
    r.m[3][1] = m[3][1] + m[0][1] * b.m[3][0] + m[1][1] * b.m[3][1] + m[2][1] * b.m[3][2];

    r.m[0][2] = m[0][2] * b.m[0][0] + m[1][2] * b.m[0][1] + m[2][2] * b.m[0][2];
    r.m[1][2] = m[0][2] * b.m[1][0] + m[1][2] * b.m[1][1] + m[2][2] * b.m[1][2];
    r.m[2][2] = m[0][2] * b.m[2][0] + m[1][2] * b.m[2][1] + m[2][2] * b.m[2][2];
    r.m[3][2] = m[3][2] + m[0][2] * b.m[3][0] + m[1][2] * b.m[3][1] + m[2][2] * b.m[3][2];
    return r;
  }
  /// multiply only 3x3 parts, translation column is zero
  INLINE TMatrix operator%(const TMatrix &b) const
  {
    TMatrix r;

    r.m[0][0] = m[0][0] * b.m[0][0] + m[1][0] * b.m[0][1] + m[2][0] * b.m[0][2];
    r.m[1][0] = m[0][0] * b.m[1][0] + m[1][0] * b.m[1][1] + m[2][0] * b.m[1][2];
    r.m[2][0] = m[0][0] * b.m[2][0] + m[1][0] * b.m[2][1] + m[2][0] * b.m[2][2];
    r.m[3][0] = 0.f;

    r.m[0][1] = m[0][1] * b.m[0][0] + m[1][1] * b.m[0][1] + m[2][1] * b.m[0][2];
    r.m[1][1] = m[0][1] * b.m[1][0] + m[1][1] * b.m[1][1] + m[2][1] * b.m[1][2];
    r.m[2][1] = m[0][1] * b.m[2][0] + m[1][1] * b.m[2][1] + m[2][1] * b.m[2][2];
    r.m[3][1] = 0.f;

    r.m[0][2] = m[0][2] * b.m[0][0] + m[1][2] * b.m[0][1] + m[2][2] * b.m[0][2];
    r.m[1][2] = m[0][2] * b.m[1][0] + m[1][2] * b.m[1][1] + m[2][2] * b.m[1][2];
    r.m[2][2] = m[0][2] * b.m[2][0] + m[1][2] * b.m[2][1] + m[2][2] * b.m[2][2];
    r.m[3][2] = 0.f;
    return r;
  }

  INLINE Matrix3 operator%(const Matrix3 &b) const
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
    return r;
  }
  /// transform #Point3 point through this matrix
  INLINE Point3 operator*(const Point3 &p) const
  {
    Point3 r;
    r[0] = m[0][0] * p[0] + m[1][0] * p[1] + m[2][0] * p[2] + m[3][0];
    r[1] = m[0][1] * p[0] + m[1][1] * p[1] + m[2][1] * p[2] + m[3][1];
    r[2] = m[0][2] * p[0] + m[1][2] * p[1] + m[2][2] * p[2] + m[3][2];
    return r;
  }

  INLINE Point2 operator*(const Point2 &p) const
  {
    Point2 r;
    r[0] = m[0][0] * p[0] + m[1][0] * p[1] + m[3][0];
    r[1] = m[0][1] * p[0] + m[1][1] * p[1] + m[3][1];
    return r;
  }

  /// transform #Point3 vector through this matrix,
  /// translation is not applied
  INLINE Point3 operator%(const Point3 &p) const
  {
    Point3 r;
    r[0] = m[0][0] * p[0] + m[1][0] * p[1] + m[2][0] * p[2];
    r[1] = m[0][1] * p[0] + m[1][1] * p[1] + m[2][1] * p[2];
    r[2] = m[0][2] * p[0] + m[1][2] * p[1] + m[2][2] * p[2];
    return r;
  }
  INLINE Quat operator*(const Quat &) const;
  INLINE TMatrix operator+(const TMatrix &) const;
  INLINE TMatrix operator-(const TMatrix &) const;

  INLINE TMatrix &operator+=(const TMatrix &);
  INLINE TMatrix &operator-=(const TMatrix &);
  INLINE TMatrix &operator*=(const TMatrix &);
  INLINE TMatrix &operator*=(real);

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
  INLINE bool operator==(const TMatrix &a) const
  {
    return (eqtm(0, 0) && eqtm(0, 1) && eqtm(0, 2) && eqtm(1, 0) && eqtm(1, 1) && eqtm(1, 2) && eqtm(2, 0) && eqtm(2, 1) &&
            eqtm(2, 2) && eqtm(3, 0) && eqtm(3, 1) && eqtm(3, 2));
  }
#undef eqtm

#define netm(i, j) (m[(i)][(j)] != a.m[(i)][(j)])
  INLINE bool operator!=(const TMatrix &a) const
  {
    return (netm(0, 0) || netm(0, 1) || netm(0, 2) || netm(1, 0) || netm(1, 1) || netm(1, 2) || netm(2, 0) || netm(2, 1) ||
            netm(2, 2) || netm(3, 0) || netm(3, 1) || netm(3, 2));
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
  INLINE void makeTM(const Point3 &axis, real ang)
  {
    real s, c;
    real xx, yy, zz, xy, yz, zx, xs, ys, zs, one_c;

    Point3 a = axis;
    c = length(a);
    if (c == 0)
    {
      identity();
      return;
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

    m[0][0] = one_c * xx + c;
    m[1][0] = one_c * xy - zs;
    m[2][0] = one_c * zx + ys;

    m[0][1] = one_c * xy + zs;
    m[1][1] = one_c * yy + c;
    m[2][1] = one_c * yz - xs;

    m[0][2] = one_c * zx - ys;
    m[1][2] = one_c * yz + xs;
    m[2][2] = one_c * zz + c;

    m[3][0] = m[3][1] = m[3][2] = 0;
  }
  INLINE void makeTM(const Quat &q)
  {
    if (q.x == 0 && q.y == 0 && q.z == 0 && q.w == 0)
    {
      identity();
      return;
    }

    m[0][0] = (q.x * q.x + q.w * q.w) * 2 - 1;
    m[1][0] = (q.x * q.y - q.z * q.w) * 2;
    m[2][0] = (q.x * q.z + q.y * q.w) * 2;

    m[0][1] = (q.y * q.x + q.z * q.w) * 2;
    m[1][1] = (q.y * q.y + q.w * q.w) * 2 - 1;
    m[2][1] = (q.y * q.z - q.x * q.w) * 2;

    m[0][2] = (q.z * q.x - q.y * q.w) * 2;
    m[1][2] = (q.z * q.y + q.x * q.w) * 2;
    m[2][2] = (q.z * q.z + q.w * q.w) * 2 - 1;

    m[3][0] = m[3][1] = m[3][2] = 0;
  }


  void orthonormalize() // Remove scale.
  {
    setcol(2, normalize(getcol(0) % getcol(1)));
    setcol(1, normalize(getcol(2) % getcol(0)));
    setcol(0, normalize(getcol(1) % getcol(2)));
  }
};

TMatrix operator*(real, const TMatrix &);
TMatrix inverse(const TMatrix &, float determinant);
TMatrix inverse(const TMatrix &);

INLINE TMatrix orthonormalized_inverse(const TMatrix &a)
{
  TMatrix r;
  r.m[0][0] = a.m[0][0];
  r.m[0][1] = a.m[1][0];
  r.m[0][2] = a.m[2][0];
  r.m[1][0] = a.m[0][1];
  r.m[1][1] = a.m[1][1];
  r.m[1][2] = a.m[2][1];
  r.m[2][0] = a.m[0][2];
  r.m[2][1] = a.m[1][2];
  r.m[2][2] = a.m[2][2];
  r.setcol(3, -(r % a.getcol(3)));
  return r;
}


INLINE TMatrix rotxTM(real a)
{
  TMatrix m;
  m.identity();
  if (a == 0)
    return m;
  m.m[1][1] = m.m[2][2] = cosf(a);
  m.m[1][2] = -(m.m[2][1] = sinf(a));
  return m;
}

INLINE TMatrix rotyTM(real a)
{
  TMatrix m;
  m.identity();
  if (a == 0)
    return m;
  m.m[2][2] = m.m[0][0] = cosf(a);
  m.m[2][0] = -(m.m[0][2] = sinf(a));
  return m;
}

INLINE TMatrix rotzTM(real a)
{
  TMatrix m;
  m.identity();
  if (a == 0)
    return m;
  m.m[0][0] = m.m[1][1] = cosf(a);
  m.m[0][1] = -(m.m[1][0] = sinf(a));
  return m;
}

INLINE TMatrix::TMatrix(real a)
{
  memset(m, 0, sizeof(m));
  m[0][0] = m[1][1] = m[2][2] = a;
}

INLINE void TMatrix::zero() { memset(m, 0, sizeof(m)); }

INLINE void TMatrix::identity()
{
  memset(m, 0, sizeof(m));
  m[0][0] = m[1][1] = m[2][2] = 1;
}

INLINE TMatrix TMatrix::operator-() const
{
  TMatrix a;
  for (int i = 0; i < 4 * 3; ++i)
    a.array[i] = -array[i];
  return a;
}

INLINE TMatrix TMatrix::operator+(const TMatrix &b) const
{
  TMatrix r;
  for (int i = 0; i < 4 * 3; ++i)
    r.array[i] = array[i] + b.array[i];
  return r;
}

INLINE TMatrix TMatrix::operator-(const TMatrix &b) const
{
  TMatrix r;
  for (int i = 0; i < 4 * 3; ++i)
    r.array[i] = array[i] - b.array[i];
  return r;
}

INLINE TMatrix &TMatrix::operator+=(const TMatrix &a)
{
  for (int i = 0; i < 4 * 3; ++i)
    array[i] += a.array[i];
  return *this;
}

INLINE TMatrix &TMatrix::operator-=(const TMatrix &a)
{
  for (int i = 0; i < 4 * 3; ++i)
    array[i] -= a.array[i];
  return *this;
}

INLINE TMatrix TMatrix::operator*(real f) const
{
  TMatrix a;
  for (int i = 0; i < 4 * 3; ++i)
    a.array[i] = array[i] * f;
  return a;
}

INLINE TMatrix operator*(real f, const TMatrix &a) { return a * f; }

INLINE TMatrix &TMatrix::operator*=(real f)
{
  for (int i = 0; i < 4 * 3; ++i)
    array[i] *= f;
  return *this;
}


INLINE Point3 operator*(const Point3 &p, const TMatrix &m)
{
  Point3 r;
  for (int i = 0; i < 3; ++i)
  {
    r[i] = m.m[3][i];
    for (int j = 0; j < 3; ++j)
      r[i] += p[j] * m.m[j][i];
  }
  return r;
}

INLINE Matrix3 operator%(const Matrix3 &a, const TMatrix &b)
{
  Matrix3 r;
  r.m[0][0] = a.m[0][0] * b.m[0][0] + a.m[1][0] * b.m[0][1] + a.m[2][0] * b.m[0][2];
  r.m[1][0] = a.m[0][0] * b.m[1][0] + a.m[1][0] * b.m[1][1] + a.m[2][0] * b.m[1][2];
  r.m[2][0] = a.m[0][0] * b.m[2][0] + a.m[1][0] * b.m[2][1] + a.m[2][0] * b.m[2][2];

  r.m[0][1] = a.m[0][1] * b.m[0][0] + a.m[1][1] * b.m[0][1] + a.m[2][1] * b.m[0][2];
  r.m[1][1] = a.m[0][1] * b.m[1][0] + a.m[1][1] * b.m[1][1] + a.m[2][1] * b.m[1][2];
  r.m[2][1] = a.m[0][1] * b.m[2][0] + a.m[1][1] * b.m[2][1] + a.m[2][1] * b.m[2][2];

  r.m[0][2] = a.m[0][2] * b.m[0][0] + a.m[1][2] * b.m[0][1] + a.m[2][2] * b.m[0][2];
  r.m[1][2] = a.m[0][2] * b.m[1][0] + a.m[1][2] * b.m[1][1] + a.m[2][2] * b.m[1][2];
  r.m[2][2] = a.m[0][2] * b.m[2][0] + a.m[1][2] * b.m[2][1] + a.m[2][2] * b.m[2][2];
  return r;
}

INLINE TMatrix &TMatrix::operator*=(const TMatrix &b)
{
  TMatrix r;
  r.m[0][0] = m[0][0] * b.m[0][0] + m[1][0] * b.m[0][1] + m[2][0] * b.m[0][2];
  r.m[1][0] = m[0][0] * b.m[1][0] + m[1][0] * b.m[1][1] + m[2][0] * b.m[1][2];
  r.m[2][0] = m[0][0] * b.m[2][0] + m[1][0] * b.m[2][1] + m[2][0] * b.m[2][2];
  r.m[3][0] = m[3][0] + m[0][0] * b.m[3][0] + m[1][0] * b.m[3][1] + m[2][0] * b.m[3][2];

  r.m[0][1] = m[0][1] * b.m[0][0] + m[1][1] * b.m[0][1] + m[2][1] * b.m[0][2];
  r.m[1][1] = m[0][1] * b.m[1][0] + m[1][1] * b.m[1][1] + m[2][1] * b.m[1][2];
  r.m[2][1] = m[0][1] * b.m[2][0] + m[1][1] * b.m[2][1] + m[2][1] * b.m[2][2];
  r.m[3][1] = m[3][1] + m[0][1] * b.m[3][0] + m[1][1] * b.m[3][1] + m[2][1] * b.m[3][2];

  r.m[0][2] = m[0][2] * b.m[0][0] + m[1][2] * b.m[0][1] + m[2][2] * b.m[0][2];
  r.m[1][2] = m[0][2] * b.m[1][0] + m[1][2] * b.m[1][1] + m[2][2] * b.m[1][2];
  r.m[2][2] = m[0][2] * b.m[2][0] + m[1][2] * b.m[2][1] + m[2][2] * b.m[2][2];
  r.m[3][2] = m[3][2] + m[0][2] * b.m[3][0] + m[1][2] * b.m[3][1] + m[2][2] * b.m[3][2];

  float *dst = &m[0][0], *src = &r.m[0][0];
  for (int i = 0; i < 12; i++)
    dst[i] = src[i];

  return *this;
}

INLINE Quat TMatrix::operator*(const Quat &q) const
{
  Quat r;
  for (int i = 0; i < 3; ++i)
  {
    real v = 0;
    for (int j = 0; j < 3; ++j)
      v += m[j][i] * q[j];
    r[i] = v;
  }
  r.w = q.w;
  return r;
}

INLINE real TMatrix::det() const
{
  return m[0][0] * m[1][1] * m[2][2] + m[0][1] * m[1][2] * m[2][0] + m[1][0] * m[2][1] * m[0][2] - m[0][2] * m[1][1] * m[2][0] -
         m[0][1] * m[1][0] * m[2][2] - m[1][2] * m[2][1] * m[0][0];
}

INLINE TMatrix inverse(const TMatrix &a, float d)
{
  TMatrix r;
  G_FAST_ASSERT(fabsf(d) > 1e-12f);
  float inv_d = 1.0f / d;
  r.m[0][0] = (a.m[1][1] * a.m[2][2] - a.m[2][1] * a.m[1][2]) * inv_d;
  r.m[0][1] = (-a.m[0][1] * a.m[2][2] + a.m[2][1] * a.m[0][2]) * inv_d;
  r.m[0][2] = (a.m[0][1] * a.m[1][2] - a.m[1][1] * a.m[0][2]) * inv_d;
  r.m[1][0] = (-a.m[1][0] * a.m[2][2] + a.m[2][0] * a.m[1][2]) * inv_d;
  r.m[1][1] = (a.m[0][0] * a.m[2][2] - a.m[2][0] * a.m[0][2]) * inv_d;
  r.m[1][2] = (-a.m[0][0] * a.m[1][2] + a.m[1][0] * a.m[0][2]) * inv_d;
  r.m[2][0] = (a.m[1][0] * a.m[2][1] - a.m[2][0] * a.m[1][1]) * inv_d;
  r.m[2][1] = (-a.m[0][0] * a.m[2][1] + a.m[2][0] * a.m[0][1]) * inv_d;
  r.m[2][2] = (a.m[0][0] * a.m[1][1] - a.m[1][0] * a.m[0][1]) * inv_d;

  r.m[3][0] = -r.m[0][0] * a.m[3][0] - r.m[1][0] * a.m[3][1] - r.m[2][0] * a.m[3][2];
  r.m[3][1] = -r.m[0][1] * a.m[3][0] - r.m[1][1] * a.m[3][1] - r.m[2][1] * a.m[3][2];
  r.m[3][2] = -r.m[0][2] * a.m[3][0] - r.m[1][2] * a.m[3][1] - r.m[2][2] * a.m[3][2];
  return r;
}
INLINE TMatrix inverse(const TMatrix &a) { return inverse(a, a.det()); }

/// make matrix that rotates around specified axis
INLINE TMatrix makeTM(const Point3 &axis, real ang)
{
  TMatrix m;
  m.makeTM(axis, ang);
  return m;
}

/// make matrix from #Quat
INLINE TMatrix makeTM(const Quat &q)
{
  TMatrix m;

  if (q.x == 0 && q.y == 0 && q.z == 0 && q.w == 0)
  {
    m.identity();
    return m;
  }

  m.m[0][0] = (q.x * q.x + q.w * q.w) * 2 - 1;
  m.m[1][0] = (q.x * q.y - q.z * q.w) * 2;
  m.m[2][0] = (q.x * q.z + q.y * q.w) * 2;

  m.m[0][1] = (q.y * q.x + q.z * q.w) * 2;
  m.m[1][1] = (q.y * q.y + q.w * q.w) * 2 - 1;
  m.m[2][1] = (q.y * q.z - q.x * q.w) * 2;

  m.m[0][2] = (q.z * q.x - q.y * q.w) * 2;
  m.m[1][2] = (q.z * q.y + q.x * q.w) * 2;
  m.m[2][2] = (q.z * q.z + q.w * q.w) * 2 - 1;

  m.m[3][0] = m.m[3][1] = m.m[3][2] = 0;

  return m;
}


INLINE Quat::Quat(const TMatrix &om)
{
  TMatrix m = om;
  if (m.det() < 0)
    m.setcol(2, -m.getcol(2));
  real xx[2], yy[2], zz[2], ww[2];
  xx[0] = qterm(1 + m.m[0][0] - m.m[1][1] - m.m[2][2]);
  xx[1] = -xx[0];
  yy[0] = qterm(1 - m.m[0][0] + m.m[1][1] - m.m[2][2]);
  yy[1] = -yy[0];
  zz[0] = qterm(1 - m.m[0][0] - m.m[1][1] + m.m[2][2]);
  zz[1] = -zz[0];
  ww[0] = qterm(1 + m.m[0][0] + m.m[1][1] + m.m[2][2]);
  ww[1] = -ww[0];
  real md = MAX_REAL;
  x = y = z = w = 0.f;
  for (int a = 0; a < 2; ++a)
    for (int b = 0; b < 2; ++b)
      for (int c = 0; c < 2; ++c)
        for (int d = 0; d < 2; ++d)
        {
          real dif = sqr(xx[a] * yy[b] * 2 - zz[c] * ww[d] * 2 - m.m[1][0]) + sqr(xx[a] * zz[c] * 2 + yy[b] * ww[d] * 2 - m.m[2][0]) +
                     sqr(yy[b] * xx[a] * 2 + zz[c] * ww[d] * 2 - m.m[0][1]) + sqr(yy[b] * zz[c] * 2 - xx[a] * ww[d] * 2 - m.m[2][1]) +
                     sqr(zz[c] * xx[a] * 2 - yy[b] * ww[d] * 2 - m.m[0][2]) + sqr(zz[c] * yy[b] * 2 + xx[a] * ww[d] * 2 - m.m[1][2]);
          if (dif < md)
          {
            md = dif;
            x = xx[a];
            y = yy[b];
            z = zz[c];
            w = ww[d];
          }
        }
}


INLINE bool are_approximately_equal(const TMatrix &a, const TMatrix &b, float epsilon = 8 * FLT_EPSILON) // Three bits precision by
                                                                                                         // default.
{
#define EQ(i, j) are_approximately_equal(a.m[(i)][(j)], b.m[(i)][(j)], epsilon)
  return (EQ(0, 0) && EQ(0, 1) && EQ(0, 2) && EQ(1, 0) && EQ(1, 1) && EQ(1, 2) && EQ(2, 0) && EQ(2, 1) && EQ(2, 2) && EQ(3, 0) &&
          EQ(3, 1) && EQ(3, 2));
#undef EQ
}

#undef INLINE

/// @}
