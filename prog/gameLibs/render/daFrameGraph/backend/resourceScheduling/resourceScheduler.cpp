// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "resourceScheduler.h"

#include <EASTL/algorithm.h>
#include <EASTL/bitvector.h>
#include <EASTL/sort.h>

#include "generic/dag_reverseView.h"
#include <perfMon/dag_statDrv.h>
#include <util/dag_convar.h>
#include <dag/dag_vectorSet.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>

#include <common/resourceUsage.h>
#include <common/genericPoint.h>
#include <id/idRange.h>
#include <backend/resourceScheduling/packer.h>
#include <debug/backendDebug.h>


#if DAFG_STATISTICS_REPORTING
CONSOLE_BOOL_VAL("dafg", report_resource_statistics, false);
#endif

CONSOLE_BOOL_VAL("dafg", dump_resources, false);

CONSOLE_INT_VAL("dafg", resource_packer, dafg::PackerType::GreedyScanline, 0, dafg::PackerType::COUNT - 1);

namespace dafg
{

static bool can_substitute_heap_group(ResourceHeapGroupProperties origin, ResourceHeapGroupProperties candidate)
{
  const bool sameFlags = candidate.flags == origin.flags;
  const bool sameCPUVisibility = candidate.isCPUVisible == origin.isCPUVisible;
  const bool sameGPULocal = candidate.isGPULocal == origin.isGPULocal;
  return sameFlags && sameCPUVisibility && sameGPULocal;
}

static uint64_t corrected_resource_size(const ResourceProperties &resources, const BadResolutionTracker::Corrections &corrections,
  intermediate::ResourceIndex idx, uint32_t frame)
{
  return corrections[frame][idx] != 0 ? corrections[frame][idx] : resources[idx].sizeInBytes;
}

struct HeapSchedulingResult
{
  dag::Vector<FrameResource> rescheduledResources;
  bool heapBecameUnused = false;
};

auto ResourceScheduler::gatherResourceProperties(const intermediate::Graph &graph,
  IResourcePropertyProvider &property_provider) -> ResourceProperties
{
  ResourceProperties resourceProperties(graph.resources.totalKeys(), ResourceAllocationProperties{});

  auto getFakePropertiesForCpu = [](const intermediate::BlobDescription &desc) {
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
            invalidateTemporalResources();

          historyResourceFlags[resNameId] = newFlags;
        }
      }
      resourceProperties[i] = res.asScheduled().isGpuResource()
                                ? property_provider.getResourceAllocationProperties(res.asScheduled().getGpuDescription())
                                : getFakePropertiesForCpu(res.asScheduled().getCpuDescription());
    }
  return resourceProperties;
}

auto ResourceScheduler::initializeHeapRequests(const ResourceProperties &resources,
  const SchedulingContext &ctx) -> eastl::pair<HeapRequests, ActiveHeapRequestMap>
{
  // First, gather all unique heap types and count the amount of resources
  // assigned to them
  // ESRAM requests should be first so that we can reschedule
  // non-fitting resources into regular heaps.
  const auto heapGroupComp = [&ctx](ResourceHeapGroup *h1, ResourceHeapGroup *h2) {
    const auto heapGroupProp1 = ctx.propertyProvider.getResourceHeapGroupProperties(h1);
    const auto heapGroupProp2 = ctx.propertyProvider.getResourceHeapGroupProperties(h2);
    if (heapGroupProp1.isDedicatedFastGPULocal != heapGroupProp2.isDedicatedFastGPULocal)
      return heapGroupProp1.isDedicatedFastGPULocal > heapGroupProp2.isDedicatedFastGPULocal;
    return eastl::less<ResourceHeapGroup *>()(h1, h2);
  };
  dag::VectorMap<ResourceHeapGroup *, uint32_t, decltype(heapGroupComp), framemem_allocator> resourcesPerHeapType(heapGroupComp);
  // TODO: remove when dag::Vector::DoInsertValue starts using resizeInplace
  resourcesPerHeapType.reserve(resources.size());
  for (const auto &resProps : resources)
    if (resProps.offsetAlignment != 0)
      ++resourcesPerHeapType[resProps.heapGroup]; //-V552
  resourcesPerHeapType.shrink_to_fit();

  HeapRequests heapRequests;
  ActiveHeapRequestMap activeRequestsForGroup;

  // The request into which resources with this heap group should go into.
  // Gets changed to a new heap request if we run out of memory
  // Hope that we'll need <=2 actual heaps per group
  activeRequestsForGroup.reserve(resourcesPerHeapType.size() * 2);
  heapRequests.reserve(resourcesPerHeapType.size() * 2);

  // Initially, point all previously existing groups into the first
  // previously available heap
  for (auto [idx, heap] : ctx.allocatedHeaps.enumerate())
  {
    if (activeRequestsForGroup.find(heap.group) == activeRequestsForGroup.end())
      activeRequestsForGroup.emplace(heap.group, idx);
    heapRequests.appendNew(HeapRequest{heap.group, 0u});
  }

  // Create new requests for heap groups that were not previously available
  for (auto [heapGroup, _] : resourcesPerHeapType)
    if (activeRequestsForGroup.find(heapGroup) == activeRequestsForGroup.end())
      activeRequestsForGroup[heapGroup] = heapRequests.appendNew(HeapRequest{heapGroup, 0u}).first;

  return {eastl::move(heapRequests), eastl::move(activeRequestsForGroup)};
}

