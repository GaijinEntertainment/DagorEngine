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

// Point2 - 2D point/vector //
/**
  2D point/vector
  @sa Point3 IPoint2 Point4
*/
class Point2
{
public:
  real x, y;
  INLINE Point2() = default;
  INLINE Point2(real ax, real ay)
  {
    x = ax;
    y = ay;
  }
  INLINE explicit Point2(const real *p)
  {
    x = p[0];
    y = p[1];
  }

  static const Point2 ZERO;
  static const Point2 ONE;

  INLINE void zero()
  {
    x = 0;
    y = 0;
  }
  INLINE void set(real _x, real _y)
  {
    x = _x;
    y = _y;
  }

  INLINE const real &operator[](int i) const { return (&x)[i]; }
  INLINE real &operator[](int i) { return (&x)[i]; }
  INLINE Point2 operator-() const { return Point2(-x, -y); }
  INLINE Point2 operator+() const { return *this; }

  INLINE Point2 operator+(const Point2 &a) const { return Point2(x + a.x, y + a.y); }
  INLINE Point2 operator-(const Point2 &a) const { return Point2(x - a.x, y - a.y); }
  INLINE real operator*(const Point2 &a) const { return x * a.x + y * a.y; }
  INLINE Point2 operator*(real a) const { return Point2(x * a, y * a); }
  INLINE Point2 operator/(real a) const { return operator*(1.0f / a); }

  INLINE Point2 &operator+=(const Point2 &a)
  {
    x += a.x;
    y += a.y;
    return *this;
  }
  INLINE Point2 &operator-=(const Point2 &a)
  {
    x -= a.x;
    y -= a.y;
    return *this;
  }
  INLINE Point2 &operator*=(real a)
  {
    x *= a;
    y *= a;
    return *this;
  }
  INLINE Point2 &operator/=(real a) { return operator*=(1.0f / a); }

  INLINE bool operator==(const Point2 &a) const { return (x == a.x && y == a.y); }
  INLINE bool operator!=(const Point2 &a) const { return (x != a.x || y != a.y); }
  INLINE real lengthSq() const { return x * x + y * y; }
  INLINE real length() const { return sqrtf(lengthSq()); }
  INLINE void normalize() { *this *= safeinv(length()); }

  INLINE real lengthF() const { return fastsqrt(lengthSq()); }
  INLINE void normalizeF() { *this *= safeinvsqrtfast(lengthSq()); }

  template <class T>
  static Point2 xy(const T &a)
  {
    return Point2(a.x, a.y);
  }
  template <class T>
  static Point2 xz(const T &a)
  {
    return Point2(a.x, a.z);
  }
  template <class T>
  static Point2 yz(const T &a)
  {
    return Point2(a.y, a.z);
  }

  template <class T>
  void set_xy(const T &a)
  {
    x = a.x;
    y = a.y;
  }
  template <class T>
  void set_xz(const T &a)
  {
    x = a.x;
    y = a.z;
  }
  template <class T>
  void set_yz(const T &a)
  {
    x = a.y;
    y = a.z;
  }
};

/// dot product
INLINE float dot(const Point2 &a, const Point2 &b) { return a * b; }

/// floor() of all components
INLINE Point2 floor(const Point2 &a) { return Point2(floorf(a.x), floorf(a.y)); }
/// ceil() of all components
INLINE Point2 ceil(const Point2 &a) { return Point2(ceilf(a.x), ceilf(a.y)); }
/// round() of all components
INLINE Point2 round(const Point2 &a) { return Point2(roundf(a.x), roundf(a.y)); }

__forceinline Point2 div(const Point2 &a, const Point2 &b) { return Point2(a.x / b.x, a.y / b.y); }
inline Point2 mul(const Point2 &a, const Point2 &b) { return Point2(a.x * b.x, a.y * b.y); }

INLINE Point2 operator*(real a, const Point2 &p) { return Point2(p.x * a, p.y * a); }
INLINE real lengthSq(const Point2 &a) { return a.x * a.x + a.y * a.y; }
INLINE real length(const Point2 &a) { return sqrtf(lengthSq(a)); }
INLINE Point2 normalize(const Point2 &a) { return a * safeinv(length(a)); }
INLINE real lengthF(const Point2 &a) { return fastsqrt(lengthSq(a)); }
INLINE Point2 normalizeF(const Point2 &a)
{
  real il = safeinvsqrtfast(lengthSq(a));
  return Point2(a.x * il, a.y * il);
}

