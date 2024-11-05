//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_mathBase.h>
#include <util/dag_globDef.h> //min/max

#define INLINE __forceinline

/// @addtogroup math
/// @{
// Point4 - Point4 Vector //
/**
  4D point/vector
  @sa TMatrix TMatrix4 Point3 Point2
*/
class Point4
{
public:
  enum CtorPtrMark
  {
    CTOR_FROM_PTR = 1
  };
  real x, y, z, w;

  INLINE Point4() = default;
  INLINE Point4(real ax, real ay, real az, real aw)
  {
    x = ax;
    y = ay;
    z = az;
    w = aw;
  }
  // Point4(const Point4& p) {x=p.x;y=p.y;z=p.z;w=p.w;}
  // Point4& operator =(const Point4& p) {x=p.x;y=p.y;z=p.z;w=p.w;return *this;}
  INLINE Point4(const real *p, CtorPtrMark /*check*/)
  {
    x = p[0];
    y = p[1];
    z = p[2];
    w = p[3];
  }

  static const Point4 ZERO;
  static const Point4 ONE;

  INLINE void zero()
  {
    x = 0;
    y = 0;
    z = 0;
    w = 0;
  }
  INLINE void set(real _x, real _y, real _z, real _w)
  {
    x = _x;
    y = _y;
    z = _z;
    w = _w;
  }

  INLINE real &operator[](int i) { return (&x)[i]; }
  INLINE const real &operator[](int i) const { return (&x)[i]; }
  INLINE Point4 operator-() const { return Point4(-x, -y, -z, -w); }
  INLINE Point4 operator+() const { return *this; }

  INLINE Point4 operator+(const Point4 &a) const { return Point4(x + a.x, y + a.y, z + a.z, w + a.w); }
  INLINE Point4 operator-(const Point4 &a) const { return Point4(x - a.x, y - a.y, z - a.z, w - a.w); }
  INLINE real operator*(const Point4 &a) const { return x * a.x + y * a.y + z * a.z + w * a.w; }
  INLINE real DotProduct3(const Point4 &a) const { return x * a.x + y * a.y + z * a.z; }
  INLINE real UDotProduct3(const Point4 &a) const { return x * a.x / (a.w * w) + y * a.y / (a.w * w) + z * a.z / (a.w * w); }
  //== Non unified
  /// cross product (only 3D part is used)
  INLINE Point4 operator%(const Point4 &a) const { return Point4(y * a.z - z * a.y, z * a.x - x * a.z, x * a.y - y * a.x, 0); }
  INLINE Point4 operator*(real a) const { return Point4(x * a, y * a, z * a, w * a); }
  INLINE Point4 operator/(real a) const { return operator*(1.0f / a); }

  INLINE Point4 &operator+=(const Point4 &a)
  {
    x += a.x;
    y += a.y;
    z += a.z;
    w += a.w;
    return *this;
  }
  INLINE Point4 &operator-=(const Point4 &a)
  {
    x -= a.x;
    y -= a.y;
    z -= a.z;
    w -= a.w;
    return *this;
  }
  INLINE Point4 &operator*=(real a)
  {
    x *= a;
    y *= a;
    z *= a;
    w *= a;
    return *this;
  }
  INLINE Point4 &operator/=(real a) { return operator*=(1.0f / a); }

  INLINE bool operator==(const Point4 &a) const { return (x == a.x && y == a.y && z == a.z && w == a.w); }
  INLINE bool operator!=(const Point4 &a) const { return (x != a.x || y != a.y || z != a.z || w != a.w); }

  INLINE void unifyF()
  {
    x /= w;
    y /= w;
    z /= w;
    w = 1;
  }
  INLINE void unify()
  {
    if (w != 0)
    {
      x /= w;
      y /= w;
      z /= w;
    }
    w = 1;
  }
  INLINE real lengthSq() const { return x * x + y * y + z * z + w * w; }
  INLINE real length() const { return sqrtf(lengthSq()); }
  INLINE void normalize()
  {
    real l = length();
    if (l == 0)
      x = y = z = w = 0;
    else
    {
      x /= l;
      y /= l;
      z /= l;
      w /= l;
    }
  }
  INLINE real lengthF() const { return fastsqrt(lengthSq()); }
  INLINE void normalizeF()
  {
    real l = lengthSq();
    if (l == 0)
      x = y = z = w = 0;
    else
    {
      l = fastinvsqrt(l);
      x *= l;
      y *= l;
      z *= l;
      w *= l;
    }
  }

