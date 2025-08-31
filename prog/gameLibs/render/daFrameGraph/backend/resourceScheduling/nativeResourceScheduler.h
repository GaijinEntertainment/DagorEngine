// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "resourceScheduler.h"
#include "lruCache.h"
#include <3d/dag_resizableTex.h>

namespace dafg
{

// Uses native gAPI heaps
class NativeResourceScheduler final : public ResourceScheduler
{
public:
  NativeResourceScheduler(IGraphDumper &dumper, const BadResolutionTracker &br_tracker) : ResourceScheduler{dumper, br_tracker} {}

  void resizeAutoResTextures(int frame, const DynamicResolutionUpdates &resolutions) override;

  ~NativeResourceScheduler() override;

private:
  D3dResource *getD3dResource(int frame, intermediate::ResourceIndex res_idx) const override;

  ResourceAllocationProperties getResourceAllocationProperties(const ResourceDescription &desc,
    bool force_not_on_chip = false) override;
  ResourceHeapGroupProperties getResourceHeapGroupProperties(ResourceHeapGroup *heap_group) override;

  DestroyedHeapSet allocateHeaps(const HeapRequests &heap_requests) override;

  void resetResourcePlacement() override;
  void placeResource(int frame, intermediate::ResourceIndex res_idx, HeapIndex heap_idx, const ResourceDescription &desc,
    uint32_t offset, const ResourceAllocationProperties &properties, const DynamicResolution &dyn_resolution) override;

  template <class T>
  void resizeTexture(intermediate::ResourceIndex res_idx, int frame, const T &resolution);
  void shutdownInternal() override;

private:
  // Heaps allocated by us
  // WARNING: this can contain nullptrs for heaps of size 0
  // this simplifies heap group logic
  IdIndexedMapping<HeapIndex, ResourceHeap *> heaps;
  // Resource hashing with key as description, offset and allignment
  // individually for each heap.
  struct ResourceKey
  {
    ResourceKey() : hash{0}, rawHash{0}, desc{} {}
    ResourceKey(const ResourceDescription &description, const ResourceAllocationProperties &properties, uint32_t allignment);
    ResourceKey(const ResourceKey &key, const IPoint2 &resolution, bool automip);
    ResourceKey(const ResourceKey &key, const IPoint3 &resolution, bool automip);
    bool operator==(const ResourceKey &key) const = default;
    size_t hash;
    size_t rawHash;
    ResourceDescription desc;
  };

  struct ResKeyHash
  {
    inline size_t operator()(const ResourceKey &k) const { return k.hash; }
  };
  struct Destroyer
  {
    void operator()(D3dResource *res) const { destroy_d3dres(res); }
  };
  using UniqueD3dRes = eastl::unique_ptr<D3dResource, Destroyer>;
  using D3dResourceCache = LruCache<ResourceKey, UniqueD3dRes, ResKeyHash, 128>;
  IdIndexedMapping<HeapIndex, D3dResourceCache> d3dResourceCaches;

  // Properties needed to create a resized texture
  // in the same place as the cached one.
  struct PlaceProperty
  {
    HeapIndex heapIndex = HeapIndex::Invalid;
    uint32_t offset = static_cast<uint32_t>(-1);
    size_t size = 0;
    ResourceKey staticKey;
    ResourceKey activeDynamicKey;
    D3dResource *activeResource = nullptr;
  };
  eastl::array<IdIndexedMapping<intermediate::ResourceIndex, PlaceProperty>, SCHEDULE_FRAME_WINDOW> placedResourceProperties;

  using PropertyCache = LruCache<ResourceDescription, ResourceAllocationProperties, eastl::hash<ResourceDescription>, 128>;
  PropertyCache allocationPropertiesCache;
};

} // namespace dafg
