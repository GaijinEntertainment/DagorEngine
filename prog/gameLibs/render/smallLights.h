// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <vecmath/dag_vecMathDecl.h>
#include <vecmath/dag_vecMath_common.h>

static inline bool is_viewed_small(const vec4f posRadius, const vec4f distance_2, float markSmallLightsAsFarLimit)
{
  vec4f view_2 = v_splat_w(posRadius);
  view_2 = v_div_x(v_mul_x(view_2, view_2), distance_2); // good approximation for small values
  return v_extract_x(view_2) < markSmallLightsAsFarLimit * markSmallLightsAsFarLimit;
}
