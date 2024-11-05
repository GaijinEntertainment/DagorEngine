//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <vecmath/dag_vecMathDecl.h>

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
