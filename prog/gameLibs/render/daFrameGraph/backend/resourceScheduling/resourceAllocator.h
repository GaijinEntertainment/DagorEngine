// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/span.h>

#include <dag/dag_vectorMap.h>
#include <dag/dag_vectorSet.h>

#include <id/idIndexedFlags.h>
#include <common/graphDumper.h>
#include <common/dynamicResolution.h>
#include <backend/badResolutionTracker.h>
#include <backend/resourceScheduling/resourcePropertyProvider.h>
#include <backend/resourceScheduling/resourceSchedulingTypes.h>


namespace dafg
{

class ResourceAllocator : public IResourcePropertyProvider
{
public:
  ResourceAllocator(IGraphDumper &dumper) : graphDumper{dumper} {}

  void applySchedule(int prev_frame, const ResourceSchedule &schedule, const intermediate::Graph &graph,
    const DynamicResolutions &dyn_resolutions, const BadResolutionTracker::Corrections &corrections,
    PotentialDeactivationSet &potential_deactivations);

  virtual void resizeAutoResTextures(int frame, const DynamicResolutionUpdates &resolutions) = 0;

  BlobView getBlob(int frame, intermediate::ResourceIndex res_idx);
  BaseTexture *getTexture(int frame, intermediate::ResourceIndex res_idx);
  Sbuffer *getBuffer(int frame, intermediate::ResourceIndex res_idx);

  void emergencyWipeBlobs(int frame, dag::VectorSet<ResNameId, eastl::less<ResNameId>, framemem_allocator> resources);

  void gatherPotentialDeactivationSet(int prev_frame, PotentialDeactivationSet &result);

  void shutdown(int frame);

  void closeTransientResources();

  virtual ~ResourceAllocator() = default;

  // Required for answering external get resource and set resolution requests
  IntermediateResources cachedIntermediateResources;
  IdSparseIndexedMapping<intermediate::ResourceIndex, intermediate::DebugResourceName> cachedIntermediateResourceNames;

  // Caching of heaps to reduce memory reallocation. For simplicity,
  // heaps are never totally removed and heap IDs are never freed.
  HeapRequests allocatedHeaps;

protected:
  virtual D3dResource *getD3dResource(int frame, intermediate::ResourceIndex res_idx) const = 0;

  // Creates actual memory storage for our resources
  virtual DestroyedHeapSet allocateHeaps(const HeapRequests &requests) = 0;

  DestroyedHeapSet allocateCpuHeaps(int prev_frame, const HeapRequests &requests,
    const PotentialDeactivationSet &potential_deactivation_set);

  // Note: this function does not deal with aliases, only with
  // representatives of alias groups.
  void placeResources(const intermediate::Graph &graph, const ResourceProperties &resource_properties,
    const AllocationLocations &allocation_locations, const DynamicResolutions &dyn_resolutions,
    const BadResolutionTracker::Corrections &corrections);

  void restoreResourceApiNames() const;

  virtual void resetResourcePlacement() = 0;
  virtual void placeResource(int frame, intermediate::ResourceIndex res_idx, HeapIndex heap_idx, const ResourceDescription &desc,
    uint64_t offset, const ResourceAllocationProperties &properties, const DynamicResolution &dyn_resolution) = 0;

  void setManagedName(int frame, intermediate::ResourceIndex resIdx, D3dResource *res);
  void refreshManagedTexture(int frame, intermediate::ResourceIndex res_idx, BaseTexture *tex);

  virtual void shutdownInternal() = 0;

protected:
  IGraphDumper &graphDumper;

private:
  struct CpuResourcePlacement
  {
    uint32_t offset;
    HeapIndex heap;
  };

  eastl::array<IdIndexedMapping<intermediate::ResourceIndex, CpuResourcePlacement>, SCHEDULE_FRAME_WINDOW> cpuResources;

  // For every heap and every frame, we keep a list of resources that
  // that we use to figure out which resources got destroyed when a
  // heap is deactivated and hence don't need to be deactivated.
  IdIndexedMapping<HeapIndex, eastl::array<dag::Vector<intermediate::ResourceIndex>, SCHEDULE_FRAME_WINDOW>> heapToResourceList;

  IdIndexedMapping<HeapIndex, dag::Vector<char>> cpuHeaps;
};

} // namespace dafg
