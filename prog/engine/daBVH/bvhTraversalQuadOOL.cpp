// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <vecmath/dag_vecMath.h>
#include <math/dag_Point2.h>
#include <daBVH/dag_swBLAS_ray.h>

namespace bvh_traverse
{

bool rayBLASQuadOOL(RayData &r, int startOffset, int blasSize)
{
  using B = BLASTraverse<false>;
  return B::rayBLAS(r, startOffset, blasSize);
}

bool rayBLASQuadOOLCullCCW(RayData &r, int startOffset, int blasSize)
{
  using B = BLASTraverse<true>;
  return B::rayBLAS(r, startOffset, blasSize);
}

} // namespace bvh_traverse
