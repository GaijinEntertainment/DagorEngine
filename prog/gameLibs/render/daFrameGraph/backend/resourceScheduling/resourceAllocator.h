// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/span.h>
#include <ska_hash_map/flat_hash_map2.hpp>

#include <dag/dag_vectorMap.h>
#include <dag/dag_vectorSet.h>

#include <id/idIndexedFlags.h>
#include <id/idIndexedMapping.h>
#include <common/graphDumper.h>
#include <common/dynamicResolution.h>
#include <backend/resourceScheduling/resourcePropertyProvider.h>
#include <backend/resourceScheduling/resourceSchedulingTypes.h>
#include <backend/resourceScheduling/heapLayout.h>


namespace dafg
{

inline HeapRequest make_heap_request(const LayoutHeap &heap)
{
  return {
    .group = heap.heapClass.group,
    .size = heap.reservedSize,
    .untracked = heap.heapClass.untracked,
  };
}

class ResourceAllocator : public IResourcePropertyProvider
{
public:
  using ResourcesChangedFmem = IdIndexedFlags<intermediate::ResourceIndex, framemem_allocator>;
  using ReleaseBarriers = IdIndexedMapping<intermediate::ResourceIndex, intermediate::EnhancedBarrier>;

  using ChangedHeaps = IdIndexedFlags<HeapIndex, framemem_allocator>;

  struct Stats
  {
    uint32_t schedulesApplied = 0;
    uint32_t frameResourcesPlaced = 0;
    uint32_t objectsCreated = 0;
    uint32_t heapsAllocated = 0;
    uint32_t heapsDestroyed = 0;
    uint32_t heapsReassigned = 0;
    uint32_t relocations = 0;
  };

  // Its old object or blob stays alive until finishRelocations
  struct Relocation
  {
    intermediate::ResourceIndex resIdx{};
    D3dResource *object = nullptr;
    void *blob = nullptr;
  };

  ResourceAllocator(IGraphDumper &dumper) : graphDumper{dumper} {}

  // The scheduler's arguments use the resource indices of the new graph. The
  // allocator's own tables move to them through the pairing during the call.
  void applySchedule(int prev_frame, const Layout &layout, const PlacementsChanged &placements_changed,
    const IdIndexedFlags<intermediate::ResourceIndex> &relocated,
    const IdIndexedMapping<intermediate::ResourceIndex, uint32_t> &keep_frames, const ResourceProperties &properties,
    const intermediate::Graph &graph, const ResourcesChangedFmem &resources_changed,
    const intermediate::HistoryPairing &historyPairing, const ReleaseBarriers &release_barriers,
    const DynamicResolutions &dyn_resolutions, PotentialDeactivationSet &potential_deactivations);

  const Stats &lastStats() const { return stats; }

  // Relocations belong to the previous frame of the last applySchedule
  const Relocation *findRelocation(intermediate::ResourceIndex res_idx) const;
  void finishRelocations();

  virtual void resizeAutoResTextures(int frame, const DynamicResolutionUpdates &resolutions) = 0;

  BlobView getBlob(int frame, intermediate::ResourceIndex res_idx);
  BaseTexture *getTexture(int frame, intermediate::ResourceIndex res_idx);
  Sbuffer *getBuffer(int frame, intermediate::ResourceIndex res_idx);

  // Returns indices of the graph the allocator last applied. The caller must
  // invalidate the history of each, or the wiped blobs stay preserved.
  dag::Vector<intermediate::ResourceIndex, framemem_allocator> emergencyWipeBlobs(int frame,
    const dag::VectorSet<ResNameId, eastl::less<ResNameId>, framemem_allocator> &resources);

  void gatherPotentialDeactivationSet(int prev_frame, PotentialDeactivationSet &result);

  void shutdown(int frame);

  void closeTransientResources();

  virtual ~ResourceAllocator() = default;

