//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_TMatrix.h>
#include <math/dag_Point4.h>

#define INLINE __forceinline

/// @addtogroup math
/// @{

// TMatrix4 - Transformation matrix
/**
  4x4 Transformation matrix
  @sa TMatrix TMatrix4 Point3 Point2 Point4
*/
class TMatrix4
{
public:
  union
  {
    struct
    {
      real _11, _12, _13, _14;
      real _21, _22, _23, _24;
      real _31, _32, _33, _34;
      real _41, _42, _43, _44;
    };
    real m[4][4];
    Point4 row[4];
  };

  static const TMatrix4 IDENT, ZERO;

  INLINE TMatrix4() = default;
  INLINE explicit TMatrix4(real);
  INLINE TMatrix4(const TMatrix &tm)
  {
    m[0][0] = tm.m[0][0], m[0][1] = tm.m[0][1], m[0][2] = tm.m[0][2], m[0][3] = 0;
    m[1][0] = tm.m[1][0], m[1][1] = tm.m[1][1], m[1][2] = tm.m[1][2], m[1][3] = 0;
    m[2][0] = tm.m[2][0], m[2][1] = tm.m[2][1], m[2][2] = tm.m[2][2], m[2][3] = 0;
    m[3][0] = tm.m[3][0], m[3][1] = tm.m[3][1], m[3][2] = tm.m[3][2], m[3][3] = 1;
  }
  INLINE TMatrix4(real _m00, real _m01, real _m02, real _m03, real _m10, real _m11, real _m12, real _m13, real _m20, real _m21,
    real _m22, real _m23, real _m30, real _m31, real _m32, real _m33)
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

  INLINE real &operator()(int iRow, int iColumn) { return m[iRow][iColumn]; }
  INLINE const real &operator()(int iRow, int iColumn) const { return m[iRow][iColumn]; }

  INLINE void identity();
  INLINE void zero();

  INLINE const real *operator[](int i) const { return m[i]; }
  INLINE real *operator[](int i) { return m[i]; }
  INLINE TMatrix4 operator-() const;
  INLINE TMatrix4 operator+() const { return *this; }

  INLINE TMatrix4 operator*(real) const;
  INLINE TMatrix4 operator*(const TMatrix4 &b) const
  {
    TMatrix4 r(*this);
    r *= b;
    return r;
  }
  INLINE TMatrix4 operator+(const TMatrix4 &) const;
  INLINE TMatrix4 operator-(const TMatrix4 &) const;

  INLINE TMatrix4 &operator+=(const TMatrix4 &);
  INLINE TMatrix4 &operator-=(const TMatrix4 &);
  INLINE TMatrix4 &operator*=(const TMatrix4 &);
  INLINE TMatrix4 &operator*=(real);

  INLINE real det() const;

  INLINE void setcol(int i, const Point4 &v)
  {
    m[0][i] = v[0];
    m[1][i] = v[1];
    m[2][i] = v[2];
    m[3][i] = v[3];
  }
  INLINE void setcol(int i, real x, real y, real z, real w)
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
  INLINE void setrow(int i, real x, real y, real z, real w)
  {
    m[i][0] = x;
    m[i][1] = y;
    m[i][2] = z;
    ;
    m[i][3] = w;
  }
  INLINE const Point4 &getrow(int i) const { return row[i]; }

