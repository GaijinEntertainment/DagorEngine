// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "nativeResourceAllocator.h"

#include <EASTL/sort.h>
#include <debug/backendDebug.h>
#include <perfMon/dag_statDrv.h>
#include <3d/dag_resizableTex.h>
#include <3d/dag_resPtr.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_commands.h>
#include <3d/dag_resourceTags.h>
#include <memory/dag_framemem.h>

#include <render/daFrameGraph/resourceCreation.h>

#include <common/genericPoint.h>
#include <common/dynamicResolution.h>
#include <id/idRange.h>


namespace dafg
{

template <typename... Ts>
static inline void hashPack(size_t &hash, size_t first, const Ts &...other)
{
  hash ^= first + 0x9e3779b9 + (hash << 6) + (hash >> 2);
  (hashPack(hash, other), ...);
}

NativeResourceAllocator::ResourceKey::ResourceKey(const ResourceDescription &description, const ResourceAllocationProperties &prop,
  uint64_t offset) :
  desc(description)
{
  switch (description.type)
  {
    case D3DResourceType::SBUF:
      rawHash = desc.hash();
      hashPack(rawHash, offset, prop.offsetAlignment);
      hash = rawHash;
      break;

    case D3DResourceType::TEX:
    case D3DResourceType::ARRTEX:
      desc.asTexRes.width = 0;
      desc.asTexRes.height = 0;
      rawHash = desc.hash();
      hashPack(rawHash, offset, prop.offsetAlignment);
      hash = rawHash;
      hashPack(hash, description.asTexRes.width, description.asTexRes.height);
      desc.asTexRes.width = description.asTexRes.width;
      desc.asTexRes.height = description.asTexRes.height;
      break;

    case D3DResourceType::CUBETEX:
    case D3DResourceType::CUBEARRTEX:
      desc.asCubeTexRes.extent = 0;
      rawHash = desc.hash();
      hashPack(rawHash, offset, prop.offsetAlignment);
      hash = rawHash;
      hashPack(hash, description.asCubeTexRes.extent);
      desc.asCubeTexRes.extent = description.asCubeTexRes.extent;
      break;

    case D3DResourceType::VOLTEX:
      desc.asVolTexRes.width = 0;
      desc.asVolTexRes.height = 0;
      desc.asVolTexRes.depth = 0;
      rawHash = desc.hash();
      hashPack(rawHash, offset, prop.offsetAlignment);
      hash = rawHash;
      hashPack(hash, description.asVolTexRes.width, description.asVolTexRes.height, description.asVolTexRes.depth);
      desc.asVolTexRes.width = description.asVolTexRes.width;
      desc.asVolTexRes.height = description.asVolTexRes.height;
      desc.asVolTexRes.depth = description.asVolTexRes.depth;
      break;

    default: G_ASSERT(false); break;
  }
}

NativeResourceAllocator::ResourceKey::ResourceKey(const ResourceKey &key, const IPoint2 &resolution, bool automip) :
  hash(key.rawHash), rawHash(key.rawHash), desc(set_tex_desc_resolution(key.desc, resolution, automip))
{
  G_ASSERT(desc.type == D3DResourceType::TEX || desc.type == D3DResourceType::ARRTEX);
  hashPack(hash, resolution.x, resolution.y);
}

NativeResourceAllocator::ResourceKey::ResourceKey(const ResourceKey &key, const IPoint3 &resolution, bool automip) :
  hash(key.rawHash), rawHash(key.rawHash), desc(set_tex_desc_resolution(key.desc, resolution, automip))
{
  G_ASSERT(desc.type == D3DResourceType::VOLTEX);
  hashPack(hash, resolution.x, resolution.y, resolution.z);
}

ResourceAllocationProperties NativeResourceAllocator::getResourceAllocationProperties(const ResourceDescription &desc,
  bool force_not_on_chip)
{
  ResourceDescription updatedDescription = desc;
  if (force_not_on_chip)
  {
#if _TARGET_XBOX
    const uint32_t esramFlag = D3DResourceType::SBUF == desc.type ? (unsigned)SBCF_MISC_ESRAM_ONLY : (unsigned)TEXCF_ESRAM_ONLY;
    updatedDescription.asBasicRes.cFlags &= ~esramFlag;
#endif
  }
  return allocationPropertiesCache.access(updatedDescription, [&]() {
    const auto resProps = d3d::get_resource_allocation_properties(updatedDescription);
    G_ASSERT(resProps.sizeInBytes != 0);
    return resProps;
  });
}

ResourceHeapGroupProperties NativeResourceAllocator::getResourceHeapGroupProperties(ResourceHeapGroup *heap_group)
{
  G_ASSERT(heap_group != CPU_HEAP_GROUP);
  auto heapProp = d3d::get_resource_heap_group_properties(heap_group);
  // For an ESRAM heap, maxHeapSize in properties will contain
  // the size of the free ESRAM which is most likely zero if we already
  // allocated the ESRAM heap.
  // ESRAM heaps never hold untracked buffers, so the tracked/untracked heap
  // split never applies here and matching this group alone is exact.
  if (heapProp.isDedicatedFastGPULocal)
    for (const auto &heap : allocatedHeaps)
      if (heap.group == heap_group)
      {
        heapProp.maxHeapSize = heap.size;
        break;
      }
  return heapProp;
}

uint64_t NativeResourceAllocator::getFreeMemoryBudget() const
{
  uint32_t totalKb = 0;
  uint32_t freeKb = 0;
  uint32_t usedKb = 0;
  d3d::driver_command(Drv3dCommand::GET_VIDEO_MEMORY_BUDGET, &totalKb, &freeKb, &usedKb);
  return totalKb == 0 ? UNKNOWN_MEMORY_BUDGET : static_cast<uint64_t>(freeKb) << 10;
}

void NativeResourceAllocator::allocateHeaps(const LayoutHeaps &layout_heaps, bool defer_destruction, ChangedHeaps &changed)
{
  TIME_PROFILE(allocateHeaps);

  FRAMEMEM_VALIDATE;

  if (heaps.size() < layout_heaps.size())
  {
    heaps.resize(layout_heaps.size());
    heapCapacities.resize(layout_heaps.size(), 0);
    d3dResourceCaches.resize(layout_heaps.size());
    acquiredObjects.resize(layout_heaps.size());
  }

  struct DiscardedHeap
  {
    ResourceHeap *heap;
    ResourceHeapGroup *group;
    uint64_t size;
    HeapIndex from;
    bool operator<(const DiscardedHeap &rhs) const { return size < rhs.size; }
  };
  dag::Vector<DiscardedHeap, framemem_allocator> discardedHeaps;

  for (auto &&[i, heap] : layout_heaps.enumerate())
  {
    const auto req = make_heap_request(heap);
    auto &allocated = allocatedHeaps[i];
    const bool sameClass = allocated.group == req.group && allocated.untracked == req.untracked;
    const bool grewWithinCapacity = req.size > allocated.size && req.size <= heapCapacities[i];
    if (heaps[i] != nullptr && sameClass && (req.size == allocated.size || grewWithinCapacity))
    {
      allocated.size = req.size;
      continue;
    }

    if (heaps[i] != nullptr)
    {
      changed.set(i, true);
      if (defer_destruction)
        retired.push_back(RetiredHeap{
          .heap = eastl::exchange(heaps[i], nullptr),
          .size = heapCapacities[i],
          .cache = eastl::move(d3dResourceCaches[i]),
          .acquired = eastl::move(acquiredObjects[i]),
        });
      else
        discardedHeaps.push_back({
          .heap = eastl::exchange(heaps[i], nullptr),
          .group = allocated.group,
          .size = heapCapacities[i],
          .from = i,
        });
      d3dResourceCaches[i] = D3dResourceCache{};
      acquiredObjects[i] = {};
      allocated.size = 0;
      heapCapacities[i] = 0;
    }
    if (req.group == CPU_HEAP_GROUP)
      continue;
    allocated.group = req.group;
    allocated.untracked = req.untracked;
    allocated.size = 0;
    heapCapacities[i] = 0;
  }

  eastl::sort(discardedHeaps.begin(), discardedHeaps.end());

  // try to reassign a discarded heap (avoiding a GPU alloc+dealloc pair), otherwise allocate new
  for (auto &&[i, heap] : layout_heaps.enumerate())
  {
    const auto req = make_heap_request(heap);
    if (req.group == CPU_HEAP_GROUP || heaps[i] != nullptr || req.size == 0)
      continue;

    const auto rangeBegin = eastl::lower_bound(discardedHeaps.begin(), discardedHeaps.end(),
      DiscardedHeap{
        .size = req.size,
      });
    const uint64_t maxSize = 2 * req.size;
    const auto rangeEnd = eastl::upper_bound(rangeBegin, discardedHeaps.end(),
      DiscardedHeap{
        .size = maxSize,
      });
    auto match = eastl::find_if(rangeBegin, rangeEnd,
      [group = req.group, i = i](const DiscardedHeap &d) { return d.group == group && d.from != i; });

    if (match != rangeEnd)
    {
      heaps[i] = match->heap;
      heapCapacities[i] = match->size;
      allocatedHeaps[i].size = req.size;
      ++stats.heapsReassigned;
      changed.set(i, true);
      debug("daFG: Reassigned a discarded heap to index %d (discarded size %dKB, requested %dKB)", eastl::to_underlying(i),
        match->size >> 10, req.size >> 10);
      discardedHeaps.erase(match);
    }
  }

  // destroy discarded heaps that were not reassigned
  for (auto &discarded : discardedHeaps)
  {
    d3d::destroy_resource_heap(discarded.heap);
    ++stats.heapsDestroyed;
    debug("daFG: Destroyed discarded heap of size %dKB", discarded.size >> 10);
  }

  // allocate new heaps only when discarded heaps are freed and driver can reuse memory
  for (auto &&[i, heap] : layout_heaps.enumerate())
  {
    const auto req = make_heap_request(heap);
    if (req.group == CPU_HEAP_GROUP || heaps[i] != nullptr || req.size == 0)
      continue;

    heaps[i] = d3d::create_resource_heap(req.group, req.size, RHCF_REQUIRES_DEDICATED_HEAP, RESTAG_FRAMEGRAPH);
    if (DAGOR_UNLIKELY(!heaps[i]))
      DAG_FATAL("daFG: Failed to allocate resource heap %d of size %dKB", eastl::to_underlying(i), req.size >> 10);
    else
      debug("daFG: Successfully allocated a new heap %d of size %dKB", eastl::to_underlying(i), req.size >> 10);
    allocatedHeaps[i].size = req.size;
    heapCapacities[i] = req.size;
    ++stats.heapsAllocated;
    changed.set(i, true);
  }
}

static auto generate_name(uint32_t heap_idx, size_t hash_value)
{
  constexpr const char *HEX_DIGITS = "0123456789ABCDEF";
  // WARNING: sizes for the fixed string and the name template must match!
  eastl::fixed_string<char, 40> result = "fg_resource_heap$$_hash$$$$$$$$$$$$$$$$";
  if (DAGOR_UNLIKELY(heap_idx >= 0xFF))
  {
    logerr("daFG: has allocated more than 255 resource heaps! This shouldn't be happening!");
    heap_idx &= 0xFF;
  }
  result[16] = HEX_DIGITS[heap_idx >> 4];
  result[17] = HEX_DIGITS[heap_idx & 0xf];
  for (uint32_t i = 38; i >= 23; --i)
  {
    result[i] = HEX_DIGITS[hash_value & 0xF];
    hash_value >>= 4;
  }
  return result;
}

D3dResource *NativeResourceAllocator::placeResource(intermediate::ResourceIndex res_idx, HeapIndex heap_idx,
  const ResourceDescription &desc, uint64_t offset, const ResourceAllocationProperties &properties,
  const DynamicResolution &dyn_resolution)
{

  const auto key = ResourceKey(desc, properties, offset);
  const auto placedResName = generate_name(eastl::to_underlying(heap_idx), key.hash);

  D3dResource *object = nullptr;

  if (desc.type == D3DResourceType::SBUF)
  {
    auto createBuf = [this, &key, &heap_idx, &placedResName, &offset, &properties]() -> UniqueD3dRes {
      TIME_PROFILE(placeBufferInHeap);
      ++stats.objectsCreated;
      auto buf = d3d::place_buffer_in_resource_heap(heaps[heap_idx], key.desc, offset, properties, placedResName.c_str());
      if (!buf)
        logerr("daFG: Failed to place buffer %s in resource heap %d (%p) at offset %d! Description was: %s", placedResName.c_str(),
          eastl::to_underlying(heap_idx), heaps[heap_idx], offset, key.desc.toDebugString());
      return UniqueD3dRes(buf);
    };

    object = acquireObject(heap_idx, key, createBuf);
  }
  else
  {
    const auto &schedRes = cachedIntermediateResources[res_idx].asScheduled();
    const bool placedDynamicTex = eastl::visit(
      [&](const auto &res) {
        if constexpr (!eastl::is_same_v<eastl::decay_t<decltype(res)>, eastl::monostate>)
        {
          const auto resolution = scale_by(res, schedRes.resolutionType->multiplier);
          const ResourceKey dynamicKey(key, resolution, schedRes.autoMipCount);
          if (dynamicKey != key)
          {
            object = placeDynamicTexture(res_idx, heap_idx, offset, properties.sizeInBytes, dynamicKey, resolution);
            return true;
          }
        }
        return false;
      },
      dyn_resolution);
    if (placedDynamicTex)
      return object;

    auto createTex = [this, &key, &heap_idx, &placedResName, &offset, &properties]() -> UniqueD3dRes {
      TIME_PROFILE(placeTextureInHeap);
      ++stats.objectsCreated;
      auto tex = d3d::place_texture_in_resource_heap(heaps[heap_idx], key.desc, offset, properties, placedResName.c_str());
      if (!tex)
        logerr("daFG: Failed to place texture %s in resource heap %d (%p) at offset %d! Description was: %s", placedResName.c_str(),
          eastl::to_underlying(heap_idx), heaps[heap_idx], offset, key.desc.toDebugString());
      return UniqueD3dRes(tex);
    };

    object = acquireObject(heap_idx, key, createTex);
  }

  return object;
}

template <class F>
D3dResource *NativeResourceAllocator::acquireObject(HeapIndex heap_idx, const ResourceKey &key, const F &creator)
{
  D3dResource *object = d3dResourceCaches[heap_idx].access(key, creator);
  if (object != nullptr)
  {
    auto &acquired = acquiredObjects[heap_idx][object];
    acquired.key = key;
    ++acquired.refs;
  }
  return object;
}

void NativeResourceAllocator::releaseObject(HeapIndex heap_idx, const D3dResource *object, uint32_t keep_frames)
{
  if (!acquiredObjects.isMapped(heap_idx))
    return;
  auto &acquired = acquiredObjects[heap_idx];
  const auto it = acquired.find(object);
  if (it == acquired.end())
    return;
  d3dResourceCaches[heap_idx].dropReference(it->second.key, keep_frames);
  if (--it->second.refs == 0)
    acquired.erase(it);
}

void NativeResourceAllocator::lruCleanup()
{
  allocationPropertiesCache.lruCleanup();

  for (auto &cache : d3dResourceCaches)
  {
    cache.retireUnreferenced();
    cache.compact();
  }
}

bool NativeResourceAllocator::doesHeapAgree(HeapIndex heap_idx, const HeapRequest &req) const
{
  const bool has = heaps[heap_idx] != nullptr;
  return has == (req.size > 0) && (!has || heapCapacities[heap_idx] >= req.size);
}

void NativeResourceAllocator::destroyRetired()
{
  for (auto &entry : retired)
  {
    entry.cache.clear();
    entry.acquired.clear();
    d3d::destroy_resource_heap(entry.heap);
    ++stats.heapsDestroyed;
    debug("daFG: Destroyed retired heap of size %dKB", entry.size >> 10);
  }
  retired.clear();
}

void NativeResourceAllocator::shutdownInternal()
{
  destroyRetired();
  allocationPropertiesCache.clear();
  d3dResourceCaches.clear();
  acquiredObjects.clear();
  for (ResourceHeap *heap : heaps)
    if (heap != nullptr)
      d3d::destroy_resource_heap(heap);
  heaps.clear();
  heapCapacities.clear();
}

NativeResourceAllocator::~NativeResourceAllocator()
{
  // D3D wants us to clear all resources first and then destroy all heaps
  NativeResourceAllocator::shutdownInternal();
}

template <class T>
D3dResource *NativeResourceAllocator::placeDynamicTexture(intermediate::ResourceIndex res_idx, HeapIndex heap_idx, uint64_t offset,
  uint64_t static_size, const ResourceKey &dynamic_key, const T &resolution)
{
  auto createTex = [&]() -> UniqueD3dRes {
    const auto placedResName = generate_name(eastl::to_underlying(heap_idx), dynamic_key.hash);
    const ResourceDescription &desc = dynamic_key.desc;
    const auto allocInfo = getResourceAllocationProperties(desc);
    if (allocInfo.sizeInBytes > static_size)
      logerr("daFG: dynamic resolution is broken! "
             "Dynamic texture takes %d bytes, which is bigger than "
             "the original static texture took %d bytes for resource %s! "
             "Desired resolution was: %@",
        allocInfo.sizeInBytes, static_size, cachedIntermediateResourceNames[res_idx].c_str(), resolution);
    ++stats.objectsCreated;
    auto tex = d3d::place_texture_in_resource_heap(heaps[heap_idx], desc, offset, allocInfo, placedResName.c_str());
    if (!tex)
    {
      logerr("daFG: Failed to place dynamic resolution texture %s (%s) in resource heap %d (%p) at offset %lld! "
             "Desired resolution was: %@. Description was: %s",
        cachedIntermediateResourceNames[res_idx].c_str(), placedResName.c_str(), eastl::to_underlying(heap_idx), heaps[heap_idx],
        offset, resolution, desc.toDebugString());
      return {};
    }
    validation_add_resource(tex);
    return UniqueD3dRes(tex);
  };

  return acquireObject(heap_idx, dynamic_key, createTex);
}

void NativeResourceAllocator::resizeAutoResTextures(int frame, const DynamicResolutionUpdates &resolutions)
{
  for (auto &cache : d3dResourceCaches)
    cache.tick();

  if (resolutions.empty())
    return;

  for (auto idx : cachedIntermediateResources.keys())
  {
    if (!cachedIntermediateResources[idx].isScheduled() || cachedIntermediateResources[idx].getResType() == ResourceType::Blob)
      continue;

    const auto &schedRes = cachedIntermediateResources[idx].asScheduled();
    if (schedRes.resourceType != ResourceType::Texture || !schedRes.resolutionType.has_value() ||
        !resolutions.isMapped(schedRes.resolutionType->id) ||
        eastl::holds_alternative<NoDynamicResolutionUpdate>(resolutions[schedRes.resolutionType->id]))
      continue;

    const AllocationLocation where = placed[frame][idx].where;
    const HeapIndex heapIdx = where.heap;
    const uint64_t offset = where.offset;
    if (heapIdx == HeapIndex::Invalid)
      continue;

    const auto &desc = schedRes.getGpuDescription();
    const ResourceKey staticKey(desc, getResourceAllocationProperties(desc), offset);

    const auto mult = schedRes.resolutionType->multiplier;
    const uint64_t staticSize = placed[frame][idx].size;
    D3dResource *object = nullptr;
    eastl::visit(
      [&](const auto &res) {
        if constexpr (!eastl::is_same_v<eastl::decay_t<decltype(res)>, NoDynamicResolutionUpdate>)
        {
          const auto resolution = scale_by(res.dynamicRes, mult);
          object = placeDynamicTexture(idx, heapIdx, offset, staticSize, ResourceKey(staticKey, resolution, schedRes.autoMipCount),
            resolution);
        }
      },
      resolutions[schedRes.resolutionType->id]);

    rebindObject(
      {
        .resIdx = idx,
        .frame = static_cast<uint32_t>(frame),
      },
      object);
    if (object != nullptr)
      refreshManagedTexture(frame, idx, static_cast<BaseTexture *>(object));
  }

  for (auto &cache : d3dResourceCaches)
    cache.retireUnreferenced();
}

} // namespace dafg
