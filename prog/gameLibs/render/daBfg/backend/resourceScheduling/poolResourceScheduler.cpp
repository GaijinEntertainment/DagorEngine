#include "poolResourceScheduler.h"

#include <perfMon/dag_statDrv.h>
#include <debug/backendDebug.h>
#include <EASTL/type_traits.h>
#include <id/idRange.h>


#define SUPPORTED_TEXTURE_RESTYPE_LIST \
  USE(PoolResourceType::Texture)       \
  USE(PoolResourceType::CubeTexture)   \
  USE(PoolResourceType::VolTexture)    \
  USE(PoolResourceType::ArrayTexture)  \
  USE(PoolResourceType::CubeArrayTexture)

#define SUPPORTED_BUFFER_RESTYPE_LIST USE(PoolResourceType::SBuffer)

#define SUPPORTED_RESTYPE_LIST   \
  SUPPORTED_TEXTURE_RESTYPE_LIST \
  SUPPORTED_BUFFER_RESTYPE_LIST

namespace dabfg
{

static uint32_t resource_names_counter = 0;

template <PoolResourceType RES_TYPE>
struct ResourceCreator
{
  ResourceDescription desc;
  eastl::string getResName()
  {
    resource_names_counter++;
    return eastl::string(eastl::string::CtorSprintf(), "fg_resource_pooled_%d", resource_names_counter);
  }
  ResourceCreator(const ResourceDescription &desc) : desc(desc) {}
  ResourceCreator() = default;
  auto operator()();
};

template <>
auto ResourceCreator<PoolResourceType::Texture>::operator()()
{
  eastl::string name = getResName();
  return UniqueTex(
    dag::create_tex(nullptr, desc.asTexRes.width, desc.asTexRes.height, desc.asTexRes.cFlags, desc.asTexRes.mipLevels, name.data()));
}

template <>
auto ResourceCreator<PoolResourceType::CubeTexture>::operator()()
{
  eastl::string name = getResName();
  return UniqueTex(dag::create_cubetex(desc.asCubeTexRes.extent, desc.asCubeTexRes.cFlags, desc.asCubeTexRes.mipLevels, name.data()));
}

template <>
auto ResourceCreator<PoolResourceType::VolTexture>::operator()()
{
  eastl::string name = getResName();
  return UniqueTex(dag::create_voltex(desc.asVolTexRes.width, desc.asVolTexRes.height, desc.asVolTexRes.depth, desc.asVolTexRes.cFlags,
    desc.asVolTexRes.mipLevels, name.data()));
}

template <>
auto ResourceCreator<PoolResourceType::ArrayTexture>::operator()()
{
  eastl::string name = getResName();
  return UniqueTex(dag::create_array_tex(desc.asArrayTexRes.width, desc.asArrayTexRes.height, desc.asArrayTexRes.arrayLayers,
    desc.asArrayTexRes.cFlags, desc.asArrayTexRes.mipLevels, name.data()));
}

template <>
auto ResourceCreator<PoolResourceType::CubeArrayTexture>::operator()()
{
  eastl::string name = getResName();
  return UniqueTex(dag::create_cube_array_tex(desc.asArrayCubeTexRes.cubes, desc.asArrayCubeTexRes.extent,
    desc.asArrayCubeTexRes.cFlags, desc.asArrayCubeTexRes.mipLevels, name.data()));
}

template <>
auto ResourceCreator<PoolResourceType::SBuffer>::operator()()
{
  eastl::string name = getResName();
  return UniqueBuf(dag::create_sbuffer(desc.asBufferRes.elementSizeInBytes, desc.asBufferRes.elementCount, desc.asBufferRes.cFlags,
    desc.asBufferRes.viewFormat, name.data()));
}

template <PoolResourceType RES_TYPE>
class ResourceCollection
{
  using ResStorageType = eastl::result_of_t<ResourceCreator<RES_TYPE>()>;
  dag::Vector<ResStorageType> resources;
  dag::Vector<ResStorageType> flipped;

  static auto &get()
  {
    static ResourceCollection collection;
    return collection;
  }

public:
  static auto &current() { return get().resources; }
  static auto &previous() { return get().flipped; }

  static void flip() { get().flipped = eastl::move(get().resources); }
};

// Try and give a compile error in case new d3d resources are added
G_STATIC_ASSERT(RES3D_SBUF == 5);
static constexpr PoolResourceType D3D_RESTYPE_TO_POOL_RESTYPE[]{PoolResourceType::Texture, PoolResourceType::CubeTexture,
  PoolResourceType::VolTexture, PoolResourceType::ArrayTexture, PoolResourceType::CubeArrayTexture, PoolResourceType::SBuffer};

enum class SimulatedHeapId : intptr_t
{
};

SimulatedHeapId groupToSimulatedHeapId(ResourceHeapGroup *group)
{
  return static_cast<SimulatedHeapId>(reinterpret_cast<intptr_t>(group));
}

ResourceHeapGroup *simulatedHeapIdToGroup(SimulatedHeapId heap_id) { return reinterpret_cast<ResourceHeapGroup *>(heap_id); }

class SimulatedHeaps
{
  IdIndexedMapping<SimulatedHeapId, ResourceDescription> heapIdxToResDesc;