  INLINE TMatrix4 transpose() const
  {
    TMatrix4 r;
    for (int i = 0; i < 4; ++i)
      for (int j = 0; j < 4; ++j)
        r.m[i][j] = m[j][i];
    return r;
  }

#define eqtm(i, j) (m[(i)][(j)] == a.m[(i)][(j)])
  INLINE bool operator==(const TMatrix4 &a) const
  {
    return (eqtm(0, 0) && eqtm(0, 1) && eqtm(0, 2) && eqtm(0, 3) && eqtm(1, 0) && eqtm(1, 1) && eqtm(1, 2) && eqtm(1, 3) &&
            eqtm(2, 0) && eqtm(2, 1) && eqtm(2, 2) && eqtm(2, 3) && eqtm(3, 0) && eqtm(3, 1) && eqtm(3, 2) && eqtm(3, 3));
  }
#undef eqtm

#define netm(i, j) (m[(i)][(j)] != a.m[(i)][(j)])
  INLINE bool operator!=(const TMatrix4 &a) const
  {
    return (netm(0, 0) || netm(0, 1) || netm(0, 2) || netm(0, 3) || netm(1, 0) || netm(1, 1) || netm(1, 2) || netm(1, 3) ||
            netm(2, 0) || netm(2, 1) || netm(2, 2) || netm(2, 3) || netm(3, 0) || netm(3, 1) || netm(3, 2) || netm(3, 3));
  }
#undef netm

  void transform(const Point3 &in_vector, Point4 &out_vector) const
  {
    for (int i = 0; i < 4; ++i)
    {
      out_vector[i] = in_vector[0] * m[0][i] + in_vector[1] * m[1][i] + in_vector[2] * m[2][i] + m[3][i];
    }
  }

  void transformNormal(const Point3 &in_vector, Point4 &out_vector) const
  {
    for (int i = 0; i < 4; ++i)
    {
      out_vector[i] = in_vector[0] * m[0][i] + in_vector[1] * m[1][i] + in_vector[2] * m[2][i];
    }
  }
};

INLINE TMatrix4 operator*(real, const TMatrix4 &);
INLINE TMatrix4 inverse43(const TMatrix4 &);

INLINE TMatrix4::TMatrix4(real a)
{
  memset(m, 0, sizeof(m));
  _11 = _22 = _33 = _44 = a;
}

INLINE void TMatrix4::zero() { memset(m, 0, sizeof(m)); }

INLINE void TMatrix4::identity()
{
  memset(m, 0, sizeof(m));
  _11 = _22 = _33 = _44 = 1;
}

INLINE TMatrix4 TMatrix4::operator-() const
{
  TMatrix4 a;
  for (uint32_t i = 0; i < 4; ++i)
    a.row[i] = -row[i];
  return a;
}

INLINE TMatrix4 TMatrix4::operator+(const TMatrix4 &b) const
{
  TMatrix4 r;
  for (uint32_t i = 0; i < 4; ++i)
    r.row[i] = row[i] + b.row[i];
  return r;
}

INLINE TMatrix4 TMatrix4::operator-(const TMatrix4 &b) const
{
  TMatrix4 r;
  for (uint32_t i = 0; i < 4; ++i)
    r.row[i] = row[i] - b.row[i];
  return r;
}

INLINE TMatrix4 &TMatrix4::operator+=(const TMatrix4 &a)
{
  for (uint32_t i = 0; i < 4; ++i)
    row[i] += a.row[i];
  return *this;
}

INLINE TMatrix4 &TMatrix4::operator-=(const TMatrix4 &a)
{
  for (uint32_t i = 0; i < 4; ++i)
    row[i] -= a.row[i];
  return *this;
}


INLINE TMatrix4 TMatrix4::operator*(real f) const
{
  TMatrix4 a;
  for (uint32_t i = 0; i < 4; ++i)
    a.row[i] = row[i] * f;
  return a;
}

INLINE TMatrix4 operator*(real f, const TMatrix4 &a) { return a * f; }

INLINE TMatrix4 &TMatrix4::operator*=(real f)
{
  for (uint32_t i = 0; i < 4; ++i)
    row[i] *= f;
  return *this;
}

