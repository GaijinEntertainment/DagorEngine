// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "nativeResourceScheduler.h"

#include <debug/backendDebug.h>
#include <perfMon/dag_statDrv.h>
#include <3d/dag_resizableTex.h>
#include <3d/dag_resPtr.h>
#include <drv/3d/dag_driver.h>
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

NativeResourceScheduler::ResourceKey::ResourceKey(const ResourceDescription &description, const ResourceAllocationProperties &prop,
  uint32_t offset) :
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

NativeResourceScheduler::ResourceKey::ResourceKey(const ResourceKey &key, const IPoint2 &resolution, bool automip) :
  hash(key.rawHash), rawHash(key.rawHash), desc(set_tex_desc_resolution(key.desc, resolution, automip))
{
  G_ASSERT(desc.type == D3DResourceType::TEX || desc.type == D3DResourceType::ARRTEX);
  hashPack(hash, resolution.x, resolution.y);
}

NativeResourceScheduler::ResourceKey::ResourceKey(const ResourceKey &key, const IPoint3 &resolution, bool automip) :
  hash(key.rawHash), rawHash(key.rawHash), desc(set_tex_desc_resolution(key.desc, resolution, automip))
{
  G_ASSERT(desc.type == D3DResourceType::VOLTEX);
  hashPack(hash, resolution.x, resolution.y, resolution.z);
}

D3dResource *NativeResourceScheduler::getD3dResource(int frame, intermediate::ResourceIndex res_idx) const
{
  G_ASSERT(cachedIntermediateResources[res_idx].asScheduled().resourceType == ResourceType::Texture ||
           cachedIntermediateResources[res_idx].asScheduled().resourceType == ResourceType::Buffer);

  D3dResource *res = placedResourceProperties[frame][res_idx].activeResource;
  G_ASSERT(res);
  return res;
}

