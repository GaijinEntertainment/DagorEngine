// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>

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

__forceinline IPoint2 scale_by(const IPoint2 &p, float s) { return IPoint2(static_cast<int>(p.x * s), static_cast<int>(p.y * s)); }
__forceinline IPoint3 scale_by(const IPoint3 &p, float s)
{
  return IPoint3(static_cast<int>(p.x * s), static_cast<int>(p.y * s), static_cast<int>(p.z * s));
}

} // namespace dafg
