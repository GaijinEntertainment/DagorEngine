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
  ResourceAllocationProperties getResourceAllocationProperties(const ResourceDescription &desc,
    bool force_not_on_chip = false) override;
  ResourceHeapGroupProperties getResourceHeapGroupProperties(ResourceHeapGroup *heap_group) override;
  bool doHeapsCountObjects() const override { return false; }
  bool doesResizeKeepContents() const override { return false; }
  uint64_t getFreeMemoryBudget() const override;

  void allocateHeaps(const LayoutHeaps &layout_heaps, bool defer_destruction, ChangedHeaps &changed) override;
  void destroyRetired() override;

  D3dResource *placeResource(intermediate::ResourceIndex res_idx, HeapIndex heap_idx, const ResourceDescription &desc, uint64_t offset,
    const ResourceAllocationProperties &properties, const DynamicResolution &dyn_resolution) override;

  void lruCleanup() override;
  void releaseObject(HeapIndex heap_idx, const D3dResource *object, uint32_t keep_frames) override;
  bool doesHeapAgree(HeapIndex heap_idx, const HeapRequest &req) const override;

  void shutdownInternal() override;

private:
  IdIndexedMapping<HeapIndex, ResourceHeap *> heaps;
  IdIndexedMapping<HeapIndex, uint64_t> heapCapacities;
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
      G_ASSERT(!ShaderGlobal::is_resource_used_as_unmanaged_pointer(res, true));
#endif
      destroy_d3dres(res);
    }
  };
  using UniqueD3dRes = eastl::unique_ptr<D3dResource, Destroyer>;
  using D3dResourceCache = ResourceLruCache<ResourceKey, D3dResource, Destroyer, ResKeyHash, 128, 8>;
  IdIndexedMapping<HeapIndex, D3dResourceCache> d3dResourceCaches;

  struct AcquiredObject
  {
    ResourceKey key;
    uint32_t refs = 0;
  };
  IdIndexedMapping<HeapIndex, ska::flat_hash_map<const D3dResource *, AcquiredObject>> acquiredObjects;

  struct RetiredHeap
  {
    ResourceHeap *heap = nullptr;
    uint64_t size = 0;
    D3dResourceCache cache;
    ska::flat_hash_map<const D3dResource *, AcquiredObject> acquired;
  };
  dag::Vector<RetiredHeap> retired;

  template <class F>
  D3dResource *acquireObject(HeapIndex heap_idx, const ResourceKey &key, const F &creator);

  template <class T>
  D3dResource *placeDynamicTexture(intermediate::ResourceIndex res_idx, HeapIndex heap_idx, uint64_t offset, uint64_t static_size,
    const ResourceKey &dynamic_key, const T &resolution);

  using PropertyCache = LruCache<ResourceDescription, ResourceAllocationProperties, eastl::hash<ResourceDescription>, 128>;
  PropertyCache allocationPropertiesCache;
};

} // namespace dafg