  // Required for answering external get resource and set resolution requests
  IntermediateResources cachedIntermediateResources;
  IdIndexedMapping<intermediate::ResourceIndex, intermediate::EnhancedBarrier> cachedReleaseBarriers;
  IdSparseIndexedMapping<intermediate::ResourceIndex, intermediate::DebugResourceName> cachedIntermediateResourceNames;

  HeapRequests allocatedHeaps;

protected:
  struct PlacedResource
  {
    AllocationLocation where = NOWHERE;
    D3dResource *object = nullptr;
    uint64_t size = 0;
  };

  D3dResource *getD3dResource(int frame, intermediate::ResourceIndex res_idx) const;

  // With defer_destruction, a discarded heap and its objects stay alive until destroyRetired
  virtual void allocateHeaps(const LayoutHeaps &layout_heaps, bool defer_destruction, ChangedHeaps &changed) = 0;
  virtual void destroyRetired() {}

  void allocateCpuHeaps(int prev_frame, const LayoutHeaps &layout_heaps, const PotentialDeactivationSet &potential_deactivation_set,
    bool defer_destruction, ChangedHeaps &changed);

  virtual D3dResource *placeResource(intermediate::ResourceIndex res_idx, HeapIndex heap_idx, const ResourceDescription &desc,
    uint64_t offset, const ResourceAllocationProperties &properties, const DynamicResolution &dyn_resolution) = 0;

  virtual void lruCleanup() {}
  virtual void releaseObject(HeapIndex, const D3dResource *, uint32_t) {}
  virtual bool doesHeapAgree(HeapIndex heap_idx, const HeapRequest &req) const = 0;
  bool doTablesAgree(const Layout &layout) const;

  void rebindObject(FrameResource what, D3dResource *object);

  void setManagedName(int frame, intermediate::ResourceIndex resIdx, D3dResource *res);
  void refreshManagedTexture(int frame, intermediate::ResourceIndex res_idx, BaseTexture *tex);

  virtual void shutdownInternal() = 0;

protected:
  IGraphDumper &graphDumper;
  Stats stats;

  eastl::array<IdIndexedMapping<intermediate::ResourceIndex, PlacedResource>, SCHEDULE_FRAME_WINDOW> placed;

private:
  // Holds duplicates until the names are written
  using TouchedObjects = dag::Vector<const D3dResource *, framemem_allocator>;

  void updateCachedResources(const intermediate::Graph &graph, const ResourcesChangedFmem &resources_changed,
    const intermediate::HistoryPairing &historyPairing, bool does_pairing_keep_indices);
  void placeFrameResource(FrameResource what, AllocationLocation location, uint64_t size, const ResourceProperties &properties,
    const DynamicResolutions &dyn_resolutions, TouchedObjects &touched);
  void releaseUnpaired(const IdIndexedFlags<intermediate::ResourceIndex, framemem_allocator> &paired, bool does_pairing_keep_indices,
    const IdIndexedMapping<intermediate::ResourceIndex, uint32_t> &keep_frames, TouchedObjects &touched);
  void dropPlacement(FrameResource what, uint32_t keep_frames = 0);
  void removeObjectUser(const D3dResource *object, FrameResource what);
  void forgetChangedHeaps(int prev_frame, const ChangedHeaps &changed, PotentialDeactivationSet &potential_deactivations);
  void unplaceFrameResource(FrameResource what, TouchedObjects &touched, uint32_t keep_frames = 0);
  void updateApiNames(TouchedObjects &touched) const;

  ska::flat_hash_map<const D3dResource *, dag::Vector<FrameResource>> objectUsers;
  IdIndexedFlags<intermediate::ResourceIndex> wipedBlobs;

  IdIndexedMapping<HeapIndex, dag::Vector<char>> cpuHeaps;
  dag::Vector<dag::Vector<char>> retiredCpuHeaps;
  dag::Vector<Relocation> pendingRelocations;
};

} // namespace dafg