auto ResourceScheduler::bucketResourcesIntoHeaps(int prev_frame, const ResourceProperties &resources,
  ActiveHeapRequestMap &active_request_for_group, HeapRequests &heap_requests, const AlreadyScheduled &already_scheduled,
  const ReusedHeaps &reused_heaps, const PreviousAllocations &previous_allocations,
  const SchedulingContext &ctx) -> IdIndexedMapping<HeapIndex, dag::Vector<FrameResource>>
{
  IdIndexedMapping<HeapIndex, dag::Vector<FrameResource>> resourcesToBeScheduled(heap_requests.size());
  const uint32_t preserveProducedOnFrame = prev_frame % SCHEDULE_FRAME_WINDOW;

  for (auto [resIdx, resProps] : resources.enumerate())
  {
    // Skip vacant ResNameId
    if (resProps.offsetAlignment == 0)
      continue;
    // sanity check
    G_ASSERT_CONTINUE(resProps.sizeInBytes != 0);
    for (uint32_t frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
    {
      // Skip frame copies already claimed by reused heaps
      if (already_scheduled[frame].test(resIdx, false))
        continue;
      HeapIndex desiredHeap = active_request_for_group.find(resProps.heapGroup)->second;

      if (preserveProducedOnFrame == frame && ctx.cachedResources[resIdx].asScheduled().history != History::No)
      {
        const auto preservedResIdx = ctx.historyPairing[resIdx];
        if (preservedResIdx != intermediate::RESOURCE_NOT_MAPPED && previous_allocations[frame].isMapped(preservedResIdx))
        {
          const auto &prevLoc = previous_allocations[frame][preservedResIdx];
          if (ctx.allocatedHeaps.isMapped(prevLoc.heap) && ctx.allocatedHeaps[prevLoc.heap].group == resProps.heapGroup)
          {
            const uint64_t prevSize =
              cachedCorrectedSizes[frame].isMapped(preservedResIdx) ? cachedCorrectedSizes[frame][preservedResIdx] : 0;
            if (prevSize == corrected_resource_size(resources, ctx.corrections, resIdx, frame))
            {
              desiredHeap = prevLoc.heap;
            }
          }
        }
      }

      // Don't bucket into a reused heap
      if (reused_heaps.test(desiredHeap, false))
      {
        // Find another heap of the same group that isn't reused
        bool foundFallback = false;
        for (auto [candidateIdx, req] : heap_requests.enumerate())
          if (req.group == resProps.heapGroup && !reused_heaps.test(candidateIdx, false))
          {
            desiredHeap = candidateIdx;
            foundFallback = true;
            break;
          }
        // If every heap of this group is reused, append a new request -- bucketing
        // into a reused heap would silently drop the resource, since reused heaps
        // are excluded from scheduling.
        if (!foundFallback)
        {
          desiredHeap = heap_requests.appendNew(HeapRequest{resProps.heapGroup, 0u}).first;
          resourcesToBeScheduled.push_back({});
          active_request_for_group[resProps.heapGroup] = desiredHeap;
        }
      }

      resourcesToBeScheduled[desiredHeap].push_back({resIdx, frame});
    }
  }
  return resourcesToBeScheduled;
}

HeapSchedulingResult ResourceScheduler::scheduleHeap(HeapIndex heap_idx, eastl::span<const FrameResource> resources_in_heap,
  uint32_t preserve_produced_on_frame, uint32_t timepoints_per_frame, bool allow_preservation, const ResourceProperties &resources,
  const PreviousAllocations &previous_allocations, ResourceSchedule &result, const SchedulingContext &ctx)
{
  HeapSchedulingResult heapResult;

  FRAMEMEM_VALIDATE;

  auto &allocations = result.allocationLocations;
  auto &heapRequests = result.heapRequests;

  // Collect resources present in this heap and build a mapping
  dag::Vector<FrameResource, framemem_allocator> inHeapIdxToFrameResource;
  dag::Vector<PackerInput::Resource, framemem_allocator> packerResources;
  inHeapIdxToFrameResource.reserve(resources.size() * SCHEDULE_FRAME_WINDOW);
  packerResources.reserve(resources.size() * SCHEDULE_FRAME_WINDOW);

  constexpr uint32_t INDEX_NOT_IN_HEAP = eastl::numeric_limits<uint32_t>::max();
  dag::Vector<uint32_t, framemem_allocator> globalToInHeapIndex(resources.size() * SCHEDULE_FRAME_WINDOW, INDEX_NOT_IN_HEAP);

  for (auto resource : resources_in_heap)
  {
    const auto &resProps = resources[resource.resIdx];

    globalToInHeapIndex[resource.resIdx * SCHEDULE_FRAME_WINDOW + resource.frame] = packerResources.size();
    packerResources.push_back({0, 0, corrected_resource_size(resources, ctx.corrections, resource.resIdx, resource.frame),
      resProps.offsetAlignment, PackerInput::NO_PIN});
    inHeapIdxToFrameResource.push_back(resource);
  }

  // Assign start/end timepoints for collected resources
  for (uint32_t i = 0; i < SCHEDULE_FRAME_WINDOW; ++i)
  {
    const auto &frameEvents = ctx.events[i];
    G_ASSERT(frameEvents.totalKeys() == timepoints_per_frame);
    for (uint32_t j = 0; j < timepoints_per_frame; ++j)
    {
      const auto nodeIdx = static_cast<intermediate::NodeIndex>(j);
      if (!frameEvents.isMapped(nodeIdx))
        continue;

      const auto &nodeEvents = frameEvents[nodeIdx];
      const uint32_t time = i * timepoints_per_frame + j;

      for (auto &event : nodeEvents)
      {
        const auto inHeapIndex = globalToInHeapIndex[event.resource * SCHEDULE_FRAME_WINDOW + event.frameResourceProducedOn];
        if (inHeapIndex == INDEX_NOT_IN_HEAP)
          continue;

        auto &packerRes = packerResources[inHeapIndex];

        const uint32_t newStart = ((preserve_produced_on_frame + 1) % SCHEDULE_FRAME_WINDOW) * timepoints_per_frame;
        const uint32_t timelineLength = SCHEDULE_FRAME_WINDOW * timepoints_per_frame;

        const auto shiftToPreservePoint = [&](uint32_t t) { return (t + newStart) % timelineLength; };

        if (eastl::holds_alternative<BarrierScheduler::Event::Activation>(event.data) ||
            eastl::holds_alternative<BarrierScheduler::Event::CpuActivation>(event.data))
          packerRes.start = shiftToPreservePoint(time);
        else if (eastl::holds_alternative<BarrierScheduler::Event::Deactivation>(event.data) ||
                 eastl::holds_alternative<BarrierScheduler::Event::CpuDeactivation>(event.data))
          packerRes.end = shiftToPreservePoint(time);
      }
    }
  }


  bool preserveHeap = false;
  if (allow_preservation)
  {
    // Try and preserve the contents of the history for resources in
    // this heap to avoid bugs in temporal algorithms.
    for (size_t i = 0; i < packerResources.size(); ++i)
    {
      auto &packerRes = packerResources[i];
      const auto [resIdx, frame] = inHeapIdxToFrameResource[i];

      if (frame != preserve_produced_on_frame)
        continue;

      if (ctx.cachedResources[resIdx].asScheduled().history == History::No)
        continue;

      const auto preservedResIdx = ctx.historyPairing[resIdx];
      if (preservedResIdx != intermediate::RESOURCE_NOT_MAPPED && previous_allocations[frame].isMapped(preservedResIdx))
      {
        const auto &prevLoc = previous_allocations[frame][preservedResIdx];
        const uint64_t prevSize =
          cachedCorrectedSizes[frame].isMapped(preservedResIdx) ? cachedCorrectedSizes[frame][preservedResIdx] : 0;
        if (prevLoc.heap == heap_idx && prevSize == packerRes.size)
        {
          // We managed to
          // a) pair the IR resources correctly,
          // b) find a location where this resource is preserved,
          // c) ensure that the size of the resource did not change,
          // and that means we can actually reuse the existing d3d resource
          // and preserve the history.
          packerRes.pin = prevLoc.offset;
          preserveHeap = true;
          preservedResources[frame][resIdx] = true;
        }
      }
    }
  }

  auto heapGroupProp = ctx.propertyProvider.getResourceHeapGroupProperties(heapRequests[heap_idx].group);

  PackerInput input{};
  input.timelineSize = SCHEDULE_FRAME_WINDOW * timepoints_per_frame;
  input.resources = packerResources;
  input.maxHeapSize = heapGroupProp.maxHeapSize;

  // When running dx12 on windows, we have a weird additional limit
  // on heap sizes that is not reported by the driver and can only be
  // known by asking MS guys on discord, and it's 64MiBs.
  if (heapGroupProp.optimalMaxHeapSize > 0)
  {
    // The funnest part of the whole thing: the 64MiB limit is NOT big
    // enough for 4K HDR monitors! 3840*2160*8 + metadata is just above
    // this limit, so we fall back to smallest resource size in this
    // case, so as to pack at LEAST a single resource.
    // This is really fubar.
    const auto sizeCmp = [](const auto &a, const auto &b) { return a.size < b.size; };
    const auto minResourceSize = eastl::min_element(packerResources.begin(), packerResources.end(), sizeCmp)->size;

    const auto permissibleMaxSize = eastl::max(heapGroupProp.optimalMaxHeapSize, minResourceSize);
    input.maxHeapSize = eastl::min(input.maxHeapSize, permissibleMaxSize);
  }

  // Prohibit increasing heap size if we want to preserve resources in it.
  if (preserveHeap && ctx.allocatedHeaps.isMapped(heap_idx) && ctx.allocatedHeaps[heap_idx].size != 0)
    input.maxHeapSize = ctx.allocatedHeaps[heap_idx].size;

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
    TIME_PROFILE(daframegraph_resource_packing)
    output = packer(input);
  }

#if DAFG_STATISTICS_REPORTING
  if (report_resource_statistics.get() && heapRequests[heap_idx].group != CPU_HEAP_GROUP)
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

  {
    uint64_t totalResMemory = 0;
    uint64_t totalResArea = 0;
    for (uint32_t i = 0; i < input.resources.size(); ++i)
      if (output.offsets[i] != PackerOutput::NOT_SCHEDULED)
      {
        const auto &res = input.resources[i];
        totalResMemory += res.size;
        totalResArea += (res.size * (res.end + (res.end <= res.start) * input.timelineSize - res.start)) >> 10;
      }
    const uint64_t totalArea = (output.heapSize * input.timelineSize) >> 10;
    const float occupancy = totalArea > 0 ? static_cast<float>(totalResArea) / static_cast<float>(totalArea) : 1;
    const float aliasingRatio = output.heapSize > 0 ? static_cast<float>(totalResMemory) / static_cast<float>(output.heapSize) : 1;

    heapStatistics.set(heap_idx, eastl::make_pair(occupancy, aliasingRatio));
  }

  if (dump_resources.get())
  {
    if (!allow_preservation)
      debug("daFG: dump of resources in heap %d (reuse of empty heap)", eastl::to_underlying(heap_idx));
    else
      debug("daFG: dump of resources in heap %d", eastl::to_underlying(heap_idx));
    debug("daFG: name\tarea (KB)\tstart\tend\tsize\toffset", eastl::to_underlying(heap_idx));
    for (uint32_t i = 0; i < input.resources.size(); ++i)
    {
      if (output.offsets[i] == PackerOutput::NOT_SCHEDULED)
        continue;

      const auto &res = input.resources[i];
      const auto frameRes = inHeapIdxToFrameResource[i];
      const auto area = (res.size * (res.end + (res.end <= res.start) * input.timelineSize - res.start)) >> 10;
      debug("daFG: %s\t%d\t%d\t%d\t%d\t%d", ctx.cachedResourceNames[frameRes.resIdx], area, res.start, res.end, res.size,
        output.offsets[i]);
    }
  }

  // We allocate one ESRAM heap with the maximum available size,
  // because there is an issue with reallocation ESRAM heap after deallocation.
  if (heapGroupProp.isDedicatedFastGPULocal)
    output.heapSize = heapGroupProp.maxHeapSize;

  // Collect resources that do not fit into a heap for further rescheduling
  G_ASSERT(!preserveHeap || output.heapSize <= ctx.allocatedHeaps[heap_idx].size);
  // Prohibit decreasing size of preserved heaps
  heapRequests[heap_idx].size = preserveHeap ? ctx.allocatedHeaps[heap_idx].size : output.heapSize;
  for (uint32_t idx = 0; idx < output.offsets.size(); ++idx)
  {
    const auto frameRes = inHeapIdxToFrameResource[idx];
    const auto [resIdx, frame] = frameRes;
    if (output.offsets[idx] == PackerOutput::NOT_SCHEDULED)
    {
      heapResult.rescheduledResources.push_back(frameRes);

      if (input.resources[idx].pin != PackerInput::NO_PIN)
      {
        preservedResources[frame][resIdx] = false;
        logerr("daFG: Resource packer failed to pin a preserved resource!");
      }

      continue;
    }
    const auto &packerInputRes = input.resources[idx];
    if (ctx.cachedResources[resIdx].asScheduled().history != History::No)
    {
      // Defensive programming: check that the packer didn't mess up and
      // respected all out hints.
      if (packerInputRes.pin != PackerInput::NO_PIN && DAGOR_UNLIKELY(packerInputRes.pin != output.offsets[idx]))
      {
        preservedResources[frame][resIdx] = false;
        logerr("daFG: Resource packer failed to pin a preserved resource!");
      }
    }
    G_ASSERT(output.offsets[idx] + input.resources[idx].size <= heapRequests[heap_idx].size);
    allocations[frame][resIdx] = {heap_idx, output.offsets[idx]};
  }

  if (heapRequests[heap_idx].size == 0 && ctx.allocatedHeaps.isMapped(heap_idx))
    heapResult.heapBecameUnused = true;

  return heapResult;
}

