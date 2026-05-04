// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "barrierScheduler.h"

#include <common/resourceUsage.h>
#include <debug/backendDebug.h>

#include <perfMon/dag_statDrv.h>
#include <memory/dag_framemem.h>
#include <dag/dag_vectorSet.h>
#include <generic/dag_reverseView.h>


namespace dafg
{

extern ConVarT<bool, false> verbose;

// TODO: in the future, we should replace activation/deactivation actions
// with aliasing barriers, as they match what is happening in HW more
// closely. This will force us to remove resource (de)activation events
// and calculate resource lifetimes separately from barrier scheduling,
// which is good and supposed to be separate.
auto BarrierScheduler::scheduleEvents(EventsCollection &node_events, const intermediate::Graph &graph,
  const PassColoring &pass_coloring, const IdIndexedFlags<intermediate::NodeIndex, framemem_allocator> &nodes_changed,
  const IdIndexedFlags<intermediate::ResourceIndex, framemem_allocator> &resources_changed) -> ResourceLifetimesChanged
{
  TIME_PROFILE(scheduleEvents);

  ResourceLifetimesChanged result;
  result.resize(graph.resources.totalKeys(), false);

  FRAMEMEM_VALIDATE;

  const auto gracePoints = computeGracePoints(graph, pass_coloring);
  auto dirtyResources = computeDirtyResources(graph, nodes_changed, resources_changed, gracePoints);

  const bool hasDirtyResources = dirtyResources.trueKeys().begin() != dirtyResources.trueKeys().end() || cachedResourceEvents.empty();

  // Compute the working size: max of new graph size and existing node_events size
  // (old events may reference node indices beyond the new graph's range)
  uint32_t nodeEventsWorkingSize = graph.nodes.totalKeys() + 1;
  for (int f = 0; f < SCHEDULE_FRAME_WINDOW; ++f)
    nodeEventsWorkingSize = eastl::max<uint32_t>(nodeEventsWorkingSize, node_events[f].size());

  // Grow to working size (don't shrink yet -- need to clean up stale entries first)
  for (int f = 0; f < SCHEDULE_FRAME_WINDOW; ++f)
    node_events[f].resize(nodeEventsWorkingSize);

  if (hasDirtyResources)
  {
    // Compute dirty nodes from OLD cached events (before cache is cleared)
    eastl::array<IdIndexedFlags<intermediate::NodeIndex, framemem_allocator>, SCHEDULE_FRAME_WINDOW> dirtyNodeFlags;
    for (auto &flags : dirtyNodeFlags)
      flags.resize(nodeEventsWorkingSize, false);

    for (auto resIdx : dirtyResources.trueKeys())
      if (cachedResourceEvents.isMapped(resIdx))
        for (const auto &pe : cachedResourceEvents[resIdx])
          dirtyNodeFlags[pe.eventFrame][static_cast<intermediate::NodeIndex>(pe.nodeTimepoint)] = true;

    for (auto resIdx : cachedResourceEvents.keys())
      if (!graph.resources.isMapped(resIdx))
        cachedResourceEvents[resIdx].clear();

    cachedResourceEvents.resize(graph.resources.totalKeys());

    updateDirtyResourceEvents(graph, dirtyResources, gracePoints);


    // Mark nodes targeted by NEW events from dirty resources
    for (auto resIdx : dirtyResources.trueKeys())
      if (cachedResourceEvents.isMapped(resIdx))
        for (const auto &pe : cachedResourceEvents[resIdx])
          dirtyNodeFlags[pe.eventFrame][static_cast<intermediate::NodeIndex>(pe.nodeTimepoint)] = true;

    {
      TIME_PROFILE(scatterEvents);

      // Remove stale events from dirty nodes (erase-remove by resource)
      for (int f = 0; f < SCHEDULE_FRAME_WINDOW; ++f)
        for (auto nodeIdx : dirtyNodeFlags[f].trueKeys())
        {
          auto &evts = node_events[f][nodeIdx];
          evts.erase(
            eastl::remove_if(evts.begin(), evts.end(), [&](const Event &e) { return dirtyResources.test(e.resource, false); }),
            evts.end());
        }

      // Scatter new events from dirty resources
      for (auto resIdx : dirtyResources.trueKeys())
        if (cachedResourceEvents.isMapped(resIdx))
          for (const auto &pe : cachedResourceEvents[resIdx])
            node_events[pe.eventFrame][static_cast<intermediate::NodeIndex>(pe.nodeTimepoint)].push_back(pe.event);

      // Re-sort only dirty nodes
      for (int f = 0; f < SCHEDULE_FRAME_WINDOW; ++f)
        for (auto nodeIdx : dirtyNodeFlags[f].trueKeys())
        {
          auto &evts = node_events[f][nodeIdx];
          eastl::sort(evts.begin(), evts.end(), [](const Event &a, const Event &b) { return a.data.index() > b.data.index(); });
        }
    }

    {
      TIME_PROFILE(recordDebugBarriers);
      // Debug barrier recording: re-record from all cached events when anything is dirty
      debug_clear_resource_barriers();
      for (auto [resIdx, placedEvents] : cachedResourceEvents.enumerate())
        for (const auto &pe : placedEvents)
          if (auto *barrier = eastl::get_if<Event::Barrier>(&pe.event.data))
            if (graph.resources.isMapped(pe.event.resource))
            {
              auto resId = graph.resources[pe.event.resource].frontendResources.front();
              debug_rec_resource_barrier(resId, pe.event.frameResourceProducedOn, pe.nodeTimepoint, pe.eventFrame, barrier->barrier);
            }
    }
  }


  {
    TIME_PROFILE(finalizeNodeEvents);
    // Shrink node_events to the actual graph size
    for (int f = 0; f < SCHEDULE_FRAME_WINDOW; ++f)
      node_events[f].resize(graph.nodes.totalKeys() + 1);
  }

  result = dirtyResources;

  return result;
}

BarrierScheduler::GracePoints BarrierScheduler::computeGracePoints(const intermediate::Graph &graph,
  const PassColoring &pass_coloring) const
{
  TIME_PROFILE(computeGracePoints);

  GracePoints gracePoints;
  gracePoints.insert(0);
  auto prevColor = MINUS_ONE_SENTINEL_FOR<PassColor>;
  for (auto key : graph.nodes.keys())
  {
    if (pass_coloring[key] != prevColor)
      gracePoints.insert(eastl::to_underlying(key));
    prevColor = pass_coloring[key];
  }
  gracePoints.insert(graph.nodes.totalKeys());
  return gracePoints;
}

BarrierScheduler::DirtyResources BarrierScheduler::computeDirtyResources(const intermediate::Graph &graph,
  const IdIndexedFlags<intermediate::NodeIndex, framemem_allocator> &nodes_changed,
  const IdIndexedFlags<intermediate::ResourceIndex, framemem_allocator> &resources_changed, const GracePoints &gracePoints)
{
  TIME_PROFILE(computeDirtyResources);

  DirtyResources dirtyResources(resources_changed);
  dirtyResources.resize(graph.resources.totalKeys(), false);

  for (auto nodeIdx : nodes_changed.trueKeys())
    if (graph.nodes.isMapped(nodeIdx))
      for (const auto &req : graph.nodes[nodeIdx].resourceRequests)
        dirtyResources[req.resource] = true;

  // Check grace points for changes -- if changed, all mapped resources are dirty
  {
    bool gracePointsChanged = gracePoints.size() != prevGracePoints.size();
    if (!gracePointsChanged)
      for (size_t i = 0; i < gracePoints.size(); ++i)
        if (gracePoints.begin()[i] != prevGracePoints[i])
        {
          gracePointsChanged = true;
          break;
        }
    if (gracePointsChanged)
    {
      for (auto resIdx : graph.resources.keys())
        dirtyResources[resIdx] = true;
      prevGracePoints.assign(gracePoints.begin(), gracePoints.end());
    }
  }

  // Treat unmapped resources with cached events as dirty (stale cleanup)
  dirtyResources.resize(eastl::max<size_t>(graph.resources.totalKeys(), cachedResourceEvents.size()), false);
  for (auto resIdx : cachedResourceEvents.keys())
    if (!graph.resources.isMapped(resIdx) && !cachedResourceEvents[resIdx].empty())
      dirtyResources[resIdx] = true;

  return dirtyResources;
}

void BarrierScheduler::updateDirtyResourceEvents(const intermediate::Graph &graph, const DirtyResources &dirtyResources,
  const GracePoints &gracePoints)
{
  TIME_PROFILE(updateDirtyResourceEvents);

  struct ResourceUsageOccurrence
  {
    intermediate::ResourceUsage usage;
    uint32_t frame;
    uint32_t nodeIndex;
  };

  IdIndexedMapping<intermediate::ResourceIndex, uint32_t, framemem_allocator> perResourceUsageCount(graph.resources.totalKeys(), 0);
  for (const auto &node : graph.nodes.values())
    for (const auto &req : node.resourceRequests)
      if (dirtyResources.test(req.resource, false))
        perResourceUsageCount[req.resource]++;

  eastl::array<
    IdIndexedMapping<intermediate::ResourceIndex, dag::Vector<ResourceUsageOccurrence, framemem_allocator>, framemem_allocator>,
    SCHEDULE_FRAME_WINDOW>
    perFrameResourceUsageTimelines;

  // Pre-allocate all timelines so that we can use framemem allocator
  // Note that the order of allocation has to be reversed so that
  // framemem can clean up properly
  for (auto &timelines : perFrameResourceUsageTimelines)
  {
    timelines.resize(graph.resources.totalKeys(), {});
    for (auto idx : dag::ReverseView(timelines.keys()))
      timelines[idx].reserve(perResourceUsageCount[idx]);
  }

  const auto processResourceInput = [&perFrameResourceUsageTimelines](int res_owner_frame, int event_frame,
                                      intermediate::NodeIndex node_idx, intermediate::ResourceIndex res_idx,
                                      intermediate::ResourceUsage usage) {
    perFrameResourceUsageTimelines[res_owner_frame][res_idx].push_back(
      ResourceUsageOccurrence{usage, static_cast<uint32_t>(event_frame), node_idx});
  };

  // We want to find the lifetime of every resource in terms
  // of timepoints. Every "pause" between two nodes being executed
  // is a timepoint. As we sometimes need textures to live for 2 frames,
  // we have nodes.size()*2 timepoints. All usage occurrences are
  // sorted into per-physical-resource bins and processed below.

  for (int frame = SCHEDULE_FRAME_WINDOW - 1; frame >= 0; --frame)
  {
    // We have to first iterate and record all nodes' history requests
    // i.e. resources owned by current frame but requested by next frame

    for (auto idx : dag::ReverseView(graph.nodes.keys()))
    {
      const int nextFrame = (frame + 1) % SCHEDULE_FRAME_WINDOW;

      for (const auto &req : graph.nodes[idx].resourceRequests)
        if (req.fromLastFrame && dirtyResources.test(req.resource, false))
          processResourceInput(frame, nextFrame, idx, req.resource, req.usage);
    }

    for (auto idx : dag::ReverseView(graph.nodes.keys()))
    {
      for (const auto &req : graph.nodes[idx].resourceRequests)
        if (!req.fromLastFrame && dirtyResources.test(req.resource, false))
          processResourceInput(frame, frame, idx, req.resource, req.usage);
    }
  }

  // Merge usages to avoid a bunch of repeating SRV barriers for different shader stages
  for (uint32_t frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
    for (auto [resIdx, timeline] : perFrameResourceUsageTimelines[frame].enumerate())
    {
      if (!dirtyResources.test(resIdx, false))
        continue;
      for (auto it = timeline.begin(); it != timeline.end();)
      {
        // Find the end of a consequent run of occurrences with same
        // type and access but different stages and merge the stages
        Stage mergedStage = it->usage.stage;
        auto runEnd = it + 1;
        while (runEnd != timeline.end() && it->usage.access == runEnd->usage.access && it->usage.type == runEnd->usage.type)
          mergedStage |= (runEnd++)->usage.stage;

        while (it != runEnd)
          (it++)->usage.stage = mergedStage;
      }
    }

  for (auto resIdx : dirtyResources.trueKeys())
    if (cachedResourceEvents.isMapped(resIdx))
      cachedResourceEvents[resIdx].clear();

  for (uint32_t frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
    for (auto [resIdx, timeline] : perFrameResourceUsageTimelines[frame].enumerate())
    {
      if (!dirtyResources.test(resIdx, false))
        continue;
      if (!graph.resources.isMapped(resIdx))
        continue;

      const auto &resource = graph.resources[resIdx];

      // Carefully place split or regular barriers between usage
      // occurrences that yield one
      if (resource.getResType() != ResourceType::Blob)
      {
        for (int i = timeline.size() - 2; i >= 0; --i)
        {
          const auto &curr = timeline[i];
          const auto &prev = timeline[i + 1];

          ResourceBarrier barrier = barrier_for_transition(prev.usage, curr.usage);

          if (barrier == RB_NONE)
            continue;

          uint32_t eventAfterPreviousNode = *gracePoints.lower_bound(prev.nodeIndex + 1);
          uint32_t eventBeforeCurrentNode = *(gracePoints.upper_bound(curr.nodeIndex) - 1);

          if (prev.frame == curr.frame && eventAfterPreviousNode > eventBeforeCurrentNode)
          {
            if (verbose)
              logwarn("daFG: Barrier scheduling had to break a pass between nodes '%s' and '%s' "
                      "because of a logical data race on resource '%s'! "
                      "The nodes are executed in arbitrary order and the barrier between them "
                      "will make performance non-deterministic. "
                      "Please sequence the nodes explicitly using a rename if this barrier is expected and desireable, "
                      "or refactor the nodes to not require this barrier.",
                graph.nodeNames[static_cast<intermediate::NodeIndex>(prev.nodeIndex)].c_str(),
                graph.nodeNames[static_cast<intermediate::NodeIndex>(curr.nodeIndex)].c_str(), graph.resourceNames[resIdx].c_str());

            // Fall back to a single barrier placed as soon as possible.
            eventAfterPreviousNode = eventBeforeCurrentNode = prev.nodeIndex + 1;
          }

          G_ASSERT(prev.frame != curr.frame || eventAfterPreviousNode <= eventBeforeCurrentNode);

          auto cacheBarrier = [&, resIdx = resIdx](uint32_t time, ResourceBarrier additional_flags) {
            Event event{resIdx, frame, Event::Barrier{barrier | additional_flags}};
            cachedResourceEvents[resIdx].push_back(PlacedEvent{event, time, prev.frame});
          };

          if (prev.frame != curr.frame)
          {
            // NOTE: split barriers shouldn't be used between frames,
            // place a regular barrier at the end of prev frame.
            cacheBarrier(graph.nodes.totalKeys(), RB_NONE);
          }
          else if (eventAfterPreviousNode == eventBeforeCurrentNode || resource.getResType() == ResourceType::Buffer)
          {
            cacheBarrier(eventAfterPreviousNode, RB_NONE);
          }
          else
          {
            cacheBarrier(eventAfterPreviousNode, RB_FLAG_SPLIT_BARRIER_BEGIN);
            cacheBarrier(eventBeforeCurrentNode, RB_FLAG_SPLIT_BARRIER_END);
          }
        }
      }

      // Now emit activation/deactivation events for scheduled resources
      if (!resource.isScheduled())
        continue;

      // Never-used resources should be impossible due to invariants
      G_ASSERT(!perFrameResourceUsageTimelines[frame][resIdx].empty());

      const auto &scheduledRes = graph.resources[resIdx].asScheduled();

      // Deactivation
      {
        const auto &lastUsageOccurrence = timeline.front();

        Event::Payload payload;

        if (scheduledRes.isGpuResource())
          payload = Event::Deactivation{};
        else
          payload = Event::CpuDeactivation{scheduledRes.getCpuDescription().dtor};

        cachedResourceEvents[resIdx].push_back(PlacedEvent{
          Event{resIdx, static_cast<uint32_t>(frame), payload}, lastUsageOccurrence.nodeIndex + 1, lastUsageOccurrence.frame});
      }

      // Activation
      {
        const auto &firstUsageOccurrence = perFrameResourceUsageTimelines[frame][resIdx].back();

        Event::Payload payload;
        if (resource.asScheduled().isGpuResource())
        {
          const auto &basicResDesc = graph.resources[resIdx].asScheduled().getGpuDescription().asBasicRes;
          payload = Event::Activation{basicResDesc.activation, resource.asScheduled().clearValue};
        }
        else
        {
          payload = Event::CpuActivation{resource.asScheduled().getCpuDescription().ctor};
        }

        cachedResourceEvents[resIdx].push_back(PlacedEvent{
          Event{resIdx, static_cast<uint32_t>(frame), payload}, firstUsageOccurrence.nodeIndex, firstUsageOccurrence.frame});
      }
    }
}

} // namespace dafg
