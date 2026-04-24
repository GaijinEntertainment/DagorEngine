// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "resourceAllocator.h"
#include "lruCache.h"
#include "debug/backendDebug.h"
#include <3d/dag_resizableTex.h>

namespace dafg
{

// Uses native gAPI heaps
class NativeResourceAllocator final : public ResourceAllocator
{
public:
  NativeResourceAllocator(IGraphDumper &dumper) : ResourceAllocator{dumper} {}

  void resizeAutoResTextures(int frame, const DynamicResolutionUpdates &resolutions) override;

  ~NativeResourceAllocator() override;

private:
  D3dResource *getD3dResource(int frame, intermediate::ResourceIndex res_idx) const override;

  ResourceAllocationProperties getResourceAllocationProperties(const ResourceDescription &desc,
    bool force_not_on_chip = false) override;
  ResourceHeapGroupProperties getResourceHeapGroupProperties(ResourceHeapGroup *heap_group) override;

  DestroyedHeapSet allocateHeaps(const HeapRequests &heap_requests) override;

  void resetResourcePlacement() override;
  void placeResource(int frame, intermediate::ResourceIndex res_idx, HeapIndex heap_idx, const ResourceDescription &desc,
    uint64_t offset, const ResourceAllocationProperties &properties, const DynamicResolution &dyn_resolution) override;

  template <class T>
  void resizeTexture(intermediate::ResourceIndex res_idx, int frame, const T &resolution);
  void shutdownInternal() override;

private:
  IdIndexedMapping<HeapIndex, ResourceHeap *> heaps;
  struct ResourceKey
  {
    ResourceKey() : hash{0}, rawHash{0}, desc{} {}
    ResourceKey(const ResourceDescription &description, const ResourceAllocationProperties &properties, uint64_t allignment);
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
    void operator()(D3dResource *res) const
    {
      validation_remove_resource(res);
#if DAGOR_DBGLEVEL > 0
      G_ASSERT(!ShaderGlobal::is_resource_used_as_umnamaged_pointer(res, true));
#endif
      destroy_d3dres(res);
    }
  };
  using UniqueD3dRes = eastl::unique_ptr<D3dResource, Destroyer>;
  using D3dResourceCache = LruCache<ResourceKey, UniqueD3dRes, ResKeyHash, 128>;
  IdIndexedMapping<HeapIndex, D3dResourceCache> d3dResourceCaches;

  struct PlaceProperty
  {
    HeapIndex heapIndex = HeapIndex::Invalid;
    uint64_t offset = ~uint64_t(0);
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
