// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <vecmath/dag_vecMath.h>
#include <math/dag_Point2.h>

namespace smooth_noise
{
// CPU smooth-noise for the daEditor landclass bake. Diverges from the runtime
// gameLibs/heightmap copy: this one uses PCG2D directly on integer cell coords so
// the output has no period (the LUT version repeats every 64 cells, which was
// visible at landscape scale). Runtime detail-noise still uses the LUT path so
// game-side visuals are unchanged.

alignas(16) static const vec4f_const V_C_6_0 = DECL_VECFLOAT4(6.0f, 6.0f, 0, 0);
alignas(16) static const vec4f_const V_C_m15_30 = DECL_VECFLOAT4(-15.0f, -15.0f, 30.f, 30.f);
alignas(16) static const vec4f_const V_C_10_m60 = DECL_VECFLOAT4(10.0f, 10.0f, -60.f, -60.f);
alignas(16) static const vec4f_const V_C_0_30 = DECL_VECFLOAT4(0.0f, 0.0f, 30.f, 30.f);

static inline vec4f v_frac(vec4f p) { return v_sub(p, sse4_floor(p)); }

// PCG2D (Mark Jarzynski hash), 4 (x,y) pairs in parallel. Mirrors the shader-side
// uvec2 pcg2d(uvec2) so a future GPU port stays bit-identical for matching coords.
__forceinline vec4i pcg2d_x4(vec4i x, vec4i y)
{
  const vec4i C = v_splatsi(1664525);
  const vec4i K = v_splatsi(1013904223);
  x = v_addi(v_muli(x, C), K);
  y = v_addi(v_muli(y, C), K);
  x = v_addi(x, v_muli(y, C));
  y = v_addi(y, v_muli(x, C));
  x = v_xori(x, v_srli(x, 16));
  y = v_xori(y, v_srli(y, 16));
  x = v_addi(x, v_muli(y, C));
  y = v_addi(y, v_muli(x, C));
  x = v_xori(x, v_srli(x, 16));
  return x;
}

// Hashes the 4 corners of the integer cell containing `uv_wrap` and returns them
// as a vec4f in [0, 255] -- byte-range so the downstream `1.f / 255` scaling in
// base_noise() / base_noise_value_2d() stays unchanged. uv_frac out-param is the
// fractional part of `uv_wrap` (full 4 lanes; only x/y are meaningful for the
// callers below).
__forceinline vec4f gather_red(vec4f uv_wrap, vec4f &uv_frac)
{
  vec4i uvIdx = v_cvt_floori(uv_wrap);
  uv_frac = v_sub(uv_wrap, v_cvt_vec4f(uvIdx));

  // Pack [xLo, yLo, xHi, yHi] then permute into the 4-corner (x, y) vectors.
  vec4i corners = v_permi_xyab(uvIdx, v_addi(uvIdx, V_CI_1));
  vec4i x = v_permi_xzxz(corners); // [xLo, xHi, xLo, xHi]
  vec4i y = v_permi_yyww(corners); // [yLo, yLo, yHi, yHi]

  vec4i hash = v_andi(pcg2d_x4(x, y), v_splatsi(0xff));
  return v_cvt_vec4f(hash);
}

__forceinline vec4f gather_red(const DPoint2 &uv_wrap, vec4f &uv_frac)
{
  DPoint2 uvIdxF(floor(uv_wrap.x), floor(uv_wrap.y));
  uv_frac = v_make_vec4f(uv_wrap.x - uvIdxF.x, uv_wrap.y - uvIdxF.y, 0, 0);
  vec4i uvIdx = v_make_vec4i((int)uvIdxF.x, (int)uvIdxF.y, 0, 0);

  vec4i corners = v_permi_xyab(uvIdx, v_addi(uvIdx, V_CI_1));
  vec4i x = v_permi_xzxz(corners);
  vec4i y = v_permi_yyww(corners);

  vec4i hash = v_andi(pcg2d_x4(x, y), v_splatsi(0xff));
  return v_cvt_vec4f(hash);
}

inline vec4f base_noise(vec4f hash, vec4f Pf) // with gradients
{
  hash = v_mul(hash, v_splats(1.f / 255));

  Pf = v_perm_xyxy(Pf);
  vec4f blend = v_mul(Pf, v_mul(Pf, v_madd(Pf, v_madd(Pf, v_madd(Pf, V_C_6_0, V_C_m15_30), V_C_10_m60), V_C_0_30)));
  vec4f res0 =
    v_lerp_vec4f(v_rot_2(v_perm_xxyy(blend)), v_perm_xyab(hash, v_perm_xzxz(hash)), v_perm_xyab(v_perm_zwxy(hash), v_perm_ywyw(hash)));
  return v_add(v_and(res0, v_cast_vec4f(V_CI_MASK1000)),
    v_mul(v_sub(v_perm_yyww(res0), v_perm_xxzz(res0)), v_perm_xyab(v_perm_xzxz(blend), v_splat_w(blend))));
}

inline vec4f noise(const DPoint2 &p) // with gradients
{
  vec4f Pf;
  vec4f hash = gather_red(p, Pf);
  return base_noise(hash, Pf);
}

inline vec4f noise(vec4f p) // with gradients
{
  vec4f Pf;
  vec4f hash = gather_red(p, Pf);
  return base_noise(hash, Pf);
}

inline float base_noise_value_2d(vec4f hash, vec4f Pf)
{
  hash = v_mul(hash, v_splats(1.f / 255));
  vec4f blend = v_mul(Pf, v_mul(Pf, v_mul(Pf, v_madd(Pf, v_madd(Pf, v_splats(6.0f), v_splats(-15.0f)), v_splats(10.f)))));
  vec4f blend2 = v_perm_xyab(blend, v_sub(V_C_ONE, blend));
  return v_extract_x(v_dot4_x(hash, v_mul(v_perm_zxzx(blend2), v_perm_wwyy(blend2))));
}
inline float noise_value_2d(const vec4f p)
{
  vec4f Pf;
  vec4f hash = gather_red(p, Pf);
  return base_noise_value_2d(hash, Pf);
}

inline float noise_value_2d(const DPoint2 &p)
{
  vec4f Pf;
  vec4f hash = gather_red(p, Pf);
  return base_noise_value_2d(hash, Pf);
}

void init();
void close();

} // namespace smooth_noise
