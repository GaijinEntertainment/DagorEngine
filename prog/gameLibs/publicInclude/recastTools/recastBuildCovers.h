//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <covers/covers.h>

#include "recastBuildEdges.h"

namespace recastbuild
{
enum
{
  COVERS_TYPEGEN_BASIC,
  COVERS_TYPEGEN_ALT1,
};

struct CoversParams
{
  bool enabled;
  int typeGen;

  float maxHeight;
  float minHeight;
  float minWidth;
  float minDepth;
  float agentMaxClimb;
  float shootWindowHeight;
  float minCollTransparent;
  float deltaHeightThreshold;
  float mergeDist;
  float shootDepth;
  float shootHeight;

  BBox3 maxVisibleBox;
  float openingTreshold;
  float boxOffset;

  Point3 testPos;
};

void build_covers(Tab<covers::Cover> &out_covers, const BBox3 &tile_box, const Tab<Edge> &edges, const CoversParams &covParams,
  const rcCompactHeightfield *place_chf, const rcHeightfield *place_solid, const rcCompactHeightfield *trace_chf,
  const rcHeightfield *trace_solid, RenderEdges *out_render_edges);

void fit_intersecting_covers(Tab<covers::Cover> &covers, int num_fixed, const BBox2 &nav_area, const CoversParams &covParams);

} // namespace recastbuild
