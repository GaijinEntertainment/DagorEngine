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
    r->collapseAndOptimize(resolve_name(), USE_GRID_FOR_RI);
  else
    logerr("collRes (%p, %s) expected to be optimized", r, resolve_name());
}