void ResourceScheduler::rescheduleOverflowResources(eastl::span<const FrameResource> rescheduled_resources, HeapIndex source_heap_idx,
  eastl::span<const HeapIndex> remaining_heaps, ResourceHeapGroupProperties source_heap_group_prop,
  IdIndexedMapping<HeapIndex, dag::Vector<FrameResource>> &resources_to_be_scheduled, const HeapRequests &heap_requests,
  dag::Vector<HeapRequest, framemem_allocator> &new_heap_requests,
  dag::Vector<dag::Vector<FrameResource>, framemem_allocator> &new_resources_to_be_scheduled, uint32_t original_resource_count,
  const SchedulingContext &ctx)
{
  // Scan next heap requests for a candidate within same group.
  // If the requested group was ESRAM, reschedule into a regular appropriate group.
  auto rescheduleHeapIdx = source_heap_idx;
  for (auto candidateHeapIdx : remaining_heaps)
  {
    const auto candidateHeapGroupProp = ctx.propertyProvider.getResourceHeapGroupProperties(heap_requests[candidateHeapIdx].group);
    if (heap_requests[candidateHeapIdx].group == heap_requests[source_heap_idx].group ||
        (source_heap_group_prop.isDedicatedFastGPULocal && can_substitute_heap_group(source_heap_group_prop, candidateHeapGroupProp)))
    {
      rescheduleHeapIdx = candidateHeapIdx;
      break;
    }
  }

  // Reschedule resources to appropriate candidate
  // or create a brand new heap request if existing heaps do not suit.
  if (rescheduleHeapIdx != source_heap_idx)
  {
    auto &resourcesPerHeap = resources_to_be_scheduled[rescheduleHeapIdx];
    resourcesPerHeap.insert(resourcesPerHeap.end(), rescheduled_resources.begin(), rescheduled_resources.end());
  }
  else
  {
    auto newHeapRequestsCandidateIdx = 0;
    for (; newHeapRequestsCandidateIdx < new_heap_requests.size(); ++newHeapRequestsCandidateIdx)
    {
      const auto candidateHeapReq = new_heap_requests[newHeapRequestsCandidateIdx];
      const auto candidateHeapGroupProp = ctx.propertyProvider.getResourceHeapGroupProperties(candidateHeapReq.group);
      if (
        candidateHeapReq.group == heap_requests[source_heap_idx].group ||
        (source_heap_group_prop.isDedicatedFastGPULocal && can_substitute_heap_group(source_heap_group_prop, candidateHeapGroupProp)))
      {
        auto &resourcesPerHeap = new_resources_to_be_scheduled[newHeapRequestsCandidateIdx];
        resourcesPerHeap.insert(resourcesPerHeap.end(), rescheduled_resources.begin(), rescheduled_resources.end());
        break;
      }
    }

    if (newHeapRequestsCandidateIdx == new_heap_requests.size())
    {
      auto newHeapRequestGroup = heap_requests[source_heap_idx].group;
      // If we didn't find candidates for ESRAM resources,
      // then reschedule them into new request of regular heap group.
      if (source_heap_group_prop.isDedicatedFastGPULocal)
      {
        const auto desc = ctx.cachedResources[rescheduled_resources.front().resIdx].asScheduled().getGpuDescription();
        newHeapRequestGroup = ctx.propertyProvider.getResourceAllocationProperties(desc, true).heapGroup;
      }

      // If we will be trying to do the exact same thing on the next
      // iteration of this loop, we'll have an infinite loop and
      // crash. Exiting the loop and proceeding would cause a crash
      // too. No other option but to crash.
      // This should NOT ever happen.
      if (rescheduled_resources.size() == original_resource_count && newHeapRequestGroup == heap_requests[source_heap_idx].group)
        DAG_FATAL("Resource packer failed to schedule every single resource! We WILL crash! "
                  "Please, take a full process dump and send it to the render team!");

      new_heap_requests.push_back(HeapRequest{newHeapRequestGroup, 0u});
      dag::Vector<FrameResource> rescheduled(rescheduled_resources.begin(), rescheduled_resources.end());
      new_resources_to_be_scheduled.push_back(eastl::move(rescheduled));
    }
  }
}

