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
  // Matrix33(const Matrix33& a) {memcpy(m,a.m,sizeof(m));}
  // Matrix33& operator =(const Matrix33& a) {memcpy(m,a.m,sizeof(m));return *this;}
  INLINE Matrix33(const float *p) { memcpy(m, p, sizeof(m)); }

  INLINE void identity();
  INLINE void zero();

  INLINE const real *operator[](int i) const { return m[i]; }
  INLINE real *operator[](int i) { return m[i]; }
  INLINE Matrix33 operator-() const;
  INLINE Matrix33 operator+() const { return *this; }

  INLINE Matrix33 operator*(real) const;
  INLINE Matrix33 operator*(const Matrix33 &b) const
  {
    Matrix33 r;

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
  INLINE Matrix33 operator+(const Matrix33 &) const;
  INLINE Matrix33 operator-(const Matrix33 &) const;

  INLINE Matrix33 &operator+=(const Matrix33 &);
  INLINE Matrix33 &operator-=(const Matrix33 &);
  INLINE Matrix33 &operator*=(const Matrix33 &);
  INLINE Matrix33 &operator*=(real);

  INLINE real det() const;

  INLINE void setcol(int i, const Point3 &v)
  {
    m[i][0] = v[0];
    m[i][1] = v[1];
    m[i][2] = v[2];
  }
  INLINE void setcol(int i, real x, real y, real z)
  {
    m[i][0] = x;
    m[i][1] = y;
    m[i][2] = z;
  }
  INLINE Point3 getcol(int i) const { return Point3(m[i][0], m[i][1], m[i][2]); }

#define eqtm(i, j) (m[(i)][(j)] == a.m[(i)][(j)])
  INLINE bool operator==(const Matrix33 &a) const
  {
    return (
      eqtm(0, 0) && eqtm(0, 1) && eqtm(0, 2) && eqtm(1, 0) && eqtm(1, 1) && eqtm(1, 2) && eqtm(2, 0) && eqtm(2, 1) && eqtm(2, 2));
  }
#undef eqtm

#define netm(i, j) (m[(i)][(j)] != a.m[(i)][(j)])
  INLINE bool operator!=(const Matrix33 &a) const
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

INLINE Matrix33 operator*(real, const Matrix33 &);
INLINE Matrix33 inverse(const Matrix33 &);

INLINE Matrix33 rotxM3(real a)
{
  Matrix33 m;
  m.rotxTM(a);
  return m;
}
INLINE Matrix33 rotyM3(real a)
{
  Matrix33 m;
  m.rotyTM(a);
  return m;
}
INLINE Matrix33 rotzM3(real a)
{
  Matrix33 m;
  m.rotzTM(a);
  return m;
}

INLINE void Matrix33::zero() { memset(m, 0, sizeof(m)); }

INLINE void Matrix33::identity()
{
  memset(m, 0, sizeof(m));
  m[0][0] = m[1][1] = m[2][2] = 1;
}

INLINE Matrix33 Matrix33::operator-() const
{
  Matrix33 a;
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

INLINE Matrix33 Matrix33::operator+(const Matrix33 &b) const
{
  Matrix33 r;
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

INLINE Matrix33 Matrix33::operator-(const Matrix33 &b) const
{
  Matrix33 r;
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

INLINE Matrix33 &Matrix33::operator+=(const Matrix33 &a)
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

INLINE Matrix33 &Matrix33::operator-=(const Matrix33 &a)
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


INLINE Matrix33 Matrix33::operator*(real f) const
{
  Matrix33 a;
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

INLINE Matrix33 operator*(real f, const Matrix33 &a) { return a * f; }

INLINE Matrix33 &Matrix33::operator*=(real f)
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

INLINE Matrix33 &Matrix33::operator*=(const Matrix33 &b)
{
  Matrix33 r;

  r.m[0][0] = m[0][0] * b.m[0][0] + m[1][0] * b.m[0][1] + m[2][0] * b.m[0][2];
  r.m[1][0] = m[0][0] * b.m[1][0] + m[1][0] * b.m[1][1] + m[2][0] * b.m[1][2];
  r.m[2][0] = m[0][0] * b.m[2][0] + m[1][0] * b.m[2][1] + m[2][0] * b.m[2][2];

  r.m[0][1] = m[0][1] * b.m[0][0] + m[1][1] * b.m[0][1] + m[2][1] * b.m[0][2];
  r.m[1][1] = m[0][1] * b.m[1][0] + m[1][1] * b.m[1][1] + m[2][1] * b.m[1][2];
  r.m[2][1] = m[0][1] * b.m[2][0] + m[1][1] * b.m[2][1] + m[2][1] * b.m[2][2];

  r.m[0][2] = m[0][2] * b.m[0][0] + m[1][2] * b.m[0][1] + m[2][2] * b.m[0][2];
  r.m[1][2] = m[0][2] * b.m[1][0] + m[1][2] * b.m[1][1] + m[2][2] * b.m[1][2];
  r.m[2][2] = m[0][2] * b.m[2][0] + m[1][2] * b.m[2][1] + m[2][2] * b.m[2][2];
  memcpy(m, r.m, sizeof(m));
  return *this;
}

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

INLINE Matrix33 transpose(const Matrix33 &a)
{
  Matrix33 r;
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

  /// identity and zero constant matrices
  static TMatrix IDENT, ZERO;

  INLINE TMatrix() {}
  INLINE TMatrix(real);
  // TMatrix(const TMatrix& a) {memcpy(m,a.m,sizeof(m));}
  // TMatrix& operator =(const TMatrix& a) {memcpy(m,a.m,sizeof(m));return *this;}

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
  /// multiply only 3x3 parts, translation column is undefined
  INLINE TMatrix operator%(const TMatrix &b) const
  {
    TMatrix r;

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
  /// multiply only 3x3 parts, translation column is undefined
  /* INLINE Matrix3 operator %(const Matrix3 &b) const {
     Matrix3 r;
     r.m[0][0]=m[0][0]*b.m[0][0]+m[1][0]*b.m[0][1]+m[2][0]*b.m[0][2];
     r.m[1][0]=m[0][0]*b.m[1][0]+m[1][0]*b.m[1][1]+m[2][0]*b.m[1][2];
     r.m[2][0]=m[0][0]*b.m[2][0]+m[1][0]*b.m[2][1]+m[2][0]*b.m[2][2];

     r.m[0][1]=m[0][1]*b.m[0][0]+m[1][1]*b.m[0][1]+m[2][1]*b.m[0][2];
     r.m[1][1]=m[0][1]*b.m[1][0]+m[1][1]*b.m[1][1]+m[2][1]*b.m[1][2];
     r.m[2][1]=m[0][1]*b.m[2][0]+m[1][1]*b.m[2][1]+m[2][1]*b.m[2][2];

     r.m[0][2]=m[0][2]*b.m[0][0]+m[1][2]*b.m[0][1]+m[2][2]*b.m[0][2];
     r.m[1][2]=m[0][2]*b.m[1][0]+m[1][2]*b.m[1][1]+m[2][2]*b.m[1][2];
     r.m[2][2]=m[0][2]*b.m[2][0]+m[1][2]*b.m[2][1]+m[2][2]*b.m[2][2];
     return r;
   }*/
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
    r[0] = m[0][0] * p[0] + m[1][0] * p[1] + m[2][0] * p[2] + m[3][0];
    r[1] = m[0][1] * p[0] + m[1][1] * p[1] + m[2][1] * p[2] + m[3][1];
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

  INLINE void setcol(int i, const Point3 &v)
  {
    m[i][0] = v[0];
    m[i][1] = v[1];
    m[i][2] = v[2];
  }
  INLINE void setcol(int i, real x, real y, real z)
  {
    m[i][0] = x;
    m[i][1] = y;
    m[i][2] = z;
  }
  INLINE Point3 getcol(int i) const { return Point3(m[i][0], m[i][1], m[i][2]); }

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
    c = a.x * a.x + a.y * a.y + a.z * a.z; // length(a);
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
};

TMatrix operator*(real, const TMatrix &);
TMatrix inverse(const TMatrix &);

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
    a.m[0][i] = -m[0][i];
  return a;
}

INLINE TMatrix TMatrix::operator+(const TMatrix &b) const
{
  TMatrix r;
  for (int i = 0; i < 4 * 3; ++i)
    r.m[0][i] = m[0][i] + b.m[0][i];
  return r;
}

INLINE TMatrix TMatrix::operator-(const TMatrix &b) const
{
  TMatrix r;
  for (int i = 0; i < 4 * 3; ++i)
    r.m[0][i] = m[0][i] - b.m[0][i];
  return r;
}

INLINE TMatrix &TMatrix::operator+=(const TMatrix &a)
{
  for (int i = 0; i < 4 * 3; ++i)
    m[0][i] += a.m[0][i];
  return *this;
}

INLINE TMatrix &TMatrix::operator-=(const TMatrix &a)
{
  for (int i = 0; i < 4 * 3; ++i)
    m[0][i] -= a.m[0][i];
  return *this;
}

INLINE TMatrix TMatrix::operator*(real f) const
{
  TMatrix a;
  for (int i = 0; i < 4 * 3; ++i)
    a.m[0][i] = m[0][i] * f;
  return a;
}

INLINE TMatrix operator*(real f, const TMatrix &a) { return a * f; }

INLINE TMatrix &TMatrix::operator*=(real f)
{
  for (int i = 0; i < 4 * 3; ++i)
    m[0][i] *= f;
  return *this;
}

class BBox2
{
public:
  Point2 lim[2];
  INLINE BBox2() { setempty(); }
  BBox2(const Point2 &a, real s) { makebox(a, s); }
  // BBox2(const BBox2& a) {lim[0]=a.lim[0];lim[1]=a.lim[1];}
  // BBox2& operator =(const BBox2& a) {lim[0]=a.lim[0];lim[1]=a.lim[1];return *this;}
  /*
  INLINE BBox2(const BBox3& b) {
  setempty();
  if(b.lim[0].x<lim[0].x) lim[0].x=b.lim[0].x;
  if(b.lim[1].x>lim[1].x) lim[1].x=b.lim[1].x;
  if(b.lim[0].y<lim[0].y) lim[0].y=b.lim[0].y;
  if(b.lim[1].y>lim[1].y) lim[1].y=b.lim[1].y;
  }
  */

  INLINE void setempty()
  {
    lim[0] = Point2(MAX_REAL, MAX_REAL);
    lim[1] = Point2(MIN_REAL, MIN_REAL);
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
  INLINE operator const Point2 *() const { return lim; }
  INLINE operator Point2 *() { return lim; }

  INLINE BBox2 &operator+=(const Point3 &p)
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
  INLINE BBox2 &operator+=(const BBox2 &b)
  {
    if (b.isempty())
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
};

INLINE real lengthSq(const Point3 &a) { return LengthSquared(a); }
INLINE real length(const Point3 &a) { return Length(a); }

class BBox3
{
public:
  Point3 lim[2];
  INLINE BBox3() { setempty(); }
  INLINE BBox3(Point3 p, real s) { makecube(p, s); }

  INLINE void setempty()
  {
    lim[0] = Point3(MAX_REAL, MAX_REAL, MAX_REAL);
    lim[1] = Point3(MIN_REAL, MIN_REAL, MIN_REAL);
  }
  INLINE bool isempty() const { return lim[0].x > lim[1].x || lim[0].y > lim[1].y || lim[0].z > lim[1].z; }
  INLINE void makecube(const Point3 &p, real s)
  {
    Point3 d(s / 2, s / 2, s / 2);
    lim[0] = p - d;
    lim[1] = p + d;
  }
  INLINE Point3 center() const { return (lim[0] + lim[1]) * 0.5f; }
  INLINE Point3 width() const { return lim[1] - lim[0]; }

  INLINE const Point3 &operator[](int i) const { return lim[i]; }
  INLINE Point3 &operator[](int i) { return lim[i]; }
  INLINE operator const Point3 *() const { return lim; }
  INLINE operator Point3 *() { return lim; }

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
  INLINE BBox3 &operator+=(const BBox3 &b)
  {
    if (b.isempty())
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

  /// check intersection with point
  bool operator&(const Point3 &p) const
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
  bool operator&(const BBox3 &b) const
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
    if (b.lim[0].z > lim[1].z)
      return 0;
    if (b.lim[1].z < lim[0].z)
      return 0;
    return 1;
  }
};

INLINE BBox3 operator*(const Matrix3 &tm, const BBox3 &b)
{
  BBox3 box;
  if (b.isempty())
    return box;
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
    {
      box += tm.PointTransform(Point3(b.lim[i].x, b.lim[j].y, b.lim[0].z));
      box += tm.PointTransform(Point3(b.lim[i].x, b.lim[j].y, b.lim[1].z));
    }
  return box;
}

class StaticMeshRayTracer
{
public:
  virtual ~StaticMeshRayTracer() {}
  //! Tests ray hit to object and returns parameters of hit (if happen)
  virtual bool traceray(Point3 &p, Point3 &dir, real &t, int &fi, Point3 &bc) = 0;

  //! trace backfaces
  virtual bool traceray_back(Point3 &p, Point3 &dir, real &t, int &fi, Point3 &bc) = 0;
  //! Builds (or rebuilds) object for given mesh
  virtual void build(Mesh &m) = 0;
};
