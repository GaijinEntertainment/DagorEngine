//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point3.h>

#define INLINE __forceinline

/// @addtogroup math
/// @{

// forward declarations for external classes
class TMatrix;


// Quat - Quaternion
/**
  Quaternion - represents 3D rotation using 4D unit vector.
  @sa TMatrix Matrix3 Point3
*/
class Quat
{
public:
  real x, y, z, w;
  INLINE Quat() = default;
  Quat(const TMatrix &);
  INLINE Quat(real ax, real ay, real az, real aw)
  {
    x = ax;
    y = ay;
    z = az;
    w = aw;
  }
  INLINE Quat(const real *p)
  {
    x = p[0];
    y = p[1];
    z = p[2];
    w = p[3];
  }
  INLINE Quat(const Point3 &axis, real a)
  {
    Point3 ax = ::normalize(axis) * sinf(a * 0.5f);
    x = ax.x;
    y = ax.y;
    z = ax.z;
    w = cosf(a * 0.5f);
  }

  INLINE void identity()
  {
    x = y = z = 0;
    w = 1;
  }

  INLINE Point3 getForward() const;
  INLINE Point3 getUp() const;
  INLINE Point3 getLeft() const;

  INLINE const real &operator[](int i) const { return (&x)[i]; }
  INLINE real &operator[](int i) { return (&x)[i]; }
  INLINE Quat operator-() const { return Quat(-x, -y, -z, -w); }
  INLINE Quat operator+() const { return *this; }

  INLINE Quat operator+(const Quat &a) const { return Quat(x + a.x, y + a.y, z + a.z, w + a.w); }
  INLINE Quat operator-(const Quat &a) const { return Quat(x - a.x, y - a.y, z - a.z, w - a.w); }
  INLINE Quat operator*(real a) const { return Quat(x * a, y * a, z * a, w * a); }
  INLINE Quat operator/(real a) const { return operator*(1.0f / a); }
  INLINE Quat operator*(const Quat &a) const;

  INLINE Quat &operator+=(const Quat &a)
  {
    x += a.x;
    y += a.y;
    z += a.z;
    w += a.w;
    return *this;
  }
  INLINE Quat &operator-=(const Quat &a)
  {
    x -= a.x;
    y -= a.y;
    z -= a.z;
    w -= a.w;
    return *this;
  }
  INLINE Quat &operator*=(real a)
  {
    x *= a;
    y *= a;
    z *= a;
    w *= a;
    return *this;
  }
  INLINE Quat &operator/=(real a) { return operator*=(1.0f / a); }

  INLINE bool operator==(const Quat &a) const { return (x == a.x && y == a.y && z == a.z && w == a.w); }
  INLINE bool operator!=(const Quat &a) const { return (x != a.x || y != a.y || z != a.z || w != a.w); }
  /// transform #Point3 by this quaternion
  INLINE Point3 operator*(const Point3 &) const;
  INLINE Quat normalize() const;

  template <class T>
  static Quat xyzw(const T &a)
  {
    return Quat(a.x, a.y, a.z, a.w);
  }
};

INLINE Quat normalize(const Quat &a)
{
  real l = sqrtf(float(a.x) * float(a.x) + float(a.y) * float(a.y) + float(a.z) * float(a.z) + float(a.w) * float(a.w));
  return (l == 0) ? Quat(0, 0, 0, 0) : Quat(a.x / l, a.y / l, a.z / l, a.w / l);
}

INLINE Quat inverse(const Quat &a) { return Quat(-a.x, -a.y, -a.z, a.w); }

INLINE Point3 Quat::operator*(const Point3 &p) const
{
  return Point3((x * x * 2 + w * w * 2 - 1) * p.x + (x * y * 2 - z * w * 2) * p.y + (x * z * 2 + y * w * 2) * p.z,
    (y * x * 2 + z * w * 2) * p.x + (y * y * 2 + w * w * 2 - 1) * p.y + (y * z * 2 - x * w * 2) * p.z,
    (z * x * 2 - y * w * 2) * p.x + (z * y * 2 + x * w * 2) * p.y + (z * z * 2 + w * w * 2 - 1) * p.z);
}

