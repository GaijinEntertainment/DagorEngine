//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace darg
{

inline E3DCOLOR color_apply_opacity(E3DCOLOR c, float o) { return E3DCOLOR(c.r * o, c.g * o, c.b * o, c.a * o); }

inline float color_comp_mul(unsigned char colorPart, float brightness)
{
  float res = colorPart * brightness;
  if (res > 255)
    res = 255;
  return res;
}

inline E3DCOLOR color_apply_mods(E3DCOLOR c, float opacity, float brightness)
{
  float o = clamp(opacity, 0.0f, 1.0f);
  float b = max(brightness, 0.0f);
  return E3DCOLOR(color_comp_mul(c.r, b) * o, color_comp_mul(c.g, b) * o, color_comp_mul(c.b, b) * o, c.a * o);
}

E3DCOLOR script_decode_e3dcolor(SQInteger c);

} // namespace darg
