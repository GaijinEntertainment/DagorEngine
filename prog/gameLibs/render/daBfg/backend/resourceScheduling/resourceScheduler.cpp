// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "resourceScheduler.h"

#include <EASTL/algorithm.h>
#include <EASTL/bitvector.h>
#include <EASTL/sort.h>

#include <perfMon/dag_statDrv.h>
#include <util/dag_convar.h>
#include <dag/dag_vectorSet.h>
#include <drv/3d/dag_driver.h>

#include <common/resourceUsage.h>
#include <common/genericPoint.h>
#include <runtime/runtime.h>
#include <id/idRange.h>
#include <id/reverseView.h>
#include <backend/resourceScheduling/packer.h>
#include <debug/backendDebug.h>


#if DABFG_STATISTICS_REPORTING
CONSOLE_BOOL_VAL("dabfg", report_resource_statistics, false);
#endif

CONSOLE_BOOL_VAL("dabfg", set_resource_names, DAGOR_DBGLEVEL > 0);

CONSOLE_BOOL_VAL("dabfg", dump_resources, false);

CONSOLE_INT_VAL("dabfg", resource_packer, dabfg::PackerType::GreedyScanline, 0, dabfg::PackerType::COUNT - 1);

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

  auto getFakePropertiesForCpu = [](const BlobDescription &desc) {
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
            temporalResourceLocations.clear();

          historyResourceFlags[resNameId] = newFlags;
        }
      }
      resourceProperties[i] = res.asScheduled().isGpuResource()
                                ? getResourceAllocationProperties(res.asScheduled().getGpuDescription())
                                : getFakePropertiesForCpu(res.asScheduled().getCpuDescription());
    }
  return resourceProperties;
}

