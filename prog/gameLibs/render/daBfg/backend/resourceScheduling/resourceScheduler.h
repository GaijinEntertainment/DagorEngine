// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/span.h>
#include <EASTL/variant.h>

#include <3d/dag_resPtr.h>
#include <math/integer/dag_IPoint2.h>
#include <dag/dag_vectorMap.h>
#include <dag/dag_vectorSet.h>
#include <memory/dag_framemem.h>

#include <generic/dag_fixedVectorMap.h>
#include <id/idIndexedFlags.h>
#include <backend/intermediateRepresentation.h>
#include <backend/resourceScheduling/barrierScheduler.h>
#include <backend/passColoring.h>
#include <common/graphDumper.h>


#define DABFG_STATISTICS_REPORTING DAGOR_DBGLEVEL > 0


class framemem_allocator;

namespace dabfg
{

using DynamicResolution = eastl::variant<eastl::monostate, IPoint2, IPoint3>;
using DynamicResolutions = IdIndexedMapping<AutoResTypeNameId, DynamicResolution, framemem_allocator>;

class ResourceScheduler
{
public:
  static constexpr int SCHEDULE_FRAME_WINDOW = BarrierScheduler::SCHEDULE_FRAME_WINDOW;

  ResourceScheduler(IGraphDumper &dumper) : graphDumper{dumper} {}

  struct BlobDeactivationRequest
  {
    TypeErasedCall destructor;
    void *blob;
  };

  using ResourceDeactivationRequest = eastl::variant<BaseTexture *, Sbuffer *, BlobDeactivationRequest>;
  using ResourceDeactivationRequestsFmem = dag::Vector<ResourceDeactivationRequest, framemem_allocator>;

  ResourceDeactivationRequestsFmem scheduleResources(int prev_frame, const intermediate::Graph &graph,
    const BarrierScheduler::EventsCollectionRef &events, const DynamicResolutions &dyn_resolutions);

  virtual void resizeAutoResTextures(int frame, const DynamicResolutions &resolutions) = 0;

  BlobView getBlob(int frame, intermediate::ResourceIndex res_idx);
  virtual ManagedTexView getTexture(int frame, intermediate::ResourceIndex res_idx) const = 0;
  virtual ManagedBufView getBuffer(int frame, intermediate::ResourceIndex res_idx) const = 0;
  const D3dResource *getD3dResource(int frame, intermediate::ResourceIndex res_idx) const;
  bool isResourcePreserved(int frame, intermediate::ResourceIndex res_idx) const;

  void emergencyWipeBlobs(int frame, dag::VectorSet<ResNameId, eastl::less<ResNameId>, framemem_allocator> resources);

  void shutdown(int frame);

  void invalidateTemporalResources();

  virtual ~ResourceScheduler() = default;

protected:
  using ResourceProperties = IdIndexedMapping<intermediate::ResourceIndex, ResourceAllocationProperties>;

  enum class HeapIndex : uint32_t
  {
    Invalid = ~0u
  };

  struct AllocationLocation
  {
    HeapIndex heap;
    uint32_t offset;
  };
  using AllocationLocations = eastl::array<IdIndexedMapping<intermediate::ResourceIndex, AllocationLocation>, SCHEDULE_FRAME_WINDOW>;

  struct HeapRequest
  {
    ResourceHeapGroup *group;
    uint32_t size;
  };
  using HeapRequests = IdIndexedMapping<HeapIndex, HeapRequest>;

  ResourceProperties gatherResourceProperties(const intermediate::Graph &graph);

  using PotentialResourceDeactivation = eastl::variant<eastl::monostate, BaseTexture *, Sbuffer *, BlobDeactivationRequest>;
  using PotentialDeactivationSet = IdIndexedMapping<intermediate::ResourceIndex, PotentialResourceDeactivation>;

  PotentialDeactivationSet gatherPotentialDeactivationSet(int prev_frame);


  using IntermediateResources = IdIndexedMapping<intermediate::ResourceIndex, intermediate::Resource>;
  using IrHistoryPairing = IdIndexedMapping<intermediate::ResourceIndex, intermediate::ResourceIndex, framemem_allocator>;

  // After recompiling, the IR graph might change drastically, so we have
  // to use some tricks to figure out which resources we are supposed to
  // reuse as history for the current frame
  IrHistoryPairing pairPreviousHistory(const IntermediateResources &resources) const;

  struct ResourceLocation
  {
    uint64_t offset;
    uint64_t size;
    uint32_t start;
    uint32_t end;
  };
  using PerHeapPerFrameHistoryResourceLocations = dag::FixedVectorMap<intermediate::ResourceIndex, ResourceLocation, 32>;
  using PerHeapHistoryResourceLocations = eastl::array<PerHeapPerFrameHistoryResourceLocations, SCHEDULE_FRAME_WINDOW>;
  using HistoryResourceLocations = IdIndexedMapping<HeapIndex, PerHeapHistoryResourceLocations>;


