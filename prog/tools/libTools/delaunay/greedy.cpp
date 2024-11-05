// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "terra.h"
#include <debug/dag_debug.h>

namespace delaunay
{

inline int goal_not_met() { return mesh->maxError() > error_threshold && mesh->pointCount() < point_limit; }

static void announce_goal()
{
  debug("Goal conditions met: error=%g (thresh = %g), points=%d", mesh->maxError(), error_threshold, mesh->pointCount());
}

void greedy_insertion()
{

  while (goal_not_met())
  {
    if (!mesh->greedyInsert())
      break;
  }

  announce_goal();
}

}; // namespace delaunay