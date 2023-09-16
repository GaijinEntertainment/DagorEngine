#pragma once

#include "resourceScheduler.h"
#include <ska_hash_map/flat_hash_map2.hpp>
#include <3d/dag_resizableTex.h>

namespace dabfg
{

// Uses native gAPI heaps
class NativeResourceScheduler final : public ResourceScheduler
{
public:
  NativeResourceScheduler(IGraphDumper &dumper) : ResourceScheduler{dumper} {}

  ManagedTexView getTexture(int frame, intermediate::ResourceIndex res_idx) const override;
  ManagedBufView getBuffer(int frame, intermediate::ResourceIndex res_idx) const override;

  void resizeAutoResTextures(int frame, const DynamicResolutions &resolutions) override;

  ~NativeResourceScheduler() override;

private:
  ResourceAllocationProperties getResourceAllocationProperties(const ResourceDescription &desc,
    bool force_not_on_chip = false) override;
  ResourceHeapGroupProperties getResourceHeapGroupProperties(ResourceHeapGroup *heap_group) override;

  template <typename ResType>
  ManagedResView<typename ResType::BaseType> getResource(int frame, intermediate::ResourceIndex res_idx) const;

  DestroyedHeapSet allocateHeaps(const HeapRequests &heap_requests) override;

  void placeResource(int frame, intermediate::ResourceIndex res_idx, HeapIndex heap_idx, const ResourceDescription &desc,
    uint32_t offset, const ResourceAllocationProperties &properties) override;

  void resizeTexture(intermediate::ResourceIndex res_idx, int frame, const Point2 &resolution);
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
    ResourceKey(const ResourceDescription &description, const ResourceAllocationProperties &properties, uint32_t allignment);
    ResourceKey(const ResourceKey &key, const Point2 &resolution);
    inline bool operator==(const ResourceKey &key) const;
    size_t hash;
    size_t rawHash;
    ResourceDescription desc;
  };
  // Properties needed to create a resized texture
  // in the same place as the cached one.
  struct PlaceProperty
  {
    HeapIndex heapIndex;
    ResourceKey key;
  };
  struct ResKeyHash
  {
    inline size_t operator()(const ResourceKey &k) const { return k.hash; }
  };
  struct ResourceEntry
  {
    uint32_t collectionIdx;
    eastl::variant<UniqueBuf, UniqueTex> resource;
  };
  using ResourceHashMap = ska::flat_hash_map<ResourceKey, ResourceEntry, ResKeyHash>;
  IdIndexedMapping<HeapIndex, ResourceHashMap> placedHeapResources;

  template <typename ResType, typename ResourceCreator>
  void placeResource(int frame, intermediate::ResourceIndex res_idx, HeapIndex heap_idx, const ResourceKey &key,
    const ResourceCreator &resourceCreator);

  dag::Vector<PlaceProperty> placedProperties;
  ska::flat_hash_map<ResourceDescription, ResourceAllocationProperties> allocationPropertiesCache;
};

} // namespace dabfg