  // Calculates a schedule of resource usage, i.e. where exactly in
  // each heap the resource will be allocated
  eastl::pair<AllocationLocations, HeapRequests> scheduleResourcesIntoHeaps(const ResourceProperties &resources,
    const BarrierScheduler::EventsCollectionRef &events, const IrHistoryPairing &history_pairing,
    const HistoryResourceLocations &previous_temporal_resource_locations);

  // This lists the heaps that were either completely destroyed
  // or recreated with a larger size (and their resources hence died)
  using DestroyedHeapSet = dag::Vector<HeapIndex, framemem_allocator>;

  // Creates actual memory storage for our resources
  virtual DestroyedHeapSet allocateHeaps(const HeapRequests &requests) = 0;

  DestroyedHeapSet allocateCpuHeaps(int prev_frame, const HeapRequests &requests,
    const PotentialDeactivationSet &potential_deactivation_set);

  // Note: this function does not deal with aliases, only with
  // representatives of alias groups.
  void placeResources(const intermediate::Graph &graph, const ResourceProperties &resource_properties,
    const AllocationLocations &allocation_locations, const DynamicResolutions &dyn_resolutions);

  void restoreResourceApiNames() const;

  virtual void placeResource(int frame, intermediate::ResourceIndex res_idx, HeapIndex heap_idx, const ResourceDescription &desc,
    uint32_t offset, const ResourceAllocationProperties &properties, const DynamicResolution &dyn_resolution) = 0;

  virtual ResourceAllocationProperties getResourceAllocationProperties(const ResourceDescription &desc,
    bool force_not_on_chip = false) = 0;
  virtual ResourceHeapGroupProperties getResourceHeapGroupProperties(ResourceHeapGroup *heap_group) = 0;

  void closeTransientResources();

  virtual void shutdownInternal() = 0;

protected:
  IGraphDumper &graphDumper;

  eastl::array<IdIndexedMapping<intermediate::ResourceIndex, uint32_t>, SCHEDULE_FRAME_WINDOW> resourceIndexInCollection;

  static constexpr uint32_t UNSCHEDULED = eastl::numeric_limits<uint32_t>::max();

  // Required for answering external get resource and set resolution requests
  IntermediateResources cachedIntermediateResources;
  IdIndexedMapping<intermediate::ResourceIndex, intermediate::DebugResourceName> cachedIntermediateResourceNames;

  // Fake heap group used for CPU resources
  inline static ResourceHeapGroup *const CPU_HEAP_GROUP = reinterpret_cast<ResourceHeapGroup *>(~uintptr_t{0});

  // Caching of heaps to reduce memory reallocation. For simplicity,
  // heaps are never totally removed and heap IDs are never freed.
  HeapRequests allocatedHeaps;

  // Maps a (heapIdx, frame, resIdx) into the location where the resource is
  // stored, if it has history enabled (i.e. it is a temporal resource).
  HistoryResourceLocations temporalResourceLocations;

  // Lists the resources whose history we managed to preserve and so don't
  // need to wipe it clean.
  eastl::array<IdIndexedFlags<intermediate::ResourceIndex>, SCHEDULE_FRAME_WINDOW> preservedResources;

  // Keep track of history resource flags,
  // because their change invalidates the preservation mechanism.
  dag::FixedVectorMap<ResNameId, uint32_t, 32> historyResourceFlags;

private:
  // For every heap and every frame, we keep a list of resources that
  // that we use to figure out which resources got destroyed when a
  // heap is deactivated and hence don't need to be deactivated.
  IdIndexedMapping<HeapIndex, eastl::array<dag::Vector<intermediate::ResourceIndex>, SCHEDULE_FRAME_WINDOW>> heapToResourceList;

  eastl::array<IdIndexedMapping<intermediate::ResourceIndex, HeapIndex>, SCHEDULE_FRAME_WINDOW> heapForCpuResource;
  IdIndexedMapping<HeapIndex, dag::Vector<char>> cpuHeaps;

#if DABFG_STATISTICS_REPORTING
  // Empirical non-normalized probability density functions for various
  // useful statistics of in-game data sets. Accumulated over all
  // compilations.
  // We assume that size and life length are independent random variables,
  // and that start location for resource are distributed uniformly.
  dag::VectorMap<uint32_t, uint32_t> resourceLifetimeDistr;
  dag::VectorMap<uint32_t, uint32_t> resourceSizeDistr;
  dag::VectorMap<uint32_t, uint32_t> timelineSizeDistr;
  dag::VectorMap<uint32_t, uint64_t> resultingHeapSizeDistr;
#endif
};

} // namespace dabfg
