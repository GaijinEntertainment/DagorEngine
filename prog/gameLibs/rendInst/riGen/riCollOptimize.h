// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gameRes/dag_collisionResource.h>

namespace rendinst
{
extern bool allowOptimizeCollResOnLoad;
} // namespace rendinst

template <class CB>
void optimize_collres_on_load(CollisionResource *r, CB resolve_name)
{
  if (r->collisionFlags & COLLISION_RES_FLAG_OPTIMIZED)
    return;

  if (rendinst::allowOptimizeCollResOnLoad)
  {
    r->collapseAndOptimize(resolve_name(), USE_GRID_FOR_RI);
    // Drop ownVertices for BLAS-resident nodes. collapseAndOptimize intentionally does NOT compact
    // (the exporter reads ownVertices back after collapse to write the asset binary), so do it here
    // at the rendinst runtime-optimisation entry point. After this, ownVertices is empty for any
    // node the BLAS absorbed; per-node fallbacks (capsule trace, intersection tests) decode from the
    // grid's vert21 array via iterateNodeVerts / resolveNodeVertsForCall instead.
    r->compactOwnVertices();
  }
  else
    logerr("collRes (%p, %s) expected to be optimized", r, resolve_name());
}
