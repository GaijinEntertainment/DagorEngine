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

auto BarrierScheduler::barrier_kind(const Event &event) -> Event::BarrierKind
{
  if (const auto *bufferBarrier = eastl::get_if<Event::EnhancedBufferBarrier>(&event.data))
    return bufferBarrier->kind;
  if (const auto *textureBarrier = eastl::get_if<Event::EnhancedTextureBarrier>(&event.data))
    return textureBarrier->kind;
  if (eastl::holds_alternative<Event::Barrier>(event.data))
    return Event::BarrierKind::Transition;
  return Event::BarrierKind::None;
}

static const d3d::PipelineSyncOperation *enhanced_pipeline_sync(const BarrierScheduler::Event &event)
{
  if (const auto *bufferBarrier = eastl::get_if<BarrierScheduler::Event::EnhancedBufferBarrier>(&event.data))
    return &bufferBarrier->barrier.pipelineSync;
  if (const auto *textureBarrier = eastl::get_if<BarrierScheduler::Event::EnhancedTextureBarrier>(&event.data))
    return &textureBarrier->barrier.pipelineSync;
  return nullptr;
}

uint32_t BarrierScheduler::execution_rank(const Event &event)
{
  if (eastl::holds_alternative<Event::CpuDeactivation>(event.data))
    return 0;
  if (eastl::holds_alternative<Event::Deactivation>(event.data))
    return 1;
  if (eastl::holds_alternative<Event::CpuActivation>(event.data))
    return 5;
  if (eastl::holds_alternative<Event::Activation>(event.data))
    return 4;
  switch (barrier_kind(event))
  {
    case Event::BarrierKind::Release: return 2;
    case Event::BarrierKind::Transition: return 3;
    case Event::BarrierKind::FirstUse: return 4;
    case Event::BarrierKind::None: break;
  }
  return 3;
}

// TODO: tracked resources still go through driver (de)activation actions.
// Expressing them as enhanced barriers, the way untracked resources already
// work, would match what is happening in HW more closely.
void BarrierScheduler::scheduleEvents(EventsCollection &node_events, const intermediate::Graph &graph,
  const ResourceLifetimes &lifetimes, const PassColoring &pass_coloring,
  const IdIndexedFlags<intermediate::NodeIndex, framemem_allocator> &nodes_changed,
  const IdIndexedFlags<intermediate::ResourceIndex, framemem_allocator> &resources_changed,
  const IdIndexedFlags<intermediate::ResourceIndex, framemem_allocator> &lifetimes_changed)
{
  TIME_PROFILE(scheduleEvents);

  FRAMEMEM_VALIDATE;

  const auto gracePoints = compute_grace_points(graph, pass_coloring);
  auto dirtyResources = computeDirtyResources(graph, nodes_changed, resources_changed, lifetimes_changed, gracePoints);

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

    updateDirtyResourceEvents(graph, lifetimes, dirtyResources, gracePoints);


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
          eastl::sort(evts.begin(), evts.end(), [](const Event &a, const Event &b) { return execution_rank(a) < execution_rank(b); });
        }
    }

    {
      TIME_PROFILE(recordDebugBarriers);
      // Debug barrier recording: re-record from all cached events when anything is dirty
      debug_clear_resource_barriers();
      for (auto [resIdx, placedEvents] : cachedResourceEvents.enumerate())
        for (const auto &pe : placedEvents)
        {
          auto *barrier = eastl::get_if<Event::Barrier>(&pe.event.data);
          auto *bufferBarrier = eastl::get_if<Event::EnhancedBufferBarrier>(&pe.event.data);
          auto *textureBarrier = eastl::get_if<Event::EnhancedTextureBarrier>(&pe.event.data);
          if ((!barrier && !bufferBarrier && !textureBarrier) || !graph.resources.isMapped(pe.event.resource))
            continue;
          const auto resId = graph.resources[pe.event.resource].frontendResources.front();
          if (barrier)
            debug_rec_resource_barrier(resId, pe.event.frameResourceProducedOn, pe.nodeTimepoint, pe.eventFrame, barrier->barrier);
          else if (bufferBarrier)
            debug_rec_enhanced_buffer_barrier(resId, pe.event.frameResourceProducedOn, pe.nodeTimepoint, pe.eventFrame,
              bufferBarrier->barrier);
          else
            debug_rec_enhanced_texture_barrier(resId, pe.event.frameResourceProducedOn, pe.nodeTimepoint, pe.eventFrame,
              textureBarrier->barrier);
        }
    }
  }


  {
    TIME_PROFILE(finalizeNodeEvents);
    // Shrink node_events to the actual graph size
    for (int f = 0; f < SCHEDULE_FRAME_WINDOW; ++f)
      node_events[f].resize(graph.nodes.totalKeys() + 1);
  }

  computeUsageSyncStages(graph);
}

