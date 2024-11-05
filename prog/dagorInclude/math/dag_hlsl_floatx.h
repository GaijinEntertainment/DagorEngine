//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_mathBase.h>
#include <math/dag_mathUtils.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_half.h>

#undef min
#undef max
#define INLINE __forceinline
#define BRANCH (0);
typedef unsigned int uint;
class TMatrix4;
struct mat43f;
typedef TMatrix4 float4x4;
typedef mat43f float4x3;

struct HlslUint2
{
  uint x, y;
  INLINE HlslUint2() {} // -V730
  INLINE HlslUint2(uint ax, uint ay) : x(ax), y(ay) {}
  INLINE uint &operator[](int i) { return (&x)[i]; }
  INLINE const uint &operator[](int i) const { return (&x)[i]; }
};

struct HlslUint3
{
  uint x, y, z;
  INLINE HlslUint3() {} // -V730
  INLINE HlslUint3(uint ax, uint ay, uint az) : x(ax), y(ay), z(az) {}
  INLINE uint &operator[](int i) { return (&x)[i]; }
  INLINE const uint &operator[](int i) const { return (&x)[i]; }
};

struct HlslUint4
{
  uint x, y, z, w;
  INLINE HlslUint4() {} // -V730
  INLINE HlslUint4(uint ax, uint ay, uint az, uint aw) : x(ax), y(ay), z(az), w(aw) {}
  INLINE uint &operator[](int i) { return (&x)[i]; }
  INLINE const uint &operator[](int i) const { return (&x)[i]; }
};

typedef HlslUint2 uint2;
typedef HlslUint3 uint3;
typedef HlslUint4 uint4;

struct HlslInt2
{
  int x, y;
  INLINE HlslInt2() {} // -V730
  INLINE HlslInt2(int ax, int ay) : x(ax), y(ay) {}
  INLINE int &operator[](int i) { return (&x)[i]; }
};

struct HlslInt3
{
  int x, y, z;
  INLINE HlslInt3() {} // -V730
  INLINE HlslInt3(int ax, int ay, int az) : x(ax), y(ay), z(az) {}
  INLINE int &operator[](int i) { return (&x)[i]; }
};

struct HlslInt4
{
  int x, y, z, w;
  INLINE HlslInt4() {} // -V730
  INLINE HlslInt4(int ax, int ay, int az, int aw) : x(ax), y(ay), z(az), w(aw) {}
  INLINE int &operator[](int i) { return (&x)[i]; }
};

typedef HlslInt2 int2;
typedef HlslInt3 int3;
typedef HlslInt4 int4;

struct Half
{
private:
  uint16_t c;

public:
  INLINE Half() : c(0) {}
  INLINE Half(float ax) { set(ax); }

  INLINE float get() const { return half_to_float(c); }
  INLINE void set(float v) { c = float_to_half(v); }

  operator float() const { return get(); }
};

struct Half2
{
  Half x, y;

  INLINE Half2() {}
  INLINE Half2(float ax, float ay)
  {
    set(0, ax);
    set(1, ay);
  }
  INLINE Half2(const Point2 &a) : Half2(a.x, a.y) {}

  INLINE float get(int i) const { return (&x)[i].get(); }
  INLINE void set(int i, float v) { (&x)[i].set(v); }

  INLINE Half operator[](int i) const { return (&x)[i]; }
  operator Point2() const { return Point2(x, y); }
};

struct Half3
{
  Half x, y, z;

  INLINE Half3() {}
  INLINE Half3(float ax, float ay, float az)
  {
    set(0, ax);
    set(1, ay);
    set(2, az);
  }
  INLINE Half3(const Point3 &a) : Half3(a.x, a.y, a.z) {}

  INLINE float get(int i) const { return (&x)[i].get(); }
  INLINE void set(int i, float v) { (&x)[i].set(v); }

  INLINE Half operator[](int i) const { return (&x)[i]; }
  operator Point3() const { return Point3(x, y, z); }
};

struct Half4
{
  Half x, y, z, w;

