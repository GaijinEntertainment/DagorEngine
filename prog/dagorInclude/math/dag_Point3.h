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

// Point3 - 3D point/vector //
/**
  3D point/vector
  @sa TMatrix Matrix3 IPoint3 DPoint3 Point2 Point4
*/
class Point3
{
public:
  enum CtorPtrMark
  {
    CTOR_FROM_PTR = 1
  };
  real x, y, z;

  INLINE Point3() = default;
  INLINE Point3(const Point3 &) = default;
  INLINE Point3(real ax, real ay, real az)
  {
    x = ax;
    y = ay;
    z = az;
  }
  /// constructs from #real array
  INLINE Point3(const real *p, CtorPtrMark /*check*/)
  {
    x = p[0];
    y = p[1];
    z = p[2];
  }
  INLINE explicit Point3(const DPoint3 &p);
  Point3 &operator=(const Point3 &) = default;

  static const Point3 ZERO;
  static const Point3 ONE;

  INLINE void zero()
  {
    x = 0;
    y = 0;
    z = 0;
  }
  INLINE void set(real _x, real _y, real _z)
  {
    x = _x;
    y = _y;
    z = _z;
  }

  INLINE const real &operator[](int i) const { return (&x)[i]; }
  INLINE real &operator[](int i) { return (&x)[i]; }
  INLINE Point3 operator-() const { return Point3(-x, -y, -z); }
  INLINE Point3 operator+() const { return *this; }

  INLINE Point3 operator+(const Point3 &a) const { return Point3(x + a.x, y + a.y, z + a.z); }
  INLINE Point3 operator-(const Point3 &a) const { return Point3(x - a.x, y - a.y, z - a.z); }
  /// dot product
  INLINE real operator*(const Point3 &a) const { return x * a.x + y * a.y + z * a.z; }
  /// cross product
  INLINE Point3 operator%(const Point3 &a) const { return Point3(y * a.z - z * a.y, z * a.x - x * a.z, x * a.y - y * a.x); }
  INLINE Point3 operator+(real a) const { return Point3(x + a, y + a, z + a); }
  INLINE Point3 operator-(real a) const { return Point3(x - a, y - a, z - a); }
  INLINE Point3 operator*(real a) const { return Point3(x * a, y * a, z * a); }
  INLINE Point3 operator/(real a) const { return operator*(1.0f / a); }

  INLINE Point3 &operator+=(const Point3 &a)
  {
    x += a.x;
    y += a.y;
    z += a.z;
    return *this;
  }
  INLINE Point3 &operator-=(const Point3 &a)
  {
    x -= a.x;
    y -= a.y;
    z -= a.z;
    return *this;
  }
  INLINE Point3 &operator*=(real a)
  {
    x *= a;
    y *= a;
    z *= a;
    return *this;
  }
  INLINE Point3 &operator/=(real a) { return operator*=(1.0f / a); }


  INLINE bool operator==(const Point3 &a) const { return (x == a.x && y == a.y && z == a.z); }
  INLINE bool operator!=(const Point3 &a) const { return (x != a.x || y != a.y || z != a.z); }

  INLINE real lengthSq() const { return x * x + y * y + z * z; }
  INLINE real length() const { return sqrtf(lengthSq()); }
  INLINE void normalize() { *this *= safeinv(length()); }
  /// fast (approximate) version
  INLINE real lengthF() const { return fastsqrt(lengthSq()); }
  /// fast (approximate) version
  INLINE void normalizeF() { *this *= safeinvsqrtfast(lengthSq()); }

  template <class T>
  static Point3 xyz(const T &a)
  {
    return Point3(a.x, a.y, a.z);
  }
  template <class T>
  static Point3 xzy(const T &a)
  {
    return Point3(a.x, a.z, a.y);
  }
  template <class T>
  static Point3 x0y(const T &a)
  {
    return Point3(a.x, 0, a.y);
  }
  template <class T>
  static Point3 x0z(const T &a)
  {
    return Point3(a.x, 0, a.z);
  }
  template <class T>
  static Point3 xy0(const T &a)
  {
    return Point3(a.x, a.y, 0);
  }
  template <class T>
  static Point3 xz0(const T &a)
  {
    return Point3(a.x, a.z, 0);
  }
  template <class T>
  static Point3 xVy(const T &a, float v)
  {
    return Point3(a.x, v, a.y);
  }
  template <class T>
  static Point3 xVz(const T &a, float v)
  {
    return Point3(a.x, v, a.z);
  }
  template <class T>
  static Point3 xyV(const T &a, float v)
  {
    return Point3(a.x, a.y, v);
  }
  template <class T>
  static Point3 xzV(const T &a, float v)
  {
    return Point3(a.x, a.z, v);
  }
  template <class T>
  static Point3 rgb(const T &a)
  {
    return Point3(a.r, a.g, a.b);
  }