void BarrierScheduler::computeUsageSyncStages(const intermediate::Graph &graph)
{
  TIME_PROFILE(computeUsageSyncStages);

  usageSyncStagesPerResource.assign(graph.resources.totalKeys(), UsageSyncStages{});
  for (auto resIdx : graph.resources.keys())
  {
    if (!cachedResourceEvents.isMapped(resIdx))
      continue;

    auto &syncStages = usageSyncStagesPerResource[resIdx];
    for (const auto &placedEvent : cachedResourceEvents[resIdx])
    {
      const auto *pipelineSync = enhanced_pipeline_sync(placedEvent.event);
      if (!pipelineSync)
        continue;

      switch (barrier_kind(placedEvent.event))
      {
        case Event::BarrierKind::Release: syncStages.lastUse |= pipelineSync->src; break;
        case Event::BarrierKind::FirstUse: syncStages.firstUse |= pipelineSync->dst; break;
        default: break;
      }
    }
  }
}

void BarrierScheduler::setAliasSyncStages(EventsCollection &node_events, intermediate::ResourceIndex res_idx,
  d3d::PipelineStageFlags sync_before, d3d::PipelineStageFlags sync_after)
{
  if (!cachedResourceEvents.isMapped(res_idx))
    return;

  const auto patch = [sync_before, sync_after](Event &event) {
    if (auto *bufferBarrier = eastl::get_if<Event::EnhancedBufferBarrier>(&event.data))
    {
      if (bufferBarrier->kind == Event::BarrierKind::FirstUse)
        bufferBarrier->barrier.pipelineSync.src = sync_before;
      else if (bufferBarrier->kind == Event::BarrierKind::Release)
        bufferBarrier->barrier.pipelineSync.dst = sync_after;
    }
    else if (auto *textureBarrier = eastl::get_if<Event::EnhancedTextureBarrier>(&event.data))
    {
      if (textureBarrier->kind == Event::BarrierKind::FirstUse)
        textureBarrier->barrier.pipelineSync.src = sync_before;
      else if (textureBarrier->kind == Event::BarrierKind::Release)
        textureBarrier->barrier.pipelineSync.dst = sync_after;
    }
  };

  for (auto &placedEvent : cachedResourceEvents[res_idx])
  {
    const auto kind = barrier_kind(placedEvent.event);
    if (kind != Event::BarrierKind::FirstUse && kind != Event::BarrierKind::Release)
      continue;

    patch(placedEvent.event);

    for (auto &event : node_events[placedEvent.eventFrame][static_cast<intermediate::NodeIndex>(placedEvent.nodeTimepoint)])
      if (event.resource == res_idx && event.frameResourceProducedOn == placedEvent.event.frameResourceProducedOn &&
          barrier_kind(event) == kind)
        patch(event);
  }
}

BarrierScheduler::DirtyResources BarrierScheduler::computeDirtyResources(const intermediate::Graph &graph,
  const IdIndexedFlags<intermediate::NodeIndex, framemem_allocator> &nodes_changed,
  const IdIndexedFlags<intermediate::ResourceIndex, framemem_allocator> &resources_changed,
  const IdIndexedFlags<intermediate::ResourceIndex, framemem_allocator> &lifetimes_changed, const GracePoints &gracePoints)
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

  for (auto resIdx : lifetimes_changed.trueKeys())
    dirtyResources.set(resIdx, true);

  return dirtyResources;
}

