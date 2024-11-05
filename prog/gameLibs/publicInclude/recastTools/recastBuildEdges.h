//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <recast.h>

#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <generic/dag_tab.h>

namespace recastbuild
{
struct Edge
{
  Point3 sp, sq;
};

struct MergeEdgeParams
{
  bool enabled;

  float maxExtrudeErrorSq;
  float extrudeLimitSq;

  Point2 walkPrecision;
  float safeCutLimitSq;
  float unsafeCutLimitSq;
  float unsafeMaxCutSpace;
};

typedef Tab<eastl::pair<Point3, Point3>> RenderEdges;

void build_edges(Tab<Edge> &out_edges, const rcContourSet *cset, const rcCompactHeightfield *chf, const MergeEdgeParams &mergeParams,
  RenderEdges *out_render_edges);
} // namespace recastbuild
