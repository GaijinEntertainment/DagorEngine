// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <vecmath/dag_vecMath.h>

namespace bvh::ri
{

inline vec4f VECTORCALL v_frac(vec4f value) { return v_sub(value, v_floor(value)); }

inline vec4f VECTORCALL random_float_from_pos(vec4f center_world_pos, uint32_t seed)
{
  if (seed)
  {
    float rndFloat = float(seed & 0x7F) / 127.0f;
    return v_make_vec4f(rndFloat, rndFloat, rndFloat, rndFloat);
  }
  static const auto rnd1 = v_make_vec4f(1, 1.618, 2.7183, 3.1415);
  static const auto rnd2 = v_make_vec4f(2.7813, 3.1415, 1.1, 1.618);

  auto colorPos = v_perm_xzxz(v_floor(v_mul(center_world_pos, v_splats(0.2))));
  auto rnd = v_add(v_mul(colorPos, rnd1), v_mul(colorPos, rnd2));
  vec4f randomVal = v_dot4(rnd, v_splats(1.0));
  return v_frac(randomVal);
}

inline vec4f VECTORCALL random_float_from_pos_for_palette(vec4f center_world_pos, bool height_locked)
{ // see painting_color_uv in paint_details_inc.dshl
  static const auto rnd1 = v_make_vec4f(1, 1.618, 2.7183, 3.1415);
  static const auto rnd2 = v_make_vec4f(2.7813, 3.1415, 1.1, 1.618);

  auto colorPos = v_perm_xzxz(v_floor(center_world_pos));
  auto rnd = v_add(v_mul(colorPos, rnd1), v_mul(colorPos, rnd2));
  vec4f randomVal = v_dot4(rnd, v_splats(1.0));
  if (!height_locked)
    randomVal = v_add(randomVal, v_mul(v_splat_y(center_world_pos), v_splats(100.0f)));
  // randomColorU = floor(frac(randomColorU) * 8192.) / 8192. + 0.5 / 8192.;
  randomVal = v_add(v_mul(v_floor(v_mul(v_frac(randomVal), v_splats(8192.0f))), v_splats(1.0f / 8192.0f)), v_splats(0.5f / 8192.0f));
  return randomVal;
}

inline vec4f VECTORCALL color_to_vector(E3DCOLOR c)
{
  return v_div(v_cvtu_vec4f(v_make_vec4i(c.b, c.g, c.r, c.a)), v_splats(float(UCHAR_MAX)));
}

inline E3DCOLOR VECTORCALL vector_to_color(vec4f v)
{
  uint32_t ic[4];
  v_stui(ic, v_cvti_vec4i(v_mul(v, v_splats(float(UCHAR_MAX)))));
  return E3DCOLOR(ic[2], ic[1], ic[0], ic[3]);
}

inline E3DCOLOR VECTORCALL random_color_from_pos(vec4f center_world_pos, uint32_t seed, E3DCOLOR color_from, E3DCOLOR color_to)
{
  auto lerp_koef = random_float_from_pos(center_world_pos, seed);
  auto color_from_v = color_to_vector(color_from);
  auto color_to_v = color_to_vector(color_to);
  auto color = v_lerp_vec4f(lerp_koef, color_from_v, color_to_v);
  color = v_mul(color, v_splat_w(color));
  // These two lines can move the color outside of the 0-1 range, so it is done in the shader
  // color = v_mul(color, v_splats(4.0));
  // color = v_pow(color, v_splats(2.2));
  return vector_to_color(color);
}

} // namespace bvh::ri