// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/metronome.h>
#include <util/dag_generationReferencedData.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>

namespace dafg::metronome::detail
{

struct StoredNode
{
  NameSpace ns;
  eastl::string name;
  eastl::string sourceLocation;
  NodeActivator activator;
};

struct SubgraphState
{
  eastl::string name;
  uint32_t maxDelayFrames = 0;
  uint32_t scheduleGeneration = 0;
  uint32_t pendingSinceTick = 0;
  UpdateStatus state = UpdateStatus::NotScheduled;
  eastl::vector<StoredNode> nodes;
  eastl::vector<NodeHandle> liveHandles;
};

struct Scheduler
{
  GenerationReferencedData<SubgraphId, SubgraphState> subgraphs;
  uint32_t tick = 0;
};

} // namespace dafg::metronome::detail
