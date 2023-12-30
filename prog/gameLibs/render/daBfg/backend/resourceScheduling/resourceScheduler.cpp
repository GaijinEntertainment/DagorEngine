#include "resourceScheduler.h"

#include <EASTL/algorithm.h>
#include <EASTL/bitvector.h>
#include <EASTL/sort.h>

#include <perfMon/dag_statDrv.h>
#include <util/dag_convar.h>
#include <dag/dag_vectorSet.h>
#include <3d/dag_drv3d.h>

#include <common/resourceUsage.h>
#include <runtime/runtime.h>
#include <id/idRange.h>
#include <backend/resourceScheduling/packer.h>
#include <debug/backendDebug.h>


#if DABFG_STATISTICS_REPORTING
CONSOLE_BOOL_VAL("dabfg", report_resource_statistics, false);
#endif

CONSOLE_BOOL_VAL("dabfg", set_resource_names, DAGOR_DBGLEVEL > 0);

CONSOLE_INT_VAL("dabfg", resource_packer, dabfg::PackerType::GreedyScanline, 0, dabfg::PackerType::COUNT);

namespace dabfg
{

struct FrameResource
{
  intermediate::ResourceIndex resIdx;
  uint32_t frame;
};

static bool can_substitute_heap_group(ResourceHeapGroupProperties origin, ResourceHeapGroupProperties candidate)
{
  const bool sameFlags = candidate.flags == origin.flags;
  const bool sameCPUVisibility = candidate.isCPUVisible == origin.isCPUVisible;
  const bool sameGPULocal = candidate.isGPULocal == origin.isGPULocal;
  return sameFlags && sameCPUVisibility && sameGPULocal;
}

auto ResourceScheduler::gatherResourceProperties(const intermediate::Graph &graph) -> ResourceProperties
{
  ResourceProperties resourceProperties(graph.resources.size(), ResourceAllocationProperties{});

  auto getFakePropertiesForCpu = [](BlobDescription desc) {
    return ResourceAllocationProperties{desc.size, desc.alignment, CPU_HEAP_GROUP};
  };

  auto getResourceFlags = [](const ResourceDescription &desc) { return desc.asBasicRes.cFlags; };

  for (auto [i, res] : graph.resources.enumerate())
    if (res.isScheduled())
    {
      // We can't use offset hint if the format of the resource is different from the old one.
      // This can happen after we change settings in which case we simply reset hinting.
      if (res.asScheduled().isGpuResource() && res.asScheduled().history != History::No)
      {
        const auto newFlags = getResourceFlags(eastl::get<ResourceDescription>(res.asScheduled().description));
        for (const auto resNameId : res.frontendResources)
        {
          const auto flags = historyResourceFlags.find(resNameId);
          if (flags != historyResourceFlags.end() && newFlags != flags->second)
            hintedResources.clear();

          historyResourceFlags[resNameId] = newFlags;
        }
      }
      resourceProperties[i] = res.asScheduled().isGpuResource()
                                ? getResourceAllocationProperties(res.asScheduled().getGpuDescription())
                                : getFakePropertiesForCpu(res.asScheduled().getCpuDescription());
    }
  return resourceProperties;
}

auto ResourceScheduler::scheduleResourcesIntoHeaps(const ResourceProperties &resources, const EventsCollectionRef &allEvents,
  const IntermediateRemapping &cached_res_idx_remapping) -> eastl::pair<AllocationLocations, HeapRequests>
{
  TIME_PROFILE(scheduleHeaps);

  FRAMEMEM_VALIDATE;

  // Note that this cannot use framemem as new heap requests appear
  // quite spontaneously while iterating resources.
  eastl::pair<AllocationLocations, HeapRequests> result;

  auto &allocations = result.first;
  auto &heapRequests = result.second;

  for (auto &framePreservedResource : preservedResources)
  {
    framePreservedResource.clear();
    framePreservedResource.resize(resources.size(), false);
  }

  for (auto &perFrameAllocations : allocations)
    perFrameAllocations.resize(resources.size());

  // First, gather all unique heap types and count the amount of resources
  // assigned to them
  // ESRAM requests should be first so that we can reschedule
  // non-fitting resources into regular heaps.
  const auto heapGroupComp = [this](ResourceHeapGroup *h1, ResourceHeapGroup *h2) {
    const auto heapGroupProp1 = getResourceHeapGroupProperties(h1);
    const auto heapGroupProp2 = getResourceHeapGroupProperties(h2);
    if (heapGroupProp1.isOnChip != heapGroupProp2.isOnChip)
      return heapGroupProp1.isOnChip > heapGroupProp2.isOnChip;
    return eastl::less<ResourceHeapGroup *>()(h1, h2);
  };
  dag::VectorMap<ResourceHeapGroup *, uint32_t, decltype(heapGroupComp), framemem_allocator> resourcesPerHeapType(heapGroupComp);
  // TODO: remove when dag::Vector::DoInsertValue starts using resizeInplace
  resourcesPerHeapType.reserve(resources.size());
  for (const auto &resProps : resources)
    if (resProps.offsetAlignment != 0)
      ++resourcesPerHeapType[resProps.heapGroup]; //-V552
  resourcesPerHeapType.shrink_to_fit();

  // The request into which resources with this heap group should go into.
  // Gets changed to a new heap request if we run out of memory
  dag::VectorMap<ResourceHeapGroup *, HeapIndex> activeRequestIndexForHeapGroup;
  // Hope that we'll need <=2 actual heaps per group
  activeRequestIndexForHeapGroup.reserve(resourcesPerHeapType.size() * 2);
  heapRequests.reserve(resourcesPerHeapType.size() * 2);


  // Initially, point all previously existing groups into the first
  // previously available heap
  for (auto [idx, heap] : allocatedHeaps.enumerate())
  {
    if (activeRequestIndexForHeapGroup.find(heap.group) == activeRequestIndexForHeapGroup.end())
      activeRequestIndexForHeapGroup.emplace(heap.group, idx);
    heapRequests.appendNew(HeapRequest{heap.group, 0u});
  }

  // Create new requests for heap groups that were not previously available
  for (auto [heapGroup, _] : resourcesPerHeapType)
    if (activeRequestIndexForHeapGroup.find(heapGroup) == activeRequestIndexForHeapGroup.end())
      activeRequestIndexForHeapGroup[heapGroup] = heapRequests.appendNew(HeapRequest{heapGroup, 0u}).first;

  // Bucket resources into requests.
  const uint32_t resMultiply = allEvents.size();
  IdIndexedMapping<HeapIndex, dag::Vector<FrameResource>> resourcesToBeScheduled(heapRequests.size());
  for (auto [group, resCount] : resourcesPerHeapType)
    resourcesToBeScheduled[activeRequestIndexForHeapGroup[group]].reserve(resCount * resMultiply);

  for (auto [resId, resProps] : resources.enumerate())
  {
    // Skip vacant ResNameId
    if (resProps.offsetAlignment == 0)
      continue;
    // sanity check
    G_ASSERT_CONTINUE(resProps.sizeInBytes != 0);
    // Multiply resources. Virtual resources (i.e. ResNameIds) are not
    // in one-to-one correspondence with actual `UniqueRes`es due to
    // the following counterexample.
    // Imagine two nodes, N1 and N2. N1 produces a resource R, and N2
    // requests both R and it's history version. Therefore at the time
    // of N2's execution two `UniqueRes`es that correspond to R must
    // exist, one for current frame version, one for last frame version.
    // This is the reason our scheduling window is 2 frames and the
    // reason we create two packer resources for every ResNameId.
    // Note that in the future we may need to create more copies due
    // to super/subsampling and VR stereo.
    for (uint32_t j = 0; j < resMultiply; ++j)
      resourcesToBeScheduled[activeRequestIndexForHeapGroup[resProps.heapGroup]].push_back({resId, j});
  }

  const uint32_t timepointsPerFrame = allEvents[0].size();

  // New heap requests might appear while iterating, so cannot use
  // range-based for.
  hintedResources.reserve(2 * heapRequests.size());
  for (uint32_t intHeapIndex = 0; intHeapIndex < heapRequests.size(); ++intHeapIndex)
  {
    FRAMEMEM_VALIDATE;

    const auto heapIdx = static_cast<HeapIndex>(intHeapIndex);

    hintedResources.expandMapping(heapIdx);
    const uint32_t resMultiply = allEvents.size();

    // Collect resources present in this heap and build a mapping
    dag::Vector<FrameResource, framemem_allocator> inHeapIdxToFrameResource;
    dag::Vector<PackerInput::Resource, framemem_allocator> packerResources;
    inHeapIdxToFrameResource.reserve(resources.size() * resMultiply);
    packerResources.reserve(resources.size() * resMultiply);

    constexpr uint32_t INDEX_NOT_IN_HEAP = eastl::numeric_limits<uint32_t>::max();
    dag::Vector<uint32_t, framemem_allocator> globalToInHeapIndex(resources.size() * resMultiply, INDEX_NOT_IN_HEAP);

    for (auto resource : resourcesToBeScheduled[heapIdx])
    {
      const auto &resProps = resources[resource.resIdx];

      globalToInHeapIndex[resource.resIdx * resMultiply + resource.frame] = packerResources.size();
      packerResources.push_back(
        {0, 0, static_cast<uint32_t>(resProps.sizeInBytes), static_cast<uint32_t>(resProps.offsetAlignment), PackerInput::NO_HINT});
      inHeapIdxToFrameResource.push_back(resource);
    }

    // Assign start/end timepoints for collected resources
    for (uint32_t i = 0; i < allEvents.size(); ++i)
    {
      const auto &frameEvents = allEvents[i];
      G_ASSERT(frameEvents.size() == timepointsPerFrame);
      for (uint32_t j = 0; j < timepointsPerFrame; ++j)
      {
        const auto &nodeEvents = frameEvents[j];
        const uint32_t time = i * timepointsPerFrame + j;

        for (auto &event : nodeEvents)
        {
          const auto inHeapIndex = globalToInHeapIndex[event.resource * resMultiply + event.frameResourceProducedOn];
          if (inHeapIndex == INDEX_NOT_IN_HEAP)
            continue;

          auto &packerRes = packerResources[inHeapIndex];

          if (eastl::holds_alternative<Event::Activation>(event.data) || eastl::holds_alternative<Event::CpuActivation>(event.data))
            packerRes.start = time;
          else if (
            eastl::holds_alternative<Event::Deactivation>(event.data) || eastl::holds_alternative<Event::CpuDeactivation>(event.data))
            packerRes.end = time;
        }
      }
    }

    bool heapHasHints = false;
    // Assign offset hints for history resources
    for (size_t i = 0; i < packerResources.size(); ++i)
    {
      auto &packerRes = packerResources[i];
      const auto [resIdx, frame] = inHeapIdxToFrameResource[i];
      if (cachedIntermediateResources[resIdx].asScheduled().history == History::No)
        continue;

      const auto preservedResIdx = cached_res_idx_remapping[resIdx];
      const auto preservedRes = hintedResources[heapIdx][frame].find(preservedResIdx);
      const bool hasHint = preservedRes != hintedResources[heapIdx][frame].end();
      const bool hasSameSize = preservedRes->second.size == packerResources[i].size;
      preservedResources[frame][resIdx] = hasHint && hasSameSize;
      if (preservedResources[frame][resIdx])
      {
        packerRes.offsetHint = preservedRes->second.offsetHint;
        heapHasHints = true;
      }
    }

    auto heapGroupProp = getResourceHeapGroupProperties(heapRequests[heapIdx].group);

    PackerInput input{};
    input.timelineSize = allEvents.size() * allEvents[0].size();
    input.resources = packerResources;
    input.maxHeapSize = heapGroupProp.maxHeapSize;

    // When running dx12 on windows, we have a weird additional limit
    // on heap sizes that is not reported by the driver and can only be
    // known by asking MS guys on discord, and it's 64MiBs.
    if (heapGroupProp.optimalMaxHeapSize > 0)
      input.maxHeapSize = eastl::min(input.maxHeapSize, heapGroupProp.optimalMaxHeapSize);

    // Do not increase the size of an existing heap if resources in it have hints.
    // Otherwise hinted resources will not be preserved, because of the heap recreation.
    if (heapHasHints && allocatedHeaps.isMapped(heapIdx) && allocatedHeaps[heapIdx].size != 0)
      input.maxHeapSize = allocatedHeaps[heapIdx].size;

    Packer packer;
    switch (resource_packer)
    {
      case PackerType::Baseline: packer = make_baseline_packer(); break;
      case PackerType::GreedyScanline: packer = make_greedy_scanline_packer(); break;
      case PackerType::Boxing: packer = make_boxing_packer(); break;
      case PackerType::AdHocBoxing: packer = make_adhoc_boxing_packer(); break;
      default: break;
    }

    PackerOutput output;
    {
      TIME_PROFILE(dabfg_resource_packing)
      output = packer(input);
    }

#if DABFG_STATISTICS_REPORTING
    if (report_resource_statistics.get() && heapRequests[heapIdx].group != CPU_HEAP_GROUP)
    {
      for (const auto &res : input.resources)
      {
        auto len = (input.timelineSize + res.end - res.start) % input.timelineSize;
        ++resourceLifetimeDistr[len];
        ++resourceSizeDistr[res.size];
      }
      ++timelineSizeDistr[input.timelineSize];
      ++resultingHeapSizeDistr[output.heapSize];
    }
#endif

    // We allocate one ESRAM heap with the maximum available size,
    // because there is an issue with reallocation ESRAM heap after deallocation.
    if (heapGroupProp.isOnChip)
      output.heapSize = heapGroupProp.maxHeapSize;

    // Collect resources that do not fit into a heap for further rescheduling
    dag::Vector<FrameResource> rescheduledResources;
    heapRequests[heapIdx].size = output.heapSize;
    for (uint32_t idx = 0; idx < output.offsets.size(); ++idx)
    {
      const auto frameRes = inHeapIdxToFrameResource[idx];
      const auto [resIdx, frame] = frameRes;
      if (output.offsets[idx] == PackerOutput::NOT_SCHEDULED)
      {
        rescheduledResources.push_back(frameRes);
        continue;
      }
      // Check if offset hints for history resource are respected
      const auto &packerRes = input.resources[idx];
      if (cachedIntermediateResources[resIdx].asScheduled().history != History::No)
      {
        if (packerRes.offsetHint != PackerInput::NO_HINT && EASTL_UNLIKELY(packerRes.offsetHint != output.offsets[idx]))
        {
          preservedResources[frame][resIdx] = false;
          const auto preservedRes = hintedResources[heapIdx][frame].find(cached_res_idx_remapping[resIdx]);
          // Packer cannot guarantee hint respecting for a resource that has it's lifetime changed.
          const bool hasSameLifetime = preservedRes->second.start == packerRes.start && preservedRes->second.end == packerRes.end;
          if (hasSameLifetime)
            logerr("Resource packer ignored offset hint for history resource!");
        }
        hintedResources[heapIdx][frame][resIdx] = {output.offsets[idx], packerRes.size, packerRes.start, packerRes.end};
      }
      G_FAST_ASSERT(output.offsets[idx] < eastl::numeric_limits<uint32_t>::max());
      allocations[frame][resIdx] = {heapIdx, static_cast<uint32_t>(output.offsets[idx])};
    }

    if (!rescheduledResources.empty())
    {
      // Scan next heap requests for a candidate within same group.
      // If the requested group was ESRAM, reschedule into a regular appropriate group.
      auto rescheduleHeapIdx = heapIdx;
      for (uint32_t j = eastl::to_underlying(heapIdx) + 1; j < heapRequests.size(); ++j)
      {
        const auto candidateHeapIdx = static_cast<HeapIndex>(j);
        const auto candidateHeapGroupProp = getResourceHeapGroupProperties(heapRequests[candidateHeapIdx].group);
        if (heapRequests[candidateHeapIdx].group == heapRequests[heapIdx].group ||
            (heapGroupProp.isOnChip && can_substitute_heap_group(heapGroupProp, candidateHeapGroupProp)))
        {
          rescheduleHeapIdx = candidateHeapIdx;
          break;
        }
      }

      // Reschedule resources to appropriate candidate
      // or create a brand new heap request if existing heaps do not suit.
      if (rescheduleHeapIdx != heapIdx)
      {
        auto &resourcesPerHeap = resourcesToBeScheduled[rescheduleHeapIdx];
        resourcesPerHeap.insert(resourcesPerHeap.end(), rescheduledResources.begin(), rescheduledResources.end());
      }
      else
      {
        auto newHeapRequestGroup = heapRequests[heapIdx].group;
        // If we didn't find candidates for ESRAM resources,
        // then reschedule them into new request of regular heap group.
        if (heapGroupProp.isOnChip)
        {
          const auto desc = cachedIntermediateResources[rescheduledResources.front().resIdx].asScheduled().getGpuDescription();
          newHeapRequestGroup = getResourceAllocationProperties(desc, true).heapGroup;
        }
        heapRequests.push_back(HeapRequest{newHeapRequestGroup, 0u});
        resourcesToBeScheduled.push_back(rescheduledResources);
      }
    }
  }

#if DABFG_STATISTICS_REPORTING
  if (report_resource_statistics.get())
  {
    debug("Frame graph resource allocation statistics report:");

    auto listToStr = [](const auto &list) {
      eastl::string str = "[";
      for (auto [k, v] : list)
      {
        str += "(" + eastl::to_string(k) + ", " + eastl::to_string(v) + "), ";
      }
      if (!list.empty())
        str.resize(str.size() - 2);
      str += "]";
      return str;
    };

    static int cumulativeDumpCounter = 0;
    if (++cumulativeDumpCounter % 100 == 0)
    {
      debug("Cumulative timeline size distribution: %s", listToStr(timelineSizeDistr));
      debug("Cumulative allocation lifetime distribution: %s", listToStr(resourceLifetimeDistr));
      debug("Cumulative allocation size distribution: %s", listToStr(resourceSizeDistr));
      debug("Cumulative heap size distribution: %s", listToStr(resultingHeapSizeDistr));
    }

    for (auto [heapIdx, heap] : heapRequests.enumerate())
      debug("Heap request %d size: %d bytes", eastl::to_underlying(heapIdx), heap.size);
  }
#endif

  return result;
}

ResourceScheduler::EventsCollectionRef ResourceScheduler::scheduleEvents(const intermediate::Graph &graph)
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

