// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "poolResourceAllocator.h"

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

template <>
auto ResourceCreator<PoolResourceType::Texture>::operator()()
{
  eastl::string name = getResName();
  return UniqueD3dRes(d3d::create_tex(nullptr, desc.asTexRes.width, desc.asTexRes.height, desc.asTexRes.cFlags,
    desc.asTexRes.mipLevels, name.data(), RESTAG_FRAMEGRAPH));
}

template <>
auto ResourceCreator<PoolResourceType::CubeTexture>::operator()()
{
  eastl::string name = getResName();
  return UniqueD3dRes(d3d::create_cubetex(desc.asCubeTexRes.extent, desc.asCubeTexRes.cFlags, desc.asCubeTexRes.mipLevels, name.data(),
    RESTAG_FRAMEGRAPH));
}

template <>
auto ResourceCreator<PoolResourceType::VolTexture>::operator()()
{
  eastl::string name = getResName();
  return UniqueD3dRes(d3d::create_voltex(desc.asVolTexRes.width, desc.asVolTexRes.height, desc.asVolTexRes.depth,
    desc.asVolTexRes.cFlags, desc.asVolTexRes.mipLevels, name.data(), RESTAG_FRAMEGRAPH));
}

template <>
auto ResourceCreator<PoolResourceType::ArrayTexture>::operator()()
{
  eastl::string name = getResName();
  return UniqueD3dRes(d3d::create_array_tex(desc.asArrayTexRes.width, desc.asArrayTexRes.height, desc.asArrayTexRes.arrayLayers,
    desc.asArrayTexRes.cFlags, desc.asArrayTexRes.mipLevels, name.data(), RESTAG_FRAMEGRAPH));
}

template <>
auto ResourceCreator<PoolResourceType::CubeArrayTexture>::operator()()
{
  eastl::string name = getResName();
  return UniqueD3dRes(d3d::create_cube_array_tex(desc.asArrayCubeTexRes.cubes, desc.asArrayCubeTexRes.extent,
    desc.asArrayCubeTexRes.cFlags, desc.asArrayCubeTexRes.mipLevels, name.data(), RESTAG_FRAMEGRAPH));
}

template <>
auto ResourceCreator<PoolResourceType::SBuffer>::operator()()
{
  eastl::string name = getResName();
  return UniqueD3dRes(d3d::create_sbuffer(desc.asBufferRes.elementSizeInBytes, desc.asBufferRes.elementCount, desc.asBufferRes.cFlags,
    desc.asBufferRes.viewFormat, name.data(), RESTAG_FRAMEGRAPH));
}

class PooledResources
{
  IdIndexedMapping<HeapIndex, dag::Vector<UniqueD3dRes>> perHeap;

  static auto &get()
  {
    static PooledResources storage;
    return storage.perHeap;
  }

public:
  static dag::Vector<UniqueD3dRes> &ofHeap(HeapIndex heap_idx)
  {
    auto &perHeap = get();
    perHeap.expandMapping(heap_idx);
    return perHeap[heap_idx];
  }

  static void clear() { get().clear(); }
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

  static const ResourceDescription &describeHeap(SimulatedHeapId heap_idx)
  {
    const auto &heapIdxToResDesc = get();
    G_ASSERT(heapIdxToResDesc.isMapped(heap_idx));
    return heapIdxToResDesc[heap_idx];
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
    if (!perDescPools.isMapped(heap_idx))
      perDescPools.set(heap_idx, ResourceCreator<RES_TYPE>(SimulatedHeaps::describeHeap(heap_idx)));
    return perDescPools[heap_idx];
  }
};

static UniqueD3dRes create_pooled_resource(SimulatedHeapId heap_id)
{
#define USE(RES_TYPE) \
  case RES_TYPE: return PoolsCollection<RES_TYPE>::poolForHeap(heap_id)();

  switch (SimulatedHeaps::resourceTypeOfHeap(heap_id))
  {
    SUPPORTED_RESTYPE_LIST
    default: G_ASSERT(false); break;
  }

#undef USE

  return UniqueD3dRes{};
}