  template <class T>
  void set_xyz(const T &a)
  {
    x = a.x, y = a.y, z = a.z;
  }
  template <class T>
  void set_xzy(const T &a)
  {
    x = a.x, y = a.z, z = a.y;
  }
  template <class T>
  void set_x0y(const T &a)
  {
    x = a.x, y = 0, z = a.y;
  }
  template <class T>
  void set_x0z(const T &a)
  {
    x = a.x, y = 0, z = a.z;
  }
  template <class T>
  void set_xy0(const T &a)
  {
    x = a.x, y = a.y, z = 0;
  }
  template <class T>
  void set_xz0(const T &a)
  {
    x = a.x, y = a.z, z = 0;
  }
  template <class T>
  void set_xVy(const T &a, float v)
  {
    x = a.x, y = v, z = a.y;
  }
  template <class T>
  void set_xVz(const T &a, float v)
  {
    x = a.x, y = v, z = a.z;
  }
  template <class T>
  void set_xyV(const T &a, float v)
  {
    x = a.x, y = a.y, z = v;
  }
  template <class T>
  void set_xzV(const T &a, float v)
  {
    x = a.x, y = a.z, z = v;
  }
  template <class T>
  void set_rgb(const T &a)
  {
    x = a.r, y = a.g, z = a.b;
  }
};

/// dot product
INLINE float dot(const Point3 &a, const Point3 &b) { return a * b; }
/// cross product
INLINE Point3 cross(const Point3 &a, const Point3 &b) { return a % b; }

INLINE Point3 operator*(real a, const Point3 &p) { return Point3(p.x * a, p.y * a, p.z * a); }
INLINE real lengthSq(const Point3 &a) { return a.x * a.x + a.y * a.y + a.z * a.z; }
INLINE real length(const Point3 &a) { return sqrtf(lengthSq(a)); }
INLINE Point3 normalize(const Point3 &a) { return a * safeinv(length(a)); }
/// fast (approximate) version
INLINE real lengthF(const Point3 &a) { return fastsqrt(lengthSq(a)); }
/// fast (approximate) version
INLINE Point3 normalizeF(const Point3 &a) { return a * safeinvsqrtfast(lengthSq(a)); }

/// floor() of all components
INLINE Point3 floor(const Point3 &a) { return Point3(floorf(a.x), floorf(a.y), floorf(a.z)); }
/// ceil() of all components
INLINE Point3 ceil(const Point3 &a) { return Point3(ceilf(a.x), ceilf(a.y), ceilf(a.z)); }
/// round() of all components
INLINE Point3 round(const Point3 &a) { return Point3(roundf(a.x), roundf(a.y), roundf(a.z)); }
INLINE Point3 frac(const Point3 &a) { return a - floor(a); }
INLINE Point3 mul(const Point3 &a, const Point3 &b) { return Point3(a.x * b.x, a.y * b.y, a.z * b.z); }
__forceinline Point3 div(const Point3 &a, const Point3 &b) { return Point3(a.x / b.x, a.y / b.y, a.z / b.z); }

__forceinline Point3 abs(const Point3 &a) { return Point3(fabsf(a.x), fabsf(a.y), fabsf(a.z)); }
__forceinline Point3 sqrt(const Point3 &a) { return Point3(sqrtf(a.x), sqrtf(a.y), sqrtf(a.z)); }
#undef max
#undef min
__forceinline Point3 max(const Point3 &a, const Point3 &b) { return Point3(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z)); }
__forceinline Point3 min(const Point3 &a, const Point3 &b) { return Point3(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z)); }
__forceinline Point3 max(const Point3 &a, const float b) { return Point3(max(a.x, b), max(a.y, b), max(a.z, b)); }
__forceinline Point3 min(const Point3 &a, const float b) { return Point3(min(a.x, b), min(a.y, b), min(a.z, b)); }
template <>
INLINE Point3 clamp(Point3 t, const Point3 min_val, const Point3 max_val)
{
  return min(max(t, min_val), max_val);
}
INLINE Point3 clamp(Point3 t, const float min_val, const float max_val) { return min(max(t, min_val), max_val); }

