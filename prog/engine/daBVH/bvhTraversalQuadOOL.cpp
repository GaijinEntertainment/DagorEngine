// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <vecmath/dag_vecMath.h>
#include <math/dag_Point2.h>
#include <daBVH/dag_swBLAS_ray.h>

namespace bvh_traverse
{

bool rayBLASQuadOOL(RayData &r, int startOffset, int blasSize) { return rayBLAS_Free<false>(r, startOffset, blasSize); }

bool rayBLASQuadOOLCullCCW(RayData &r, int startOffset, int blasSize) { return rayBLAS_Free<true>(r, startOffset, blasSize); }

} // namespace bvh_traverse
