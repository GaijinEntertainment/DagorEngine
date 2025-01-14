//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <vecmath/dag_vecMath.h>

namespace AnimV20
{
// Key data
struct AnimKeyReal
{
  real p, k1, k2, k3;
};
struct AnimKeyPoint3
{
  vec3f p, k1, k2, k3;
};
struct AnimKeyQuat
{
  quat4f p, b0, b1;
};
} // end of namespace AnimV20


namespace AnimV20Math
{
// Legacy key data

__forceinline real interp_key(const AnimV20::AnimKeyReal &a, real t) { return ((a.k3 * t + a.k2) * t + a.k1) * t + a.p; }

__forceinline vec3f interp_key(const AnimV20::AnimKeyPoint3 &a, vec4f t)
{
  return v_madd(v_madd(v_madd(a.k3, t, a.k2), t, a.k1), t, a.p);
}

__forceinline vec4f interp_key(const AnimV20::AnimKeyQuat &a, const AnimV20::AnimKeyQuat &b, real t)
{
  return v_quat_qsquad(t, a.p, a.b0, a.b1, b.p);
}

} // end of namespace AnimV20Math
