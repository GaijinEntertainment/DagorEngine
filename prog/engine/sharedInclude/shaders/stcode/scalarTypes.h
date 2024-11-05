// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <cstdint>
#include <math/dag_color.h>
#include <math/dag_TMatrix4more.h>
#include <drv/3d/dag_consts.h>
#include <shaders/shInternalTypes.h>
#include <EASTL/algorithm.h>

namespace stcode::cpp
{

typedef uint32_t uint;

template <typename Tv4>
Tv4 get_swizzle(Tv4 &v, const char *const channels)
{
  size_t numChannels = strlen(channels);
  Tv4 res(1);
  decltype(v.r) *dst[4] = {&res.r, &res.g, &res.b, &res.a};
  for (int i = 0; i < numChannels; ++i)
  {
    switch (channels[i])
    {
      case 'r':
      case 'x': *dst[i] = v.r; break;
      case 'g':
      case 'y': *dst[i] = v.g; break;
      case 'b':
      case 'z': *dst[i] = v.b; break;
      case 'a':
      case 'w': *dst[i] = v.a; break;
    }
  }
  return res;
}

struct float4 : Color4
{
  float4() : Color4(0, 0, 0, 0) {}
  float4(float r, float g, float b, float a) : Color4(r, g, b, a) {}
  float4(float r) : Color4(r, r, r, r) {}
  float4(Color4 other) : Color4(other) {}

  operator Point4() const { return Point4(r, g, b, a); }

  float4 swizzle(const char *const channels) { return get_swizzle(*this, channels); }
};

struct int4
{
  int32_t r, g, b, a;

  int4() : r(0), g(0), b(0), a(0) {}
  int4(int32_t r, int32_t g, int32_t b, int32_t a) : r(r), g(g), b(b), a(a) {}
  int4(int32_t r) : r(r), g(r), b(r), a(r) {}
  int4(const float4 &f4) : r(f4.r), g(f4.g), b(f4.b), a(f4.a) {}

  operator float4() const { return float4(r, g, b, a); }

  float4 swizzle(const char *const channels) { return get_swizzle(*this, channels); }
};

inline float4 operator+(float r, float4 v) { return float4(r) + v; }

inline float4 operator-(float r, float4 v) { return float4(r) - v; }

inline float4 operator*(float r, float4 v) { return float4(r) * v; }

inline float4 operator/(float r, float4 v) { return float4(r) / v; }

inline float4 operator+(float4 v, float r) { return float4(r) + v; }

inline float4 operator-(float4 v, float r) { return v - float4(r); }

inline float4 operator*(float4 v, float r) { return float4(r) * v; }

inline float4 operator/(float4 v, float r) { return v / float4(r); }

struct float4x4 : public TMatrix4_vec4
{
  float4x4 &operator=(const float4 other[4])
  {
    eastl::copy_n(other, 4, row);
    return *this;
  }

  float4x4 &operator=(const TMatrix4_vec4 other)
  {
    TMatrix4_vec4::operator=(other);
    return *this;
  }
};

} // namespace stcode::cpp
