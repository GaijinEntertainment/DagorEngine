//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// IPoint3 - integer coordinate 3D point/vector //

#include <math/dag_mathBase.h>
#include <util/dag_globDef.h>
#include <stdlib.h>
#include <math/dag_Point3.h>

#define INLINE __forceinline

/**
  integer coordinate 3D point/vector
  @sa Point3 DPoint3
*/
class IPoint3
{
public:
  int x, y, z;

  constexpr INLINE IPoint3() = default;
  constexpr INLINE IPoint3(int ax, int ay, int az) : x(ax), y(ay), z(az) {}
  // IPoint3(const IPoint3& p) {x=p.x;y=p.y;z=p.z;}
  // IPoint3& operator =(const Point3& p) {x=p.x;y=p.y;z=p.z;return *this;}
  INLINE explicit IPoint3(const Point3 &p)
  {
    x = (int)p.x;
    y = (int)p.y;
    z = (int)p.z;
  }
  constexpr INLINE explicit IPoint3(const int *p) : x(p[0]), y(p[1]), z(p[2]) {}

  static const IPoint3 ZERO;
  static const IPoint3 ONE;

  constexpr INLINE void zero()
  {
    x = 0;
    y = 0;
    z = 0;
  }
  constexpr INLINE void set(int _x, int _y, int _z)
  {
    x = _x;
    y = _y;
    z = _z;
  }

  INLINE const int &operator[](int i) const { return (&x)[i]; }
  INLINE int &operator[](int i) { return (&x)[i]; }
  INLINE operator Point3() const { return Point3(x, y, z); }
  constexpr INLINE IPoint3 operator-() const { return IPoint3(-x, -y, -z); }
  constexpr INLINE IPoint3 operator+() const { return *this; }

  constexpr INLINE IPoint3 operator+(const IPoint3 &a) const { return IPoint3(x + a.x, y + a.y, z + a.z); }
  constexpr INLINE IPoint3 operator-(const IPoint3 &a) const { return IPoint3(x - a.x, y - a.y, z - a.z); }

  /// Dot product.
  constexpr INLINE int operator*(const IPoint3 &a) const { return x * a.x + y * a.y + z * a.z; }

  /// Cross product.
  constexpr INLINE IPoint3 operator%(const IPoint3 &a) const
  {
    return IPoint3(y * a.z - z * a.y, z * a.x - x * a.z, x * a.y - y * a.x);
  }

  constexpr INLINE IPoint3 operator+(int a) const { return IPoint3(x + a, y + a, z + a); }
  constexpr INLINE IPoint3 operator-(int a) const { return IPoint3(x - a, y - a, z - a); }
  constexpr INLINE IPoint3 operator*(int a) const { return IPoint3(x * a, y * a, z * a); }
  constexpr INLINE IPoint3 operator/(int a) const { return IPoint3(x / a, y / a, z / a); }
  constexpr INLINE IPoint3 operator%(int a) const { return IPoint3(x % a, y % a, z % a); }
  constexpr INLINE IPoint3 operator>>(int a) const { return IPoint3(x >> a, y >> a, z >> a); }
  constexpr INLINE IPoint3 operator<<(int a) const { return IPoint3(x << a, y << a, z << a); }

  constexpr INLINE IPoint3 &operator+=(const IPoint3 &a)
  {
    x += a.x;
    y += a.y;
    z += a.z;
    return *this;
  }
  constexpr INLINE IPoint3 &operator-=(const IPoint3 &a)
  {
    x -= a.x;
    y -= a.y;
    z -= a.z;
    return *this;
  }
  constexpr INLINE IPoint3 &operator+=(int a)
  {
    x += a;
    y += a;
    z += a;
    return *this;
  }
  constexpr INLINE IPoint3 &operator-=(int a)
  {
    x -= a;
    y -= a;
    z -= a;
    return *this;
  }
  constexpr INLINE IPoint3 &operator*=(int a)
  {
    x *= a;
    y *= a;
    z *= a;
    return *this;
  }
  constexpr INLINE IPoint3 &operator/=(int a)
  {
    x /= a;
    y /= a;
    z /= a;
    return *this;
  }
  constexpr INLINE IPoint3 &operator%=(int a)
  {
    x %= a;
    y %= a;
    z %= a;
    return *this;
  }
  constexpr INLINE IPoint3 &operator>>=(int a)
  {
    x >>= a;
    y >>= a;
    z >>= a;
    return *this;
  }
  constexpr INLINE IPoint3 &operator<<=(int a)
  {
    x <<= a;
    y <<= a;
    z <<= a;
    return *this;
  }

