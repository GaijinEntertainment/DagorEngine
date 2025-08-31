// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "poolResourceScheduler.h"

#include <perfMon/dag_statDrv.h>
#include <debug/backendDebug.h>
#include <EASTL/type_traits.h>
#include <id/idRange.h>


#define SUPPORTED_RESTYPE_LIST            \
  USE(PoolResourceType::Texture)          \
  USE(PoolResourceType::CubeTexture)      \
  USE(PoolResourceType::VolTexture)       \
  USE(PoolResourceType::ArrayTexture)     \
  USE(PoolResourceType::CubeArrayTexture) \
  USE(PoolResourceType::SBuffer)

namespace dafg
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

struct Destroyer
{
  void operator()(D3dResource *res) const { destroy_d3dres(res); }
};
using UniqueD3dRes = eastl::unique_ptr<D3dResource, Destroyer>;

template <>
auto ResourceCreator<PoolResourceType::Texture>::operator()()
{
  eastl::string name = getResName();
  return UniqueD3dRes(
    d3d::create_tex(nullptr, desc.asTexRes.width, desc.asTexRes.height, desc.asTexRes.cFlags, desc.asTexRes.mipLevels, name.data()));
}

template <>
auto ResourceCreator<PoolResourceType::CubeTexture>::operator()()
{
  eastl::string name = getResName();
  return UniqueD3dRes(
    d3d::create_cubetex(desc.asCubeTexRes.extent, desc.asCubeTexRes.cFlags, desc.asCubeTexRes.mipLevels, name.data()));
}

template <>
auto ResourceCreator<PoolResourceType::VolTexture>::operator()()
{
  eastl::string name = getResName();
  return UniqueD3dRes(d3d::create_voltex(desc.asVolTexRes.width, desc.asVolTexRes.height, desc.asVolTexRes.depth,
    desc.asVolTexRes.cFlags, desc.asVolTexRes.mipLevels, name.data()));
}

template <>
auto ResourceCreator<PoolResourceType::ArrayTexture>::operator()()
{
  eastl::string name = getResName();
  return UniqueD3dRes(d3d::create_array_tex(desc.asArrayTexRes.width, desc.asArrayTexRes.height, desc.asArrayTexRes.arrayLayers,
    desc.asArrayTexRes.cFlags, desc.asArrayTexRes.mipLevels, name.data()));
}

template <>
auto ResourceCreator<PoolResourceType::CubeArrayTexture>::operator()()
{
  eastl::string name = getResName();
  return UniqueD3dRes(d3d::create_cube_array_tex(desc.asArrayCubeTexRes.cubes, desc.asArrayCubeTexRes.extent,
    desc.asArrayCubeTexRes.cFlags, desc.asArrayCubeTexRes.mipLevels, name.data()));
}

template <>
auto ResourceCreator<PoolResourceType::SBuffer>::operator()()
{
  eastl::string name = getResName();
  return UniqueD3dRes(d3d::create_sbuffer(desc.asBufferRes.elementSizeInBytes, desc.asBufferRes.elementCount, desc.asBufferRes.cFlags,
    desc.asBufferRes.viewFormat, name.data()));
}

template <PoolResourceType RES_TYPE>
class ResourceCollection
{
  using ResStorageType = eastl::invoke_result_t<ResourceCreator<RES_TYPE>>;
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

static constexpr PoolResourceType d3d_restype_to_pool_restype(D3DResourceType type)
{
  switch (type)
  {
    case D3DResourceType::TEX: return PoolResourceType::Texture;
    case D3DResourceType::CUBETEX: return PoolResourceType::CubeTexture;
    case D3DResourceType::VOLTEX: return PoolResourceType::VolTexture;
    case D3DResourceType::ARRTEX: return PoolResourceType::ArrayTexture;
    case D3DResourceType::CUBEARRTEX: return PoolResourceType::CubeArrayTexture;
    case D3DResourceType::SBUF: return PoolResourceType::SBuffer;
  }
  G_ASSERT(false);
  DAGOR_UNREACHABLE;
}

enum class SimulatedHeapId : intptr_t
{
};

static SimulatedHeapId groupToSimulatedHeapId(ResourceHeapGroup *group)
{
  return static_cast<SimulatedHeapId>(reinterpret_cast<intptr_t>(group));
}

static ResourceHeapGroup *simulatedHeapIdToGroup(SimulatedHeapId heap_id) { return reinterpret_cast<ResourceHeapGroup *>(heap_id); }

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
    return d3d_restype_to_pool_restype(heapIdxToResDesc[heap_idx].type);
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
    G_ASSERT(d3d_restype_to_pool_restype(desc.type) == RES_TYPE);

    const auto simHeapId = SimulatedHeaps::heapIndexForDesc(desc);

    auto &perDescPools = get();

    if (perDescPools.isMapped(simHeapId))
      return;

    perDescPools.set(simHeapId, ResourceCreator<RES_TYPE>(desc));
  }
};

D3dResource *PoolResourceScheduler::getD3dResource(int frame, intermediate::ResourceIndex res_idx) const
{
  G_ASSERT(res_idx < resourceIndexInCollection[frame].size());
  const auto resIdxInCollection = resourceIndexInCollection[frame][res_idx];
  G_ASSERT_RETURN(resIdxInCollection != UNSCHEDULED, {});

#define USE(RES_TYPE) \
  case RES_TYPE: return ResourceCollection<RES_TYPE>::current()[resIdxInCollection].get();

  switch (resourceTypes[res_idx])
  {
    SUPPORTED_RESTYPE_LIST
    default: G_ASSERTF(false, "Resource %s is not a d3d resource!", cachedIntermediateResourceNames[res_idx]); break;
  }

#undef USE

  G_ASSERT(false);
  return nullptr;
}

ResourceAllocationProperties PoolResourceScheduler::getResourceAllocationProperties(const ResourceDescription &desc, bool)
{
#define USE(RES_TYPE) \
  case RES_TYPE: PoolsCollection<RES_TYPE>::lazyInitPool(desc); break;

  switch (d3d_restype_to_pool_restype(desc.type))
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

void PoolResourceScheduler::resetResourcePlacement()
{
  for (auto &indices : resourceIndexInCollection)
    indices.assign(cachedIntermediateResources.size(), UNSCHEDULED);
}

void PoolResourceScheduler::placeResource(int frame, intermediate::ResourceIndex res_idx, HeapIndex heap_idx,
  const ResourceDescription &desc, uint32_t offset, const ResourceAllocationProperties &, const DynamicResolution &)
{
  resourceIndexInCollection[frame][res_idx] = heapStartOffsets[heap_idx] + offset;

  if (res_idx >= resourceTypes.size())
    resourceTypes.resize(res_idx + 1);
  resourceTypes[res_idx] = d3d_restype_to_pool_restype(desc.type);
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

} // namespace dafg