  for (uint32_t frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
    for (auto [resIdx, timeline] : perFrameResourceUsageTimelines[frame].enumerate())
    {
      // Carefully place split or regular barriers between usage
      // occurrences that yield one
      const auto &resource = graph.resources[resIdx];
      if (resource.getResType() != ResourceType::Blob)
      {
        int lastState = timeline.size() - 1;
        for (int i = timeline.size() - 2; i >= 0; --i)
        {
          const auto &curr = timeline[i];
          const auto &prev = timeline[eastl::exchange(lastState, i)];

          ResourceBarrier barrier = barrier_for_transition(prev.usage, curr.usage);

          if (barrier == RB_NONE)
            continue;

          const uint32_t eventAfterPreviousNode = prev.nodeIndex + 1;
          const uint32_t eventBeforeCurrentNode = curr.nodeIndex;

          const Event barrierEvent{resIdx, frame, Event::Barrier{barrier}};
          auto &frameEvents = scheduledEvents[prev.frame];

          auto tryRecBarrierForDebug = //
            [&, resIdx = resIdx](int time, ResourceBarrier additional_flags) {
              if (debug_graph_generation.get())
              {
                // NOTE: barriers are executed on physical resources,
                // while debug visualizer works with virtual resources.
                // We could try and find the "correct" virtual resource to
                // use for this physical res at the specified time, but it's
                // pointless, as the visualization won't look any different,
                // so we lie about it.
                auto resId = *graph.resources[resIdx].frontendResources.begin();
                debug_rec_resource_barrier(resId, frame, time, prev.frame, barrier | additional_flags);
              }
            };

          if (prev.frame != curr.frame)
          {
            // NOTE: split barriers shouldn't be used between frames,
            // place a regular barrier at the end of prev frame.
            frameEvents.back().emplace_back(barrierEvent);

            tryRecBarrierForDebug(eventAfterPreviousNode, {});
          }
          else if (prev.nodeIndex + 1 == curr.nodeIndex || resource.getResType() == ResourceType::Buffer)
          {
            frameEvents[eventAfterPreviousNode].emplace_back(barrierEvent);

            tryRecBarrierForDebug(eventAfterPreviousNode, {});
          }
          else
          {
            Event beginBarrierEvent = barrierEvent;
            beginBarrierEvent.data = Event::Barrier{barrier | RB_FLAG_SPLIT_BARRIER_BEGIN};
            frameEvents[eventAfterPreviousNode].emplace_back(beginBarrierEvent);

            Event endBarrierEvent = barrierEvent;
            endBarrierEvent.data = Event::Barrier{barrier | RB_FLAG_SPLIT_BARRIER_END};
            frameEvents[eventBeforeCurrentNode].emplace_back(endBarrierEvent);

            tryRecBarrierForDebug(eventAfterPreviousNode, RB_FLAG_SPLIT_BARRIER_BEGIN);
            tryRecBarrierForDebug(eventBeforeCurrentNode, RB_FLAG_SPLIT_BARRIER_END);
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
          payload = Event::CpuDeactivation{scheduledRes.getCpuDescription().deactivate};

        scheduledEvents[lastUsageOccurrence.frame][lastUsageOccurrence.nodeIndex + 1].emplace_back(
          Event{resIdx, static_cast<uint32_t>(frame), payload});
      }

      {
        const auto &firstUsageOccurrence = perFrameResourceUsageTimelines[frame][resIdx].back();

        Event::Payload payload;
        if (resource.asScheduled().isGpuResource())
        {
          const auto &basicResDesc = graph.resources[resIdx].asScheduled().getGpuDescription().asBasicRes;
          payload = Event::Activation{basicResDesc.activation, basicResDesc.clearValue};
        }
        else
        {
          payload = Event::CpuActivation{resource.asScheduled().getCpuDescription().activate};
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

const D3dResource *ResourceScheduler::getD3dResource(int frame, intermediate::ResourceIndex res_idx) const
{
  const D3dResource *res = nullptr;
  switch (cachedIntermediateResources[res_idx].asScheduled().resourceType)
  {
    case ResourceType::Blob:
    case ResourceType::Invalid: res = nullptr; break;
    case ResourceType::Texture: res = getTexture(frame, res_idx).getBaseTex(); break;
    case ResourceType::Buffer: res = getBuffer(frame, res_idx).getBuf(); break;
  }
  return res;
}

ResourceScheduler::DestroyedHeapSet ResourceScheduler::allocateCpuHeaps(int prev_frame, const HeapRequests &requests,
  const PotentialDeactivationSet &potential_deactivation_set)
{
  DestroyedHeapSet result;
  result.reserve(SCHEDULE_FRAME_WINDOW * cachedIntermediateResources.size());

  FRAMEMEM_VALIDATE;


  for (const auto [heapIdx, req] : requests.enumerate())
  {
    if (req.group != CPU_HEAP_GROUP)
      continue;

    cpuHeaps.expandMapping(heapIdx);

    // sanity check
    G_ASSERT(allocatedHeaps[heapIdx].size == cpuHeaps[heapIdx].size());

    if (req.size <= cpuHeaps[heapIdx].size())
      continue;

    if (!cpuHeaps[heapIdx].empty())
    {
      // If we are relocating a CPU heap, we need to call destructors
      // on all CPU resources present in it. Note that resource
      // scheduling guarantees that all of these resources are NOT
      // preserved.
      // Also note that this is not the case for GPU heaps, there,
      // deactivation is a simple memory barrier, which we don't in fact
      // need or want.
      for (const auto resIdx : heapToResourceList[heapIdx][prev_frame])
        if (potential_deactivation_set[resIdx].index() == 3)
        {
          auto [f, x] = eastl::get<3>(potential_deactivation_set[resIdx]);
          f(x);
        }

      result.push_back(heapIdx);

      for (auto &heapHintedResources : hintedResources[heapIdx])
        heapHintedResources.clear();
    }
    else // sanity check
      G_ASSERT(!heapToResourceList.isMapped(heapIdx) || heapToResourceList[heapIdx][prev_frame].empty());

    cpuHeaps[heapIdx].resize(req.size, 0);
    allocatedHeaps[heapIdx].size = req.size;
  }

  return result;
}

void ResourceScheduler::placeResources(const intermediate::Graph &graph, const ResourceProperties &resource_properties,
  const AllocationLocations &allocation_locations)
{
  TIME_PROFILE(placeResources);

  heapToResourceList.resize(allocatedHeaps.size());

  for (int frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
  {
    IdIndexedMapping<HeapIndex, uint32_t> resCountPerHeap(allocatedHeaps.size(), 0);
    for (auto resIdx : IdRange<intermediate::ResourceIndex>(graph.resources.size()))
      if (graph.resources[resIdx].isScheduled())
        ++resCountPerHeap[allocation_locations[frame][resIdx].heap];

    for (auto [heapIdx, count] : resCountPerHeap.enumerate())
    {
      heapToResourceList[heapIdx][frame].clear();
      heapToResourceList[heapIdx][frame].reserve(count);
    }
  }

  for (auto [i, res] : graph.resources.enumerate())
  {
    if (!res.isScheduled())
      continue;

    const auto &resProps = resource_properties[i];

    G_ASSERT(resProps.offsetAlignment != 0);

    for (int frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
    {
      auto [heapIdx, offset] = allocation_locations[frame][i];

      heapToResourceList[heapIdx][frame].push_back(i);

      if (res.asScheduled().isGpuResource())
      {
        placeResource(frame, i, heapIdx, res.asScheduled().getGpuDescription(), offset, resProps);

        validation_add_resource(getD3dResource(frame, i));
      }
      else
      {
        resourceIndexInCollection[frame][i] = offset;
        heapForCpuResource[frame][i] = heapIdx;
      }

      if (debug_graph_generation.get())
      {
        for (auto resId : cachedIntermediateResources[i].frontendResources)
          debug_rec_resource_placement(resId, frame, eastl::to_underlying(heapIdx), offset, resProps.sizeInBytes);
      }
    }
  }
  if (set_resource_names.get())
    restoreResourceApiNames();
}

void ResourceScheduler::restoreResourceApiNames() const
{
  FRAMEMEM_REGION;

  struct strInfo
  {
    size_t size;
    eastl::basic_string<char, framemem_allocator> str;
  };

  ska::flat_hash_map<const D3dResource *, strInfo, eastl::hash<const D3dResource *>, eastl::equal_to<const D3dResource *>,
    framemem_allocator>
    names;
  names.reserve(SCHEDULE_FRAME_WINDOW * cachedIntermediateResources.size());

  const char *separatorStr = ", ";
  // node: size of elements should be equal
  const char *frameStr[SCHEDULE_FRAME_WINDOW] = {"|0", "|1"};
  const size_t frameStrSize = strlen(frameStr[0]);
  const size_t separatorStrSize = strlen(separatorStr);

  for (int frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
    for (auto resIdx : IdRange<intermediate::ResourceIndex>(cachedIntermediateResources.size()))
    {
      if (!cachedIntermediateResources[resIdx].isScheduled())
        continue;

      const D3dResource *pResource = getD3dResource(frame, resIdx);
      if (pResource != nullptr)
      {
        auto &strRef = names[pResource];
        if (strRef.size != 0)
          strRef.size += separatorStrSize;
        strRef.size += cachedIntermediateResourceNames[resIdx].length() + frameStrSize;
      }
    }

  for (auto &[d3dRes, str] : names)
    names[d3dRes].str.reserve(str.size);

  for (int frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
    for (auto resIdx : IdRange<intermediate::ResourceIndex>(cachedIntermediateResources.size()))
    {
      if (!cachedIntermediateResources[resIdx].isScheduled())
        continue;

      const D3dResource *pResource = getD3dResource(frame, resIdx);
      if (pResource != nullptr)
      {
        auto &name = names[pResource];
        if (!name.str.empty())
          name.str += separatorStr;
        name.str += cachedIntermediateResourceNames[resIdx].c_str();
        name.str += frameStr[frame];
      }
    }

  for (auto &[d3dRes, name] : names)
    d3dRes->setResApiName(name.str.c_str());
}

ResourceScheduler::PotentialDeactivationSet ResourceScheduler::gatherPotentialDeactivationSet(int prev_frame)
{
  PotentialDeactivationSet result;
  result.resize(cachedIntermediateResources.size());

  for (auto [idx, res] : cachedIntermediateResources.enumerate())
    if (res.isScheduled() && res.asScheduled().history != History::No)
      switch (res.asScheduled().resourceType)
      {
        case ResourceType::Texture: result[idx] = getTexture(prev_frame, idx).getBaseTex(); break;
        case ResourceType::Buffer: result[idx] = getBuffer(prev_frame, idx).getBuf(); break;
        case ResourceType::Blob:
          result[idx] = BlobDeactivationRequest{res.asScheduled().getCpuDescription().deactivate, getBlob(prev_frame, idx).data};
          break;
        default: G_ASSERT(false); break;
      }

  return result;
}

void ResourceScheduler::closeTransientResources()
{
  TIME_PROFILE(closeTransientResources);
  validation_restart();
  eventStorage.clear();
}

void ResourceScheduler::shutdown(int frame)
{
  // Call destructor on all CPU resources. Asan complains otherwise.
  for (auto [i, res] : cachedIntermediateResources.enumerate())
    if (res.isScheduled() && res.asScheduled().history != History::No && res.asScheduled().isCpuResource())
      res.asScheduled().getCpuDescription().deactivate(getBlob(frame, i).data);
  cpuHeaps.clear();

  for (int i = 0; i < SCHEDULE_FRAME_WINDOW; ++i)
  {
    eastl::fill(resourceIndexInCollection[i].begin(), resourceIndexInCollection[i].end(), UNSCHEDULED);
    eastl::fill(heapForCpuResource[i].begin(), heapForCpuResource[i].end(), HeapIndex::Invalid);
    preservedResources[i].clear();
  }
  hintedResources.clear();

  shutdownInternal();

  heapToResourceList.clear();
  allocatedHeaps.clear();
  cachedIntermediateResources.clear();
}

ResourceScheduler::IntermediateRemapping ResourceScheduler::remapResources(const IntermediateResources &new_resources) const
{
  TIME_PROFILE(Remapping resource);

  IntermediateRemapping remapping;
  remapping.resize(eastl::max(new_resources.size(), cachedIntermediateResources.size()), intermediate::RESOURCE_NOT_MAPPED);

  FRAMEMEM_VALIDATE;

  dag::VectorMap<ResNameId, intermediate::ResourceIndex, eastl::less<ResNameId>, framemem_allocator> resNameToIR;
  resNameToIR.reserve(eastl::max<size_t>(new_resources.size(), cachedIntermediateResources.size() * 8));

  for (auto [oldResIdx, res] : cachedIntermediateResources.enumerate())
    for (const auto resNameId : res.frontendResources)
      resNameToIR[resNameId] = oldResIdx;

  for (auto [newResIdx, res] : new_resources.enumerate())
  {
    if (remapping[newResIdx] != intermediate::RESOURCE_NOT_MAPPED)
      continue;
    for (const auto resNameId : res.frontendResources)
    {
      const auto cachedResIdx = resNameToIR.find(resNameId);
      if (cachedResIdx != resNameToIR.end() &&
          cachedIntermediateResources[cachedResIdx->second].multiplexingIndex == res.multiplexingIndex)
        remapping[newResIdx] = cachedResIdx->second;
    }
  }

  return remapping;
}

ResourceScheduler::SchedulingResult ResourceScheduler::scheduleResources(int prev_frame, const intermediate::Graph &graph)
{
  SchedulingResult result;
  result.deactivations.reserve(cachedIntermediateResources.size());

  FRAMEMEM_VALIDATE;

  auto potentialDeactivations = gatherPotentialDeactivationSet(prev_frame);

  closeTransientResources();

  const auto cachedResIdxRemapping = remapResources(graph.resources);

  cachedIntermediateResources = graph.resources;
  cachedIntermediateResourceNames = graph.resourceNames;

  const auto resourceProperties = gatherResourceProperties(graph);

  result.events = scheduleEvents(graph);

  auto [allocationLocations, heapRequests] = scheduleResourcesIntoHeaps(resourceProperties, result.events, cachedResIdxRemapping);

  // Don't deactivate resources that were preserved
  for (auto [idx, res] : cachedIntermediateResources.enumerate())
    if (isResourcePreserved(prev_frame, idx))
      potentialDeactivations[cachedResIdxRemapping[idx]] = eastl::monostate{};

  const auto oldHeapCount = allocatedHeaps.size();

  // Start tracking newly requested heap groups
  if (allocatedHeaps.size() < heapRequests.size())
    for (auto it = heapRequests.begin() + allocatedHeaps.size(); it != heapRequests.end(); ++it)
      allocatedHeaps.appendNew(HeapRequest{it->group, 0u});

  {
    // allocateHeaps might destroy some physical resources due to
    // their heap being re-allocated, so don't try to deactivate those
    // (otherwise we'll get assertions and maybe a crash)
    const auto wipeDeactivations = [this, prev_frame, &potentialDeactivations](const DestroyedHeapSet &set) {
      for (auto heapIdx : set)
        for (auto resIdx : heapToResourceList[heapIdx][prev_frame])
          potentialDeactivations[resIdx] = eastl::monostate{};
    };

    {
      const auto destroyedGpuHeaps = allocateHeaps(heapRequests);
      // NOTE: this method is responsible for updating allocatedHeaps size
      wipeDeactivations(destroyedGpuHeaps);
    }

    {
      const auto destroyedCpuHeaps = allocateCpuHeaps(prev_frame, heapRequests, potentialDeactivations);
      wipeDeactivations(destroyedCpuHeaps);
    }
  }

  for (int i = 0; i < SCHEDULE_FRAME_WINDOW; ++i)
  {
    resourceIndexInCollection[i].assign(cachedIntermediateResources.size(), UNSCHEDULED);
    heapForCpuResource[i].assign(cachedIntermediateResources.size(), HeapIndex::Invalid);
  }

  placeResources(graph, resourceProperties, allocationLocations);

  // Outside world doesn't need to know old resource indices, so repack
  // the deactivation map into a regular array.
  for (auto [idx, res] : cachedIntermediateResources.enumerate())
    if (const auto oldIdx = cachedResIdxRemapping[idx]; oldIdx != intermediate::RESOURCE_NOT_MAPPED)
      eastl::visit(
        [&result](const auto &res) {
          if constexpr (!eastl::is_same_v<eastl::remove_cvref_t<decltype(res)>, eastl::monostate>)
            result.deactivations.emplace_back(res);
        },
        potentialDeactivations[oldIdx]);

  for (auto i = oldHeapCount; i < heapToResourceList.size(); ++i)
    if (const auto &list = heapToResourceList[static_cast<HeapIndex>(i)][0]; list.size() == 1)
      logwarn("Heap %d had to be created for containing a single resource '%s'", i, cachedIntermediateResourceNames[list[0]].c_str());

  return result;
}

BlobView ResourceScheduler::getBlob(int frame, intermediate::ResourceIndex res_idx)
{
  const auto offset = resourceIndexInCollection[frame][res_idx];
  const auto heapIdx = heapForCpuResource[frame][res_idx];
  G_ASSERT_RETURN(offset != UNSCHEDULED && heapIdx != HeapIndex::Invalid, {});

  return BlobView{cpuHeaps[heapIdx].data() + offset, cachedIntermediateResources[res_idx].asScheduled().getCpuDescription().typeTag};
}

bool ResourceScheduler::isResourcePreserved(int frame, intermediate::ResourceIndex res_idx) const
{
  G_ASSERT(frame < preservedResources.size());
  if (!preservedResources[frame].isMapped(res_idx))
    return false;

  return preservedResources[frame][res_idx];
}

} // namespace dabfg