  template <class T>
  static Point4 xyz0(const T &a)
  {
    return Point4(a.x, a.y, a.z, 0);
  }
  template <class T>
  static Point4 xyz1(const T &a)
  {
    return Point4(a.x, a.y, a.z, 1);
  }
  template <class T>
  static Point4 xyzw(const T &a)
  {
    return Point4(a.x, a.y, a.z, a.w);
  }
  template <class T>
  static Point4 rgba(const T &a)
  {
    return Point4(a.r, a.g, a.b, a.a);
  }
  template <class T>
  static Point4 xyzV(const T &a, float v)
  {
    return Point4(a.x, a.y, a.z, v);
  }

  template <class T>
  void set_xyz0(const T &a)
  {
    x = a.x, y = a.y, z = a.z, w = 0;
  }
  template <class T>
  void set_xyz1(const T &a)
  {
    x = a.x, y = a.y, z = a.z, w = 1;
  }
  template <class T>
  void set_xyzw(const T &a)
  {
    x = a.x, y = a.y, z = a.z, w = a.w;
  }
  template <class T>
  void set_rgba(const T &a)
  {
    x = a.r, y = a.g, z = a.b, w = a.a;
  }
  template <class T>
  void set_xyzV(const T &a, float v)
  {
    x = a.x, y = a.y, z = a.z, w = v;
  }
};

/// dot product
INLINE float dot(const Point4 &a, const Point4 &b) { return a * b; }
INLINE Point4 operator*(real a, const Point4 &p) { return Point4(p.x * a, p.y * a, p.z * a, p.w * a); }
INLINE real lengthSq(const Point4 &a) { return a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w; }
INLINE real length(const Point4 &a) { return sqrtf(lengthSq(a)); }
INLINE Point4 normalize(const Point4 &a)
{
  real l = length(a);
  return (l == 0) ? Point4(0, 0, 0, 0) : Point4(a.x / l, a.y / l, a.z / l, a.w / l);
}
INLINE real lengthF(const Point4 &a) { return fastsqrt(lengthSq(a)); }
INLINE Point4 normalizeF(const Point4 &a)
{
  real l = lengthSq(a);
  return (l == 0) ? Point4(0, 0, 0, 0) : a * fastinvsqrt(l);
}

/// floor() of all components
INLINE Point4 floor(const Point4 &a) { return Point4(floorf(a.x), floorf(a.y), floorf(a.z), floorf(a.w)); }
/// ceil() of all components
INLINE Point4 ceil(const Point4 &a) { return Point4(ceilf(a.x), ceilf(a.y), ceilf(a.z), ceilf(a.w)); }
INLINE Point4 frac(const Point4 &a) { return a - floor(a); }
INLINE Point4 mul(const Point4 &a, const Point4 &b) { return Point4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w); }
INLINE Point4 div(const Point4 &a, const Point4 &b) { return Point4(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w); }
INLINE Point4 abs(const Point4 &a) { return Point4(fabsf(a.x), fabsf(a.y), fabsf(a.z), fabsf(a.w)); }
INLINE Point4 sqrt(const Point4 &a) { return Point4(sqrtf(a.x), sqrtf(a.y), sqrtf(a.z), sqrtf(a.w)); }
#undef max
#undef min
INLINE Point4 max(const Point4 &a, const Point4 &b) { return Point4(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z), max(a.w, b.w)); }
INLINE Point4 min(const Point4 &a, const Point4 &b) { return Point4(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z), min(a.w, b.w)); }
template <>
INLINE Point4 clamp(Point4 t, const Point4 min_val, const Point4 max_val)
{
  return min(max(t, min_val), max_val);
}

#undef INLINE

/// @}