void BarrierScheduler::updateDirtyResourceEvents(const intermediate::Graph &graph, const ResourceLifetimes &lifetimes,
  const DirtyResources &dirtyResources, const GracePoints &gracePoints)
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

      const bool untrackedBuffer = resource.isUntrackedBuffer();
      const bool untrackedTexture = resource.isUntrackedTexture();
      const bool untracked = untrackedBuffer || untrackedTexture;

      const auto declaredUsage = [](auto begin, auto end) -> eastl::optional<intermediate::ResourceUsage> {
        const auto it = eastl::find_if(begin, end, [](const auto &occ) { return occ.usage.type != Usage::UNKNOWN; });
        if (it == end)
          return eastl::nullopt;
        return it->usage;
      };

      // Carefully place split or regular barriers between usage
      // occurrences that yield one
      if (resource.getResType() != ResourceType::Blob)
      {
        const int timelineSize = static_cast<int>(timeline.size());
        for (int i = timelineSize - 2; i >= 0; --i)
        {
          const auto &curr = timeline[i];

          int prevIdx = i + 1;
          if (untracked)
          {
            if (curr.usage.type == Usage::UNKNOWN)
              continue;
            while (prevIdx < timelineSize && timeline[prevIdx].usage.type == Usage::UNKNOWN)
              ++prevIdx;
            if (prevIdx == timelineSize)
              continue;
          }

          const auto &prev = timeline[prevIdx];

          ResourceBarrier barrier = RB_NONE;
          intermediate::EnhancedBarrier enhancedBarrier;
          if (untracked)
            enhancedBarrier = enhanced_barrier_for_transition(prev.usage, curr.usage, resource.getResType());
          else
            barrier = barrier_for_transition(prev.usage, curr.usage);

          if (barrier == RB_NONE && eastl::holds_alternative<eastl::monostate>(enhancedBarrier))
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

          if (prevIdx != i + 1 && eventBeforeCurrentNode > eventAfterPreviousNode)
            eventAfterPreviousNode = eventBeforeCurrentNode;

          auto cacheBarrier = [&, resIdx = resIdx](uint32_t time, ResourceBarrier additional_flags) {
            Event::Payload payload;
            if (untracked)
            {
              // TODO: enhanced barriers drop split-barrier flags
              G_ASSERTF(additional_flags == RB_NONE,
                "daFG: enhanced barriers can't carry split-barrier flags for untracked resources");
              if (const auto *bufferBarrier = eastl::get_if<d3d::BufferBarrier>(&enhancedBarrier))
                payload = Event::EnhancedBufferBarrier{*bufferBarrier, Event::BarrierKind::Transition};
              else
                payload =
                  Event::EnhancedTextureBarrier{eastl::get<d3d::TextureBarrier>(enhancedBarrier), Event::BarrierKind::Transition};
            }
            else
              payload = Event::Barrier{barrier | additional_flags};
            Event event{resIdx, frame, payload};
            cachedResourceEvents[resIdx].push_back(PlacedEvent{event, time, prev.frame});
          };

          if (prev.frame != curr.frame)
          {
            // NOTE: split barriers shouldn't be used between frames,
            // place a regular barrier at the end of prev frame.
            cacheBarrier(graph.nodes.totalKeys(), RB_NONE);
          }
          else if (eventAfterPreviousNode == eventBeforeCurrentNode || resource.getResType() == ResourceType::Buffer || untracked)
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

      if (!resource.isScheduled())
        continue;

      const auto &scheduledRes = resource.asScheduled();
      const auto &lifetime = lifetimes[frame][resIdx];

      G_ASSERTF(!timeline.empty() && lifetime.firstUse.frame == timeline.back().frame &&
                  lifetime.firstUse.timepoint <= timeline.back().nodeIndex && lifetime.release.frame == timeline.front().frame &&
                  lifetime.release.timepoint > timeline.front().nodeIndex,
        "daFG: lifetime of resource '%s' does not cover its usage timeline!", graph.resourceNames[resIdx].c_str());

      const auto cacheLifetimeEvent = [&, resIdx = resIdx](LifetimePoint at, Event::Payload payload) {
        cachedResourceEvents[resIdx].push_back(
          PlacedEvent{Event{resIdx, static_cast<uint32_t>(frame), eastl::move(payload)}, at.timepoint, at.frame});
      };

      if (untracked)
      {
        if (const auto lastUsage = declaredUsage(timeline.begin(), timeline.end()))
          cacheLifetimeEvent(lifetime.release,
            untrackedBuffer
              ? Event::Payload{Event::EnhancedBufferBarrier{
                  enhanced_buffer_barrier_for_release(*lastUsage, d3d::PipelineStageFlag::All), Event::BarrierKind::Release}}
              : Event::Payload{Event::EnhancedTextureBarrier{
                  enhanced_texture_barrier_for_release(*lastUsage, d3d::PipelineStageFlag::All), Event::BarrierKind::Release}});
      }
      else if (scheduledRes.isGpuResource())
        cacheLifetimeEvent(lifetime.release, Event::Deactivation{});
      else
        cacheLifetimeEvent(lifetime.release, Event::CpuDeactivation{scheduledRes.getCpuDescription().dtor});

      // First use
      if (untracked)
      {
        if (const auto firstUsage = declaredUsage(timeline.rbegin(), timeline.rend()))
          cacheLifetimeEvent(lifetime.firstUse,
            untrackedBuffer
              ? Event::Payload{Event::EnhancedBufferBarrier{
                  enhanced_buffer_barrier_for_activation(*firstUsage, d3d::PipelineStageFlag::All), Event::BarrierKind::FirstUse}}
              : Event::Payload{Event::EnhancedTextureBarrier{
                  enhanced_texture_barrier_for_activation(*firstUsage, d3d::PipelineStageFlag::All), Event::BarrierKind::FirstUse}});
      }
      else if (scheduledRes.isGpuResource())
        cacheLifetimeEvent(lifetime.firstUse,
          Event::Activation{scheduledRes.getGpuDescription().asBasicRes.activation, scheduledRes.clearValue});
      else
        cacheLifetimeEvent(lifetime.firstUse, Event::CpuActivation{scheduledRes.getCpuDescription().ctor});
    }
}

} // namespace dafg
