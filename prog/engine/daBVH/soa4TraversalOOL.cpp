// Copyright (C) Gaijin Games KFT.  All rights reserved.

// Out-of-line SoA4 BLAS traversal entries: callers that only need the default hit callback link
// these instead of instantiating the templated walkers in their own TU (i-cache pressure, same
// policy as bvhTraversalQuadOOL.cpp for the stackless walkers).

#include <vecmath/dag_vecMath.h>
#include <daBVH/dag_swBLAS_soa4.h>

namespace soa4
{

bool rayClosestOOL(RayData &r, RootRef root) { return rayClosest<false>(r, root); }
bool rayClosestOOLCullCCW(RayData &r, RootRef root) { return rayClosest<true>(r, root); }
bool rayAnyHitOOL(RayData &r, RootRef root) { return rayAnyHit<false>(r, root); }
bool rayAnyHitOOLCullCCW(RayData &r, RootRef root) { return rayAnyHit<true>(r, root); }

} // namespace soa4