void ResourceScheduler::reportSchedulingStatistics(const HeapRequests &heap_requests)
{
#if DAFG_STATISTICS_REPORTING
  if (report_resource_statistics.get())
  {
    debug("daFG: Frame graph resource allocation statistics report:");

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
      debug("daFG: Cumulative timeline size distribution: %s", listToStr(timelineSizeDistr));
      debug("daFG: Cumulative allocation lifetime distribution: %s", listToStr(resourceLifetimeDistr));
      debug("daFG: Cumulative allocation size distribution: %s", listToStr(resourceSizeDistr));
      debug("daFG: Cumulative heap size distribution: %s", listToStr(resultingHeapSizeDistr));
    }

    for (auto [heapIdx, heap] : heap_requests.enumerate())
      debug("daFG: Heap request %d size: %d bytes", eastl::to_underlying(heapIdx), heap.size);
  }
#else
  G_UNUSED(heap_requests);
#endif

  // Don't spam logs, every once in a while is enough.
  if (uint32_t frame = dagor_frame_no(); frame - heapStatisticsLastFramePrinted > 3600)
  {
    // do NOT replace with eastl::string, it cannot expand inplace
    String buffer(nullptr, framemem_ptr());
    for (auto [heapIdx, p] : heapStatistics.enumerate())
    {
      const auto [occupancy, aliasingRatio] = p;
      buffer.aprintf(0, "%d\t%.3f\t%.3f\n", eastl::to_underlying(heapIdx), occupancy, aliasingRatio);
    }
    debug("daFG: heap schedules are packed:\nidx\toccupancy\taliasing ratio\n%s", buffer.c_str());
    heapStatisticsLastFramePrinted = frame;
  }
}