  INLINE Half4() {}
  INLINE Half4(float ax, float ay, float az, float aw)
  {
    set(0, ax);
    set(1, ay);
    set(2, az);
    set(3, aw);
  }
  INLINE Half4(const Point4 &a) : Half4(a.x, a.y, a.z, a.w) {}

  INLINE float get(int i) const { return (&x)[i].get(); }
  INLINE void set(int i, float v) { (&x)[i].set(v); }

  INLINE Half operator[](int i) const { return (&x)[i]; }
  operator Point4() const { return Point4(x, y, z, w); }
};

typedef Half half;
typedef Half2 half2;
typedef Half3 half3;
typedef Half4 half4;

class float2 : public Point2
{
public:
  INLINE float2() {}
  INLINE float2(real ax, real ay)
  {
    x = ax;
    y = ay;
  }
  INLINE float2(const Point2 &a) : Point2(a) {}
  INLINE float2 operator-() const { return float2(-x, -y); }
  INLINE float2 operator+() const { return *this; }
  INLINE float2 operator-(const Point2 &a) const { return float2(x - a.x, y - a.y); }
  INLINE float2 operator+(const Point2 &a) const { return float2(x + a.x, y + a.y); }
  INLINE float2 operator*(const Point2 &a) const { return float2(x * a.x, y * a.y); }
  INLINE float2 operator/(const Point2 &a) const { return float2(x / a.x, y / a.y); }
  INLINE float2 operator*(float a) const { return float2(x * a, y * a); }
  INLINE float2 operator/(float a) const { return float2(x / a, y / a); }
  INLINE float2 &operator*=(float a)
  {
    x *= a;
    y *= a;
    return *this;
  }
  INLINE float2 &operator*=(const Point2 &a)
  {
    x *= a.x;
    y *= a.y;
    return *this;
  }
};

class float3 : public Point3
{
public:
  INLINE float3() {}
  INLINE float3(real ax, real ay, real az)
  {
    x = ax;
    y = ay;
    z = az;
  }
  INLINE float3(const Point3 &a) : Point3(a) {}
  INLINE float3 operator-() const { return float3(-x, -y, -z); }
  INLINE float3 operator+() const { return *this; }
  INLINE float3 operator-(const Point3 &a) const { return float3(x - a.x, y - a.y, z - a.z); }
  INLINE float3 operator+(const Point3 &a) const { return float3(x + a.x, y + a.y, z + a.z); }
  INLINE float3 operator*(const Point3 &a) const { return float3(x * a.x, y * a.y, z * a.z); }
  INLINE float3 operator/(const Point3 &a) const { return float3(x / a.x, y / a.y, z / a.z); }
  INLINE float3 operator*(float a) const { return float3(x * a, y * a, z * a); }
  INLINE float3 operator/(float a) const { return float3(x / a, y / a, z / a); }
  INLINE float3 &operator*=(float a)
  {
    x *= a;
    y *= a;
    z *= a;
    return *this;
  }
  INLINE float3 &operator*=(const Point3 &a)
  {
    x *= a.x;
    y *= a.y;
    z *= a.z;
    return *this;
  }
};