struct DECLSPEC_ALIGN(16) Point3_vec4 : public Point3
{
  float resv;
  INLINE Point3_vec4() = default;
  INLINE Point3_vec4(float xx, float yy, float zz) : Point3(xx, yy, zz) {} //-V730
  INLINE Point3_vec4 &operator=(const Point3 &b)
  {
    Point3::operator=(b);
    return *this;
  }
  INLINE Point3_vec4(const Point3 &b) : Point3(b) {} //-V730
} ATTRIBUTE_ALIGN(16);

// DPoint3 - double precision 3D point/vector //

/**
  double precision 3D point/vector
  @sa Point3 IPoint3
*/
class DPoint3
{
public:
  enum CtorPtrMark
  {
    CTOR_FROM_PTR = 1
  };
  double x, y, z;

  INLINE DPoint3() = default;
  INLINE DPoint3(double ax, double ay, double az)
  {
    x = ax;
    y = ay;
    z = az;
  }
  INLINE DPoint3(const double *p, CtorPtrMark /*check*/)
  {
    x = p[0];
    y = p[1];
    z = p[2];
  }
  INLINE explicit DPoint3(const Point3 &p)
  {
    x = p.x;
    y = p.y;
    z = p.z;
  }

  INLINE void zero()
  {
    x = 0;
    y = 0;
    z = 0;
  }
  INLINE void set(double _x, double _y, double _z)
  {
    x = _x;
    y = _y;
    z = _z;
  }

  INLINE const double &operator[](int i) const { return (&x)[i]; }
  INLINE double &operator[](int i) { return (&x)[i]; }
  INLINE operator Point3() const { return Point3(x, y, z); }
  INLINE DPoint3 operator-() const { return DPoint3(-x, -y, -z); }
  INLINE DPoint3 operator+() const { return *this; }

  INLINE DPoint3 operator+(const DPoint3 &a) const { return DPoint3(x + a.x, y + a.y, z + a.z); }
  INLINE DPoint3 operator-(const DPoint3 &a) const { return DPoint3(x - a.x, y - a.y, z - a.z); }

  /// Dot product.
  INLINE double operator*(const DPoint3 &a) const { return x * a.x + y * a.y + z * a.z; }

  /// Cross product.
  INLINE DPoint3 operator%(const DPoint3 &a) const { return DPoint3(y * a.z - z * a.y, z * a.x - x * a.z, x * a.y - y * a.x); }

  INLINE DPoint3 operator*(double a) const { return DPoint3(x * a, y * a, z * a); }
  INLINE DPoint3 operator/(double a) const { return operator*(1.0 / a); }

  INLINE DPoint3 &operator+=(const DPoint3 &a)
  {
    x += a.x;
    y += a.y;
    z += a.z;
    return *this;
  }
  INLINE DPoint3 &operator-=(const DPoint3 &a)
  {
    x -= a.x;
    y -= a.y;
    z -= a.z;
    return *this;
  }
  INLINE DPoint3 &operator+=(const Point3 &a)
  {
    x += a.x;
    y += a.y;
    z += a.z;
    return *this;
  }
  INLINE DPoint3 &operator-=(const Point3 &a)
  {
    x -= a.x;
    y -= a.y;
    z -= a.z;
    return *this;
  }
  INLINE DPoint3 &operator*=(double a)
  {
    x *= a;
    y *= a;
    z *= a;
    return *this;
  }
  INLINE DPoint3 &operator/=(double a) { return operator*=(1.0 / a); }
  INLINE DPoint3 &operator=(const Point3 &a)
  {
    x = a.x;
    y = a.y;
    z = a.z;
    return *this;
  }

  INLINE bool operator==(const DPoint3 &a) const { return (x == a.x && y == a.y && z == a.z); }
  INLINE bool operator!=(const DPoint3 &a) const { return (x != a.x || y != a.y || z != a.z); }
  INLINE double lengthSq() const { return x * x + y * y + z * z; }
  INLINE double length() const { return sqrt(lengthSq()); }
  INLINE void normalize() { *this *= safeinv(length()); }