__forceinline Point2 abs(const Point2 &a) { return Point2(fabsf(a.x), fabsf(a.y)); }
__forceinline Point2 sqrt(const Point2 &a) { return Point2(sqrtf(a.x), sqrtf(a.y)); }
#undef max
#undef min
__forceinline Point2 max(const Point2 &a, const Point2 &b) { return Point2(max(a.x, b.x), max(a.y, b.y)); }
__forceinline Point2 min(const Point2 &a, const Point2 &b) { return Point2(min(a.x, b.x), min(a.y, b.y)); }
template <>
INLINE Point2 clamp(Point2 t, const Point2 min_val, const Point2 max_val)
{
  return min(max(t, min_val), max_val);
}

struct DECLSPEC_ALIGN(16) Point2_vec4 : public Point2
{
  float resv1, resv2;
  INLINE Point2_vec4() = default;
  INLINE Point2_vec4 &operator=(const Point2 &b)
  {
    Point2::operator=(b);
    return *this;
  }
  INLINE Point2_vec4(const Point2 &b) : Point2(b) {} //-V730
} ATTRIBUTE_ALIGN(16);

// DPoint2 - double 2D point/vector //
/**
  2D point/vector
  @sa Point3 IPoint2 Point4
*/
class DPoint2
{
public:
  double x, y;
  INLINE DPoint2() = default;
  INLINE DPoint2(double ax, double ay)
  {
    x = ax;
    y = ay;
  }

  INLINE void zero()
  {
    x = 0;
    y = 0;
  }
  INLINE DPoint2(const double *p)
  {
    x = p[0];
    y = p[1];
  }
  INLINE void set(double _x, double _y)
  {
    x = _x;
    y = _y;
  }

  INLINE const double &operator[](int i) const { return (&x)[i]; }
  INLINE double &operator[](int i) { return (&x)[i]; }
  INLINE DPoint2 operator-() const { return DPoint2(-x, -y); }
  INLINE DPoint2 operator+() const { return *this; }

  INLINE DPoint2 operator+(const DPoint2 &a) const { return DPoint2(x + a.x, y + a.y); }
  INLINE DPoint2 operator-(const DPoint2 &a) const { return DPoint2(x - a.x, y - a.y); }
  INLINE double operator*(const DPoint2 &a) const { return x * a.x + y * a.y; }
  INLINE DPoint2 operator*(double a) const { return DPoint2(x * a, y * a); }
  INLINE DPoint2 operator/(double a) const { return operator*(1.0 / a); }

  INLINE DPoint2 &operator+=(const DPoint2 &a)
  {
    x += a.x;
    y += a.y;
    return *this;
  }
  INLINE DPoint2 &operator-=(const DPoint2 &a)
  {
    x -= a.x;
    y -= a.y;
    return *this;
  }
  INLINE DPoint2 &operator*=(double a)
  {
    x *= a;
    y *= a;
    return *this;
  }
  INLINE DPoint2 &operator/=(double a) { return operator*=(1.0 / a); }

  INLINE bool operator==(const DPoint2 &a) const { return (x == a.x && y == a.y); }
  INLINE bool operator!=(const DPoint2 &a) const { return (x != a.x || y != a.y); }
  INLINE double lengthSq() const { return x * x + y * y; }
  INLINE double length() const { return sqrtf(lengthSq()); }
  INLINE void normalize() { *this *= safeinv(length()); }

  template <class T>
  static DPoint2 xy(const T &a)
  {
    return DPoint2(a.x, a.y);
  }
  template <class T>
  static DPoint2 xz(const T &a)
  {
    return DPoint2(a.x, a.z);
  }
  template <class T>
  static DPoint2 yz(const T &a)
  {
    return DPoint2(a.y, a.z);
  }

  template <class T>
  void set_xy(const T &a)
  {
    x = a.x;
    y = a.y;
  }
  template <class T>
  void set_xz(const T &a)
  {
    x = a.x;
    y = a.z;
  }
  template <class T>
  void set_yz(const T &a)
  {
    x = a.y;
    y = a.z;
  }
};

/// floor() of all components
INLINE DPoint2 floor(const DPoint2 &a) { return DPoint2(::floor(a.x), ::floor(a.y)); }
/// ceil() of all components
INLINE DPoint2 ceil(const DPoint2 &a) { return DPoint2(::ceil(a.x), ::ceil(a.y)); }


INLINE DPoint2 operator*(double a, const DPoint2 &p) { return DPoint2(p.x * a, p.y * a); }
INLINE double lengthSq(const DPoint2 &a) { return a.x * a.x + a.y * a.y; }
INLINE double length(const DPoint2 &a) { return sqrt(lengthSq(a)); }
INLINE DPoint2 normalize(const DPoint2 &a) { return a * safeinv(length(a)); }

INLINE DPoint2 dpoint2(const Point2 &p) { return DPoint2::xy(p); }
INLINE Point2 point2(const DPoint2 &p) { return Point2::xy(p); }

inline DPoint2 mul(const DPoint2 &a, const DPoint2 &b) { return DPoint2(a.x * b.x, a.y * b.y); }


#undef INLINE

/// @}
