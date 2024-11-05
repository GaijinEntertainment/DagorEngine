// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_colorMatrix.h>


TMatrix make_brightness_color_matrix(const Color3 &scale)
{
  TMatrix r;
  r.zero();
  r[0][0] = scale[0];
  r[1][1] = scale[1];
  r[2][2] = scale[2];
  return r;
}


TMatrix make_contrast_color_matrix(const Color3 &scale, const Color3 &pivot)
{
  TMatrix r;
  r.zero();
  r[0][0] = scale[0];
  r[1][1] = scale[1];
  r[2][2] = scale[2];
  r.setcol(3, pivot[0] * (1 - scale[0]), pivot[1] * (1 - scale[1]), pivot[2] * (1 - scale[2]));
  return r;
}


TMatrix make_saturation_color_matrix(const Color3 &s, const Color3 &gray)
{
  TMatrix r;
  const real rw = 0.299f, gw = 0.587f, bw = 0.114f;

  r.setcol(0, rw * gray.r * (1 - s.r) + s.r, rw * gray.g * (1 - s.g), rw * gray.b * (1 - s.b));
  r.setcol(1, gw * gray.r * (1 - s.r), gw * gray.g * (1 - s.g) + s.g, gw * gray.b * (1 - s.b));
  r.setcol(2, bw * gray.r * (1 - s.r), bw * gray.g * (1 - s.g), bw * gray.b * (1 - s.b) + s.b);

  r.setcol(3, 0, 0, 0);
  return r;
}


TMatrix make_hue_shift_color_matrix(real degrees) { return makeTM(Point3(1, 1, 1), DegToRad(degrees)); }


TMatrix make_blend_to_color_matrix(const Color3 &to_color, real blend_factor)
{
  TMatrix r;
  r.zero();
  r[0][0] = 1 - blend_factor;
  r[1][1] = 1 - blend_factor;
  r[2][2] = 1 - blend_factor;
  r.setcol(3, to_color.r * blend_factor, to_color.g * blend_factor, to_color.b * blend_factor);
  return r;
}
