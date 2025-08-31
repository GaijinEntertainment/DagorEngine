// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "barrierScheduler.h"

#include <common/resourceUsage.h>
#include <debug/backendDebug.h>

#include <perfMon/dag_statDrv.h>
#include <memory/dag_framemem.h>
#include <dag/dag_vectorSet.h>


namespace dafg
{

// TODO: in the future, we should replace activation/deactivation actions
// with aliasing barriers, as they match what is happening in HW more
// closely. This will force us to remove resource (de)activation events
// and calculate resource lifetimes separately from barrier scheduling,
// which is good and supposed to be separate.
BarrierScheduler::EventsCollectionRef BarrierScheduler::scheduleEvents(const intermediate::Graph &graph,
  const PassColoring &pass_coloring)
{
  TIME_PROFILE(scheduleEvents);

  FRAMEMEM_VALIDATE;

  struct ResourceUsageOccurrence
  {
    intermediate::ResourceUsage usage;
    uint32_t frame;
    uint32_t nodeIndex;
  };

  IdIndexedMapping<intermediate::ResourceIndex, uint32_t, framemem_allocator> perResourceUsageCount(graph.resources.size(), 0);
  for (const auto &node : graph.nodes)
    for (const auto &req : node.resourceRequests)
      perResourceUsageCount[req.resource]++;

  // Timelines of when and how every resource was used
  eastl::array<
    IdIndexedMapping<intermediate::ResourceIndex, dag::Vector<ResourceUsageOccurrence, framemem_allocator>, framemem_allocator>,
    SCHEDULE_FRAME_WINDOW>
    perFrameResourceUsageTimelines;

  // Pre-allocate all timelines so that we can use framemem allocator
  // Note that the order of allocation has to be reversed so that
  // framemem can clean up properly
  for (auto &timelines : perFrameResourceUsageTimelines)
  {
    timelines.resize(graph.resources.size(), {});
    for (uint32_t i = 0; i < timelines.size(); ++i)
    {
      // Could look a lot less ugly with std::ranges :/
      const auto idx = static_cast<intermediate::ResourceIndex>(timelines.size() - 1 - i);
      timelines[idx].reserve(perResourceUsageCount[idx]);
    }
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

    for (int i = graph.nodes.size() - 1; i >= 0; --i)
    {
      const int nextFrame = (frame + 1) % SCHEDULE_FRAME_WINDOW;

      auto idx = static_cast<intermediate::NodeIndex>(i);
      for (const auto &req : graph.nodes[idx].resourceRequests)
        if (req.fromLastFrame)
          processResourceInput(frame, nextFrame, idx, req.resource, req.usage);
    }

    for (int i = graph.nodes.size() - 1; i >= 0; --i)
    {
      auto idx = static_cast<intermediate::NodeIndex>(i);
      for (const auto &req : graph.nodes[idx].resourceRequests)
        if (!req.fromLastFrame)
          processResourceInput(frame, frame, idx, req.resource, req.usage);
    }
  }


  // Merge usages to avoid a bunch of repeating SRV barriers for different shader stages
  for (uint32_t frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
    for (auto [resIdx, timeline] : perFrameResourceUsageTimelines[frame].enumerate())
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

  // We need to find "grace points" in the coloring, i.e. points at which
  // we can insert barriers without being worried about breaking a pass
  dag::VectorSet<uint32_t, eastl::less<uint32_t>, framemem_allocator> gracePoints;
  gracePoints.insert(0);
  for (uint32_t i = 1; i < pass_coloring.size(); ++i)
    if (pass_coloring[static_cast<intermediate::NodeIndex>(i)] != pass_coloring[static_cast<intermediate::NodeIndex>(i - 1)])
      gracePoints.insert(i);
  gracePoints.insert(pass_coloring.size());

  // Emit events from per-physical-resource usage occurrences
  // NOTE: memory for the events gets insertions in an extremely
  // non-uniform manner, so we try to mend this issue by pre-allocating
  // a bunch of data in framemem and restoring it afterwards in case
  // a pre-allocation's size wasn't big enough
  FRAMEMEM_REGION;

  using NodeEvents = dag::Vector<Event, framemem_allocator>;
  using FrameEvents = dag::Vector<NodeEvents, framemem_allocator>;
  using EventsCollection = eastl::array<FrameEvents, SCHEDULE_FRAME_WINDOW>;

  EventsCollection scheduledEvents;
  for (FrameEvents &frameEvents : scheduledEvents)
  {
    frameEvents.resize(graph.nodes.size() + 1);
    for (auto &events : frameEvents)
      events.reserve(16);
  }

  debug_clear_resource_barriers();

  for (uint32_t frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
    for (auto [resIdx, timeline] : perFrameResourceUsageTimelines[frame].enumerate())
    {
      // Carefully place split or regular barriers between usage
      // occurrences that yield one
      const auto &resource = graph.resources[resIdx];
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
            logwarn("daFG: Barrier scheduling had to break a pass between nodes '%s' and '%s' "
                    "because of a logical data race on resource '%s'! This is fine, but suboptimal!",
              graph.nodeNames[static_cast<intermediate::NodeIndex>(prev.nodeIndex)].c_str(),
              graph.nodeNames[static_cast<intermediate::NodeIndex>(curr.nodeIndex)].c_str(), graph.resourceNames[resIdx].c_str());
            // Fall back to a single barrier placed as soon as possible.
            eventAfterPreviousNode = eventBeforeCurrentNode = prev.nodeIndex + 1;
          }

          G_ASSERT(prev.frame != curr.frame || eventAfterPreviousNode <= eventBeforeCurrentNode);

          auto &frameEvents = scheduledEvents[prev.frame];

          auto recBarrier = [&, resIdx = resIdx](uint32_t time, ResourceBarrier additional_flags) {
            Event event{resIdx, frame, Event::Barrier{barrier | additional_flags}};
            frameEvents[time].emplace_back(event);

            // NOTE: barriers are executed on physical resources,
            // while debug visualizer works with virtual resources.
            // We could try and find the "correct" virtual resource to
            // use for this physical res at the specified time, but it's
            // pointless, as the visualization won't look any different,
            // so we lie about it.
            auto resId = graph.resources[resIdx].frontendResources.front();
            debug_rec_resource_barrier(resId, frame, time, prev.frame, barrier | additional_flags);
          };

          if (prev.frame != curr.frame)
          {
            // NOTE: split barriers shouldn't be used between frames,
            // place a regular barrier at the end of prev frame.
            recBarrier(frameEvents.size() - 1, RB_NONE);
          }
          else if (eventAfterPreviousNode == eventBeforeCurrentNode || resource.getResType() == ResourceType::Buffer)
          {
            recBarrier(eventAfterPreviousNode, RB_NONE);
          }
          else
          {
            recBarrier(eventAfterPreviousNode, RB_FLAG_SPLIT_BARRIER_BEGIN);
            recBarrier(eventBeforeCurrentNode, RB_FLAG_SPLIT_BARRIER_END);
          }
        }
      }

      // Now emit activation/deactivation events for scheduled resources
      if (!resource.isScheduled())
        continue;

      // Never-used resources should be impossible due to invariants
      G_ASSERT(!perFrameResourceUsageTimelines[frame][resIdx].empty());

      const auto &scheduledRes = graph.resources[resIdx].asScheduled();

      {
        const auto &lastUsageOccurrence = timeline.front();

        Event::Payload payload;

        if (scheduledRes.isGpuResource())
          payload = Event::Deactivation{};
        else
          payload = Event::CpuDeactivation{scheduledRes.getCpuDescription().dtor};

        scheduledEvents[lastUsageOccurrence.frame][lastUsageOccurrence.nodeIndex + 1].emplace_back(
          Event{resIdx, static_cast<uint32_t>(frame), payload});
      }

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
        scheduledEvents[firstUsageOccurrence.frame][firstUsageOccurrence.nodeIndex].emplace_back(
          Event{resIdx, static_cast<uint32_t>(frame), payload});
      }
    }

  // Sort per-node events to ensure following order:
  // * deactivate all
  // * barriers
  // * activate all
  for (auto &frameEvents : scheduledEvents)
    for (auto &nodeEvents : frameEvents)
      eastl::sort(nodeEvents.begin(), nodeEvents.end(),
        [](const Event &a, const Event &b) { return a.data.index() > b.data.index(); });

  eventStorage.clear();

  size_t totalEvents = 0;
  for (const auto &frameEvents : scheduledEvents)
    for (const auto &nodeEvents : frameEvents)
      totalEvents += nodeEvents.size();

  // guarantees no reallocations when inserting, therefore it's okay to take subspans
  eventStorage.reserve(totalEvents);

  EventsCollectionRef result;
  for (int j = 0; j < SCHEDULE_FRAME_WINDOW; ++j)
  {
    eventRefStorage[j].clear();
    eventRefStorage[j].reserve(scheduledEvents[j].size());
    for (auto &eventsForNode : scheduledEvents[j])
    {
      Event *groupStart = &*eventStorage.end();
      eventStorage.insert(eventStorage.end(), eventsForNode.begin(), eventsForNode.end());
      eventRefStorage[j].emplace_back(groupStart, eventsForNode.size());
    }
    result[j] = eventRefStorage[j];
  }

  return result;
}

} // namespace dafg