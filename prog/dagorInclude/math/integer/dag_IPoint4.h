//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

// IPoint3 - integer coordinate 3D point/vector //
#include <math/dag_mathBase.h>
#include <util/dag_globDef.h>
#include <stdlib.h>
#include <math/dag_Point4.h>

#define INLINE __forceinline


/**
  integer coordinate 4D point/vector
  @sa Point4 DPoint4
*/
class IPoint4
{
public:
  int x, y, z, w;

  INLINE IPoint4() = default;
  INLINE IPoint4(int ax, int ay, int az, int aw)
  {
    x = ax;
    y = ay;
    z = az;
    w = aw;
  }
  // IPoint4(const IPoint4& p) {x=p.x;y=p.y;z=p.z;}
  // IPoint4& operator =(const Point3& p) {x=p.x;y=p.y;z=p.z;return *this;}
  INLINE IPoint4(const Point4 &p)
  {
    x = (int)p.x;
    y = (int)p.y;
    z = (int)p.z;
    w = (int)p.w;
  }
  INLINE explicit IPoint4(const int *p)
  {
    x = p[0];
    y = p[1];
    z = p[2];
    w = p[3];
  }

  static const IPoint4 ZERO;
  static const IPoint4 ONE;

  INLINE void zero()
  {
    x = 0;
    y = 0;
    z = 0;
    w = 0;
  }
  INLINE void set(int _x, int _y, int _z, int _w)
  {
    x = _x;
    y = _y;
    z = _z;
    w = _w;
  }

  INLINE const int &operator[](int i) const { return (&x)[i]; }
  INLINE int &operator[](int i) { return (&x)[i]; }
  INLINE operator Point4() const { return Point4(x, y, z, w); }
  INLINE IPoint4 operator-() const { return IPoint4(-x, -y, -z, -w); }
  INLINE IPoint4 operator+() const { return *this; }

  INLINE IPoint4 operator+(const IPoint4 &a) const { return IPoint4(x + a.x, y + a.y, z + a.z, w + a.w); }
  INLINE IPoint4 operator-(const IPoint4 &a) const { return IPoint4(x - a.x, y - a.y, z - a.z, w - a.w); }

  /// Dot product.
  INLINE int operator*(const IPoint4 &a) const { return x * a.x + y * a.y + z * a.z + w * a.w; }

  INLINE IPoint4 operator*(int a) const { return IPoint4(x * a, y * a, z * a, w * a); }
  INLINE IPoint4 operator/(int a) const { return IPoint4(x / a, y / a, z / a, w / a); }
  INLINE IPoint4 operator%(int a) const { return IPoint4(x % a, y % a, z % a, w % a); }
  INLINE IPoint4 operator>>(int a) const { return IPoint4(x >> a, y >> a, z >> a, w >> a); }
  INLINE IPoint4 operator<<(int a) const { return IPoint4(x << a, y << a, z << a, w << a); }

  INLINE IPoint4 &operator+=(const IPoint4 &a)
  {
    x += a.x;
    y += a.y;
    z += a.z;
    w += a.w;
    return *this;
  }
  INLINE IPoint4 &operator-=(const IPoint4 &a)
  {
    x -= a.x;
    y -= a.y;
    z -= a.z;
    w -= a.w;
    return *this;
  }
  INLINE IPoint4 &operator*=(int a)
  {
    x *= a;
    y *= a;
    z *= a;
    w *= a;
    return *this;
  }
  INLINE IPoint4 &operator/=(int a)
  {
    x /= a;
    y /= a;
    z /= a;
    w /= a;
    return *this;
  }
  INLINE IPoint4 &operator%=(int a)
  {
    x %= a;
    y %= a;
    z %= a;
    w %= a;
    return *this;
  }
  INLINE IPoint4 &operator>>=(int a)
  {
    x >>= a;
    y >>= a;
    z >>= a;
    w >>= a;
    return *this;
  }
  INLINE IPoint4 &operator<<=(int a)
  {
    x <<= a;
    y <<= a;
    z <<= a;
    w <<= a;
    return *this;
  }

  INLINE bool operator==(const IPoint4 &a) const { return (x == a.x && y == a.y && z == a.z && w == a.w); }
  INLINE bool operator!=(const IPoint4 &a) const { return (x != a.x || y != a.y || z != a.z || w != a.w); }
  INLINE int lengthSq() const { return x * x + y * y + z * z + w * w; }
  INLINE float length() const { return sqrtf(lengthSq()); }
  INLINE real lengthF() const { return fastsqrt(lengthSq()); }
};

INLINE IPoint4 operator*(int a, const IPoint4 &p) { return IPoint4(p.x * a, p.y * a, p.z * a, p.w * a); }
INLINE int lengthSq(const IPoint4 &a) { return a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w; }

__forceinline IPoint4 abs(const IPoint4 &a) { return IPoint4(abs(a.x), abs(a.y), abs(a.z), abs(a.w)); }
#undef max
#undef min
__forceinline IPoint4 max(const IPoint4 &a, const IPoint4 &b)
{
  return IPoint4(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z), max(a.w, b.w));
}
__forceinline IPoint4 min(const IPoint4 &a, const IPoint4 &b)
{
  return IPoint4(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z), min(a.z, b.z));
}
template <>
INLINE IPoint4 clamp(IPoint4 t, const IPoint4 min_val, const IPoint4 max_val)
{
  return min(max(t, min_val), max_val);
}

INLINE IPoint4 ipoint4(const Point4 &p) { return IPoint4(real2int(p.x), real2int(p.y), real2int(p.z), real2int(p.w)); }
INLINE Point4 point4(const IPoint4 &p) { return Point4(p.x, p.y, p.z, p.w); }

#undef INLINE
