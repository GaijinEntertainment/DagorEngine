//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "dag_color.h"

struct HsvColor
{
public:
  float h; // [0..360]
  float s; // [0..1]
  float v; // [0..1]

  HsvColor() = default;
  HsvColor(float h, float s, float v) : h(h), s(s), v(v) {}
  HsvColor(const Color3 &rgb) { fromRGB(rgb.r, rgb.g, rgb.b); }
  HsvColor(const Color4 &rgba) { fromRGB(rgba.r, rgba.g, rgba.b); }

  Color3 toRGB() const
  {
    int i;
    double f, p, q, t;
    Color3 rgb = Color3(0, 0, 0);

    if (s == 0)
      rgb.r = rgb.g = rgb.b = v;
    else
    {
      float seg = fmodf(h, 360.0) / 60;

      i = seg;
      f = seg - i;
      p = v * (1.0 - s);
      q = v * (1.0 - (s * f));
      t = v * (1.0 - (s * (1.0 - f)));
      G_ASSERT(i >= 0 && i <= 5);
      switch (i)
      {
        case 0:
          rgb.r = v;
          rgb.g = t;
          rgb.b = p;
          break;

        case 1:
          rgb.r = q;
          rgb.g = v;
          rgb.b = p;
          break;

        case 2:
          rgb.r = p;
          rgb.g = v;
          rgb.b = t;
          break;

        case 3:
          rgb.r = p;
          rgb.g = q;
          rgb.b = v;
          break;

        case 4:
          rgb.r = t;
          rgb.g = p;
          rgb.b = v;
          break;

        case 5:
          rgb.r = v;
          rgb.g = p;
          rgb.b = q;
          break;
      }
    }

    return rgb;
  }

private:
  void fromRGB(float r, float g, float b)
  {
    double imax = max(r, max(g, b));
    double imin = min(r, min(g, b));
    double rc, gc, bc;

    h = s = 0;

    v = imax;
    if (imax != 0)
      s = (imax - imin) / imax;
    else
      s = 0;

    if (s == 0)
      h = 0;
    else
    {
      rc = (imax - r) / (imax - imin);
      gc = (imax - g) / (imax - imin);
      bc = (imax - b) / (imax - imin);
      if (r == imax)
        h = bc - gc;
      else if (g == imax)
        h = 2.0 + rc - bc;
      else
        h = 4.0 + gc - rc;
      h *= 60.0;
      if (h < 0.0)
        h += 360.0;
    }
  }
};

struct HsvaColor
{
public:
  float h; // [0..360]
  float s; // [0..1]
  float v; // [0..1]
  float a; // [0..1]

  HsvaColor() = default;
  HsvaColor(float h, float s, float v, float a) : h(h), s(s), v(v), a(a) {}
  HsvaColor(const HsvColor &hsv, float a = 1) : h(hsv.h), s(hsv.s), v(hsv.v), a(a) {}
  HsvaColor(const Color3 &rgb, float a = 1) : a(a)
  {
    HsvColor hsv(rgb);
    h = hsv.h;
    s = hsv.s;
    v = hsv.v;
  }
  HsvaColor(const Color4 &rgba)
  {
    HsvColor hsv(rgba);
    h = hsv.h;
    s = hsv.s;
    v = hsv.v;
    a = rgba.a;
  }

  Color4 toRGBA() const { return color4(HsvColor(h, s, v).toRGB(), a); }
};
