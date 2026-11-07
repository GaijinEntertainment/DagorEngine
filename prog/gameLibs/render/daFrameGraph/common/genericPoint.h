// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <math.h>

namespace dafg
{

__forceinline bool all_greater(const IPoint2 &a, const IPoint2 &b) { return a.x > b.x && a.y > b.y; }
__forceinline bool all_greater(const IPoint2 &a, int b) { return a.x > b && a.y > b; }
__forceinline bool all_greater(const IPoint3 &a, const IPoint3 &b) { return a.x > b.x && a.y > b.y && a.z > b.z; }
__forceinline bool all_greater(const IPoint3 &a, int b) { return a.x > b && a.y > b && a.z > b; }

__forceinline bool any_greater(const IPoint2 &a, const IPoint2 &b) { return a.x > b.x || a.y > b.y; }
__forceinline bool any_greater(const IPoint2 &a, int b) { return a.x > b || a.y > b; }
__forceinline bool any_greater(const IPoint3 &a, const IPoint3 &b) { return a.x > b.x || a.y > b.y || a.z > b.z; }
__forceinline bool any_greater(const IPoint3 &a, int b) { return a.x > b || a.y > b || a.z > b; }

// The epsilon keeps an exact multiple from rounding up through the error of the float product
__forceinline int scale_axis(int v, float s, bool round_up)
{
  const float scaled = v * s;
  return static_cast<int>(round_up ? ceilf(scaled - 1e-3f) : scaled);
}
__forceinline IPoint2 scale_by(const IPoint2 &p, float s, bool round_up = false)
{
  return IPoint2(scale_axis(p.x, s, round_up), scale_axis(p.y, s, round_up));
}
__forceinline IPoint3 scale_by(const IPoint3 &p, float s, bool round_up = false)
{
  return IPoint3(scale_axis(p.x, s, round_up), scale_axis(p.y, s, round_up), scale_axis(p.z, s, round_up));
}

} // namespace dafg