ResourceAllocationProperties PoolResourceAllocator::getResourceAllocationProperties(const ResourceDescription &desc, bool)
{
  ResourceAllocationProperties result{};
  result.sizeInBytes = 1;
  result.offsetAlignment = 1;
  result.heapGroup = simulatedHeapIdToGroup(SimulatedHeaps::heapIndexForDesc(desc));
  return result;
}

ResourceHeapGroupProperties PoolResourceAllocator::getResourceHeapGroupProperties(ResourceHeapGroup *)
{
  ResourceHeapGroupProperties heapGroupProp{};
  heapGroupProp.flags = 0;
  heapGroupProp.maxHeapSize = eastl::numeric_limits<uint64_t>::max();
  heapGroupProp.maxResourceSize = eastl::numeric_limits<uint64_t>::max();
  return heapGroupProp;
}

bool PoolResourceAllocator::doesHeapAgree(HeapIndex heap_idx, const HeapRequest &req) const
{
  return PooledResources::ofHeap(heap_idx).size() == req.size;
}

void PoolResourceAllocator::allocateHeaps(const LayoutHeaps &layout_heaps, bool defer_destruction, ChangedHeaps &changed)
{
  TIME_PROFILE(allocateHeaps);
  G_ASSERT(!defer_destruction);
  G_UNUSED(defer_destruction);

  FRAMEMEM_VALIDATE;

  for (auto &&[heapIdx, layoutHeap] : layout_heaps.enumerate())
  {
    const auto heap = make_heap_request(layoutHeap);
    if (heap.group == CPU_HEAP_GROUP)
    {
      auto &objects = PooledResources::ofHeap(heapIdx);
      if (!objects.empty())
      {
        objects.clear();
        ++stats.heapsDestroyed;
        changed.set(heapIdx, true);
        allocatedHeaps[heapIdx] = {};
      }
      continue;
    }

    // NOTE: pool scheduler doesn't actually need to leave more
    // textures allocated than necessary, pools already do that
    // sort of caching automatically.
    auto &objects = PooledResources::ofHeap(heapIdx);
    if (allocatedHeaps[heapIdx].group != heap.group && !objects.empty())
    {
      objects.clear();
      ++stats.heapsDestroyed;
    }
    const bool wasEmpty = objects.empty();
    const bool shrank = objects.size() > heap.size;
    if (shrank)
      objects.resize(heap.size);
    else
    {
      const auto simHeapId = groupToSimulatedHeapId(heap.group);
      while (objects.size() < heap.size)
      {
        objects.push_back(create_pooled_resource(simHeapId));
        ++stats.objectsCreated;
      }
    }
    if (wasEmpty && !objects.empty())
      ++stats.heapsAllocated;
    if (shrank || (wasEmpty && !objects.empty()))
      changed.set(heapIdx, true);
    allocatedHeaps[heapIdx] = heap;
  }
}

D3dResource *PoolResourceAllocator::placeResource(intermediate::ResourceIndex res_idx, HeapIndex heap_idx, const ResourceDescription &,
  uint64_t offset, const ResourceAllocationProperties &, const DynamicResolution &)
{
  G_UNUSED(res_idx);

  auto &objects = PooledResources::ofHeap(heap_idx);
  G_ASSERT_RETURN(offset < objects.size(), nullptr);
  return objects[offset].get();
}

// The heap ids outlive a device reset, because the layout keeps its heaps
// and names their pools by id
void PoolResourceAllocator::shutdownInternal()
{
  PooledResources::clear();
#define USE(RES_TYPE) PoolsCollection<RES_TYPE>::clear();
  SUPPORTED_RESTYPE_LIST
#undef USE
}

PoolResourceAllocator::~PoolResourceAllocator()
{
  PoolResourceAllocator::shutdownInternal();
  SimulatedHeaps::clear();
}

} // namespace dafg