  static auto &get()
  {
    static SimulatedHeaps storage;
    return storage.heapIdxToResDesc;
  }

public:
  static SimulatedHeapId heapIndexForDesc(const ResourceDescription &desc)
  {
    auto &heapIdxToResDesc = get();
    auto it = eastl::find(heapIdxToResDesc.begin(), heapIdxToResDesc.end(), desc);
    if (it == heapIdxToResDesc.end())
      it = heapIdxToResDesc.emplace(it, desc);
    return static_cast<SimulatedHeapId>(eastl::distance(heapIdxToResDesc.begin(), it));
  }

  static PoolResourceType resourceTypeOfHeap(SimulatedHeapId heap_idx)
  {
    const auto &heapIdxToResDesc = get();
    G_ASSERT(heapIdxToResDesc.isMapped(heap_idx));
    int resType = heapIdxToResDesc[heap_idx].resType;
    G_ASSERT(resType <= eastl::size(D3D_RESTYPE_TO_POOL_RESTYPE));
    return D3D_RESTYPE_TO_POOL_RESTYPE[resType];
  }

  static void clear() { get().clear(); }
};

template <PoolResourceType RES_TYPE>
class PoolsCollection
{
  IdIndexedMapping<SimulatedHeapId, ResourceCreator<RES_TYPE>> perDescPools;

  static auto &get()
  {
    static PoolsCollection storage;
    return storage.perDescPools;
  }

public:
  static void clear() { get().clear(); }

  static ResourceCreator<RES_TYPE> &poolForHeap(SimulatedHeapId heap_idx)
  {
    auto &perDescPools = get();
    G_ASSERT(perDescPools.isMapped(heap_idx));
    return perDescPools[heap_idx];
  }

