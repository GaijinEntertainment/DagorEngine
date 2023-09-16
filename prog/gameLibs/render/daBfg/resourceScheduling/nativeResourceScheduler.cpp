#include "nativeResourceScheduler.h"

#include <debug/backendDebug.h>
#include <perfMon/dag_statDrv.h>
#include <3d/dag_resizableTex.h>
#include <3d/dag_resPtr.h>
#include <memory/dag_framemem.h>

#include <render/daBfg/resourceCreation.h>

#include <id/idRange.h>


namespace dabfg
{

static uint32_t auto_mip_levels_count(uint32_t w, uint32_t h)
{
  uint32_t lev = 1;
  uint32_t t = min(w, h);
  while (t > 1)
  {
    lev++;
    t >>= 1;
  }
  return lev;
}

static ResourceDescription fix_tex_params(const ResourceDescription &desc)
{
  ResourceDescription fixed_desc = desc;
  if (desc.asTexRes.mipLevels == AUTO_MIP_COUNT)
    fixed_desc.asTexRes.mipLevels = auto_mip_levels_count(desc.asTexRes.width, desc.asTexRes.height);
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
  if (description.resType == RES3D_SBUF)
  {
    rawHash = desc.hash();
    hashPack(rawHash, offset, prop.offsetAlignment);
    hash = rawHash;
  }
  else
  {
    desc.asTexRes.width = 0;
    desc.asTexRes.height = 0;
    rawHash = desc.hash();
    hashPack(rawHash, offset, prop.offsetAlignment);
    hash = rawHash;
    hashPack(hash, description.asTexRes.width, description.asTexRes.height);
    desc.asTexRes.width = description.asTexRes.width;
    desc.asTexRes.height = description.asTexRes.height;
    desc = fix_tex_params(desc);
  }
}

NativeResourceScheduler::ResourceKey::ResourceKey(const ResourceKey &key, const Point2 &resolution) :
  hash(key.rawHash), rawHash(key.rawHash), desc(key.desc)
{
  hashPack(hash, resolution.x, resolution.y);
  desc.asTexRes.mipLevels = AUTO_MIP_COUNT;
  desc.asTexRes.width = resolution.x;
  desc.asTexRes.height = resolution.y;
  desc = fix_tex_params(desc);
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
  resView->setResName(cachedIntermediateResources[res_id].name.c_str());
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
  auto updatedDescription = desc;
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
  return allocationPropertiesCache[updatedDescription] = d3d::get_resource_allocation_properties(updatedDescription);
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
      continue;
    }

    if (heaps[i] != nullptr)
    {
      placedHeapResources[i].clear();
      result.push_back(i);
      for (auto &heapHintedResources : hintedResources[i])
        heapHintedResources.clear();
      d3d::destroy_resource_heap(eastl::exchange(heaps[i], nullptr));
    }
    if (req.size > 0)
      heaps[i] = d3d::create_resource_heap(req.group, req.size, RHCF_REQUIRES_DEDICATED_HEAP);
    allocatedHeaps[i].size = req.size;
  }

  return result;
}

template <typename ResType, typename ResourceCreator>
void NativeResourceScheduler::placeResource(int frame, intermediate::ResourceIndex res_idx, HeapIndex heap_idx, const ResourceKey &key,
  const ResourceCreator &resourceCreator)
{
  // Place resource only if heap does not contain resource
  // with same description, offset and allignment.
  auto &placedResources = placedHeapResources[heap_idx];
  if (placedResources.find(key) == placedResources.end())
  {
    placedProperties.emplace_back(PlaceProperty{heap_idx, key});
    placedResources.emplace(key, ResourceEntry{placedProperties.size() - 1, resourceCreator()});
  }
  G_ASSERT(placedProperties.size() > placedResources[key].collectionIdx);
  resourceIndexInCollection[frame][res_idx] = placedResources[key].collectionIdx;
}

static PerCompString generate_name(uint32_t heap_idx, size_t hash_value)
{
  const eastl::string HEX_DIGITS = "0123456789ABCDEF";
  PerCompString result = "fg_resource_heap$$_hash$$$$$$$$";
  G_ASSERT(heap_idx <= 0xFF);
  result[16] = HEX_DIGITS[heap_idx & 0xf];
  result[17] = HEX_DIGITS[heap_idx >> 4];
  for (uint32_t i = 30; i >= 23; --i)
  {
    result[i] = HEX_DIGITS[hash_value & 0xF];
    hash_value >>= 4;
  }
  return result;
}

void NativeResourceScheduler::placeResource(int frame, intermediate::ResourceIndex res_idx, HeapIndex heap_idx,
  const ResourceDescription &desc, uint32_t offset, const ResourceAllocationProperties &properties)
{
  const auto key = ResourceKey(desc, properties, offset);
  const PerCompString placedResName = generate_name(eastl::to_underlying(heap_idx), key.hash);
  if (desc.resType == RES3D_SBUF)
  {
    auto createBuf = [this, &key, &heap_idx, &placedResName, &offset, &properties]() -> UniqueBuf {
      return UniqueBuf(dag::place_buffer_in_resource_heap(heaps[heap_idx], key.desc, offset, properties, placedResName.c_str()));
    };
    placeResource<UniqueBuf>(frame, res_idx, heap_idx, key, createBuf);
  }
  else
  {
    auto createTex = [this, &key, &heap_idx, &placedResName, &offset, &properties]() -> UniqueTex {
      return UniqueTex(dag::place_texture_in_resource_heap(heaps[heap_idx], key.desc, offset, properties, placedResName.c_str()));
    };
    placeResource<UniqueTex>(frame, res_idx, heap_idx, key, createTex);
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

void NativeResourceScheduler::resizeTexture(intermediate::ResourceIndex res_idx, int frame, const Point2 &resolution)
{
  const auto scheduledResIdx = resourceIndexInCollection[frame][res_idx];
  const auto &resizableTex = getTexture(frame, res_idx);
  const auto &resProp = placedProperties[scheduledResIdx];
  const auto key = ResourceKey(resProp.key, resolution);
  auto createTex = [&key, &resProp, &resizableTex, &resolution]() -> UniqueTex {
    const PerCompString placedResName = generate_name(eastl::to_underlying(resProp.heapIndex), key.hash);
    auto tex = dag::alias_tex(resizableTex.getTex2D(), nullptr, resolution.x, resolution.y, key.desc.asTexRes.cFlags,
      key.desc.asTexRes.mipLevels, placedResName.c_str());

    G_ASSERT_RETURN(tex, {});

    return tex;
  };
  placeResource<UniqueTex>(frame, res_idx, resProp.heapIndex, key, createTex);
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
    resizeTexture(idx, frame, IPoint2{static_cast<int>(mult * res.x), static_cast<int>(mult * res.y)});
  }
}

} // namespace dabfg
