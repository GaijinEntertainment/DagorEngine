//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "atmosphere.h"
#include <vecmath/dag_vecMath.h>

namespace gamephys
{
namespace atmosphere
{

inline const vec4f V_DENSITY_COEFFS[5] = {v_splats(DENSITY_COEFFS[0]), v_splats(DENSITY_COEFFS[1]), v_splats(DENSITY_COEFFS[2]),
  v_splats(DENSITY_COEFFS[3]), v_splats(DENSITY_COEFFS[4])};
inline const vec4f V_TEMPERATURE_COEFFS[5] = {v_splats(TEMPERATURE_COEFFS[0]), v_splats(TEMPERATURE_COEFFS[1]),
  v_splats(TEMPERATURE_COEFFS[2]), v_splats(TEMPERATURE_COEFFS[3]), v_splats(TEMPERATURE_COEFFS[4])};

inline const vec4f V_SONIC_SPEED_COEFF = v_splats(20.1f);

VECTORCALL VECMATH_FINLINE vec4f v_poly5(vec4f x, const vec4f (&cs)[5])
{
  return v_madd(v_madd(v_madd(v_madd(cs[4], x, cs[3]), x, cs[2]), x, cs[1]), x, cs[0]);
}

VECTORCALL VECMATH_FINLINE vec4f v_density(vec4f h, vec4f h_max, vec4f h_max_by_rho0)
{
  vec4f lo = v_min(h, h_max);
  vec4f hi = v_max(h, h_max);
  return v_div(v_mul(v_poly5(lo, V_DENSITY_COEFFS), h_max_by_rho0), hi);
}

VECTORCALL VECMATH_FINLINE vec4f v_sonic_speed(vec4f h, vec4f h_max, vec4f t0)
{
  vec4f lo = v_min(h, h_max);
  return v_mul(v_sqrt(v_mul(v_poly5(lo, V_TEMPERATURE_COEFFS), t0)), V_SONIC_SPEED_COEFF);
}

} // namespace atmosphere
} // namespace gamephys
