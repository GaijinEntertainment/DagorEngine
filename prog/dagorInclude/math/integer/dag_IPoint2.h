//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

// IPoint2 - integer coordinate 2D point/vector //
#include <math/dag_mathBase.h>
#include <util/dag_globDef.h>
#include <stdlib.h>
#include <math/dag_Point2.h>

#define INLINE __forceinline


/**
  integer coordinate 2D point/vector
  @sa Point2 IPoint3 Point3 Point4
*/
class IPoint2
{
public:
  int x, y;

  INLINE IPoint2() = default;
  INLINE IPoint2(int ax, int ay)
  {
    x = ax;
    y = ay;
  }
  // IPoint2(const IPoint2& p) {x=p.x;y=p.y;}
  // IPoint2& operator =(const Point2& p) {x=p.x;y=p.y;return *this;}
  INLINE explicit IPoint2(const int *p)
  {
    x = p[0];
    y = p[1];
  }

  static const IPoint2 ZERO;
  static const IPoint2 ONE;

  INLINE void zero()
  {
    x = 0;
    y = 0;
  }
  INLINE void set(int _x, int _y)
  {
    x = _x;
    y = _y;
  }

  INLINE const int &operator[](int i) const { return (&x)[i]; }
  INLINE int &operator[](int i) { return (&x)[i]; }
  INLINE operator Point2() const { return Point2((real)x, (real)y); }
  INLINE IPoint2 operator-() const { return IPoint2(-x, -y); }
  INLINE IPoint2 operator+() const { return *this; }

  INLINE IPoint2 operator+(const IPoint2 &a) const { return IPoint2(x + a.x, y + a.y); }
  INLINE IPoint2 operator-(const IPoint2 &a) const { return IPoint2(x - a.x, y - a.y); }
  INLINE int operator*(const IPoint2 &a) const { return x * a.x + y * a.y; }
  INLINE IPoint2 operator*(int a) const { return IPoint2(x * a, y * a); }
  INLINE IPoint2 operator/(int a) const { return IPoint2(x / a, y / a); }
  INLINE IPoint2 operator%(int a) const { return IPoint2(x % a, y % a); }
  INLINE IPoint2 operator>>(int a) const { return IPoint2(x >> a, y >> a); }
  INLINE IPoint2 operator<<(int a) const { return IPoint2(x << a, y << a); }

  INLINE IPoint2 &operator+=(const IPoint2 &a)
  {
    x += a.x;
    y += a.y;
    return *this;
  }
  INLINE IPoint2 &operator-=(const IPoint2 &a)
  {
    x -= a.x;
    y -= a.y;
    return *this;
  }
  INLINE IPoint2 &operator*=(int a)
  {
    x *= a;
    y *= a;
    return *this;
  }
  INLINE IPoint2 &operator/=(int a)
  {
    x /= a;
    y /= a;
    return *this;
  }
  INLINE IPoint2 &operator%=(int a)
  {
    x %= a;
    y %= a;
    return *this;
  }
  INLINE IPoint2 &operator>>=(int a)
  {
    x >>= a;
    y >>= a;
    return *this;
  }
  INLINE IPoint2 &operator<<=(int a)
  {
    x <<= a;
    y <<= a;
    return *this;
  }

  INLINE bool operator==(const IPoint2 &a) const { return (x == a.x && y == a.y); }
  INLINE bool operator!=(const IPoint2 &a) const { return (x != a.x || y != a.y); }
  INLINE int lengthSq() const { return x * x + y * y; }
  INLINE float length() const { return sqrtf(lengthSq()); }
  INLINE real lengthF() const { return rsqrt(lengthSq()); }

  template <class T>
  static IPoint2 xy(const T &a)
  {
    return IPoint2((int)a.x, (int)a.y);
  }
  template <class T>
  static IPoint2 xz(const T &a)
  {
    return IPoint2((int)a.x, (int)a.z);
  }
  template <class T>
  static IPoint2 yz(const T &a)
  {
    return IPoint2((int)a.y, (int)a.z);
  }

  template <class T>
  void set_xy(const T &a)
  {
    x = (int)a.x, y = (int)a.y;
  }
  template <class T>
  void set_xz(const T &a)
  {
    x = (int)a.x, y = (int)a.z;
  }
  template <class T>
  void set_yz(const T &a)
  {
    x = (int)a.y, y = (int)a.z;
  }
};

INLINE IPoint2 operator*(int a, const IPoint2 &p) { return IPoint2(p.x * a, p.y * a); }
INLINE int lengthSq(const IPoint2 &a) { return a.x * a.x + a.y * a.y; }

__forceinline IPoint2 abs(const IPoint2 &a) { return IPoint2(abs(a.x), abs(a.y)); }
#undef max
#undef min
__forceinline IPoint2 max(const IPoint2 &a, const IPoint2 &b) { return IPoint2(max(a.x, b.x), max(a.y, b.y)); }
__forceinline IPoint2 min(const IPoint2 &a, const IPoint2 &b) { return IPoint2(min(a.x, b.x), min(a.y, b.y)); }
template <>
INLINE IPoint2 clamp(IPoint2 t, const IPoint2 min_val, const IPoint2 max_val)
{
  return min(max(t, min_val), max_val);
}

INLINE IPoint2 ipoint2(const Point2 &p) { return IPoint2(real2int(p.x), real2int(p.y)); }
INLINE Point2 point2(const IPoint2 &p) { return Point2(p.x, p.y); }

INLINE IPoint2 ipoint2(const DPoint2 &p) { return IPoint2(int(p.x), int(p.y)); }
INLINE DPoint2 dpoint2(const IPoint2 &p) { return DPoint2(p.x, p.y); }

#undef INLINE
