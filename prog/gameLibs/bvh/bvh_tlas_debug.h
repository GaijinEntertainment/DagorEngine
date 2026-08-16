// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <bvh/bvh.h>
#include <math/dag_Point3.h>

namespace bvh::debug
{

// The instance counts build() put into the TLAS upload buffers this frame. The debug tools derive the
// buffer layout from them: a region's start is the sum of the counts before it, in the order the copy
// happens. They cannot be queried back from the producers, because build() drops some of them
// depending on the bvh_*_enable debug flags.
struct TlasSizes
{
  // Total the main TLAS build consumed, only used to catch the derived layout drifting from build().
  uint32_t mainInstanceCount = 0;

  uint32_t impostorCount = 0;
  uint32_t riExtraCount = 0;
  uint32_t cpuCount = 0;
  uint32_t grassCount = 0;
  uint32_t gpuObjectCount = 0;
  uint32_t dagdpCount = 0;
  uint32_t gpuGrassCount = 0;

  uint32_t terrainCount = 0;

  uint32_t fxCount = 0;
  uint32_t smokeTracerCount = 0;

  // The GPU fed regions are consumed at full capacity, so everything past the producer's live count,
  // which sits at dword 0 of these, has to be zeroed. Same counters bvh_hwinstance_copy reads.
  Sbuffer *grassCounter = nullptr;
  Sbuffer *gpuObjectCounter = nullptr;
  Sbuffer *dagdpCounter = nullptr;
  Sbuffer *gpuGrassCounter = nullptr;
  Sbuffer *fxCounter = nullptr;
  Sbuffer *smokeTracerCounter = nullptr;
};

void validate_tlas_instances(ContextId context_id, const TlasSizes &sizes);
void probe_tlas(ContextId context_id, const TlasSizes &sizes, const Point3 &camera_pos);
void tlas_debug_teardown();

void draw_tlas_debug_imgui();

} // namespace bvh::debug