ResourceAllocationProperties NativeResourceScheduler::getResourceAllocationProperties(const ResourceDescription &desc,
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

ResourceHeapGroupProperties NativeResourceScheduler::getResourceHeapGroupProperties(ResourceHeapGroup *heap_group)
{
  if (heap_group != CPU_HEAP_GROUP)
  {
    auto heapProp = d3d::get_resource_heap_group_properties(heap_group);
    // For an ESRAM heap, maxHeapSize in properties will contain
    // the size of the free ESRAM which is most likely zero if we already
    // allocated the ESRAM heap.
    if (heapProp.isDedicatedFastGPULocal)
      for (auto [group, size] : allocatedHeaps)
        if (group == heap_group)
        {
          heapProp.maxHeapSize = size;
          break;
        }
    return heapProp;
  }

  ResourceHeapGroupProperties fakeHeapGroupProp{};
  fakeHeapGroupProp.flags = 0;
  fakeHeapGroupProp.maxHeapSize = eastl::numeric_limits<uint64_t>::max();
  fakeHeapGroupProp.maxResourceSize = eastl::numeric_limits<uint64_t>::max();
  return fakeHeapGroupProp;
}

ResourceScheduler::DestroyedHeapSet NativeResourceScheduler::allocateHeaps(const HeapRequests &heap_requests)
{
  TIME_PROFILE(allocateHeaps);

  DestroyedHeapSet result;
  result.reserve(heap_requests.size());

  FRAMEMEM_VALIDATE;

  debug("daFG: attempting to reuse %d heaps and allocate %d new ones (%d total):", heaps.size(), heap_requests.size() - heaps.size(),
    heap_requests.size());

  if (heaps.size() < heap_requests.size())
  {
    heaps.resize(heap_requests.size());
    d3dResourceCaches.resize(heap_requests.size());
  }

  for (auto [i, req] : heap_requests.enumerate())
  {
    if (req.group == CPU_HEAP_GROUP)
      continue;

    static constexpr float MIN_OCCUPANCY_RATIO = 0.75f;

    // Only reuse previously allocated heaps if they are big enough
    // AND not too big (requested / allocated > ~3/4)
    // The not to big part is needed to clean up after "burst" memory
    // requests that occur during screenshots in x2 resolution and with
    // tons of history resources
    if (allocatedHeaps.isMapped(i) && req.size <= allocatedHeaps[i].size &&
        static_cast<float>(req.size) / static_cast<float>(allocatedHeaps[i].size) > MIN_OCCUPANCY_RATIO)
    {
      debug("daFG: Reusing heap %d of size %dKB instead of allocating a new %dKB heap", eastl::to_underlying(i),
        allocatedHeaps[i].size >> 10, req.size >> 10);
      continue;
    }

    if (heaps[i] != nullptr)
    {
      d3dResourceCaches[i].clear();
      result.push_back(i);

      d3d::destroy_resource_heap(eastl::exchange(heaps[i], nullptr));
      debug("daFG: Destroyed heap %d because it was too small/big (size was %dKB, but %dKB are required)", eastl::to_underlying(i),
        allocatedHeaps[i].size >> 10, req.size >> 10);
    }
    if (req.size > 0)
    {
      heaps[i] = d3d::create_resource_heap(req.group, req.size, RHCF_REQUIRES_DEDICATED_HEAP);
      if (DAGOR_UNLIKELY(!heaps[i]))
        DAG_FATAL("daFG: Failed to allocate resource heap %d of size %dKB", eastl::to_underlying(i), req.size >> 10);
      else
        debug("daFG: Successfully allocated a new heap %d of size %dKB", eastl::to_underlying(i), req.size >> 10);
    }
    allocatedHeaps[i].size = req.size;
  }

  return result;
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
  result[16] = HEX_DIGITS[heap_idx & 0xf];
  result[17] = HEX_DIGITS[heap_idx >> 4];
  for (uint32_t i = 38; i >= 23; --i)
  {
    result[i] = HEX_DIGITS[hash_value & 0xF];
    hash_value >>= 4;
  }
  return result;
}

void NativeResourceScheduler::resetResourcePlacement()
{
  for (auto &properties : placedResourceProperties)
    properties.assign(cachedIntermediateResources.size(), PlaceProperty{});

  for (auto &cache : d3dResourceCaches)
    cache.lruCleanup();
  allocationPropertiesCache.lruCleanup();
}

void NativeResourceScheduler::placeResource(int frame, intermediate::ResourceIndex res_idx, HeapIndex heap_idx,
  const ResourceDescription &desc, uint32_t offset, const ResourceAllocationProperties &properties,
  const DynamicResolution &dyn_resolution)
{
  const auto key = ResourceKey(desc, properties, offset);
  const auto placedResName = generate_name(eastl::to_underlying(heap_idx), key.hash);

  PlaceProperty property{
    .heapIndex = heap_idx,
    .offset = offset,
    .size = properties.sizeInBytes,
    .staticKey = key,
    .activeDynamicKey = key,
  };

  if (desc.type == D3DResourceType::SBUF)
  {
    auto createBuf = [this, &key, &heap_idx, &placedResName, &offset, &properties]() -> UniqueD3dRes {
      auto buf = d3d::place_buffer_in_resource_heap(heaps[heap_idx], key.desc, offset, properties, placedResName.c_str());
      if (!buf)
        logerr("daFG: Failed to place buffer %s in resource heap %d (%p) at offset %d! Description was: %s", placedResName.c_str(),
          eastl::to_underlying(heap_idx), heaps[heap_idx], offset, key.desc.toDebugString());
      return UniqueD3dRes(buf);
    };

    property.activeResource = d3dResourceCaches[heap_idx].access(key, createBuf).get();
  }
  else
  {
    auto createTex = [this, &key, &heap_idx, &placedResName, &offset, &properties]() -> UniqueD3dRes {
      auto tex = d3d::place_texture_in_resource_heap(heaps[heap_idx], key.desc, offset, properties, placedResName.c_str());
      if (!tex)
        logerr("daFG: Failed to place texture %s in resource heap %d (%p) at offset %d! Description was: %s", placedResName.c_str(),
          eastl::to_underlying(heap_idx), heaps[heap_idx], offset, key.desc.toDebugString());
      return UniqueD3dRes(tex);
    };

    property.activeResource = d3dResourceCaches[heap_idx].access(key, createTex).get();
  }

  placedResourceProperties[frame][res_idx] = eastl::move(property);

  if (desc.type != D3DResourceType::SBUF)
  {
    // TODO: it is possible to skip placing a full-resolution texture and place a
    // dynamic resolution one from the start, but that would require some refactoring.
    const auto autotype = cachedIntermediateResources[res_idx].asScheduled().resolutionType;
    eastl::visit(
      [&](const auto &res) {
        if constexpr (!eastl::is_same_v<eastl::decay_t<decltype(res)>, eastl::monostate>)
          resizeTexture(res_idx, frame, scale_by(res, autotype->multiplier));
      },
      dyn_resolution);
  }
}

void NativeResourceScheduler::shutdownInternal()
{
  d3dResourceCaches.clear();
  placedResourceProperties.fill({});
  for (ResourceHeap *heap : heaps)
    if (heap != nullptr)
      d3d::destroy_resource_heap(heap);
  heaps.clear();
}

NativeResourceScheduler::~NativeResourceScheduler()
{
  // D3D wants us to clear all resources first and then destroy all heaps
  NativeResourceScheduler::shutdownInternal();
}

template <class T>
void NativeResourceScheduler::resizeTexture(intermediate::ResourceIndex res_idx, int frame, const T &resolution)
{
  auto &resProperties = placedResourceProperties[frame][res_idx];

  resProperties.activeDynamicKey =
    ResourceKey(resProperties.staticKey, resolution, cachedIntermediateResources[res_idx].asScheduled().autoMipCount);

  auto createTex = [&]() -> UniqueD3dRes {
    const auto placedResName = generate_name(eastl::to_underlying(resProperties.heapIndex), resProperties.activeDynamicKey.hash);
    const ResourceDescription &desc = resProperties.activeDynamicKey.desc;
    const auto allocInfo = getResourceAllocationProperties(desc);
    if (allocInfo.sizeInBytes > resProperties.size)
      logerr("daFG: dynamic resolution is broken! "
             "Dynamic texture takes %d bytes, which is bigger than "
             "the original static texture took %d bytes for resource %s (frame %d)! "
             "Desired resolution was: %@",
        allocInfo.sizeInBytes, resProperties.size, cachedIntermediateResourceNames[res_idx].c_str(), frame, resolution);
    auto tex = d3d::place_texture_in_resource_heap(heaps[resProperties.heapIndex], desc, resProperties.offset, allocInfo,
      placedResName.c_str());
    G_ASSERT_RETURN(tex, {});
    return UniqueD3dRes(tex);
  };

  resProperties.activeResource = d3dResourceCaches[resProperties.heapIndex].access(resProperties.activeDynamicKey, createTex).get();
}

void NativeResourceScheduler::resizeAutoResTextures(int frame, const DynamicResolutionUpdates &resolutions)
{
  // For safety, we clean up old resources before creating the ones we
  // will need on this frame, just in case the LRU cache is broken (it shouldn't be)
  for (auto &cache : d3dResourceCaches)
    cache.lruCleanup();

  for (auto idx : IdRange<intermediate::ResourceIndex>(cachedIntermediateResources.size()))
  {
    if (!cachedIntermediateResources[idx].isScheduled() || cachedIntermediateResources[idx].getResType() == ResourceType::Blob)
      continue;

    const auto &schedRes = cachedIntermediateResources[idx].asScheduled();

    if (schedRes.resourceType == ResourceType::Texture && schedRes.resolutionType.has_value() &&
        resolutions.isMapped(schedRes.resolutionType->id) &&
        !eastl::holds_alternative<NoDynamicResolutionUpdate>(resolutions[schedRes.resolutionType->id]))
    {
      const auto mult = schedRes.resolutionType->multiplier;
      const auto res = resolutions[schedRes.resolutionType->id];
      eastl::visit(
        [&](const auto &res) {
          if constexpr (!eastl::is_same_v<eastl::decay_t<decltype(res)>, NoDynamicResolutionUpdate>)
            resizeTexture(idx, frame, scale_by(res.dynamicRes, mult));
        },
        res);

      refreshManagedTexture(frame, idx, static_cast<BaseTexture *>(getD3dResource(frame, idx)));
    }
    else
    {
      const auto &props = placedResourceProperties[frame][idx];
      d3dResourceCaches[props.heapIndex].touch(props.activeDynamicKey);
    }
  }
}

} // namespace dafg
