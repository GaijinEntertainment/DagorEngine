//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_e3dColor.h>
#include <math/dag_mathBase.h>
#include <util/dag_globDef.h> //min/max

#undef min
#undef max
#define colorinline __forceinline

// Colors used in Driver3d materials and lighting //

struct Color4
{
  real r, g, b, a;
  colorinline Color4() = default;
  colorinline Color4(float rr, float gg, float bb, float aa = 1)
  {
    r = rr;
    g = gg;
    b = bb;
    a = aa;
  }
  colorinline Color4(const real *p)
  {
    r = p[0];
    g = p[1];
    b = p[2];
    a = p[3];
  }
  colorinline Color4(E3DCOLOR c)
  {
    r = float(c.r) / 255.f;
    g = float(c.g) / 255.f;
    b = float(c.b) / 255.f;
    a = float(c.a) / 255.f;
  }

  colorinline void set(real k)
  {
    r = k;
    g = k;
    b = k;
    a = k;
  }
  colorinline void set(real _r, real _g, real _b, real _a)
  {
    r = _r;
    g = _g;
    b = _b;
    a = _a;
  }
  colorinline void zero() { set(0); }

  colorinline real &operator[](int i) { return (&r)[i]; }
  colorinline const real &operator[](int i) const { return (&r)[i]; }

  colorinline Color4 operator+() const { return *this; }
  colorinline Color4 operator-() const { return Color4(-r, -g, -b, -a); }
  colorinline Color4 operator*(real k) const { return Color4(r * k, g * k, b * k, a * k); }
  colorinline Color4 operator/(real k) const { return operator*(1.0f / k); }
  colorinline Color4 operator*(const Color4 &c) const { return Color4(r * c.r, g * c.g, b * c.b, a * c.a); }
  colorinline Color4 operator/(const Color4 &c) const { return Color4(r / c.r, g / c.g, b / c.b, a / c.a); }
  colorinline Color4 operator+(const Color4 &c) const { return Color4(r + c.r, g + c.g, b + c.b, a + c.a); }
  colorinline Color4 operator-(const Color4 &c) const { return Color4(r - c.r, g - c.g, b - c.b, a - c.a); }
  colorinline Color4 &operator+=(const Color4 &c)
  {
    r += c.r;
    g += c.g;
    b += c.b;
    a += c.a;
    return *this;
  }
  colorinline Color4 &operator-=(const Color4 &c)
  {
    r -= c.r;
    g -= c.g;
    b -= c.b;
    a -= c.a;
    return *this;
  }
  colorinline Color4 &operator*=(const Color4 &c)
  {
    r *= c.r;
    g *= c.g;
    b *= c.b;
    a *= c.a;
    return *this;
  }
  colorinline Color4 &operator/=(const Color4 &c)
  {
    r /= c.r;
    g /= c.g;
    b /= c.b;
    a /= c.a;
    return *this;
  }
  colorinline Color4 &operator*=(real k)
  {
    r *= k;
    g *= k;
    b *= k;
    a *= k;
    return *this;
  }
  colorinline Color4 &operator/=(real k) { return operator*=(1.0f / k); }

  colorinline bool operator==(const Color4 &c) const { return (r == c.r && g == c.g && b == c.b && a == c.a); }
  colorinline bool operator!=(const Color4 &c) const { return (r != c.r || g != c.g || b != c.b || a != c.a); }

  colorinline void clamp0()
  {
    if (r < 0)
      r = 0;
    if (g < 0)
      g = 0;
    if (b < 0)
      b = 0;
    if (a < 0)
      a = 0;
  }
  colorinline void clamp1()
  {
    if (r > 1)
      r = 1;
    if (g > 1)
      g = 1;
    if (b > 1)
      b = 1;
    if (a > 1)
      a = 1;
  }
  colorinline void clamp01()
  {
    if (r < 0)
      r = 0;
    else if (r > 1)
      r = 1;
    if (g < 0)
      g = 0;
    else if (g > 1)
      g = 1;
    if (b < 0)
      b = 0;
    else if (b > 1)
      b = 1;
    if (a < 0)
      a = 0;
    else if (a > 1)
      a = 1;
  }