auto ResourceScheduler::scheduleResourcesIntoHeaps(const ResourceProperties &resources,
  const BarrierScheduler::EventsCollectionRef &allEvents, const IrHistoryPairing &history_pairing,
  const HistoryResourceLocations &previous_temporal_resource_locations) -> eastl::pair<AllocationLocations, HeapRequests>
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

  temporalResourceLocations.reserve(2 * heapRequests.size());

  // New heap requests might appear while iterating, so cannot use
  // range-based for.
  for (uint32_t intHeapIndex = 0; intHeapIndex < heapRequests.size(); ++intHeapIndex)
  {
    FRAMEMEM_VALIDATE;

    const auto heapIdx = static_cast<HeapIndex>(intHeapIndex);

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

          if (eastl::holds_alternative<BarrierScheduler::Event::Activation>(event.data) ||
              eastl::holds_alternative<BarrierScheduler::Event::CpuActivation>(event.data))
            packerRes.start = time;
          else if (eastl::holds_alternative<BarrierScheduler::Event::Deactivation>(event.data) ||
                   eastl::holds_alternative<BarrierScheduler::Event::CpuDeactivation>(event.data))
            packerRes.end = time;
        }
      }
    }


    bool preserveHeap = false;

    if (previous_temporal_resource_locations.isMapped(heapIdx))
    {
      // Try and preserve the contents of the history for resources in
      // this heap to avoid bugs in temporal algorithms.
      for (size_t i = 0; i < packerResources.size(); ++i)
      {
        auto &packerRes = packerResources[i];
        const auto [resIdx, frame] = inHeapIdxToFrameResource[i];
        if (cachedIntermediateResources[resIdx].asScheduled().history == History::No)
          continue;

        const auto &locs = previous_temporal_resource_locations[heapIdx][frame];
        const auto preservedResIdx = history_pairing[resIdx];
        const auto it = locs.find(preservedResIdx);
        if (it != locs.end() && it->second.size == packerResources[i].size)
        {
          // We managed to
          // a) pair the IR resources correctly,
          // b) find a location where this resource is preserved,
          // c) ensure that the size of the resource did not change,
          // and that means we can actually reuse the existing d3d resource
          // and preserve the history.
          packerRes.offsetHint = it->second.offset;
          preserveHeap = true;
          preservedResources[frame][resIdx] = true;
        }
        else
        {
          preservedResources[frame][resIdx] = false;
        }
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
    if (preserveHeap && allocatedHeaps.isMapped(heapIdx) && allocatedHeaps[heapIdx].size != 0)
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
      debug("daBfg: packed a schedule for heap %d with %f space-time occupancy and %f aliasing ratio (both higher is better).",
        eastl::to_underlying(heapIdx), occupancy, aliasingRatio);
    }

    if (dump_resources.get())
    {
      debug("daBfg: dump of resources in heap %d", eastl::to_underlying(heapIdx));
      debug("daBfg: name\tarea (KB)\tstart\tend\tsize\toffset", eastl::to_underlying(heapIdx));
      for (uint32_t i = 0; i < input.resources.size(); ++i)
      {
        if (output.offsets[i] == PackerOutput::NOT_SCHEDULED)
          continue;

        const auto &res = input.resources[i];
        const auto frameRes = inHeapIdxToFrameResource[i];
        const auto area = (res.size * (res.end + (res.end <= res.start) * input.timelineSize - res.start)) >> 10;
        debug("daBfg: %s\t%d\t%d\t%d\t%d\t%d", cachedIntermediateResourceNames[frameRes.resIdx], area, res.start, res.end, res.size,
          output.offsets[i]);
      }
    }

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
      const auto &packerInputRes = input.resources[idx];
      if (cachedIntermediateResources[resIdx].asScheduled().history != History::No)
      {
        // Defensive programming: check that the packer didn't mess up and
        // respected all out hints.
        if (packerInputRes.offsetHint != PackerInput::NO_HINT && DAGOR_UNLIKELY(packerInputRes.offsetHint != output.offsets[idx]))
        {
          preservedResources[frame][resIdx] = false;

          const auto &locs = previous_temporal_resource_locations[heapIdx][frame];

          const auto historyPair = history_pairing[resIdx];
          const auto it = locs.find(historyPair);
          // Packer cannot guarantee hint respecting for a resource that has it's lifetime changed.
          const bool hasSameLifetime =
            it != locs.end() && it->second.start == packerInputRes.start && it->second.end == packerInputRes.end;
          if (hasSameLifetime)
            logerr("Resource packer ignored offset hint for history resource!");
        }

        // Record where this resource is stored.
        ResourceLocation loc{output.offsets[idx], packerInputRes.size, packerInputRes.start, packerInputRes.end};
        temporalResourceLocations.get(heapIdx)[frame][resIdx] = loc;
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

        // If we will be trying to do the exact same thing on the next
        // iteration of this loop, we'll have an infinite loop and
        // crash. Exiting the loop and proceeding would cause a crash
        // too. No other option but to crash.
        // This should NOT ever happen.
        if (rescheduledResources.size() == packerResources.size() && newHeapRequestGroup == heapRequests[heapIdx].group)
          DAG_FATAL("Resource packer failed to schedule every single resource! We WILL crash! "
                    "Please, take a full process dump and send it to the render team!");

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
    {
      debug("daBfg: Reusing CPU pseudo-heap %d of size %dKB instead of allocating a new %dKB heap", //
        eastl::to_underlying(heapIdx), allocatedHeaps[heapIdx].size >> 10, req.size >> 10);
      continue;
    }

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

      if (temporalResourceLocations.isMapped(heapIdx))
        for (auto &locs : temporalResourceLocations[heapIdx])
          locs.clear();

      debug("daBfg: Destroyed CPU pseudo-heap %d because it was too small (size was %dKB, but %dKB are required)",
        eastl::to_underlying(heapIdx), allocatedHeaps[heapIdx].size >> 10, req.size >> 10);
    }
    else // sanity check
      G_ASSERT(!heapToResourceList.isMapped(heapIdx) || heapToResourceList[heapIdx][prev_frame].empty());

    cpuHeaps[heapIdx].resize(req.size, 0);
    allocatedHeaps[heapIdx].size = req.size;
    debug("daBfg: Successfully allocated a new CPU pseudo-heap %d of size %dKB", //
      eastl::to_underlying(heapIdx), req.size >> 10);
  }

  return result;
}

void ResourceScheduler::placeResources(const intermediate::Graph &graph, const ResourceProperties &resource_properties,
  const AllocationLocations &allocation_locations, const DynamicResolutions &dyn_resolutions)
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
        DynamicResolution dynResolution = eastl::monostate{};

        if (const auto &type = res.asScheduled().resolutionType; type.has_value() && dyn_resolutions.isMapped(type->id))
          dynResolution = dyn_resolutions[type->id];

        placeResource(frame, i, heapIdx, res.asScheduled().getGpuDescription(), offset, resProps, dynResolution);

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
}

void ResourceScheduler::invalidateTemporalResources() { temporalResourceLocations.clear(); }

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
  temporalResourceLocations.clear();

  shutdownInternal();

  heapToResourceList.clear();
  allocatedHeaps.clear();
  cachedIntermediateResources.clear();
}

