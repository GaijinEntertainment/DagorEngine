// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stdint.h>

namespace darg
{

// Always-on counters of the scene update/rebuild pipeline for performance
// baselining. Monotonic since creation or last reset(); consumers snapshot
// and diff. Accessed only from serialized scene API calls (ApiThreadCheck).
struct PerfStats
{
  uint32_t updates = 0;            // GuiScene::update() calls
  uint32_t rebuildBatches = 0;     // rebuildInvalidatedParts() calls that had work
  uint32_t invalidations = 0;      // invalidated elements processed
  uint32_t builderEvals = 0;       // component builder closure evaluations
  uint32_t statefulCtorRuns = 0;   // StatefulComp ctor runs, one per instance mount
  uint32_t elemsSetupInitial = 0;  // Element::setup(SM_INITIAL)
  uint32_t elemsSetupRebuild = 0;  // Element::setup(SM_REBUILD_UPDATE)
  uint32_t elemsSetupRealtime = 0; // Element::setup(SM_REALTIME_UPDATE)
  uint32_t elemsCreated = 0;
  uint32_t elemsFreed = 0;
  uint32_t layoutFixedSizeRoots = 0;
  uint32_t layoutSizeRoots = 0;
  uint32_t layoutFlowRoots = 0;
  uint32_t stacksRebuilds = 0; // Screen::rebuildStacks() calls

  void reset() { *this = PerfStats(); }
};

} // namespace darg
