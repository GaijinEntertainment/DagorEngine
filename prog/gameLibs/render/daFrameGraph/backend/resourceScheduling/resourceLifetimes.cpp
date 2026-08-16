// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "resourceLifetimes.h"

#include <EASTL/algorithm.h>
#include <drv/3d/dag_driverDesc.h>
#include <perfMon/dag_statDrv.h>
#include <memory/dag_framemem.h>


namespace dafg
{

auto ResourceLifetimeCalculator::recalculate(const intermediate::Graph &graph, const PassColoring &pass_coloring) -> LifetimesChanged
{
  TIME_PROFILE(recalculateResourceLifetimes);

  const size_t oldSize = resourceLifetimes[0].totalKeys();
  const size_t newSize = graph.resources.totalKeys();

  LifetimesChanged changed;
  changed.resize(eastl::max(oldSize, newSize), false);
  for (size_t i = newSize; i < oldSize; ++i)
    changed[static_cast<intermediate::ResourceIndex>(i)] = true;

  FRAMEMEM_VALIDATE;

  const bool widenUntrackedLifetimes = d3d::get_driver_desc().caps.hasTileBasedArchitecture;
  const auto gracePoints = widenUntrackedLifetimes ? compute_grace_points(graph, pass_coloring) : GracePoints{};

  struct UsageSpan
  {
    uint32_t firstNode;
    uint32_t lastNode;
    bool used;
  };
  static constexpr UsageSpan UNUSED{0, 0, false};

  IdIndexedMapping<intermediate::ResourceIndex, UsageSpan, framemem_allocator> currentFrameSpans(newSize, UNUSED);
  IdIndexedMapping<intermediate::ResourceIndex, UsageSpan, framemem_allocator> historySpans(newSize, UNUSED);

  for (auto [nodeIdx, node] : graph.nodes.enumerate())
    for (const auto &req : node.resourceRequests)
    {
      auto &span = req.fromLastFrame ? historySpans[req.resource] : currentFrameSpans[req.resource];
      const auto timepoint = static_cast<uint32_t>(eastl::to_underlying(nodeIdx));
      span = span.used ? UsageSpan{span.firstNode, timepoint, true} : UsageSpan{timepoint, timepoint, true};
    }

  for (auto &frameLifetimes : resourceLifetimes)
    frameLifetimes.resize(newSize);

  for (uint32_t i = 0; i < newSize; ++i)
  {
    const auto resIdx = static_cast<intermediate::ResourceIndex>(i);

    const bool scheduled = graph.resources.isMapped(resIdx) && graph.resources[resIdx].isScheduled();
    const bool widenToPassBoundary = widenUntrackedLifetimes && scheduled && graph.resources[resIdx].isUntracked();
    const auto &currentFrameSpan = currentFrameSpans[resIdx];
    const auto &historySpan = historySpans[resIdx];

    // Never-used resources should be impossible due to invariants
    G_ASSERT(!scheduled || currentFrameSpan.used || historySpan.used);

    for (uint32_t frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
    {
      ResourceLifetime lifetime{};
      if (scheduled && (currentFrameSpan.used || historySpan.used))
      {
        const uint32_t historyFrame = (frame + 1) % SCHEDULE_FRAME_WINDOW;

        lifetime.firstUse = currentFrameSpan.used ? LifetimePoint{frame, currentFrameSpan.firstNode}
                                                  : LifetimePoint{historyFrame, historySpan.firstNode};

        const auto lastUse =
          historySpan.used ? LifetimePoint{historyFrame, historySpan.lastNode} : LifetimePoint{frame, currentFrameSpan.lastNode};

        const auto releaseNodeIdx = graph.nodes.getNextUsed(static_cast<intermediate::NodeIndex>(lastUse.timepoint));
        G_ASSERTF(graph.nodes.isMapped(releaseNodeIdx), "daFG: %d is the last node! This is impossible by design", lastUse.timepoint);
        lifetime.release = {lastUse.frame, static_cast<uint32_t>(eastl::to_underlying(releaseNodeIdx))};

        if (widenToPassBoundary)
        {
          lifetime.firstUse.timepoint = *(gracePoints.upper_bound(lifetime.firstUse.timepoint) - 1);
          lifetime.release.timepoint = *gracePoints.lower_bound(lifetime.release.timepoint);
        }
      }

      if (i >= oldSize || resourceLifetimes[frame][resIdx] != lifetime)
        changed[resIdx] = true;
      resourceLifetimes[frame][resIdx] = lifetime;
    }
  }

  return changed;
}

} // namespace dafg
