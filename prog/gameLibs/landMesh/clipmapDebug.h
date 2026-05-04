// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/array.h>
#include <EASTL/functional.h>
#include <math/dag_Point2.h>
#include <math/integer/dag_IPoint2.h>

struct ClipmapDebugStats
{
  eastl::array<int, TEX_MIPS> tilesFromFeedback;
  eastl::array<int, TEX_MIPS> tilesNeedUpdate;
  eastl::array<int, TEX_MIPS> tilesInvalid;
  eastl::array<int, TEX_MIPS> tilesUsed;
  eastl::array<int, TEX_MIPS> tilesUnused;
  int tilesUnassigned = 0;
  int texTileSize = 0;
  int texTileInnerSize = 0;
  float texelSize = 1.f;
  int texMips = TEX_MIPS;
  IPoint2 cacheDim = IPoint2{0, 0};
  int tilesCount = 0;
  int tileZoom = 0;
  ClipmapDebugStats() { reset(); }
  void reset()
  {
    for (int i = 0; i < TEX_MIPS; ++i)
      tilesFromFeedback[i] = tilesNeedUpdate[i] = tilesInvalid[i] = tilesUsed[i] = tilesUnused[i] = 0;
    tilesUnassigned = 0;
  }
};

extern ClipmapDebugStats clipmapDebugStats;

bool clipmap_should_collect_debug_stats();

using InvalidatePointFn = eastl::function<void(const Point2 &world_xz)>;
void clipmap_debug_before_prepare_feedback(const InvalidatePointFn &invalidatePointFn);