  template <class T>
  static Color4 xyzw(const T &a)
  {
    return Color4(a.x, a.y, a.z, a.w);
  }
  template <class T>
  static Color4 xyz0(const T &a)
  {
    return Color4(a.x, a.y, a.z, 0);
  }
  template <class T>
  static Color4 xyz1(const T &a)
  {
    return Color4(a.x, a.y, a.z, 1);
  }
  template <class T>
  static Color4 rgb0(const T &a)
  {
    return Color4(a.r, a.g, a.b, 0);
  }
  template <class T>
  static Color4 rgb1(const T &a)
  {
    return Color4(a.r, a.g, a.b, 1);
  }
  template <class T>
  static Color4 rgbV(const T &a, float v)
  {
    return Color4(a.r, a.g, a.b, v);
  }

  template <class T>
  void set_xyzw(const T &v)
  {
    r = v.x, g = v.y, b = v.z, a = v.w;
  }
  template <class T>
  void set_xyz0(const T &v)
  {
    r = v.x, g = v.y, b = v.z, a = 0;
  }
  template <class T>
  void set_xyz1(const T &v)
  {
    r = v.x, g = v.y, b = v.z, a = 1;
  }
};

colorinline real rgbsum(Color4 c) { return c.r + c.g + c.b; }
colorinline real average(Color4 c) { return rgbsum(c) / 3.0f; }
// NTSC brightness Weights: r=.299 g=.587 b=.114
colorinline real brightness(Color4 c) { return c.r * .299f + c.g * .587f + c.b * .114; }

colorinline real lengthSq(Color4 c) { return (c.r * c.r + c.g * c.g + c.b * c.b); }
colorinline real length(Color4 c) { return sqrtf(lengthSq(c)); }

colorinline Color4 max(const Color4 &a, const Color4 &b) { return Color4(max(a.r, b.r), max(a.g, b.g), max(a.b, b.b), max(a.a, b.a)); }
colorinline Color4 min(const Color4 &a, const Color4 &b) { return Color4(min(a.r, b.r), min(a.g, b.g), min(a.b, b.b), min(a.a, b.a)); }
template <>
colorinline Color4 clamp(Color4 t, const Color4 min_val, const Color4 max_val)
{
  return min(max(t, min_val), max_val);
}

colorinline Color4 operator*(real k, Color4 c) { return c * k; }
colorinline Color4 color4(E3DCOLOR c) { return Color4(c); }

colorinline E3DCOLOR e3dcolor(const Color4 &c)
{
  return E3DCOLOR_MAKE(real2uchar(c.r), real2uchar(c.g), real2uchar(c.b), real2uchar(c.a));
}
colorinline E3DCOLOR e3dcolor_swapped(const Color4 &c)
{
  return E3DCOLOR_MAKE_SWAPPED(real2uchar(c.r), real2uchar(c.g), real2uchar(c.b), real2uchar(c.a));
}

struct Color3
{
  real r, g, b;

  colorinline Color3() = default;
  colorinline Color3(real ar, real ag, real ab)
  {
    r = ar;
    g = ag;
    b = ab;
  }
  colorinline Color3(const real *p)
  {
    r = p[0];
    g = p[1];
    b = p[2];
  }
  colorinline Color3(E3DCOLOR c)
  {
    r = float(c.r) / 255.f;
    g = float(c.g) / 255.f;
    b = float(c.b) / 255.f;
  }

  colorinline void set(real k)
  {
    r = k;
    g = k;
    b = k;
  }
  colorinline void set(real _r, real _g, real _b)
  {
    r = _r;
    g = _g;
    b = _b;
  }
  colorinline void zero() { set(0); }

  colorinline real &operator[](int i) { return (&r)[i]; }
  colorinline const real &operator[](int i) const { return (&r)[i]; }

