#include "color.h"


namespace darg
{

ColorHsva rgb2hsv(const Color4 &in)
{
  ColorHsva out;
  out.a = in.a;

  float min = in.r < in.g ? in.r : in.g;
  min = min < in.b ? min : in.b;

  float max = in.r > in.g ? in.r : in.g;
  max = max > in.b ? max : in.b;

  out.v = max;
  float delta = max - min;
  if (delta < 1e-6f)
  {
    out.s = 0;
    out.h = 0;
    return out;
  }

  if (max > 1e-5f)
  {
    out.s = (delta / max);
  }
  else
  {
    // if max is 0, then r = g = b = 0
    // s = 0, v is undefined
    out.s = 0;
    out.h = 0; // undefined
    return out;
  }

  if (in.r >= max)
    out.h = (in.g - in.b) / delta; // between yellow & magenta
  else if (in.g >= max)
    out.h = 2.0 + (in.b - in.r) / delta; // between cyan & yellow
  else
    out.h = 4.0 + (in.r - in.g) / delta; // between magenta & cyan

  out.h *= 60.0f; // degrees

  if (out.h < 0.0)
    out.h += 360.0f;

  return out;
}


Color4 hsv2rgb(const ColorHsva &in)
{
  Color4 out;

  out.a = in.a;

  if (in.s < 1e-5f)
  {
    out.r = in.v;
    out.g = in.v;
    out.b = in.v;
    return out;
  }

  float hh = in.h;
  if (hh >= 360.0f)
    hh = 0.0f;
  hh /= 60.0f;
  int i = (int)hh;
  float ff = hh - i;
  float p = in.v * (1.0f - in.s);
  float q = in.v * (1.0f - (in.s * ff));
  float t = in.v * (1.0f - (in.s * (1.0f - ff)));

  switch (i)
  {
    case 0:
      out.r = in.v;
      out.g = t;
      out.b = p;
      break;
    case 1:
      out.r = q;
      out.g = in.v;
      out.b = p;
      break;
    case 2:
      out.r = p;
      out.g = in.v;
      out.b = t;
      break;
    case 3:
      out.r = p;
      out.g = q;
      out.b = in.v;
      break;
    case 4:
      out.r = t;
      out.g = p;
      out.b = in.v;
      break;
    case 5:
    default:
      out.r = in.v;
      out.g = p;
      out.b = q;
      break;
  }
  return out;
}

} // namespace darg