class float4 : public Point4
{
public:
  INLINE float4() {}
  INLINE float4(real ax, real ay, real az, real aw)
  {
    x = ax;
    y = ay;
    z = az;
    w = aw;
  }
  INLINE float4(const Point2 &xy, const Point2 &zw)
  {
    x = xy.x;
    y = xy.y;
    z = zw.x;
    w = zw.y;
  }
  INLINE float4(real ax, const Point2 &yz, real aw)
  {
    x = ax;
    y = yz.x;
    z = yz.y;
    w = aw;
  }
  INLINE float4(const Point4 &a) : Point4(a) {}
  INLINE float4(const Point3 &a, float w) : Point4(a.x, a.y, a.z, w) {}
  INLINE float4 operator-() const { return float4(-x, -y, -z, -w); }
  INLINE float4 operator+() const { return *this; }
  INLINE float4 operator-(const Point4 &a) const { return float4(x - a.x, y - a.y, z - a.z, w - a.w); }
  INLINE float4 operator+(const Point4 &a) const { return float4(x + a.x, y + a.y, z + a.z, w + a.w); }
  INLINE float4 operator*(const Point4 &a) const { return float4(x * a.x, y * a.y, z * a.z, w * a.w); }
  INLINE float4 operator/(const Point4 &a) const { return float4(x / a.x, y / a.y, z / a.z, w / a.w); }
  INLINE float4 operator*(float a) const { return float4(x * a, y * a, z * a, w * a); }
  INLINE float4 operator/(float a) const { return float4(x / a, y / a, z / a, w / a); }
  INLINE float4 &operator*=(float a)
  {
    x *= a;
    y *= a;
    z *= a;
    w *= a;
    return *this;
  }
  INLINE float4 &operator*=(const Point4 &a)
  {
    x *= a.x;
    y *= a.y;
    z *= a.z;
    w *= a.w;
    return *this;
  }
};

INLINE float2 operator*(float a, const float2 &p) { return float2(p.x * a, p.y * a); }
INLINE float3 operator*(float a, const float3 &p) { return float3(p.x * a, p.y * a, p.z * a); }
INLINE float4 operator*(float a, const float4 &p) { return float4(p.x * a, p.y * a, p.z * a, p.w * a); }

INLINE float2 max(const float2 &a, const float2 &b) { return float2(max(a.x, b.x), max(a.y, b.y)); }
INLINE float2 min(const float2 &a, const float2 &b) { return float2(min(a.x, b.x), min(a.y, b.y)); }
template <>
INLINE float2 clamp(float2 t, const float2 min_val, const float2 max_val)
{
  return min(max(t, min_val), max_val);
}

INLINE float3 max(const float3 &a, const float3 &b) { return float3(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z)); }
INLINE float3 min(const float3 &a, const float3 &b) { return float3(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z)); }
template <>
INLINE float3 clamp(float3 t, const float3 min_val, const float3 max_val)
{
  return min(max(t, min_val), max_val);
}

INLINE float4 max(const float4 &a, const float4 &b) { return float4(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z), max(a.w, b.w)); }
INLINE float4 min(const float4 &a, const float4 &b) { return float4(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z), min(a.w, b.w)); }
template <>
INLINE float4 clamp(float4 t, const float4 min_val, const float4 max_val)
{
  return min(max(t, min_val), max_val);
}

INLINE float dot(const float3 &a, const float3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
INLINE float dot(const float4 &a, const float4 &b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
INLINE float dot(const float3 &a, const float *b) { return dot(a, *(float3 *)b); }
INLINE float dot(const float4 &a, const float *b) { return dot(a, *(float4 *)b); }

INLINE float3 reflect(const float3 &i, const float3 &n) { return i - 2.f * n * dot(i, n); }
INLINE float3 sign(const float3 &a) { return float3(sign(a.x), sign(a.y), sign(a.z)); }

INLINE float2 exp(const float2 &a) { return float2(expf(a.x), expf(a.y)); }
INLINE float3 exp(const float3 &a) { return float3(expf(a.x), expf(a.y), expf(a.z)); }
INLINE float4 exp(const float4 &a) { return float4(expf(a.x), expf(a.y), expf(a.z), expf(a.w)); }
INLINE float2 saturate(const float2 &a) { return min(max(a, float2(0, 0)), float2(1, 1)); }
INLINE float3 saturate(const float3 &a) { return min(max(a, float3(0, 0, 0)), float3(1, 1, 1)); }
INLINE float4 saturate(const float4 &a) { return min(max(a, float4(0, 0, 0, 0)), float4(1, 1, 1, 1)); }
inline float smoothstep(float edge0, float edge1, float x)
{
  // Scale, bias and saturate x to 0..1 range
  x = clamp((x - edge0) / (edge1 - edge0), 0.f, 1.f);
  // Evaluate polynomial
  return x * x * (3.f - 2.f * x);
}
#undef INLINE
