#pragma once


#include <math/dag_color.h>


namespace darg
{

class ColorHsva
{
public:
  float h, s, v, a;

  ColorHsva() : h(0), s(0), v(0), a(0) {}

  ColorHsva(float h_, float s_, float v_, float a_ = 0) : h(h_), s(s_), v(v_), a(a_) {}

  ColorHsva operator*(float k) const { return ColorHsva(h * k, s * k, v * k, a * k); }

  ColorHsva operator+(const ColorHsva &c) const { return ColorHsva(h + c.h, s + c.s, v + c.v, a + c.a); }
};


ColorHsva rgb2hsv(const Color4 &in);
Color4 hsv2rgb(const ColorHsva &in);


} // namespace darg