  template <class T>
  static DPoint3 xyz(const T &a)
  {
    return DPoint3(a.x, a.y, a.z);
  }
  template <class T>
  static DPoint3 xzy(const T &a)
  {
    return DPoint3(a.x, a.z, a.y);
  }
  template <class T>
  static DPoint3 x0y(const T &a)
  {
    return DPoint3(a.x, 0, a.y);
  }
  template <class T>
  static DPoint3 x0z(const T &a)
  {
    return DPoint3(a.x, 0, a.z);
  }
  template <class T>
  static DPoint3 xy0(const T &a)
  {
    return DPoint3(a.x, a.y, 0);
  }
  template <class T>
  static DPoint3 xz0(const T &a)
  {
    return DPoint3(a.x, a.z, 0);
  }
  template <class T>
  static DPoint3 xVy(const T &a, double v)
  {
    return DPoint3(a.x, v, a.y);
  }
  template <class T>
  static DPoint3 xVz(const T &a, double v)
  {
    return DPoint3(a.x, v, a.z);
  }
  template <class T>
  static DPoint3 xyV(const T &a, double v)
  {
    return DPoint3(a.x, a.y, v);
  }
  template <class T>
  static DPoint3 xzV(const T &a, double v)
  {
    return DPoint3(a.x, a.z, v);
  }
  template <class T>
  static DPoint3 rgb(const T &a)
  {
    return DPoint3(a.r, a.g, a.b);
  }

  template <class T>
  void set_xyz(const T &a)
  {
    x = a.x, y = a.y, z = a.z;
  }
  template <class T>
  void set_xzy(const T &a)
  {
    x = a.x, y = a.z, z = a.y;
  }
  template <class T>
  void set_x0y(const T &a)
  {
    x = a.x, y = 0, z = a.y;
  }
  template <class T>
  void set_x0z(const T &a)
  {
    x = a.x, y = 0, z = a.z;
  }
  template <class T>
  void set_xy0(const T &a)
  {
    x = a.x, y = a.y, z = 0;
  }
  template <class T>
  void set_xz0(const T &a)
  {
    x = a.x, y = a.z, z = 0;
  }
  template <class T>
  void set_xVy(const T &a, double v)
  {
    x = a.x, y = v, z = a.y;
  }
  template <class T>
  void set_xVz(const T &a, double v)
  {
    x = a.x, y = v, z = a.z;
  }
  template <class T>
  void set_xyV(const T &a, double v)
  {
    x = a.x, y = a.y, z = v;
  }
  template <class T>
  void set_xzV(const T &a, double v)
  {
    x = a.x, y = a.z, z = v;
  }
  template <class T>
  void set_rgb(const T &a)
  {
    x = a.r, y = a.g, z = a.b;
  }
};

INLINE Point3::Point3(const DPoint3 &p)
{
  x = p.x;
  y = p.y;
  z = p.z;
}

INLINE DPoint3 operator*(double a, const DPoint3 &p) { return DPoint3(p.x * a, p.y * a, p.z * a); }
INLINE double lengthSq(const DPoint3 &a) { return a.x * a.x + a.y * a.y + a.z * a.z; }
INLINE double length(const DPoint3 &a) { return sqrt(lengthSq(a)); }
INLINE DPoint3 normalize(const DPoint3 &a) { return a * safeinv(length(a)); }

INLINE DPoint3 mul(const DPoint3 &a, const DPoint3 &b) { return DPoint3(a.x * b.x, a.y * b.y, a.z * b.z); }

INLINE DPoint3 abs(const DPoint3 &a) { return DPoint3(fabs(a.x), fabs(a.y), fabs(a.z)); }
INLINE DPoint3 max(const DPoint3 &a, const DPoint3 &b) { return DPoint3(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z)); }
INLINE DPoint3 min(const DPoint3 &a, const DPoint3 &b) { return DPoint3(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z)); }
template <>
INLINE DPoint3 clamp(DPoint3 t, const DPoint3 min_val, const DPoint3 max_val)
{
  return min(max(t, min_val), max_val);
}

INLINE DPoint3 dpoint3(const Point3 &p) { return DPoint3::xyz(p); }
INLINE DPoint3 dpoint3(const DPoint3 &p) { return p; }
INLINE Point3 point3(const DPoint3 &p) { return Point3::xyz(p); }
INLINE Point3 point3(const Point3 &p) { return p; }

INLINE Point3 perpendicular(const Point3 &v)
{
  if (fabs(v.x) < fabs(v.y))
  {
    if (fabs(v.x) < fabs(v.z))
      return normalize(v % Point3(1.0, 0.0, 0.0));
    else
      return normalize(v % Point3(0.0, 0.0, 1.0));
  }
  else
  {
    if (fabs(v.y) < fabs(v.z))
      return normalize(v % Point3(0.0, 1.0, 0.0));
    else
      return normalize(v % Point3(1.0, 0.0, 1.0));
  }
}

#undef INLINE

/// @}