  static void lazyInitPool(const ResourceDescription &desc)
  {
    G_ASSERT(D3D_RESTYPE_TO_POOL_RESTYPE[desc.resType] == RES_TYPE);

    const auto simHeapId = SimulatedHeaps::heapIndexForDesc(desc);

    auto &perDescPools = get();

    if (perDescPools.isMapped(simHeapId))
      return;

    perDescPools.set(simHeapId, ResourceCreator<RES_TYPE>(desc));
  }
};

ManagedTexView PoolResourceScheduler::getTexture(int frame, intermediate::ResourceIndex res_idx) const
{
  G_ASSERT(res_idx < resourceIndexInCollection[frame].size());
  const auto resIdxInCollection = resourceIndexInCollection[frame][res_idx];
  G_ASSERT_RETURN(resIdxInCollection != UNSCHEDULED, {});

  ManagedTexView texView;

#define USE(RES_TYPE)                                                      \
  case RES_TYPE:                                                           \
    texView = ResourceCollection<RES_TYPE>::current()[resIdxInCollection]; \
    texView->setResName(cachedIntermediateResourceNames[res_idx].c_str()); \
    return texView;

  switch (resourceTypes[res_idx])
  {
    SUPPORTED_TEXTURE_RESTYPE_LIST
    default: G_ASSERTF(false, "Resource %s is not a texture!", cachedIntermediateResourceNames[res_idx]); break;
  }

#undef USE

  G_ASSERT(false);
  return {};
}

ManagedBufView PoolResourceScheduler::getBuffer(int frame, intermediate::ResourceIndex res_idx) const
{
  G_ASSERT(res_idx < resourceIndexInCollection[frame].size());
  const auto resIdxInCollection = resourceIndexInCollection[frame][res_idx];
  G_ASSERT_RETURN(resIdxInCollection != UNSCHEDULED, {});

  ManagedBufView bufView;

#define USE(RES_TYPE)                                                      \
  case RES_TYPE:                                                           \
    bufView = ResourceCollection<RES_TYPE>::current()[resIdxInCollection]; \
    bufView->setResName(cachedIntermediateResourceNames[res_idx].c_str()); \
    return bufView;

  switch (resourceTypes[res_idx])
  {
    SUPPORTED_BUFFER_RESTYPE_LIST
    default: G_ASSERTF(false, "Resource %s is not a buffer!", cachedIntermediateResourceNames[res_idx]); break;
  }

#undef USE

  return {};
}

ResourceAllocationProperties PoolResourceScheduler::getResourceAllocationProperties(const ResourceDescription &desc, bool)
{
#define USE(RES_TYPE) \
  case RES_TYPE: PoolsCollection<RES_TYPE>::lazyInitPool(desc); break;

  switch (D3D_RESTYPE_TO_POOL_RESTYPE[desc.resType])
  {
    SUPPORTED_RESTYPE_LIST
    default: G_ASSERT(false); break;
  }

#undef USE

  ResourceAllocationProperties result{};
  result.sizeInBytes = 1;
  result.offsetAlignment = 1;
  result.heapGroup = simulatedHeapIdToGroup(SimulatedHeaps::heapIndexForDesc(desc));
  return result;
}

ResourceHeapGroupProperties PoolResourceScheduler::getResourceHeapGroupProperties(ResourceHeapGroup *)
{
  ResourceHeapGroupProperties heapGroupProp{};
  heapGroupProp.flags = 0;
  heapGroupProp.maxHeapSize = eastl::numeric_limits<uint64_t>::max();
  heapGroupProp.maxResourceSize = eastl::numeric_limits<uint64_t>::max();
  return heapGroupProp;
}

template <PoolResourceType RES_TYPE>
static uint32_t allocateHeap(SimulatedHeapId simulated_heap_id, uint32_t heap_size, uint32_t previous_heap_start_offset,
  uint32_t previous_heap_size)
{
  auto &currStorage = ResourceCollection<RES_TYPE>::current();
  auto &prevStorage = ResourceCollection<RES_TYPE>::previous();

  const uint32_t result = currStorage.size();

  const uint32_t reusableResCount = min<uint32_t>(heap_size, previous_heap_size);

  if (reusableResCount > 0)
  {
    // Move already leased resources
    currStorage.resize(currStorage.size() + reusableResCount);
    eastl::move(prevStorage.begin() + previous_heap_start_offset, prevStorage.begin() + previous_heap_start_offset + reusableResCount,
      currStorage.end() - reusableResCount);
  }

  // Lease new resources
  auto &pool = PoolsCollection<RES_TYPE>::poolForHeap(simulated_heap_id);
  for (uint32_t j = reusableResCount; j < heap_size; ++j)
    currStorage.push_back(pool());

  return result;
}

ResourceScheduler::DestroyedHeapSet PoolResourceScheduler::allocateHeaps(const HeapRequests &heap_requests)
{
  TIME_PROFILE(allocateHeaps);

  // WIPE ALL OF THEM LOL
  // It's completely pointless do do anything smarter here, as DX11 does
  // not in fact care about deactivations at all.
  DestroyedHeapSet result;
  result.reserve(allocatedHeaps.size());
  for (auto idx : IdRange<HeapIndex>(allocatedHeaps.size()))
    if (allocatedHeaps[idx].size > 0)
      result.push_back(idx);

  FRAMEMEM_VALIDATE;

  if (heapStartOffsets.size() < heap_requests.size())
    heapStartOffsets.resize(heap_requests.size());

#define USE(RES_TYPE) ResourceCollection<RES_TYPE>::flip();
  SUPPORTED_RESTYPE_LIST
#undef USE

  for (auto [heapIdx, heap] : heap_requests.enumerate())
    if (heap.group != CPU_HEAP_GROUP)
    {
      const uint32_t reusableOffset = heapStartOffsets[heapIdx];
      const uint32_t reusableSize = allocatedHeaps[heapIdx].size;

      const auto simHeapId = groupToSimulatedHeapId(heap.group);

      switch (SimulatedHeaps::resourceTypeOfHeap(simHeapId))
      {
#define USE(RES_TYPE) \
  case RES_TYPE: heapStartOffsets[heapIdx] = allocateHeap<RES_TYPE>(simHeapId, heap.size, reusableOffset, reusableSize); break;
        SUPPORTED_RESTYPE_LIST
#undef USE

        default: G_ASSERT(false); break;
      }

      // NOTE: pool scheduler doesn't actually need to leave more
      // textures allocated than necessary, pools already do that
      // sort of caching automatically.
      allocatedHeaps[heapIdx].size = heap.size;
    }


#define USE(RES_TYPE) ResourceCollection<RES_TYPE>::previous().clear();
  SUPPORTED_RESTYPE_LIST
#undef USE

  return result;
}

void PoolResourceScheduler::placeResource(int frame, intermediate::ResourceIndex res_idx, HeapIndex heap_idx,
  const ResourceDescription &desc, uint32_t offset, const ResourceAllocationProperties &)
{
  resourceIndexInCollection[frame][res_idx] = heapStartOffsets[heap_idx] + offset;

  if (res_idx >= resourceTypes.size())
    resourceTypes.resize(res_idx + 1);
  resourceTypes[res_idx] = D3D_RESTYPE_TO_POOL_RESTYPE[desc.resType];
}

template <PoolResourceType RES_TYPE>
static void shutdownPoolFor()
{
  ResourceCollection<RES_TYPE>::current().clear();
  PoolsCollection<RES_TYPE>::clear();
}

void PoolResourceScheduler::shutdownInternal()
{
#define USE(RES_TYPE) shutdownPoolFor<RES_TYPE>();
  SUPPORTED_RESTYPE_LIST
#undef USE
  heapStartOffsets.clear();
  SimulatedHeaps::clear();
}

PoolResourceScheduler::~PoolResourceScheduler() { PoolResourceScheduler::shutdownInternal(); }

} // namespace dabfg
