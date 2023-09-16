#pragma once
#include <EASTL/span.h>
#include <EASTL/variant.h>

#include <3d/dag_resPtr.h>
#include <math/integer/dag_IPoint2.h>
#include <dag/dag_vectorMap.h>
#include <memory/dag_framemem.h>

#include <generic/dag_fixedVectorMap.h>
#include <id/idIndexedFlags.h>
#include "intermediateRepresentation.h"
#include "graphDumper.h"


#define DABFG_STATISTICS_REPORTING DAGOR_DBGLEVEL > 0


class framemem_allocator;

namespace dabfg
{

using DynamicResolutions = IdIndexedMapping<AutoResTypeNameId, IPoint2, framemem_allocator>;

class ResourceScheduler
{
public:
  struct Event
  {
    struct Activation
    {
      ResourceActivationAction action;
      ResourceClearValue clearValue;
    };

    struct CpuActivation
    {
      TypeErasedCall func;
    };

    struct CpuDeactivation
    {
      TypeErasedCall func;
    };

    struct Barrier
    {
      ResourceBarrier barrier;
    };

    struct Deactivation
    {};

    intermediate::ResourceIndex resource;
    uint32_t frameResourceProducedOn;

    // NOTE: at each timepoint events are executed in order of their
    // type, from last one to first one. Meaning deactivations happen
    // before barriers, barriers before activations.
    using Payload = eastl::variant<CpuActivation, Activation, Barrier, Deactivation, CpuDeactivation>;
    Payload data;
  };

  static const int SCHEDULE_FRAME_WINDOW = 2; // even and odd frames

  using NodeEventsRef = eastl::span<const Event>;
  using FrameEventsRef = dag::Vector<NodeEventsRef>;
  using EventsCollectionRef = eastl::array<FrameEventsRef, SCHEDULE_FRAME_WINDOW>;


public:
  ResourceScheduler(IGraphDumper &dumper) : graphDumper{dumper} {}

  struct BlobDeactivationRequest
  {
    void (*destructor)(void *);
    void *blob;
  };

  using ResourceDeactivationRequest = eastl::variant<Texture *, Sbuffer *, BlobDeactivationRequest>;

  struct SchedulingResult
  {
    EventsCollectionRef events;
    dag::Vector<ResourceDeactivationRequest, framemem_allocator> deactivations;
  };

  // Returned spans are valid until the method gets called again
  SchedulingResult scheduleResources(int prev_frame, const intermediate::Graph &graph);

  virtual void resizeAutoResTextures(int frame, const DynamicResolutions &resolutions) = 0;

  BlobView getBlob(int frame, intermediate::ResourceIndex res_idx);
  virtual ManagedTexView getTexture(int frame, intermediate::ResourceIndex res_idx) const = 0;
  virtual ManagedBufView getBuffer(int frame, intermediate::ResourceIndex res_idx) const = 0;
  const D3dResource *getD3dResource(int frame, intermediate::ResourceIndex res_idx) const;
  bool isResourcePreserved(int frame, intermediate::ResourceIndex res_idx) const;

  void shutdown(int frame);

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


  // Returns per node events for even and odd frames. Modifies eventStorage and returns views to it.
  EventsCollectionRef scheduleEvents(const intermediate::Graph &graph);

  using PotentialResourceDeactivation = eastl::variant<eastl::monostate, BaseTexture *, Sbuffer *, BlobDeactivationRequest>;
  using PotentialDeactivationSet = IdIndexedMapping<intermediate::ResourceIndex, PotentialResourceDeactivation>;

  PotentialDeactivationSet gatherPotentialDeactivationSet(int prev_frame);


  using IntermediateResources = IdIndexedMapping<intermediate::ResourceIndex, intermediate::Resource>;
  using IntermediateRemapping = IdIndexedMapping<intermediate::ResourceIndex, intermediate::ResourceIndex, framemem_allocator>;

  // After recompiling, the resource index may change for the same resource.
  // So we must map the new idx to the old idx in order to check the offset hints.
  IntermediateRemapping remapResources(const IntermediateResources &resources) const;

  // Calculates a schedule of resource usage, i.e. where exactly in
  // each heap the resource will be allocated
  eastl::pair<AllocationLocations, HeapRequests> scheduleResourcesIntoHeaps(const ResourceProperties &resources,
    const EventsCollectionRef &events, const IntermediateRemapping &cached_res_idx_remapping);

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
    const AllocationLocations &allocation_locations);

  void restoreResourceApiNames() const;

  virtual void placeResource(int frame, intermediate::ResourceIndex res_idx, HeapIndex heap_idx, const ResourceDescription &desc,
    uint32_t offset, const ResourceAllocationProperties &properties) = 0;

  virtual ResourceAllocationProperties getResourceAllocationProperties(const ResourceDescription &desc,
    bool force_not_on_chip = false) = 0;
  virtual ResourceHeapGroupProperties getResourceHeapGroupProperties(ResourceHeapGroup *heap_group) = 0;

  void closeTransientResources();

  virtual void shutdownInternal() = 0;

protected:
  IGraphDumper &graphDumper;


  // The following 2 fields are wiped and
  // recalculated on each compilation

  dag::Vector<Event> eventStorage;
  eastl::array<IdIndexedMapping<intermediate::ResourceIndex, uint32_t>, SCHEDULE_FRAME_WINDOW> resourceIndexInCollection;

  static constexpr uint32_t UNSCHEDULED = eastl::numeric_limits<uint32_t>::max();

  // Required for answering external get resource and set resolution requests
  IntermediateResources cachedIntermediateResources;


  // Fake heap group used for CPU resources
  inline static ResourceHeapGroup *const CPU_HEAP_GROUP = reinterpret_cast<ResourceHeapGroup *>(~uintptr_t{0});

  // Caching of heaps to reduce memory reallocation. For simplicity,
  // heaps are never totally removed and heap IDs are never freed.
  HeapRequests allocatedHeaps;
  // Offset hints and sizes for history resource per heap in case of rescheduling
  struct HintedResource
  {
    uint64_t offsetHint;
    uint64_t size;
    uint32_t start;
    uint32_t end;
  };
  IdIndexedMapping<HeapIndex,
    eastl::array<dag::FixedVectorMap<intermediate::ResourceIndex, HintedResource, 32>, SCHEDULE_FRAME_WINDOW>>
    hintedResources;
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