ResourceScheduler::IrHistoryPairing ResourceScheduler::pairPreviousHistory(const IntermediateResources &new_resources) const
{
  TIME_PROFILE(pairPreviousHistory);

  IrHistoryPairing pairing;
  pairing.resize(new_resources.size(), intermediate::RESOURCE_NOT_MAPPED);

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
    for (const auto &res : cachedIntermediateResources)
      maxMultiplexing = eastl::max(maxMultiplexing, eastl::to_underlying(res.multiplexingIndex));
    for (const auto &res : new_resources)
      maxMultiplexing = eastl::max(maxMultiplexing, eastl::to_underlying(res.multiplexingIndex));

    // Frontend resources might've appeared or disappeared, take maximum of both old and new
    eastl::underlying_type_t<ResNameId> maxResNameId = 0;
    for (const auto &res : cachedIntermediateResources)
      for (const auto resNameId : res.frontendResources)
        maxResNameId = eastl::max(maxResNameId, eastl::to_underlying(resNameId));
    for (const auto &res : new_resources)
      for (const auto resNameId : res.frontendResources)
        maxResNameId = eastl::max(maxResNameId, eastl::to_underlying(resNameId));

    frontendResToAvailableHistory.resize(maxMultiplexing + 1);
    for (auto &mapping : ReverseView{frontendResToAvailableHistory})
      mapping.resize(maxResNameId + 1, intermediate::RESOURCE_NOT_MAPPED);
  }

  // The old cached resource contains the history for the last frontend resource
  // in the rename chain, hence the frontendResources.back() here.
  for (auto [oldResIdx, res] : cachedIntermediateResources.enumerate())
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

ResourceScheduler::ResourceDeactivationRequestsFmem ResourceScheduler::scheduleResources(int prev_frame,
  const intermediate::Graph &graph, const BarrierScheduler::EventsCollectionRef &events, const DynamicResolutions &dyn_resolutions)
{
  ResourceDeactivationRequestsFmem result;
  result.reserve(cachedIntermediateResources.size());

  FRAMEMEM_VALIDATE;

  auto potentialDeactivations = gatherPotentialDeactivationSet(prev_frame);

  closeTransientResources();

  const auto prevHistoryPairing = pairPreviousHistory(graph.resources);

  cachedIntermediateResources = graph.resources;
  cachedIntermediateResourceNames = graph.resourceNames;

  const auto resourceProperties = gatherResourceProperties(graph);

  // Intentional copy, allocations are unavoidable here, sadly.
  const auto previousTemporalResourceLocations = temporalResourceLocations;
  for (auto &heap_locs : temporalResourceLocations)
    for (auto &locs : heap_locs)
      locs.clear();

  auto [allocationLocations, heapRequests] =
    scheduleResourcesIntoHeaps(resourceProperties, events, prevHistoryPairing, previousTemporalResourceLocations);

  // Don't deactivate resources that were preserved
  for (auto [idx, res] : cachedIntermediateResources.enumerate())
    if (isResourcePreserved(prev_frame, idx))
      potentialDeactivations[prevHistoryPairing[idx]] = eastl::monostate{};

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

  placeResources(graph, resourceProperties, allocationLocations, dyn_resolutions);

  // Outside world doesn't need to know old resource indices, so repack
  // the deactivation map into a regular array.
  for (auto [idx, res] : cachedIntermediateResources.enumerate())
    if (const auto oldIdx = prevHistoryPairing[idx]; oldIdx != intermediate::RESOURCE_NOT_MAPPED)
      eastl::visit(
        [&result](const auto &res) {
          if constexpr (!eastl::is_same_v<eastl::remove_cvref_t<decltype(res)>, eastl::monostate>)
            result.emplace_back(res);
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

void ResourceScheduler::emergencyWipeBlobs(int frame, dag::VectorSet<ResNameId, eastl::less<ResNameId>, framemem_allocator> resources)
{
  for (auto [resIdx, res] : cachedIntermediateResources.enumerate())
    if (res.isScheduled() && res.asScheduled().isCpuResource() && resources.count(res.frontendResources.front()))
    {
      auto &desc = res.asScheduled().getCpuDescription();
      if (res.asScheduled().history != History::No)
        desc.deactivate(getBlob(frame, resIdx).data);

      // After deactivation, we have to trick the system into not trying to
      // deactivate/preserve it on next recompilation.
      // Removing the history mark will:
      // - prevent it being marked as a potential deactivation
      // - prevent it from being considered for history preservation
      res.asScheduled().history = History::No;
      res.asScheduled().description = BlobDescription{};

      debug("daBfg: Wiped resource %s", cachedIntermediateResourceNames[resIdx]);
    }
}


} // namespace dabfg