  constexpr INLINE bool operator==(const IPoint3 &a) const { return (x == a.x && y == a.y && z == a.z); }
  constexpr INLINE bool operator!=(const IPoint3 &a) const { return (x != a.x || y != a.y || z != a.z); }
  constexpr INLINE int lengthSq() const { return x * x + y * y + z * z; }
  INLINE float length() const { return sqrtf(lengthSq()); }
  INLINE real lengthF() const { return fastsqrt(lengthSq()); }

  template <class T>
  constexpr static IPoint3 xyz(const T &a)
  {
    return IPoint3((int)a.x, (int)a.y, (int)a.z);
  }
  template <class T>
  constexpr static IPoint3 xzy(const T &a)
  {
    return IPoint3((int)a.x, (int)a.z, (int)a.y);
  }
  template <class T>
  constexpr static IPoint3 x0y(const T &a)
  {
    return IPoint3((int)a.x, 0, (int)a.y);
  }
  template <class T>
  constexpr static IPoint3 x0z(const T &a)
  {
    return IPoint3((int)a.x, 0, (int)a.z);
  }
  template <class T>
  constexpr static IPoint3 xy0(const T &a)
  {
    return IPoint3((int)a.x, (int)a.y, 0);
  }
  template <class T>
  constexpr static IPoint3 xz0(const T &a)
  {
    return IPoint3((int)a.x, (int)a.z, 0);
  }
  template <class T>
  constexpr static IPoint3 xVy(const T &a, int v)
  {
    return IPoint3((int)a.x, v, (int)a.y);
  }
  template <class T>
  constexpr static IPoint3 xVz(const T &a, int v)
  {
    return IPoint3((int)a.x, v, (int)a.z);
  }
  template <class T>
  constexpr static IPoint3 xyV(const T &a, int v)
  {
    return IPoint3((int)a.x, (int)a.y, v);
  }
  template <class T>
  constexpr static IPoint3 xzV(const T &a, int v)
  {
    return IPoint3((int)a.x, (int)a.z, v);
  }

  template <class T>
  constexpr void set_xyz(const T &a)
  {
    x = (int)a.x, y = (int)a.y, z = (int)a.z;
  }
  template <class T>
  constexpr void set_xzy(const T &a)
  {
    x = (int)a.x, y = (int)a.z, z = (int)a.y;
  }
  template <class T>
  constexpr void set_x0y(const T &a)
  {
    x = (int)a.x, y = 0, z = (int)a.y;
  }
  template <class T>
  constexpr void set_x0z(const T &a)
  {
    x = (int)a.x, y = 0, z = (int)a.z;
  }
  template <class T>
  constexpr void set_xy0(const T &a)
  {
    x = (int)a.x, y = (int)a.y, z = 0;
  }
  template <class T>
  constexpr void set_xz0(const T &a)
  {
    x = (int)a.x, y = (int)a.z, z = 0;
  }
  template <class T>
  constexpr void set_xVy(const T &a, int v)
  {
    x = (int)a.x, y = v, z = (int)a.y;
  }
  template <class T>
  constexpr void set_xVz(const T &a, int v)
  {
    x = (int)a.x, y = v, z = (int)a.z;
  }
  template <class T>
  constexpr void set_xyV(const T &a, int v)
  {
    x = (int)a.x, y = (int)a.y, z = v;
  }
  template <class T>
  constexpr void set_xzV(const T &a, int v)
  {
    x = (int)a.x, y = (int)a.z, z = v;
  }
};

constexpr INLINE IPoint3 operator*(int a, const IPoint3 &p) { return IPoint3(p.x * a, p.y * a, p.z * a); }
constexpr INLINE int lengthSq(const IPoint3 &a) { return a.x * a.x + a.y * a.y + a.z * a.z; }

__forceinline IPoint3 abs(const IPoint3 &a) { return IPoint3(abs(a.x), abs(a.y), abs(a.z)); }
#undef max
#undef min
constexpr __forceinline IPoint3 max(const IPoint3 &a, const IPoint3 &b)
{
  return IPoint3(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z));
}
constexpr __forceinline IPoint3 min(const IPoint3 &a, const IPoint3 &b)
{
  return IPoint3(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z));
}
template <>
INLINE IPoint3 clamp(IPoint3 t, const IPoint3 min_val, const IPoint3 max_val)
{
  return min(max(t, min_val), max_val);
}
constexpr INLINE IPoint3 mul(const IPoint3 &a, const IPoint3 &b) { return IPoint3(a.x * b.x, a.y * b.y, a.z * b.z); }
constexpr INLINE IPoint3 div(const IPoint3 &a, const IPoint3 &b) { return IPoint3(a.x / b.x, a.y / b.y, a.z / b.z); }

INLINE IPoint3 ipoint3(const Point3 &p) { return IPoint3(real2int(p.x), real2int(p.y), real2int(p.z)); }
INLINE Point3 point3(const IPoint3 &p) { return Point3(p.x, p.y, p.z); }

#undef INLINE
