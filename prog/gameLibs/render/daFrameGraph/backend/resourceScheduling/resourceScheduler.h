// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vectorMap.h>

#include <generic/dag_fixedVectorMap.h>
#include <id/idIndexedFlags.h>
#include <backend/badResolutionTracker.h>
#include <common/graphDumper.h>
#include <common/dynamicResolution.h>
#include <backend/resourceScheduling/resourceSchedulingTypes.h>
#include <backend/resourceScheduling/resourcePropertyProvider.h>


#define DAFG_STATISTICS_REPORTING DAGOR_DBGLEVEL > 0


namespace dafg
{

struct FrameResource
{
  intermediate::ResourceIndex resIdx;
  uint32_t frame;
};

struct HeapSchedulingResult;

class ResourceScheduler // -V730
{
public:
  struct SchedulingContext
  {
    const intermediate::Graph &graph;
    const BarrierScheduler::EventsCollection &events;
    const BarrierScheduler::ResourceLifetimesChanged &lifetimeChangedResources;
    const IrHistoryPairing &historyPairing;
    const BadResolutionTracker::Corrections &corrections;
    IResourcePropertyProvider &propertyProvider;
    const HeapRequests &allocatedHeaps;
    const IntermediateResources &cachedResources;
    const IdSparseIndexedMapping<intermediate::ResourceIndex, intermediate::DebugResourceName> &cachedResourceNames;
  };

  ResourceScheduler() = default;

  const ResourceSchedule &computeSchedule(int prev_frame, const SchedulingContext &ctx);

  // After recompiling, the IR graph might change drastically, so we have
  // to use some tricks to figure out which resources we are supposed to
  // reuse as history for the current frame
  IrHistoryPairing pairPreviousHistory(const IntermediateResources &old_resources, const IntermediateResources &new_resources) const;

  bool isResourcePreserved(int frame, intermediate::ResourceIndex res_idx) const;

  void invalidateTemporalResources();

private:
  using PlacementChangedFlags = IdIndexedFlags<intermediate::ResourceIndex, framemem_allocator>;

  ResourceProperties gatherResourceProperties(const intermediate::Graph &graph, IResourcePropertyProvider &property_provider);

  // Calculates a schedule of resource usage, i.e. where exactly in
  // each heap the resource will be allocated
  void scheduleResourcesIntoHeaps(int prev_frame, const ResourceProperties &resources, const PlacementChangedFlags &placement_changed,
    const SchedulingContext &ctx);

  using ActiveHeapRequestMap = dag::VectorMap<ResourceHeapGroup *, HeapIndex>;

  eastl::pair<HeapRequests, ActiveHeapRequestMap> initializeHeapRequests(const ResourceProperties &resources,
    const SchedulingContext &ctx);

  using AlreadyScheduled = eastl::array<IdIndexedFlags<intermediate::ResourceIndex, framemem_allocator>, SCHEDULE_FRAME_WINDOW>;
  using ReusedHeaps = IdIndexedFlags<HeapIndex, framemem_allocator>;

  using PreviousAllocations =
    eastl::array<IdIndexedMapping<intermediate::ResourceIndex, AllocationLocation, framemem_allocator>, SCHEDULE_FRAME_WINDOW>;

  IdIndexedMapping<HeapIndex, dag::Vector<FrameResource>> bucketResourcesIntoHeaps(int prev_frame, const ResourceProperties &resources,
    ActiveHeapRequestMap &active_request_for_group, HeapRequests &heap_requests, const AlreadyScheduled &already_scheduled,
    const ReusedHeaps &reused_heaps, const PreviousAllocations &previous_allocations, const SchedulingContext &ctx);

  HeapSchedulingResult scheduleHeap(HeapIndex heap_idx, eastl::span<const FrameResource> resources_in_heap,
    uint32_t preserve_produced_on_frame, uint32_t timepoints_per_frame, bool allow_preservation, const ResourceProperties &resources,
    const PreviousAllocations &previous_allocations, ResourceSchedule &result, const SchedulingContext &ctx);

  void cacheCorrectedSizes(const ResourceProperties &resources, const SchedulingContext &ctx);
  PlacementChangedFlags computePlacementChangedFlags(const ResourceProperties &new_properties, const SchedulingContext &ctx) const;

  void rescheduleOverflowResources(eastl::span<const FrameResource> rescheduled_resources, HeapIndex source_heap_idx,
    eastl::span<const HeapIndex> remaining_heaps, ResourceHeapGroupProperties source_heap_group_prop,
    IdIndexedMapping<HeapIndex, dag::Vector<FrameResource>> &resources_to_be_scheduled, const HeapRequests &heap_requests,
    dag::Vector<HeapRequest, framemem_allocator> &new_heap_requests,
    dag::Vector<dag::Vector<FrameResource>, framemem_allocator> &new_resources_to_be_scheduled, uint32_t original_resource_count,
    const SchedulingContext &ctx);

  void reportSchedulingStatistics(const HeapRequests &heap_requests);

  // Cached schedule from the previous computeSchedule call.
  // Used for temporal resource preservation (pinning offsets) and
  // replaces the old temporalResourceLocations + heapForResource members.
  ResourceSchedule cachedSchedule;
  eastl::array<IdIndexedMapping<intermediate::ResourceIndex, uint64_t>, SCHEDULE_FRAME_WINDOW> cachedCorrectedSizes;

  // Lists the resources whose history we managed to preserve and so don't
  // need to wipe it clean.
  eastl::array<IdIndexedFlags<intermediate::ResourceIndex>, SCHEDULE_FRAME_WINDOW> preservedResources;

  // Keep track of history resource flags,
  // because their change invalidates the preservation mechanism.
  dag::FixedVectorMap<ResNameId, uint32_t, 32> historyResourceFlags;

  // These live here so that we have a single heap allocation that we reuse
  IdIndexedMapping<HeapIndex, eastl::pair<float, float>> heapStatistics;
  uint32_t heapStatisticsLastFramePrinted = 0;

#if DAFG_STATISTICS_REPORTING
  dag::VectorMap<uint32_t, uint32_t> resourceLifetimeDistr;
  dag::VectorMap<uint32_t, uint32_t> resourceSizeDistr;
  dag::VectorMap<uint32_t, uint32_t> timelineSizeDistr;
  dag::VectorMap<uint32_t, uint64_t> resultingHeapSizeDistr;
#endif
};

} // namespace dafg