void ResourceScheduler::scheduleResourcesIntoHeaps(int prev_frame, const ResourceProperties &resources,
  const PlacementChangedFlags &placement_changed, const SchedulingContext &ctx)
{
  TIME_PROFILE(scheduleHeaps);

  FRAMEMEM_VALIDATE;

  // Snapshot the previous allocations before we start overwriting them.
  // scheduleHeap reads previous locations for preservation decisions while
  // simultaneously writing new locations into cachedSchedule.allocationLocations.
  PreviousAllocations previousAllocations;
  for (uint32_t frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
  {
    const auto &src = cachedSchedule.allocationLocations[frame];
    previousAllocations[frame].assign(src.begin(), src.end());
  }

  auto &allocations = cachedSchedule.allocationLocations;
  auto &heapRequests = cachedSchedule.heapRequests;

  for (auto &framePreservedResource : preservedResources)
  {
    framePreservedResource.clear();
    framePreservedResource.resize(resources.size(), false);
  }

  for (auto &perFrameAllocations : allocations)
    perFrameAllocations.resize(resources.size());

  const uint32_t preserveProducedOnFrame = prev_frame % SCHEDULE_FRAME_WINDOW;

  // Identify heaps that can be reused from cache before bucketing.
  // Build a reverse map: for each cached heap, collect which resources are in it.
  // If ALL resources in a heap are unchanged, reuse it entirely.
  AlreadyScheduled alreadyScheduled;
  for (auto &perFrame : alreadyScheduled)
    perFrame.resize(resources.size(), false);
  ReusedHeaps reusedHeaps(heapRequests.size(), false);

  for (auto [heapIdx, req] : heapRequests.enumerate())
  {
    if (req.size == 0)
      continue;

    // Collect resources that were in this heap last frame
    bool allUnchanged = true;
    dag::Vector<FrameResource, framemem_allocator> heapResources;
    for (uint32_t frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
      for (auto [resIdx, loc] : allocations[frame].enumerate())
        if (loc.heap == heapIdx && resources.isMapped(resIdx) && resources[resIdx].offsetAlignment != 0)
        {
          heapResources.push_back({resIdx, frame});
          if (placement_changed.test(resIdx, true))
            allUnchanged = false;
        }

    if (!allUnchanged || heapResources.empty())
      continue;

    // Reuse this heap: mark all its resources as already scheduled
    // and set preservation flags for temporal resources
    reusedHeaps.set(heapIdx, true);
    for (const auto &[resIdx, frame] : heapResources)
    {
      alreadyScheduled[frame].set(resIdx, true);
      // Mirror scheduleHeap's preservation conditions: marking a resource as
      // preserved tells the runtime to skip its activation and trust that its
      // memory still holds valid data from the previous frame. That is only
      // safe when the resource has a valid historical pair that lived at the
      // exact same heap location and had the same size -- otherwise the offset
      // holds bytes from an unrelated resource (e.g. after a frontend rename
      // the new resource has historyPairing == NOT_MAPPED).
      if (frame != preserveProducedOnFrame || ctx.cachedResources[resIdx].asScheduled().history == History::No)
        continue;
      const auto preservedResIdx = ctx.historyPairing[resIdx];
      if (preservedResIdx == intermediate::RESOURCE_NOT_MAPPED || !cachedSchedule.allocationLocations[frame].isMapped(preservedResIdx))
        continue;
      const auto &prevLoc = cachedSchedule.allocationLocations[frame][preservedResIdx];
      if (prevLoc.heap != heapIdx)
        continue;
      const uint64_t prevSize =
        cachedCorrectedSizes[frame].isMapped(preservedResIdx) ? cachedCorrectedSizes[frame][preservedResIdx] : 0;
      if (prevSize != corrected_resource_size(resources, ctx.corrections, resIdx, frame))
        continue;
      preservedResources[frame][resIdx] = true;
    }
  }

  // Save old heap sizes before overwriting -- reused heaps need to retain their sizes.
  HeapRequests previousHeapRequests = eastl::move(heapRequests);

  auto [initialHeapRequests, activeRequestForGroup] = initializeHeapRequests(resources, ctx);
  heapRequests = eastl::move(initialHeapRequests);

  for (auto [idx, req] : heapRequests.enumerate())
    if (previousHeapRequests.isMapped(idx))
      req.size = previousHeapRequests[idx].size;

  auto resourcesToBeScheduled = bucketResourcesIntoHeaps(prev_frame, resources, activeRequestForGroup, heapRequests, alreadyScheduled,
    reusedHeaps, previousAllocations, ctx);

  const uint32_t timepointsPerFrame = ctx.events[0].totalKeys();

  dag::Vector<HeapIndex, framemem_allocator> idxs;
  idxs.reserve(heapRequests.size());
  for (uint32_t i = 0; i < heapRequests.size(); ++i)
    if (!reusedHeaps.test(static_cast<HeapIndex>(i), false))
      idxs.push_back(static_cast<HeapIndex>(i));

  dag::Vector<HeapRequest, framemem_allocator> newHeapRequests;
  newHeapRequests.reserve(heapRequests.size());
  dag::Vector<dag::Vector<FrameResource>, framemem_allocator> newResourcesToBeScheduled;
  newResourcesToBeScheduled.reserve(heapRequests.size());

  IdIndexedFlags<HeapIndex, framemem_allocator> heapIsUnused(ctx.allocatedHeaps.size(), false);

  heapStatistics.reserve(heapRequests.size());

  bool allowPreservation = true;
  while (!idxs.empty())
  {
    for (auto it = idxs.begin(); it != idxs.end(); ++it)
    {
      const auto heapIdx = *it;

      auto heapResult = scheduleHeap(heapIdx, resourcesToBeScheduled[heapIdx], preserveProducedOnFrame, timepointsPerFrame,
        allowPreservation, resources, previousAllocations, cachedSchedule, ctx);

      if (heapResult.heapBecameUnused)
        heapIsUnused[heapIdx] = true;

      if (!heapResult.rescheduledResources.empty())
      {
        auto heapGroupProp = ctx.propertyProvider.getResourceHeapGroupProperties(heapRequests[heapIdx].group);
        size_t nextOffset = it - idxs.begin() + 1;
        eastl::span<const HeapIndex> remainingHeaps{idxs.data() + nextOffset, idxs.size() - nextOffset};
        rescheduleOverflowResources(heapResult.rescheduledResources, heapIdx, remainingHeaps, heapGroupProp, resourcesToBeScheduled,
          heapRequests, newHeapRequests, newResourcesToBeScheduled, static_cast<uint32_t>(resourcesToBeScheduled[heapIdx].size()),
          ctx);
      }
    }

    idxs.clear();
    for (uint32_t i = 0; i < newHeapRequests.size(); ++i)
    {
      eastl::optional<HeapIndex> newIdx{};
      for (auto [idx, isUnused] : heapIsUnused.enumerate())
      {
        if (isUnused && heapRequests[idx].group == newHeapRequests[i].group)
        {
          newIdx = idx;
          heapIsUnused[idx] = false;
          break;
        }
      }
      if (newIdx.has_value())
      {
        heapRequests[newIdx.value()] = eastl::move(newHeapRequests[i]);
        resourcesToBeScheduled[newIdx.value()] = eastl::move(newResourcesToBeScheduled[i]);
        idxs.push_back(newIdx.value());
      }
      else
      {
        idxs.push_back(static_cast<HeapIndex>(heapRequests.size()));
        heapRequests.push_back(eastl::move(newHeapRequests[i]));
        resourcesToBeScheduled.push_back(eastl::move(newResourcesToBeScheduled[i]));
      }
    }

    newHeapRequests.clear();
    newResourcesToBeScheduled.clear();
    allowPreservation = false;
  }

  reportSchedulingStatistics(heapRequests);
}

void ResourceScheduler::invalidateTemporalResources()
{
  cachedSchedule = {};
  for (auto &sizes : cachedCorrectedSizes)
    sizes.clear();
}

void ResourceScheduler::cacheCorrectedSizes(const ResourceProperties &resources, const SchedulingContext &ctx)
{
  for (uint32_t frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
  {
    cachedCorrectedSizes[frame].assign(resources.size(), 0);
    for (auto [idx, props] : resources.enumerate())
      if (props.offsetAlignment != 0 && ctx.cachedResources.isMapped(idx) &&
          ctx.cachedResources[idx].asScheduled().history != History::No)
        cachedCorrectedSizes[frame][idx] = corrected_resource_size(resources, ctx.corrections, idx, frame);
  }
}

auto ResourceScheduler::computePlacementChangedFlags(const ResourceProperties &new_properties,
  const SchedulingContext &ctx) const -> PlacementChangedFlags
{
  const auto newCount = new_properties.size();

  // First frame or graph recompilation changed resource count -- treat everything as changed
  if (cachedSchedule.resourceProperties.empty() || cachedSchedule.resourceProperties.size() != newCount)
    return PlacementChangedFlags(newCount, true);

  PlacementChangedFlags changed(newCount, false);

  const auto &oldProps = cachedSchedule.resourceProperties;
  for (auto [idx, newProp] : new_properties.enumerate())
  {
    const auto &oldProp = oldProps[idx];
    if (newProp.sizeInBytes != oldProp.sizeInBytes || newProp.offsetAlignment != oldProp.offsetAlignment ||
        newProp.heapGroup != oldProp.heapGroup)
    {
      changed[idx] = true;
      continue;
    }

    for (uint32_t frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
    {
      const uint64_t newCorrected = corrected_resource_size(new_properties, ctx.corrections, idx, frame);
      const uint64_t oldCorrected = cachedCorrectedSizes[frame].isMapped(idx) ? cachedCorrectedSizes[frame][idx] : 0;
      if (newCorrected != oldCorrected)
      {
        changed[idx] = true;
        break;
      }
    }
  }

  for (auto resIdx : ctx.lifetimeChangedResources.trueKeys())
    if (changed.isMapped(resIdx))
      changed[resIdx] = true;

  return changed;
}

IrHistoryPairing ResourceScheduler::pairPreviousHistory(const IntermediateResources &old_resources,
  const IntermediateResources &new_resources) const
{
  TIME_PROFILE(pairPreviousHistory);

  IrHistoryPairing pairing;
  pairing.resize(new_resources.totalKeys(), intermediate::RESOURCE_NOT_MAPPED);

  FRAMEMEM_VALIDATE;

  // Recall that we transform the initial graph into an IR one. ResNameIds
  // correspond to string resource names as seen by the user, IR ResourceIndexes
  // correspond to the resources as seen by the backend. The relation is N-to-N
  // between those, generally speaking, as multiple names get merged into a
  // single IR resource when renames are involved, and a single name might
  // have multiple IR resources due to multiplexing.
  //
  // Note that the following situations are possible:
  // Let A,B,... be intermediate::ResourceIndexes, X,Y,... be ResNameIds.
  // Before the recompilation the IR<->frontend mapping was:
  //   A -> {X, Y}
  // The graph changed, and now it is:
  //   A -> X
  //   B -> Y
  // (here, {X, Y} means that the user did create(X) and rename(X, Y))
  // So intermediate resources kind of "split", in which case we can't
  // really provide the history for the new A/X, as it was rewritten
  // via a rename on the previous frame, but for the new B/Y we CAN provide
  // the old A/X/Y's history, as renames work such that only the history for
  // the last resource in the rename chain is available.
  // Inverse is possible too, we could've had this mapping:
  //   A -> X
  //   B -> Y
  // And then a kind of a "merge" might have happened, and so we get:
  //   A -> {X, Y}
  // In this case, we use the old B/Y's history for the new A/X/Y.

  // TODO: the implementation of this function leaks logical details of
  // how the IR graph is constructed from the frontend, so this should NOT
  // be constructed here but rather somewhere on the frontend side.

  // Ah, and don't forget that multiplexing exists.

  using FrontToAvailableIrHistory = IdIndexedMapping<ResNameId, intermediate::ResourceIndex, framemem_allocator>;
  IdIndexedMapping<intermediate::MultiplexingIndex, FrontToAvailableIrHistory, framemem_allocator> frontendResToAvailableHistory;

  {
    // Multiplexing might've changed, take maximum of both old and new so
    // as not to crash. Generally speaking, we don't support history preservation
    // when multiplexing changes, but for some nodes/resource it might've stayed
    // the same, in which case we should be able to provide it successfully.
    eastl::underlying_type_t<intermediate::MultiplexingIndex> maxMultiplexing = 0;
    for (const auto &res : old_resources.values())
      maxMultiplexing = eastl::max(maxMultiplexing, eastl::to_underlying(res.multiplexingIndex));
    for (const auto &res : new_resources.values())
      maxMultiplexing = eastl::max(maxMultiplexing, eastl::to_underlying(res.multiplexingIndex));

    // Frontend resources might've appeared or disappeared, take maximum of both old and new
    eastl::underlying_type_t<ResNameId> maxResNameId = 0;
    for (const auto &res : old_resources.values())
      for (const auto resNameId : res.frontendResources)
        maxResNameId = eastl::max(maxResNameId, eastl::to_underlying(resNameId));
    for (const auto &res : new_resources.values())
      for (const auto resNameId : res.frontendResources)
        maxResNameId = eastl::max(maxResNameId, eastl::to_underlying(resNameId));

    frontendResToAvailableHistory.resize(maxMultiplexing + 1);
    for (auto &mapping : dag::ReverseView{frontendResToAvailableHistory})
      mapping.resize(maxResNameId + 1, intermediate::RESOURCE_NOT_MAPPED);
  }

  // The old cached resource contains the history for the last frontend resource
  // in the rename chain, hence the frontendResources.back() here.
  for (auto [oldResIdx, res] : old_resources.enumerate())
    frontendResToAvailableHistory[res.multiplexingIndex][res.frontendResources.back()] = oldResIdx;

  for (auto [newResIdx, res] : new_resources.enumerate())
  {
    // If history is requested for newResIdx, we want to provide the data
    // matching the last frontend resource in the rename chain.
    const auto wantResNameId = res.frontendResources.back();
    const auto availableOldIndex = frontendResToAvailableHistory[res.multiplexingIndex][wantResNameId];
    pairing[newResIdx] = availableOldIndex; // might be NOT_MAPPED
  }

  return pairing;
}

auto ResourceScheduler::computeSchedule(int prev_frame, const SchedulingContext &ctx) -> const ResourceSchedule &
{
  auto resourceProperties = gatherResourceProperties(ctx.graph, ctx.propertyProvider);

  auto placementChanged = computePlacementChangedFlags(resourceProperties, ctx);

  cachedSchedule.resourceProperties = eastl::move(resourceProperties);
  scheduleResourcesIntoHeaps(prev_frame, cachedSchedule.resourceProperties, placementChanged, ctx);
  cacheCorrectedSizes(cachedSchedule.resourceProperties, ctx);

  return cachedSchedule;
}

bool ResourceScheduler::isResourcePreserved(int frame, intermediate::ResourceIndex res_idx) const
{
  G_ASSERT(frame < preservedResources.size());
  if (!preservedResources[frame].isMapped(res_idx))
    return false;

  return preservedResources[frame][res_idx];
}


} // namespace dafg
