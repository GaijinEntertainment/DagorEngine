// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <cstdint>
#include <math/dag_color.h>
#include <math/dag_TMatrix4more.h>
#include <drv/3d/dag_consts.h>
#include <shaders/shInternalTypes.h>
#include <EASTL/algorithm.h>
#include <EASTL/bit.h>

namespace stcode::cpp
{

typedef uint32_t uint;

// @TODO: simdify all of this

namespace detail
{

enum class Swizzle
{
  R,
  G,
  B,
  A,
  NONE
};

struct Mask
{
  Swizzle r = Swizzle::NONE;
  Swizzle g = Swizzle::NONE;
  Swizzle b = Swizzle::NONE;
  Swizzle a = Swizzle::NONE;
};

constexpr bool mask_is_valid(const Mask &mask)
{
  if (mask.r == Swizzle::NONE)
    return false;
  if (mask.g == Swizzle::NONE && (mask.b != Swizzle::NONE || mask.a != Swizzle::NONE))
    return false;
  if (mask.b == Swizzle::NONE && mask.a != Swizzle::NONE)
    return false;
  return true;
}

constexpr Mask make_mask(const char *val, size_t n)
{
  Mask mask{};
  Swizzle *dst[] = {&mask.r, &mask.g, &mask.b, &mask.a};
  for (size_t i = 0; i < n; ++i)
  {
    switch (val[i])
    {
      case 'r':
      case 'x': *dst[i] = Swizzle::R; break;
      case 'g':
      case 'y': *dst[i] = Swizzle::G; break;
      case 'b':
      case 'z': *dst[i] = Swizzle::B; break;
      case 'a':
      case 'w': *dst[i] = Swizzle::A; break;
      default: return {};
    }
  }
  return mask;
}

template <Mask MASK, typename Tv4>
inline Tv4 get_swizzle(const Tv4 &v)
{
  static_assert(mask_is_valid(MASK));

  using Vt = decltype(v.r);
  static_assert(sizeof(Vt) == sizeof(float));

  constexpr Vt def = eastl::bit_cast<Vt>(float(1.f));

  Tv4 res{def};

  auto applySwizzle = [&v](Vt &to, Swizzle swz) {
    switch (swz)
    {
      case Swizzle::R: to = v.r; break;
      case Swizzle::G: to = v.g; break;
      case Swizzle::B: to = v.b; break;
      case Swizzle::A: to = v.a; break;
      default: break;
    }
  };

  applySwizzle(res.r, MASK.r);
  applySwizzle(res.g, MASK.g);
  applySwizzle(res.b, MASK.b);
  applySwizzle(res.a, MASK.a);

  return res;
}

} // namespace detail

// Hack for backwards compat with string masks
constexpr detail::Mask operator""_mask(const char *val, size_t n) { return detail::make_mask(val, n); }

#define CAT(v, s)      v##s
#define DECODE_MASK(d) CAT(d, _mask)

struct float4 : Color4
{
  float4() : Color4(0, 0, 0, 0) {}
  float4(float r, float g, float b, float a) : Color4(r, g, b, a) {}
  float4(float r) : Color4(r, r, r, r) {}
  float4(Color4 other) : Color4(other) {}

  operator Point4() const { return Point4(r, g, b, a); }

  template <detail::Mask MASK>
  float4 swizzleMask()
  {
    return detail::get_swizzle<MASK>(*this);
  }
};

struct int4
{
  int32_t r, g, b, a;

  int4() : r(0), g(0), b(0), a(0) {}
  int4(int32_t r, int32_t g, int32_t b, int32_t a) : r(r), g(g), b(b), a(a) {}
  int4(int32_t r) : r(r), g(r), b(r), a(r) {}
  int4(const float4 &f4) : r(f4.r), g(f4.g), b(f4.b), a(f4.a) {}

  operator float4() const { return float4(r, g, b, a); }

  template <detail::Mask MASK>
  float4 swizzleMask()
  {
    return detail::get_swizzle<MASK>(*this);
  }
};

#define swizzle(mask_) swizzleMask<DECODE_MASK(mask_)>()

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

// Helper types for partial writes of packed consts,
// for example, (float2 &)const_arr[N] = *some float4 expr*; will only write .xy of the expr
struct float1
{
  float r;
  float1 &operator=(float4 f4)
  {
    r = f4.r;
    return *this;
  }
};
struct float2
{
  float r, g;
  float2 &operator=(float4 f4)
  {
    r = f4.r;
    g = f4.g;
    return *this;
  }
};
struct float3
{
  float r, g, b;
  float3 &operator=(float4 f4)
  {
    r = f4.r;
    g = f4.g;
    b = f4.b;
    return *this;
  }
};
struct int1
{
  int32_t r;
  int1 &operator=(int4 i4)
  {
    r = i4.r;
    return *this;
  }
};
struct int2
{
  int32_t r, g;
  int2 &operator=(int4 i4)
  {
    r = i4.r;
    g = i4.g;
    return *this;
  }
};
struct int3
{
  int32_t r, g, b;
  int3 &operator=(int4 i4)
  {
    r = i4.r;
    g = i4.g;
    b = i4.b;
    return *this;
  }
};

} // namespace stcode::cpp