INLINE Quat Quat::operator*(const Quat &a) const
{
  return Quat(w * a.x + x * a.w + y * a.z - z * a.y, w * a.y + y * a.w + z * a.x - x * a.z, w * a.z + z * a.w + x * a.y - y * a.x,
    w * a.w - x * a.x - y * a.y - z * a.z);
}

INLINE Point3 Quat::getForward() const
{
  return ::normalize(Point3(2.f * (x * x + w * w - 0.5f), 2.f * (y * x + z * w), 2.f * (z * x - y * w)));
}

INLINE Point3 Quat::getUp() const
{
  return ::normalize(Point3(2.f * (x * y - z * w), 2.f * (y * y + w * w - 0.5f), 2.f * (z * y + x * w)));
}

INLINE Point3 Quat::getLeft() const
{
  return ::normalize(Point3(2.f * (x * z + y * w), 2.f * (y * z - x * w), 2.f * (z * z + w * w - 0.5f)));
}

INLINE Quat Quat::normalize() const { return ::normalize(*this); }


/// spherical interpolation from @a a to @a b by parameter @a t
/// (t=0 is a, t=1 is b, may be out of [0.\ .1] range to extrapolate)
INLINE Quat qinterp(const Quat &a, const Quat &b, real t)
{
  real f = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
  if (f >= 0.9999f)
  {
    return a + (b - a) * t;
  }
  else if (f <= -0.9999f)
  {
    return (a + b) * t - a;
  }
  else
  {
    Quat bUse = (f < 0) ? -b : b;
    f = fabsf(f);
    real w = acosf(f);
    real sinw = sinf(w);
    return a * (sinf(w * (1 - t)) / sinw) + bUse * (sinf(w * t) / sinw);
  }
}

/**
  fast interpolation, uses some precomputed values.
*/
INLINE Quat fastqinterp(const Quat &a, const Quat &b, real w, real sinw, real t)
{
  if (w == 0)
    return a + (b - a) * t;
  return (a * sinf(w * (1 - t)) + b * sinf(w * t)) / sinw;
}

template <>
INLINE Quat approach<Quat>(Quat from, Quat to, float dt, float viscosity)
{
  if (viscosity < 1e-5)
    return to;
  return qinterp(from, to, 1.0f - expf(-dt / viscosity));
}

/// Calculate quaternion that rotates one vector to another.
/// Vectors do not have to be normalized.
INLINE Quat quat_rotation_arc(const Point3 &in_v0, const Point3 &in_v1)
{
  const float F_DOT_ERROR = 0.0001f;

  Point3 v0 = normalize(in_v0);
  Point3 v1 = normalize(in_v1);
  Point3 c = v0 % v1;
  float d = v0 * v1;

  if (d < -1.0f + F_DOT_ERROR)
  {
    Point3 p;
    if (fabsf(v0[2]) > M_SQRT1_2)
    {
      // choose p in y-z plane
      float a = v0[1] * v0[1] + v0[2] * v0[2];
      float k = safediv(1.0f, sqrtf(a));
      p[0] = 0;
      p[1] = -v0[2] * k;
      p[2] = v0[1] * k;
    }
    else
    {
      // choose p in x-y plane
      float a = v0[0] * v0[0] + v0[1] * v0[1];
      float k = safediv(1.0f, sqrtf(a));
      p[0] = -v0[1] * k;
      p[1] = v0[0] * k;
      p[2] = 0;
    }
    return Quat(p.x, p.y, p.z, 0.0f); // just pick any vector that is orthogonal to v0
  }

  float s = sqrtf((1.0f + d) * 2.0f);
  float rs = safediv(1.0f, s);
  return Quat(c.x * rs, c.y * rs, c.z * rs, s * 0.5f);
}

struct DECLSPEC_ALIGN(16) Quat_vec4 : public Quat
{
  INLINE Quat_vec4() = default;
  INLINE Quat_vec4 &operator=(const Quat &b)
  {
    Quat::operator=(b);
    return *this;
  }
  INLINE Quat_vec4(const Quat &b) : Quat(b) {}
} ATTRIBUTE_ALIGN(16);

// intentionally overload, so template function won't be used.
// these functions are not implemented, use qinterp, quasi_slerp, etc instead
Quat lerp(const Quat a, const Quat b, float t);
Quat lerp(const Quat a, const Quat b, double t);

#undef INLINE

/// @}