  colorinline Color3 operator+() const { return *this; }
  colorinline Color3 operator-() const { return Color3(-r, -g, -b); }
  colorinline Color3 operator*(real a) const { return Color3(r * a, g * a, b * a); }
  colorinline Color3 operator/(real a) const { return operator*(1.0f / a); }
  colorinline Color3 operator*(const Color3 &c) const { return Color3(r * c.r, g * c.g, b * c.b); }
  colorinline Color3 operator/(const Color3 &c) const { return Color3(r / c.r, g / c.g, b / c.b); }
  colorinline Color3 operator+(const Color3 &c) const { return Color3(r + c.r, g + c.g, b + c.b); }
  colorinline Color3 operator-(const Color3 &c) const { return Color3(r - c.r, g - c.g, b - c.b); }
  colorinline Color3 &operator+=(const Color3 &c)
  {
    r += c.r;
    g += c.g;
    b += c.b;
    return *this;
  }
  colorinline Color3 &operator-=(const Color3 &c)
  {
    r -= c.r;
    g -= c.g;
    b -= c.b;
    return *this;
  }
  colorinline Color3 &operator*=(const Color3 &c)
  {
    r *= c.r;
    g *= c.g;
    b *= c.b;
    return *this;
  }
  colorinline Color3 &operator/=(const Color3 &c)
  {
    r /= c.r;
    g /= c.g;
    b /= c.b;
    return *this;
  }
  colorinline Color3 &operator*=(real k)
  {
    r *= k;
    g *= k;
    b *= k;
    return *this;
  }
  colorinline Color3 &operator/=(real k) { return operator*=(1.0f / k); }

  colorinline bool operator==(const Color3 &c) const { return (r == c.r && g == c.g && b == c.b); }
  colorinline bool operator!=(const Color3 &c) const { return (r != c.r || g != c.g || b != c.b); }

  colorinline void clamp0()
  {
    if (r < 0)
      r = 0;
    if (g < 0)
      g = 0;
    if (b < 0)
      b = 0;
  }
  colorinline void clamp1()
  {
    if (r > 1)
      r = 1;
    if (g > 1)
      g = 1;
    if (b > 1)
      b = 1;
  }
  colorinline void clamp01()
  {
    if (r < 0)
      r = 0;
    else if (r > 1)
      r = 1;
    if (g < 0)
      g = 0;
    else if (g > 1)
      g = 1;
    if (b < 0)
      b = 0;
    else if (b > 1)
      b = 1;
  }

  template <class T>
  static Color3 xyz(const T &a)
  {
    return Color3(a.x, a.y, a.z);
  }
  template <class T>
  static Color3 rgb(const T &a)
  {
    return Color3(a.r, a.g, a.b);
  }

  template <class T>
  void set_xyz(const T &v)
  {
    r = v.x, g = v.y, b = v.z;
  }
  template <class T>
  void set_rgb(const T &v)
  {
    r = v.r, g = v.g, b = v.b;
  }
};

colorinline real rgbsum(const Color3 &c) { return c.r + c.g + c.b; }
colorinline real average(const Color3 &c) { return rgbsum(c) / 3.0f; }
// NTSC brightness Weights: r=.299 g=.587 b=.114
colorinline real brightness(const Color3 &c) { return c.r * .299f + c.g * .587f + c.b * .114; }

colorinline real lengthSq(const Color3 &c) { return (c.r * c.r + c.g * c.g + c.b * c.b); }
colorinline real length(const Color3 &c) { return sqrtf(lengthSq(c)); }

colorinline Color3 max(const Color3 &a, const Color3 &b) { return Color3(max(a.r, b.r), max(a.g, b.g), max(a.b, b.b)); }
colorinline Color3 min(const Color3 &a, const Color3 &b) { return Color3(min(a.r, b.r), min(a.g, b.g), min(a.b, b.b)); }
template <>
colorinline Color3 clamp(Color3 t, const Color3 min_val, const Color3 max_val)
{
  return min(max(t, min_val), max_val);
}

colorinline Color3 operator*(real a, const Color3 &c) { return c * a; }
colorinline Color3 color3(E3DCOLOR c) { return Color3(c); }

colorinline Color4 color4(const Color3 &c, real a) { return Color4(c.r, c.g, c.b, a); }
colorinline Color3 color3(const Color4 &c) { return Color3(c.r, c.g, c.b); }

colorinline E3DCOLOR e3dcolor(const Color3 &c, unsigned a = 255)
{
  return E3DCOLOR_MAKE(real2uchar(c.r), real2uchar(c.g), real2uchar(c.b), a);
}
colorinline E3DCOLOR e3dcolor_swapped(const Color3 &c, unsigned a = 255)
{
  return E3DCOLOR_MAKE_SWAPPED(real2uchar(c.r), real2uchar(c.g), real2uchar(c.b), a);
}

#undef colorinline
