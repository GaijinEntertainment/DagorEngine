// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/dafgVisualizerStructuresCommon.h>

#include <frontend/internalRegistry.h>

#include <EASTL/fixed_vector.h>
#include <dag/dag_vectorMap.h>
#include <drv/3d/dag_consts.h>


namespace dafg::visualization
{
// structures from old visualizator
struct ResourceInfo
{
  eastl::string name;

  int firstUsageNodeIdx = eastl::numeric_limits<int>::max();
  int lastUsageNodeIdx = eastl::numeric_limits<int>::min();

  int heapIndex = -1;
  int offset = 0;
  int size = 0;

  bool createdByRename = false;
  bool consumedByRename = false;

  // Node execution index -> usage
  dag::VectorMap<int, ResourceUsage> usageTimeline;

  eastl::fixed_vector<eastl::pair<int, ResourceBarrier>, 32> barrierEvents;
};

struct ResourcePlacementEntry
{
  ResNameId id;
  int frame;
  int heap;
  int offset;
  int size;
};

struct ResourceBarrierEntry
{
  ResNameId res_id;
  int res_frame;
  int exec_time;
  int exec_frame;
  ResourceBarrier barrier;
};

} // namespace dafg::visualization