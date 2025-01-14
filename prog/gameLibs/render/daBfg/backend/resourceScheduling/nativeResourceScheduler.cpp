// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "nativeResourceScheduler.h"

#include <debug/backendDebug.h>
#include <perfMon/dag_statDrv.h>
#include <3d/dag_resizableTex.h>
#include <3d/dag_resPtr.h>
#include <drv/3d/dag_driver.h>
#include <memory/dag_framemem.h>

#include <render/daBfg/resourceCreation.h>

#include <common/genericPoint.h>
#include <id/idRange.h>


namespace dabfg
{

static ResourceDescription fixup_description(const ResourceDescription &desc)
{
  ResourceDescription fixed_desc = desc;
  switch (desc.resType)
  {
    case RES3D_TEX:
    case RES3D_ARRTEX:
      if (desc.asBasicTexRes.mipLevels == AUTO_MIP_COUNT)
        fixed_desc.asBasicTexRes.mipLevels = auto_mip_levels_count(desc.asTexRes.width, desc.asTexRes.height, 1);
      break;

    case RES3D_CUBETEX:
    case RES3D_CUBEARRTEX:
      if (desc.asBasicTexRes.mipLevels == AUTO_MIP_COUNT)
        fixed_desc.asBasicTexRes.mipLevels = auto_mip_levels_count(desc.asCubeTexRes.extent, 1);
      break;

    case RES3D_VOLTEX:
      if (desc.asBasicTexRes.mipLevels == AUTO_MIP_COUNT)
        fixed_desc.asBasicTexRes.mipLevels =
          auto_mip_levels_count(desc.asVolTexRes.width, desc.asVolTexRes.height, desc.asVolTexRes.depth, 1);
      break;

    default: break;
  }

  return fixed_desc;
}

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
  switch (description.resType)
  {
    case RES3D_SBUF:
      rawHash = desc.hash();
      hashPack(rawHash, offset, prop.offsetAlignment);
      hash = rawHash;
      break;

    case RES3D_TEX:
    case RES3D_ARRTEX:
      desc.asTexRes.width = 0;
      desc.asTexRes.height = 0;
      rawHash = desc.hash();
      hashPack(rawHash, offset, prop.offsetAlignment);
      hash = rawHash;
      hashPack(hash, description.asTexRes.width, description.asTexRes.height);
      desc.asTexRes.width = description.asTexRes.width;
      desc.asTexRes.height = description.asTexRes.height;
      break;

    case RES3D_CUBETEX:
    case RES3D_CUBEARRTEX:
      desc.asCubeTexRes.extent = 0;
      rawHash = desc.hash();
      hashPack(rawHash, offset, prop.offsetAlignment);
      hash = rawHash;
      hashPack(hash, description.asCubeTexRes.extent);
      desc.asCubeTexRes.extent = description.asCubeTexRes.extent;
      break;

    case RES3D_VOLTEX:
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

NativeResourceScheduler::ResourceKey::ResourceKey(const ResourceKey &key, const Point2 &resolution) :
  hash(key.rawHash), rawHash(key.rawHash), desc(key.desc)
{
  G_ASSERT(desc.resType == RES3D_TEX || desc.resType == RES3D_ARRTEX);
  hashPack(hash, resolution.x, resolution.y);
  desc.asTexRes.width = resolution.x;
  desc.asTexRes.height = resolution.y;
}

NativeResourceScheduler::ResourceKey::ResourceKey(const ResourceKey &key, const Point3 &resolution) :
  hash(key.rawHash), rawHash(key.rawHash), desc(key.desc)
{
  G_ASSERT(desc.resType == RES3D_VOLTEX);
  hashPack(hash, resolution.x, resolution.y, resolution.z);
  desc.asVolTexRes.width = resolution.x;
  desc.asVolTexRes.height = resolution.y;
  desc.asVolTexRes.depth = resolution.z;
}

inline bool NativeResourceScheduler::ResourceKey::operator==(const ResourceKey &key) const { return hash == key.hash; }

template <typename ResType>
ManagedResView<typename ResType::BaseType> NativeResourceScheduler::getResource(int frame, intermediate::ResourceIndex res_id) const
{
  const auto resIdxInCollection = resourceIndexInCollection[frame][res_id];
  G_ASSERT_RETURN(resIdxInCollection != UNSCHEDULED, {});

  const auto &resProp = placedProperties[resIdxInCollection];
  const auto &placedResources = placedHeapResources[resProp.heapIndex];
  const auto it = placedResources.find(resProp.key);
  G_ASSERT_RETURN(it != placedResources.end(), {});

  ManagedResView<typename ResType::BaseType> resView = eastl::get<ResType>(it->second.resource);
  G_ASSERT_RETURN(resView, {});
  resView->setResName(cachedIntermediateResourceNames[res_id].c_str(), cachedIntermediateResourceNames[res_id].size());
  return resView;
}

ManagedTexView NativeResourceScheduler::getTexture(int frame, intermediate::ResourceIndex res_id) const
{
  return getResource<UniqueTex>(frame, res_id);
}

ManagedBufView NativeResourceScheduler::getBuffer(int frame, intermediate::ResourceIndex res_id) const
{
  return getResource<UniqueBuf>(frame, res_id);
}

ResourceAllocationProperties NativeResourceScheduler::getResourceAllocationProperties(const ResourceDescription &desc,
  bool force_not_on_chip)
{
  auto updatedDescription = fixup_description(desc);
  if (force_not_on_chip)
  {
#if _TARGET_XBOX
    const uint32_t esramFlag = RES3D_SBUF == desc.resType ? (unsigned)SBCF_MISC_ESRAM_ONLY : (unsigned)TEXCF_ESRAM_ONLY;
    updatedDescription.asBasicRes.cFlags &= ~esramFlag;
#endif
  }
  auto cachedValueIter = allocationPropertiesCache.find(updatedDescription);
  if (cachedValueIter != allocationPropertiesCache.end())
    return cachedValueIter->second;

  const auto resProps = d3d::get_resource_allocation_properties(updatedDescription);
  G_ASSERT(resProps.sizeInBytes != 0);
  return allocationPropertiesCache[updatedDescription] = resProps;
}

ResourceHeapGroupProperties NativeResourceScheduler::getResourceHeapGroupProperties(ResourceHeapGroup *heap_group)
{
  if (heap_group != CPU_HEAP_GROUP)
  {
    auto heapProp = d3d::get_resource_heap_group_properties(heap_group);
    // For an ESRAM heap, maxHeapSize in properties will contain
    // the size of the free ESRAM which is most likely zero if we already
    // allocated the ESRAM heap.
    if (heapProp.isOnChip)
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

  debug("daBfg: attempting to reuse %d heaps and allocate %d new ones (%d total):", heaps.size(), heap_requests.size() - heaps.size(),
    heap_requests.size());

  if (heaps.size() < heap_requests.size())
  {
    heaps.resize(heap_requests.size());
    placedHeapResources.resize(heap_requests.size());
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
      debug("daBfg: Reusing heap %d of size %dKB instead of allocating a new %dKB heap", eastl::to_underlying(i),
        allocatedHeaps[i].size >> 10, req.size >> 10);
      continue;
    }

    if (heaps[i] != nullptr)
    {
      placedHeapResources[i].clear();
      result.push_back(i);

      if (temporalResourceLocations.isMapped(i))
        for (auto &locs : temporalResourceLocations[i])
          locs.clear();

      d3d::destroy_resource_heap(eastl::exchange(heaps[i], nullptr));
      debug("daBfg: Destroyed heap %d because it was too small/big (size was %dKB, but %dKB are required)", eastl::to_underlying(i),
        allocatedHeaps[i].size >> 10, req.size >> 10);
    }
    if (req.size > 0)
    {
      heaps[i] = d3d::create_resource_heap(req.group, req.size, RHCF_REQUIRES_DEDICATED_HEAP);
      if (DAGOR_UNLIKELY(!heaps[i]))
        DAG_FATAL("daBfg: Failed to allocate resource heap %d of size %dKB", eastl::to_underlying(i), req.size >> 10);
      else
        debug("daBfg: Successfully allocated a new heap %d of size %dKB", eastl::to_underlying(i), req.size >> 10);
    }
    allocatedHeaps[i].size = req.size;
  }

  return result;
}

template <typename ResType, typename ResourceCreator>
void NativeResourceScheduler::placeResource(int frame, intermediate::ResourceIndex res_idx, HeapIndex heap_idx, uint32_t offset,
  const ResourceKey &key, const ResourceCreator &resourceCreator)
{
  // Place resource only if heap does not contain resource
  // with same description, offset and allignment.
  auto &placedResources = placedHeapResources[heap_idx];
  if (placedResources.find(key) == placedResources.end())
  {
    placedProperties.emplace_back(PlaceProperty{heap_idx, offset, key});
    placedResources.emplace(key, ResourceEntry{placedProperties.size() - 1, resourceCreator()});
  }
  G_ASSERT(placedProperties.size() > placedResources[key].collectionIdx);
  resourceIndexInCollection[frame][res_idx] = placedResources[key].collectionIdx;
}

static auto generate_name(uint32_t heap_idx, size_t hash_value)
{
  constexpr const char *HEX_DIGITS = "0123456789ABCDEF";
  // WARNING: sizes for the fixed string and the name template must match!
  eastl::fixed_string<char, 40> result = "fg_resource_heap$$_hash$$$$$$$$$$$$$$$$";
  if (DAGOR_UNLIKELY(heap_idx >= 0xFF))
  {
    logerr("daBfg has allocated more than 255 resource heaps! This shouldn't be happening!");
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

void NativeResourceScheduler::placeResource(int frame, intermediate::ResourceIndex res_idx, HeapIndex heap_idx,
  const ResourceDescription &desc, uint32_t offset, const ResourceAllocationProperties &properties,
  const DynamicResolution &dyn_resolution)
{
  const auto key = ResourceKey(desc, properties, offset);
  const auto placedResName = generate_name(eastl::to_underlying(heap_idx), key.hash);
  if (desc.resType == RES3D_SBUF)
  {
    auto createBuf = [this, &key, &heap_idx, &placedResName, &offset, &properties]() -> UniqueBuf {
      auto buf = dag::place_buffer_in_resource_heap(heaps[heap_idx], key.desc, offset, properties, placedResName.c_str());
      if (!buf)
        logerr("Failed to place buffer %s in resource heap %d (%p) at offset %d! Description was: %s", placedResName.c_str(),
          eastl::to_underlying(heap_idx), heaps[heap_idx], offset, key.desc.toDebugString());
      return UniqueBuf(eastl::move(buf));
    };
    placeResource<UniqueBuf>(frame, res_idx, heap_idx, offset, key, createBuf);
  }
  else
  {
    auto createTex = [this, &key, &heap_idx, &placedResName, &offset, &properties]() -> UniqueTex {
      const auto fixedDesc = fixup_description(key.desc);
      auto tex = dag::place_texture_in_resource_heap(heaps[heap_idx], fixedDesc, offset, properties, placedResName.c_str());
      if (!tex)
        logerr("Failed to place texture %s in resource heap %d (%p) at offset %d! Description was: %s", placedResName.c_str(),
          eastl::to_underlying(heap_idx), heaps[heap_idx], offset, fixedDesc.toDebugString());
      return UniqueTex(eastl::move(tex));
    };
    placeResource<UniqueTex>(frame, res_idx, heap_idx, offset, key, createTex);

    // TODO: it is possible to skip placing a full-resolution texture and place a
    // dynamic resolution one from the start, but that would require some refactoring.
    const auto autoResType = cachedIntermediateResources[res_idx].asScheduled().resolutionType;
    eastl::visit(
      [&](const auto &res) {
        if constexpr (!eastl::is_same_v<eastl::decay_t<decltype(res)>, eastl::monostate>)
          resizeTexture(res_idx, frame, scale_by(res, autoResType->multiplier));
      },
      dyn_resolution);
  }
}

void NativeResourceScheduler::shutdownInternal()
{
  placedHeapResources.clear();
  placedProperties.clear();
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
  const auto scheduledResIdx = resourceIndexInCollection[frame][res_idx];
  const auto resProp = placedProperties[scheduledResIdx];
  const auto key = ResourceKey(resProp.key, resolution);
  auto createTex = [this, &key, &resProp]() -> UniqueTex {
    const auto placedResName = generate_name(eastl::to_underlying(resProp.heapIndex), key.hash);
    const auto desc = fixup_description(key.desc);
    const auto allocInfo = getResourceAllocationProperties(desc);
    auto tex = dag::place_texture_in_resource_heap(heaps[resProp.heapIndex], desc, resProp.offset, allocInfo, placedResName.c_str());

    G_ASSERT_RETURN(tex, {});

    return tex;
  };
  placeResource<UniqueTex>(frame, res_idx, resProp.heapIndex, resProp.offset, key, createTex);
}

void NativeResourceScheduler::resizeAutoResTextures(int frame, const DynamicResolutions &resolutions)
{
  for (auto idx : IdRange<intermediate::ResourceIndex>(cachedIntermediateResources.size()))
  {
    if (!cachedIntermediateResources[idx].isScheduled())
      continue;

    const auto &schedRes = cachedIntermediateResources[idx].asScheduled();

    if (schedRes.resourceType != ResourceType::Texture || !schedRes.resolutionType.has_value())
      continue;

    if (!resolutions.isMapped(schedRes.resolutionType->id))
      continue;

    const auto mult = schedRes.resolutionType->multiplier;
    const auto res = resolutions[schedRes.resolutionType->id];
    eastl::visit(
      [&](const auto &res) {
        if constexpr (!eastl::is_same_v<eastl::decay_t<decltype(res)>, eastl::monostate>)
          resizeTexture(idx, frame, scale_by(res, mult));
      },
      res);
  }
}

} // namespace dabfg