INLINE TMatrix4 &TMatrix4::operator*=(const TMatrix4 &b)
{
  TMatrix4 r;
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

INLINE Point4 operator*(const Point4 &p, const TMatrix4 &m)
{
  Point4 r;
  for (int i = 0; i < 4; ++i)
  {
    r[i] = 0;
    for (int j = 0; j < 4; ++j)
      r[i] += p[j] * m.m[j][i];
  }
  return r;
}

INLINE Point3 operator*(const Point3 &p, const TMatrix4 &m)
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

INLINE Point3 operator%(const Point3 &p, const TMatrix4 &m)
{
  Point3 r;
  for (int i = 0; i < 3; ++i)
  {
    r[i] = 0;
    for (int j = 0; j < 3; ++j)
      r[i] += p[j] * m.m[j][i];
  }
  return r;
}

INLINE Quat operator*(const Quat &q, const TMatrix4 &m)
{
  Quat r;
  for (int i = 0; i < 3; ++i)
  {
    real v = 0;
    for (int j = 0; j < 3; ++j)
      v += q[j] * m.m[j][i];
    r[i] = v;
  }
  r.w = q.w;
  return r;
}

/// @todo returns determinant of 3x3 part!
INLINE real TMatrix4::det() const
{
  //==
  return m[0][0] * m[1][1] * m[2][2] + m[0][1] * m[1][2] * m[2][0] + m[1][0] * m[2][1] * m[0][2] - m[0][2] * m[1][1] * m[2][0] -
         m[0][1] * m[1][0] * m[2][2] - m[1][2] * m[2][1] * m[0][0];
}

bool is_invertible(const TMatrix4 &mat);

double det4x4(const TMatrix4 &m); //<returns determinant 4x4

TMatrix4 inverse44(const TMatrix4 &a); //<produce a fatal if det() < 0

bool inverse44(const TMatrix4 &in, TMatrix4 &result, float &det); //<returns false if det() < 0

/// @todo inverts as 4x3 matrix!
INLINE TMatrix4 inverse43(const TMatrix4 &a)
{
  //==
  TMatrix4 r;
  r.identity();
  real inv_d = 1.0f / a.det();
  int i;
  for (i = 0; i < 3; ++i)
  {
    int i1 = 0, i2 = 1;
    if (i1 == i)
      ++i1;
    if (i2 == i)
      ++i2;
    if (i2 == i1)
      ++i2;
    for (int j = 0; j < 3; ++j)
    {
      int j1 = 0, j2 = 1;
      if (j1 == j)
        ++j1;
      if (j2 == j)
        ++j2;
      if (j2 == j1)
        ++j2;
      r.m[j][i] = (a.m[i1][j1] * a.m[i2][j2] - a.m[i1][j2] * a.m[i2][j1]) * inv_d;
      if ((i + j) & 1)
        r.m[j][i] = -r.m[j][i];
    }
  }
  for (i = 0; i < 3; ++i)
  {
    r.m[3][i] = 0;
    for (int j = 0; j < 3; ++j)
      r.m[3][i] -= r.m[j][i] * a.m[3][j];
  }
  return r;
}

class Plane3;
extern TMatrix4 matrix_reflect(const Plane3 &plane);

INLINE TMatrix4 matrix_look_at_lh(const Point3 &eye, const Point3 &at, const Point3 &up)
{
  TMatrix4 result;

  Point3 zAxis = normalize(at - eye);
  Point3 xAxis = normalize(up % zAxis);
  Point3 yAxis = zAxis % xAxis;

  result._11 = xAxis.x;
  result._12 = yAxis.x;
  result._13 = zAxis.x;
  result._14 = 0.0f;

  result._21 = xAxis.y;
  result._22 = yAxis.y;
  result._23 = zAxis.y;
  result._24 = 0.0f;

  result._31 = xAxis.z;
  result._32 = yAxis.z;
  result._33 = zAxis.z;
  result._34 = 0.0f;

  result._41 = -(xAxis * eye);
  result._42 = -(yAxis * eye);
  result._43 = -(zAxis * eye);
  result._44 = 1.0f;

  return result;
}


INLINE TMatrix4 matrix_ortho_lh_forward(float width, float height, float z_near, float z_far)
{
  TMatrix4 result;

  result._11 = 2.0f / width;
  result._12 = 0.0f;
  result._13 = 0.0f;
  result._14 = 0.0f;

  result._21 = 0.0f;
  result._22 = 2.0f / height;
  result._23 = 0.0f;
  result._24 = 0.0f;

  result._31 = 0.0f;
  result._32 = 0.0f;
  result._33 = 1.0f / (z_far - z_near);
  result._34 = 0.0f;

  result._41 = 0.0f;
  result._42 = 0.0f;
  result._43 = z_near / (z_near - z_far);
  result._44 = 1.0f;

  return result;
}

INLINE TMatrix4 matrix_ortho_lh_reverse(float width, float height, float z_near, float z_far)
{
  return matrix_ortho_lh_forward(width, height, z_far, z_near); //-V764
}

INLINE TMatrix4 matrix_ortho_lh(float width, float height, float z_near, float z_far)
{
  return matrix_ortho_lh_forward(width, height, z_near, z_far);
}

INLINE TMatrix4 matrix_ortho_off_center_lh(float l, float r, float b, float t, float z_near, float z_far)
{
  TMatrix4 result;

  result._11 = 2.0f / (r - l);
  result._12 = 0.0f;
  result._13 = 0.0f;
  result._14 = 0.0f;

  result._21 = 0.0f;
  result._22 = 2.0f / (t - b);
  result._23 = 0.0f;
  result._24 = 0.0f;

  result._31 = 0.0f;
  result._32 = 0.0f;
  result._33 = safeinv(z_far - z_near);
  result._34 = 0.0f;

  result._41 = -(r + l) / (r - l);
  result._42 = -(t + b) / (t - b);
  result._43 = z_near * safeinv(z_near - z_far);
  result._44 = 1.0f;

  return result;
}

INLINE TMatrix4 matrix_perspective_reverse(float wk, float hk, float zn, float zf)
{
  return TMatrix4(wk, 0, 0, 0, 0, hk, 0, 0, 0, 0, -zn / (zf - zn), 1, 0, 0, zn * zf / (zf - zn), 0);
}

INLINE TMatrix4 matrix_perspective_forward(float wk, float hk, float z_near, float z_far)
{
  float q = z_far / (z_far - z_near);
  return TMatrix4(wk, 0, 0, 0, 0, hk, 0, 0, 0, 0, q, 1, 0, 0, -q * z_near, 0);
}

INLINE TMatrix4 matrix_perspective(float wk, float hk, float z_near, float z_far)
{
  return matrix_perspective_reverse(wk, hk, z_near, z_far);
}

INLINE TMatrix4 matrix_perspective_lh(float width_at_near, float height_at_near, float z_near, float z_far)
{
  return matrix_perspective(2.0f * z_near / width_at_near, 2.0f * z_near / height_at_near, z_near, z_far);
}

// creates a non symmetrical perspective
// matrix from l,r,t,b,zn,zf where four first arguments are distances from the
// screen space origin on a near plane defined in camera space and last two arguments
// distances to near/far plane
INLINE TMatrix4 matrix_perspective_off_center_forward(float l, float r, float b, float t, float zn, float zf)
{
  return TMatrix4(2 * zn / (r - l), 0, 0, 0, 0, 2 * zn / (t - b), 0, 0, (l + r) / (l - r), (t + b) / (b - t), zf / (zf - zn), 1, 0, 0,
    -zn * zf / (zf - zn), 0);
}

// creates a non symmetrical perspective
// matrix from l,r,t,b,zn,zf where four first arguments are distances from the
// screen space origin on a near plane defined in camera space and last two arguments
// distances to near/far plane
INLINE TMatrix4 matrix_perspective_off_center_reverse(float l, float r, float b, float t, float zn, float zf)
{
  return TMatrix4(2 * zn / (r - l), 0, 0, 0, 0, 2 * zn / (t - b), 0, 0, (l + r) / (l - r), (t + b) / (b - t), -zn / (zf - zn), 1, 0, 0,
    zn * zf / (zf - zn), 0);
}

// makes a scaling matrix which when multiplying on a
// perspective matrix transforms a origin frustum to l,r,t,b extents which are
// distances from the screen space origin on a near plane defined in normalized
// device coordinates
INLINE TMatrix4 matrix_perspective_crop(float l, float r, float b, float t, float zn, float zf)
{
  return TMatrix4(2.0f / (r - l), 0, 0, 0, 0, 2.0f / (t - b), 0, 0, 0, 0, 1.0f / (zf - zn), 0, (l + r) / (l - r), (t + b) / (b - t),
    -zn / (zf - zn), 1.0f);
}

INLINE TMatrix tmatrix(const TMatrix4 &a)
{
  TMatrix tm;

  tm.m[0][0] = a.m[0][0], tm.m[0][1] = a.m[0][1], tm.m[0][2] = a.m[0][2];
  tm.m[1][0] = a.m[1][0], tm.m[1][1] = a.m[1][1], tm.m[1][2] = a.m[1][2];
  tm.m[2][0] = a.m[2][0], tm.m[2][1] = a.m[2][1], tm.m[2][2] = a.m[2][2];
  tm.m[3][0] = a.m[3][0], tm.m[3][1] = a.m[3][1], tm.m[3][2] = a.m[3][2];

  return tm;
}

struct DECLSPEC_ALIGN(16) TMatrix4_vec4 : public TMatrix4
{
  INLINE TMatrix4_vec4() = default;
  INLINE TMatrix4_vec4 &operator=(const TMatrix4 &b)
  {
    TMatrix4::operator=(b);
    return *this;
  }
  INLINE TMatrix4_vec4(const TMatrix4 &b) : TMatrix4(b) {}
  INLINE TMatrix4_vec4(real _m00, real _m01, real _m02, real _m03, real _m10, real _m11, real _m12, real _m13, real _m20, real _m21,
    real _m22, real _m23, real _m30, real _m31, real _m32, real _m33)
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
} ATTRIBUTE_ALIGN(16);

INLINE float get_sign(float a)
{
  if (a > 0.0F)
    return (1.0F);
  if (a < 0.0F)
    return (-1.0F);
  return (0.0F);
}

// if isPersp works only with perspective matrix, but faster and more precise

template <bool isPersp>
INLINE TMatrix4 oblique_projection_matrix_forward(const TMatrix4 &forwardProj, const TMatrix &view_itm, const Point4 &worldClipPlane)
{
  TMatrix4 transposedIVTM = TMatrix4(view_itm).transpose();

  // Calculate the clip-space corner point opposite the clipping plane
  // as (sgn(clipPlane.x), sgn(clipPlane.y), 1, 1) and
  // transform it into camera space by multiplying it
  // by the inverse of the projection matrix
  ///*
  Point4 clipPlane = worldClipPlane * transposedIVTM;
  Point4 qB(get_sign(clipPlane.x), get_sign(clipPlane.y), 1, 1);
  Point4 q;
  if (isPersp)
    // optimized for perspective projection, no need to calc inverse
    q = Point4(qB.x / forwardProj(0, 0), qB.y / forwardProj(1, 1), qB.z, (qB.w - forwardProj(2, 2)) / forwardProj(3, 2));
  else
    q = qB * inverse44(forwardProj);

  Point4 c = clipPlane * (1.0f / (clipPlane * q));

  TMatrix4 waterProj = forwardProj;
  waterProj.setcol(2, c); // Point4(c.x, c.y, c.z, -c.w)
  return waterProj;
}

template <bool isPersp>
INLINE TMatrix4 oblique_projection_matrix_reverse(const TMatrix4 &proj, const TMatrix &view_itm, const Point4 &worldClipPlane)
{
  TMatrix4 reverseDepthMatrix;
  reverseDepthMatrix.identity();
  reverseDepthMatrix(2, 2) = -1.0f;
  reverseDepthMatrix(3, 2) = 1.0f;
  TMatrix4 forwardProj = proj * reverseDepthMatrix;
  TMatrix4 waterProj = oblique_projection_matrix_forward<isPersp>(forwardProj, view_itm, worldClipPlane);
  waterProj = waterProj * reverseDepthMatrix;
  return waterProj;
}

#undef INLINE
/// @}
