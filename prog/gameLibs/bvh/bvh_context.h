// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <osApiWrappers/dag_atomic_types.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_eventQueryHolder.h>
#include <drv/3d/dag_bindless.h>
#include <util/dag_multicastEvent.h>
#include <util/dag_threadPool.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_spinlock.h>
#include <osApiWrappers/dag_miscApi.h>
#include <dag/dag_vector.h>
#include <perfMon/dag_statDrv.h>
#include <generic/dag_enumerate.h>
#include <EASTL/unordered_map.h>
#include <EASTL/unordered_set.h>
#include <EASTL/optional.h>
#include <EASTL/string.h>
#include <EASTL/numeric_limits.h>
#include <EASTL/deque.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <bvh/bvh.h>
#include <math/dag_bits.h>
#include <generic/dag_span.h>
#include <math/dag_hlsl_floatx.h>
#include "shaders/bvh_mesh_meta.hlsli"

class LandMeshManager;

namespace bvh
{

#if _TARGET_C2




#elif _TARGET_APPLE
constexpr bool is_blas_compaction_enabled() { return false; }
constexpr bool is_blas_compaction_cheap() { return false; }
#else
constexpr bool is_blas_compaction_enabled() { return true; }
constexpr bool is_blas_compaction_cheap() { return false; }
#endif

// To be stored in InstanceContributionToHitGroupIndex
inline uint32_t pack_color8_to_color777(uint32_t color)
{
  return ((color & 0xFEu) << 2) | ((color & 0xFE00u) << 1) | ((color & 0xFE0000u) << 0);
}

#if _TARGET_APPLE
static constexpr bool use_icthgi_for_per_instance_data = false;
#else
static constexpr bool use_icthgi_for_per_instance_data = true;
#endif

static constexpr ResourceBarrier bindlessSRVBarrier = ResourceBarrier::RB_RO_SRV | ResourceBarrier::RB_STAGE_ALL_SHADERS;
static constexpr ResourceBarrier bindlessUAVBarrier = ResourceBarrier::RB_RW_UAV | ResourceBarrier::RB_STAGE_ALL_SHADERS;

extern bool is_in_lost_device_state;

template <typename T>
bool handle_lost_device_state(const T &resource)
{
  if (is_in_lost_device_state)
    return true;

  if (!resource)
  {
    G_ASSERT(d3d::device_lost(nullptr));
    logdbg("[BVH] Device is lost. Entering lost device state.");
    is_in_lost_device_state = true;
  }

  return is_in_lost_device_state;
}

#define HANDLE_LOST_DEVICE_STATE(resource, return_value) \
  if (handle_lost_device_state(resource))                \
  return return_value
#define CHECK_LOST_DEVICE_STATE() \
  if (is_in_lost_device_state)    \
  return
#define CHECK_LOST_DEVICE_STATE_RET(retVal) \
  if (is_in_lost_device_state)              \
  return retVal

// Using the LinearHeapAllocator would be nice here, but we can't.
// We cannot allow moving the elements in the heap, but LinearHeapAllocator
// not only does that on defragmentation, but also on allocation when the
// heap is getting expanded.

struct BVHHeapAllocatorAllocId
{
  unsigned short slabIndex;
  unsigned char offset;
  unsigned char size;

  bool operator==(const BVHHeapAllocatorAllocId &) const = default;
};

template <typename HeapManager, int pool_size>
struct BVHHeapAllocator
{
  using Heap = typename HeapManager::Heap;
  using Elem = typename HeapManager::Elem;
  using AllocId = BVHHeapAllocatorAllocId;

  inline static constexpr int PoolSize = pool_size;
  inline static constexpr int SlabSize = eastl::numeric_limits<uint32_t>::digits;
  static_assert(PoolSize % SlabSize == 0);

  inline static constexpr AllocId INVALID_ALLOC_ID = {0, 0xFF, 0xFF};

  eastl::optional<AllocId> findFreeSlot(size_t size)
  {
    for (auto [index, occupancy] : enumerate(slabOccupancy))
    {
      uint32_t freeRanges = ~occupancy;
      for (int i = 1; i < size; ++i)
        freeRanges &= (~occupancy) >> i;

      if (freeRanges)
        return AllocId{(unsigned short)index, (unsigned char)__bsf_unsafe(freeRanges), (unsigned char)size};
    }

    return eastl::nullopt;
  }

  static constexpr uint32_t get_occupancy_mask(AllocId allocation) { return ((1 << allocation.size) - 1) << allocation.offset; }
  static constexpr bool is_valid(AllocId allocation) { return allocation.offset < SlabSize && allocation.size <= SlabSize; }
  static constexpr int decode(AllocId allocation)
  {
    return is_valid(allocation) ? allocation.slabIndex * SlabSize + allocation.offset : -1;
  };

  AllocId allocate(int size)
  {
    eastl::optional<AllocId> candidate = findFreeSlot(size);
    if (!candidate)
    {
      manager.increaseHeap(pool_size);
      auto newSlabIt = slabOccupancy.insert(slabOccupancy.end(), pool_size / SlabSize, 0);
      candidate = AllocId{(unsigned short)eastl::distance(slabOccupancy.begin(), newSlabIt), (unsigned char)0, (unsigned char)size};
    }

    slabOccupancy[candidate->slabIndex] |= get_occupancy_mask(*candidate);

    return *candidate;
  }

  AllocId allocate(dag::ConstSpan<Elem> e)
  {
    AllocId allocation = allocate(e.size());
    for (const auto &[i, e] : enumerate(e))
      manager.set(decode(allocation) + i, e);
    return allocation;
  }

  AllocId allocate(const Elem &e)
  {
    AllocId allocation = allocate(1);
    manager.set(decode(allocation), e);
    return allocation;
  }

  void free(AllocId allocation)
  {
#if DAGOR_DBGLEVEL > 0
    if (allocation.slabIndex * SlabSize + allocation.offset + allocation.size > manager.size())
    {
      logerr("[BVH] Meta (index, offset, size) is (%d, %d, %d), which is invalid for free.", allocation.slabIndex, allocation.offset,
        allocation.size);
      return;
    }
#endif
    for (int i = 0; i < allocation.size; i++)
      manager.reset(decode(allocation) + i);

    slabOccupancy[allocation.slabIndex] &= ~get_occupancy_mask(allocation);
  }

  dag::Span<Elem> get(AllocId allocation)
  {
    auto *begin = &manager.get(decode(allocation));
    return dag::Span<Elem>(begin, allocation.size);
  }
  dag::ConstSpan<Elem> get(AllocId allocation) const
  {
    const auto *begin = &manager.get(decode(allocation));
    return dag::ConstSpan<Elem>(begin, allocation.size);
  }

  Elem &get(int index) { return manager.get(index); }
  const Elem &get(int index) const { return manager.get(index); }

  int size() const { return manager.size(); }

  int allocated() const
  {
    int result = 0;
    for (auto v : slabOccupancy)
      result += __popcount(v);
    return result;
  }

  const Elem *data(int bucket) const { return manager.data(bucket); }

  Heap &getHeap() { return manager.getHeap(); }
  const Heap &getHeap() const { return manager.getHeap(); }

private:
  HeapManager manager;

  dag::Vector<uint32_t> slabOccupancy;
};

struct BindlessRange
{
  BindlessRange() = default;
  BindlessRange(BindlessRange &&other) : range(other.range), size(other.size)
  {
    other.range = -1;
    other.size = 0;
  }
  BindlessRange &operator=(BindlessRange &&other)
  {
    range = other.range;
    size = other.size;
    other.range = -1;
    other.size = 0;
    return *this;
  }

  uint32_t range = -1;
  uint32_t size = 0;
};

enum class BindlessRangeType
{
  TEXTURE,
  CUBE_TEXTURE,
  BUFFER,
  COUNT
};

static constexpr D3DResourceType operator*(BindlessRangeType type)
{
  return type == BindlessRangeType::BUFFER ? D3DResourceType::SBUF
                                           : (type == BindlessRangeType::TEXTURE ? D3DResourceType::TEX : D3DResourceType::CUBETEX);
}

struct TextureIdHash
{
  size_t operator()(TEXTUREID id) const { return (unsigned)id; }
};

template <BindlessRangeType type>
struct BindlessResourceHeap
{
  using ResourceType = eastl::conditional_t<type == BindlessRangeType::BUFFER, Sbuffer *, Texture *>;

  void resize(int size)
  {
    if (size <= resources.size())
      return;

    resources.resize(size);
  }

  void updateBindings()
  {
    const size_t extraCount = 100;

    size_t allDirtyBeyond = 0xFFFFFFFFU;

    if (resources.size() > range.size)
    {
      TIME_PROFILE(resizeBindless);

      allDirtyBeyond = range.size;

      if (range.size == 0)
        range.range = d3d::allocate_bindless_resource_range(*type, resources.size() + extraCount);
      else
        range.range = d3d::resize_bindless_resource_range(*type, range.range, range.size, resources.size() + extraCount);

      range.size = resources.size() + extraCount;
    }

    TIME_PROFILE(updateBindless);

    for (auto [index, resource] : enumerate(resources))
      if (index >= allDirtyBeyond || resource.dirty)
      {
        if (resource.res)
          d3d::update_bindless_resource(*type, range.range + index, resource.res);
        else
          d3d::update_bindless_resources_to_null(*type, range.range + index, 1);
        resource.dirty = false;
      }
  }

  void set(int index, ResourceType resource)
  {
    G_ASSERT(resource);

    if constexpr (type == BindlessRangeType::BUFFER)
      G_ASSERT(resource->getFlags() & SBCF_BIND_SHADER_RES);

    resources[index].res = resource;
    resources[index].dirty = true;
  }

  void reset(int index)
  {
    resources[index].res = nullptr;
    resources[index].dirty = true;
  }

  uint32_t size() const { return resources.size(); }
  uint32_t location() const { return range.range; }

  ResourceType &get(int index) { return resources[index].res; }
  const ResourceType &get(int index) const { return resources[index].res; }

  ~BindlessResourceHeap()
  {
    if (range.size)
      d3d::free_bindless_resource_range(*type, range.range, range.size);
  }

private:
  struct Slot
  {
    ResourceType res = nullptr;
    bool dirty = true;
  };
  dag::Vector<Slot> resources;
  BindlessRange range;
};

template <BindlessRangeType type>
struct BindlessResourceHeapManager
{
public:
  using Heap = BindlessResourceHeap<type>;
  using Elem = typename Heap::ResourceType;

  void increaseHeap(int pool_size) { heap.resize(heap.size() + pool_size); }

  void set(int index, const Elem &e) { heap.set(index, e); }
  void reset(int index) { heap.reset(index); }

  Elem &get(int index) { return heap.get(index); }
  const Elem &get(int index) const { return heap.get(index); }

  int size() const { return heap.size(); }

  const Elem *data(int bucket) const { return heap.data(bucket); }

  Heap &getHeap() { return heap; }
  const Heap &getHeap() const { return heap; }

private:
  Heap heap;
};

using BindlessTextureAllocator = BVHHeapAllocator<BindlessResourceHeapManager<BindlessRangeType::TEXTURE>, 32>;
using BindlessCubeTextureAllocator = BVHHeapAllocator<BindlessResourceHeapManager<BindlessRangeType::CUBE_TEXTURE>, 32>;
using BindlessBufferAllocator = BVHHeapAllocator<BindlessResourceHeapManager<BindlessRangeType::BUFFER>, 32>;

struct BindlessTexture
{
  BVHHeapAllocatorAllocId allocId = {};
  uint32_t samplerIndex = 0;
  uint32_t referenceCount = 0;
  Texture *texture = nullptr;
};

struct BindlessBuffer
{
  BindlessBufferAllocator::AllocId allocId = {};
  uint32_t referenceCount = 0;
};

struct MeshMeta : public BVHMeta
{
  static constexpr uint32_t bvhMaterialTerrain = 0;
  static constexpr uint32_t bvhMaterialRendinst = 1;
  static constexpr uint32_t bvhMaterialInterior = 2;
  static constexpr uint32_t bvhMaterialParticle = 3;
  static constexpr uint32_t bvhMaterialCable = 4;

  static constexpr uint32_t bvhMaterialAlphaTest = 1 << 16;
  static constexpr uint32_t bvhMaterialPainted = 1 << 17;
  static constexpr uint32_t bvhMaterialImpostor = 1 << 18;
  static constexpr uint32_t bvhMaterialAtlas = 1 << 19;
  static constexpr uint32_t bvhInstanceColor = 1 << 20;
  static constexpr uint32_t bvhMaterialCamo = 1 << 21;
  static constexpr uint32_t bvhMaterialLayered = 1 << 22;
  static constexpr uint32_t bvhMaterialGrass = 1 << 23;
  static constexpr uint32_t bvhMaterialEmissive = 1 << 24;
  static constexpr uint32_t bvhMaterialMFD = 1 << 25;
  static constexpr uint32_t bvhMaterialTexcoordAdd = 1 << 26;
  static constexpr uint32_t bvhMaterialPerlinLayered = 1 << 27;
  static constexpr uint32_t bvhMaterialAlphaInRed = 1 << 28;
  static constexpr uint32_t bvhMaterialEye = 1 << 29;
  static constexpr uint32_t bvhMaterialUseInstanceTextures = 1 << 30;

  static constexpr uint32_t INVALID_TEXTURE = 0xFFFFu;
  static constexpr uint32_t INVALID_SAMPLER = 0xFFFFu;

  MeshMeta()
  {
    materialData1 = {};
    materialData2 = {};
    layerData = {};
    initialized = 0;
    materialType = 0;
    alphaTextureIndex = INVALID_TEXTURE;
    alphaSamplerIndex = INVALID_SAMPLER;
    ahsVertexBufferIndex = BVH_BINDLESS_BUFFER_MAX;
    padding = 0;
    colorOffset = 0xFFu;
    indexCount = 0;
    texcoordOffset = 0xFFu;
    normalOffset = 0xFFu;
    indexBit = 1;
    texcoordFormat = 0x7FFFFFFFu;
    indexBufferIndex = BVH_BINDLESS_BUFFER_MAX;
    vertexStride = 0xFFu;
    vertexBufferIndexHigh = 0xFu;
    vertexBufferIndexLow = 0xFFFFu;
    albedoTextureIndex = INVALID_TEXTURE;
    albedoAndNormalSamplerIndex = INVALID_SAMPLER;
    normalTextureIndex = INVALID_TEXTURE;
    extraTextureIndex = INVALID_TEXTURE;
    extraSamplerIndex = INVALID_SAMPLER;
    startIndex = 0;
    startVertex = 0;
    texcoordScale = 1.0f;
    atlasTileSize = 0;
    atlasFirstLastTile = 0;
    vertexOffset = 0;
    texcoordAdd = 0;
  }

  bool isInitialized() const { return initialized; }
  void markInitialized() { initialized = true; }

  void setIndexBit(uint32_t index_format)
  {
    G_ASSERT(index_format == 2 || index_format == 4);
    indexBit = index_format == 4 ? 1 : 0;
  }
  void setTexcoordFormat(uint32_t texcoord_format)
  {
    G_ASSERT((texcoord_format >> 31) == 0 || texcoord_format == 0xFFFFFFFFU);
    texcoordFormat = texcoord_format & 0x7FFFFFFFU;
  }
  void setIndexBitAndTexcoordFormat(uint32_t index_format, uint32_t texcoord_format)
  {
    indexBit = 0;
    texcoordFormat = 0;
    setIndexBit(index_format);
    setTexcoordFormat(texcoord_format);
  }
  void setIndexBufferIndex(uint32_t index)
  {
    G_ASSERT(index <= BVH_BINDLESS_BUFFER_MAX);
    indexBufferIndex = index;
  }
  void setVertexBufferIndex(uint32_t index)
  {
    G_ASSERT(index <= BVH_BINDLESS_BUFFER_MAX);
    vertexBufferIndexLow = index & BVH_BINDLESS_BUFFER_LOW_MASK;
    vertexBufferIndexHigh = (index & BVH_BINDLESS_BUFFER_HIGH_MASK) >> 16;
  }
  void setAhsVertexBufferIndex(uint32_t index)
  {
    G_ASSERT(index <= BVH_BINDLESS_BUFFER_MAX);
    ahsVertexBufferIndex = index;
  }
  // Helper functions because we can't pass the address of bitfields
  Texture *holdAlbedoTex(Context *context_id, TEXTUREID texture_id);
  Texture *holdNormalTex(Context *context_id, TEXTUREID texture_id);
  Texture *holdAlphaTex(Context *context_id, TEXTUREID texture_id);
  Texture *holdExtraTex(Context *context_id, TEXTUREID texture_id);
};
static_assert(sizeof(MeshMeta) == sizeof(BVHMeta));

template <int pool_size_pow, int pool_count>
struct MeshMetaHeapManager
{
public:
  using Elem = MeshMeta;
  using Heap = eastl::unique_ptr<Elem[]>;

  inline static constexpr int PoolSize = 1 << pool_size_pow;
  inline static constexpr int PoolSizeBits = pool_size_pow;
  inline static constexpr int PoolCount = pool_count;

  void increaseHeap(int increase)
  {
    G_UNUSED(increase);
    G_ASSERT(increase == PoolSize);

    if (nextPool >= PoolCount)
      logmessage(LOGLEVEL_FATAL, "MeshMetaHeapManager: Cannot increase heap size beyond %d", PoolCount);

    heap[nextPool++] = eastl::make_unique<Elem[]>(PoolSize);
  }

  void set(int index, const Elem &e) { get(index) = e; }
  void reset(int index) { get(index).materialType = 0; } // This resets the isInitialized bit

  Elem &get(int index) { return heap[index >> PoolSizeBits][index & (PoolSize - 1)]; }
  const Elem &get(int index) const { return heap[index >> PoolSizeBits][index & (PoolSize - 1)]; }

  int size() const { return nextPool * PoolSize; }

  const Elem *data(int bucket) const { return bucket < nextPool ? heap[bucket].get() : nullptr; }

private:
  Heap heap[PoolCount];
  int nextPool = 0;
};

static constexpr int mm_pool_size_pow = 10;
using MeshMetaAllocator = BVHHeapAllocator<MeshMetaHeapManager<mm_pool_size_pow, 256>, 1 << mm_pool_size_pow>;

struct Mesh
{
  void teardown(ContextId context_id);

  const BufferProcessor *vertexProcessor = nullptr;

  TEXTUREID albedoTextureId = BAD_TEXTUREID;
  TEXTUREID alphaTextureId = BAD_TEXTUREID;
  TEXTUREID normalTextureId = BAD_TEXTUREID;
  TEXTUREID extraTextureId = BAD_TEXTUREID;
  uint32_t indexCount = 0;
  uint32_t indexFormat = 0;
  uint32_t vertexCount = 0;
  uint32_t positionFormat = 0;
  uint32_t positionOffset = 0;
  uint32_t processedPositionFormat = 0;
  uint32_t texcoordOffset = 0;
  uint32_t texcoordFormat = 0;
  uint32_t secTexcoordOffset = 0;
  uint32_t normalOffset = 0;
  uint32_t colorOffset = 0;
  uint32_t indicesOffset = 0;
  uint32_t weightsOffset = 0;
  uint32_t vertexStride = 0;
  uint32_t baseVertex = 0;
  uint32_t startVertex = 0;
  uint32_t startIndex = 0;
  uint32_t materialType = 0;

  BSphere3 boundingSphere;

  UniqueBVHBufferWithOffset processedIndices;
  UniqueBVHBufferWithOffset processedVertices;
  UniqueBVHBufferWithOffset ahsVertices;

  uint32_t piBindlessIndex = -1;
  uint32_t pvBindlessIndex = -1;

  uint32_t albedoTextureLevel = 0;

  Point4 posMul;
  Point4 posAdd;

  bool isHeliRotor = false;
  bool isPaintedHeightLocked = false;

  float impostorHeightOffset = 0;
  Point4 impostorScale;
  Point4 impostorSliceTm1;
  Point4 impostorSliceTm2;
  Point4 impostorSliceClippingLines1;
  Point4 impostorSliceClippingLines2;
  Point4 impostorOffsets[4];
};

struct Object
{
  UniqueBLAS blas;
  dag::Vector<Mesh> meshes;
  MeshMetaAllocator::AllocId metaAllocId = MeshMetaAllocator::INVALID_ALLOC_ID;
  bool isAnimated = false;

  void teardown(ContextId context_id);

  dag::Span<MeshMeta> createAndGetMeta(ContextId context_id, int size);
};

struct TerrainPatch
{
  Point2 position;
  UniqueBVHBuffer vertices;
  UniqueBLAS blas;
  MeshMetaAllocator::AllocId metaAllocId = MeshMetaAllocator::INVALID_ALLOC_ID;
  bool hasHole = false;

  TerrainPatch() = default;
  TerrainPatch(const Point2 &position, UniqueBVHBuffer &&vertices, UniqueBLAS &&blas) :
    position(position), vertices(eastl::move(vertices)), blas(eastl::move(blas))
  {}
  TerrainPatch(const Point2 &position, UniqueBVHBuffer &&vertices, UniqueBLAS &&blas, MeshMetaAllocator::AllocId meta_alloc_id) :
    position(position), vertices(eastl::move(vertices)), blas(eastl::move(blas)), metaAllocId(meta_alloc_id)
  {}
  TerrainPatch(TerrainPatch &&other) :
    position(other.position), vertices(eastl::move(other.vertices)), blas(eastl::move(other.blas)), metaAllocId(other.metaAllocId)
  {
    other.metaAllocId = MeshMetaAllocator::INVALID_ALLOC_ID;
  }
  TerrainPatch &operator=(TerrainPatch &&other)
  {
    position = other.position;
    vertices = eastl::move(other.vertices);
    blas = eastl::move(other.blas);
    metaAllocId = other.metaAllocId;
    other.metaAllocId = MeshMetaAllocator::INVALID_ALLOC_ID;
    return *this;
  }
  ~TerrainPatch() { G_ASSERT(metaAllocId == MeshMetaAllocator::INVALID_ALLOC_ID); }

  void teardown(ContextId context_id);
};

struct TerrainLOD
{
  dag::Vector<TerrainPatch> patches;
};

struct ReferencedTransformData
{
  BVHBufferReference buffer;
  UniqueBLAS blas;
  MeshMetaAllocator::AllocId metaAllocId = MeshMetaAllocator::INVALID_ALLOC_ID;
  bool used = false;
};

struct ReferencedTransformDataWithAge
{
  eastl::unordered_map<uint64_t, ReferencedTransformData> elems;
  int age = 0;
};

struct BLASesWithAtomicCursor
{
  dag::AtomicInteger<int> cursor = 0;
  dag::Vector<UniqueBLAS> blases;
};

struct ProcessBufferAllocator
{
  ProcessBufferAllocator(uint32_t buffer_size, uint32_t buffer_count) : bufferSize(buffer_size), bufferCount(buffer_count) {}

  BVHBufferReference allocate()
  {
    for (auto &pool : pools)
      if (!pool.freeIndices.empty())
      {
        int index = pool.freeIndices.back();
        pool.freeIndices.pop_back();
        return BVHBufferReference{pool.buffer.get(), index * bufferSize};
      }

    auto &pool = pools.push_back();

    String name(64, "ProcessBufferAllocator_%u_%u", bufferSize, ++counter);
    auto buffer = d3d::buffers::create_ua_sr_byte_address(bufferCount * bufferSize / sizeof(uint32_t), name);
    HANDLE_LOST_DEVICE_STATE(buffer, BVHBufferReference());
    pool.buffer.reset(buffer);
    pool.freeIndices.reserve(bufferCount);
    for (uint32_t i = 0; i < bufferCount - 1; ++i)
      pool.freeIndices.push_back(i);

    return BVHBufferReference{pool.buffer.get(), (bufferCount - 1) * bufferSize};
  }

  void free(BVHBufferReference &ref)
  {
    if (!ref.buffer)
      return;

    for (auto &pool : pools)
      if (pool.buffer.get() == ref.buffer)
      {
        pool.freeIndices.push_back(ref.offset / bufferSize);
        ref.buffer = nullptr;
        ref.offset = 0;
        return;
      }

    G_ASSERT(false);
  }

  void reset() { pools.clear(); }

  struct Pool
  {
    dag::Vector<uint32_t> freeIndices;
    UniqueBVHBuffer buffer;
  };

  uint32_t bufferSize;
  uint32_t bufferCount;

  dag::Vector<Pool> pools;

  uint32_t counter = 0;
};

struct DECLSPEC_ALIGN(16) HWInstance
{
  mat43f transform;
  unsigned instanceID : 24;
  unsigned instanceMask : 8;
  unsigned instanceContributionToHitGroupIndex : 24;
  unsigned flags : 8;
  uint64_t blasGpuAddress;
} ATTRIBUTE_ALIGN(16);

using ObjectMap = ska::flat_hash_map<uint64_t, Object>;

static constexpr int ri_gen_thread_count = 16;
static constexpr int ri_extra_thread_count = 8;

extern int ri_thread_count_ofset;

inline int get_ri_gen_worker_count()
{
  return max(min(ri_gen_thread_count, threadpool::get_num_workers()) + ri_thread_count_ofset, 1);
}
inline int get_ri_extra_worker_count()
{
  return max(min(ri_extra_thread_count, threadpool::get_num_workers()) + ri_thread_count_ofset, 1);
}

struct UPoint2
{
  uint32_t x;
  uint32_t y;
};

struct Context
{
  friend struct DeathrowJob;

  struct Instance
  {
    enum class AnimationUpdateMode
    {
      DO_CULLING,
      FORCE_OFF,
      FORCE_ON
    };
    mat43f transform;
    uint64_t objectId;

    TMatrix4 invWorldTm;
    eastl::function<void()> setTransformsFn;
    eastl::function<void(Point4 &, Point4 &)> getHeliParamsFn;
    eastl::function<void(float &, Point2 &)> getDeformParamsFn;
    BVHBufferReference *uniqueTransformedBuffer;
    UniqueBLAS *uniqueBlas;
    bool uniqueIsRecycled;
    bool noShadow;
    AnimationUpdateMode animationUpdateMode;
    MeshMetaAllocator::AllocId metaAllocId;
    bool hasInstanceColor;
    eastl::optional<UPoint2> perInstanceData;

    TreeData tree;
    FlagData flag;
  };

  struct BLASCompaction
  {
    enum class Stage
    {
      Prepared,
      WaitingSize,
      WaitingGPUTime,
      WaitingCompaction,
      MovedFrom,
    };

    BLASCompaction() = default;
    BLASCompaction(BLASCompaction &&other)
    {
      objectId = other.objectId;
      compactedBlas = eastl::move(other.compactedBlas);
      compactedSize = eastl::move(other.compactedSize);
      compactedSizeValue = other.compactedSizeValue;
      other.compactedSizeValue = -1;
      other.objectId = 0;
      query = eastl::move(other.query);
      stage = other.stage;
      other.stage = Stage::MovedFrom; // For debugging
    }
    BLASCompaction &operator=(BLASCompaction &&other)
    {
      if (this == &other)
        return *this;

      objectId = other.objectId;
      compactedBlas = eastl::move(other.compactedBlas);
      compactedSize = eastl::move(other.compactedSize);
      compactedSizeValue = other.compactedSizeValue;
      other.compactedSizeValue = -1;
      other.objectId = 0;
      query = eastl::move(other.query);
      stage = other.stage;
      other.stage = Stage::MovedFrom; // For debugging
      return *this;
    }
    ~BLASCompaction()
    {
      if (blasCreateJob)
      {
        threadpool::wait(blasCreateJob);
        delete blasCreateJob;
      }
    }

    void beginSizeQuery()
    {
      if (compactedSize && compactedSize->lock(0, 0, static_cast<void **>(nullptr), VBLOCK_READONLY))
        compactedSize->unlock();
      d3d::issue_event_query(query.get());
      stage = Stage::WaitingSize;
    }

    uint64_t objectId = 0;
    uint32_t compactedSizeValue = -1;
    UniqueBLAS compactedBlas;
    UniqueBVHBuffer compactedSize;
    EventQueryHolder query;
    cpujobs::IJob *blasCreateJob = nullptr;

    Stage stage = Stage::Prepared;
  };

  using InstanceMap = dag::Vector<Instance>;
  using HWInstanceMap = dag::Vector<HWInstance>;

  static constexpr size_t hardware_destructive_interference_size =
#if defined(__x86_64__) || defined(_M_X64)
    128;
#else
    64;
#endif

  template <typename T>
  struct alignas(hardware_destructive_interference_size) Padded : T
  {};

  Context();
  ~Context() { teardown(); }

  void teardown();

  MeshMetaAllocator::AllocId allocateMetaRegion(int size);
  void freeMetaRegion(MeshMetaAllocator::AllocId &id);

  Texture *holdTexture(TEXTUREID id, uint32_t &texture_bindless_index, uint32_t &sampler_bindless_index,
    d3d::SamplerHandle sampler = d3d::INVALID_SAMPLER_HANDLE, bool forceRefreshSrvsWhenLoaded = false);
  bool releaseTexure(TEXTUREID id);
  bool releaseTextureFromPackedIndices(uint32_t texture_and_sampler_bindless_indices);
  void markChangedTextures();

  void holdBuffer(Sbuffer *buffer, uint32_t &bindless_index);
  bool releaseBuffer(Sbuffer *buffer);

  bool has(uint32_t feature) const { return (features & feature) != 0; }

  BLASCompaction *beginBLASCompaction(uint64_t object_id);
  void cancelCompaction(uint64_t object_id);

  String name;

  Features features = static_cast<Features>(0);

  float grassRange = 100;

  InstanceMap genericInstances;
  Padded<InstanceMap> riGenInstances[ri_gen_thread_count];
  Padded<HWInstanceMap> riExtraInstances[ri_extra_thread_count];
  Padded<dag::Vector<UPoint2>> riExtraInstanceData[ri_extra_thread_count];
  Padded<InstanceMap> riExtraTreeInstances[ri_extra_thread_count];
  Padded<eastl::unordered_map<dynrend::ContextId, InstanceMap>> dynrendInstances;
  Padded<HWInstanceMap> impostorInstances[ri_gen_thread_count];
  Padded<dag::Vector<UPoint2>> impostorInstanceData[ri_gen_thread_count];


  struct RingBuffers
  {
#if _TARGET_PC_WIN
    static constexpr uint32_t ringSize = 4;
#else
    static constexpr uint32_t ringSize = 2;
#endif

    inline static int ringIndex = 0;
    eastl::array<UniqueBuf, ringSize> buffers = {};

    static void step() { ringIndex = (ringIndex + 1) % ringSize; }

    operator bool() const { return !!buffers[0]; }

    Sbuffer *operator->() const { return buffers[ringIndex].getBuf(); }

    Sbuffer *getBuf() const { return buffers[ringIndex].getBuf(); }

    D3DRESID getBufId() const { return buffers[ringIndex].getBufId(); }

    bool allocate(int struct_size, int elements, int type, const char *name, ContextId context_id);

    int totalSize() const
    {
      int size = 0;
      for (auto &buf : buffers)
        size += buf ? buf->getSize() : 0;
      return size;
    }

    void close()
    {
      for (auto &buf : buffers)
        buf.close();
    }
  };

  RingBuffers tlasUploadMain;
  RingBuffers tlasUploadTerrain;
  RingBuffers meshMeta;
  RingBuffers perInstanceData;

  UniqueBuf tlasUploadParticles;

  ObjectMap objects;
  ObjectMap impostors;
  UniqueTLAS tlasMain;
  UniqueTLAS tlasTerrain;
  UniqueTLAS tlasParticles;

  eastl::unordered_map<uint32_t, uint32_t> camoTextures;

  bool tlasMainValid = false;
  bool tlasTerrainValid = false;
  bool tlasParticlesValid = false;

  eastl::unordered_set<TEXTUREID, TextureIdHash> texturesWaitingForLoad;
  eastl::unordered_map<TEXTUREID, BindlessTexture, TextureIdHash> usedTextures;
  eastl::unordered_map<Sbuffer *, BindlessBuffer> usedBuffers;

  eastl::unordered_set<uint64_t> halfBakedObjects;

  using CompQueue = eastl::deque<eastl::optional<BLASCompaction>>;
  CompQueue blasCompactions;
  eastl::unordered_map<uint64_t, CompQueue::iterator> blasCompactionsAccel;
  struct PendingCompactSizeBuffer
  {
    UniqueBVHBuffer buf;
    EventQueryHolder query;
  };
  dag::Vector<PendingCompactSizeBuffer> pendingCompactedSizeBuffersCache;
  dag::Vector<UniqueBVHBuffer> compactedSizeBufferCache;

  HeightProvider *heightProvider = nullptr;
  dag::Vector<TerrainLOD> terrainLods;
  Point2 terrainMiddlePoint = Point2(-1000000, -1000000);

  static constexpr int maxUniqueLods = 8;

  eastl::unordered_map<uint32_t, eastl::unordered_map<uint64_t, ReferencedTransformData>> uniqueHeliRotorBuffers;
  eastl::unordered_map<uint32_t, eastl::unordered_map<uint64_t, ReferencedTransformData>> uniqueDeformedBuffers;
  eastl::unordered_map<uint64_t, eastl::unordered_map<uint64_t, ReferencedTransformData>> uniqueRiExtraTreeBuffers;
  eastl::unordered_map<uint64_t, eastl::unordered_map<uint64_t, ReferencedTransformData>> uniqueRiExtraFlagBuffers;
  eastl::unordered_map<uint64_t, ReferencedTransformDataWithAge> uniqueTreeBuffers[maxUniqueLods];
  eastl::unordered_map<uint32_t, ReferencedTransformDataWithAge> uniqueSkinBuffers;

  eastl::unordered_map<uint64_t, BLASesWithAtomicCursor> freeUniqueTreeBLASes;
  eastl::unordered_map<uint64_t, BLASesWithAtomicCursor> freeUniqueSkinBLASes;

  eastl::unordered_map<uint64_t, ProcessBufferAllocator> processBufferAllocators;

  OSSpinlock pendingMeshAddActionsLock;
  OSSpinlock pendingMeshRemoveActionsLock;
  eastl::unordered_map<uint64_t, ObjectInfo> pendingObjectAddActions DAG_TS_GUARDED_BY(pendingMeshAddActionsLock);
  eastl::unordered_set<uint64_t> pendingObjectRemoveActions DAG_TS_GUARDED_BY(pendingMeshRemoveActionsLock);
  dag::AtomicInteger<bool> hasPendingMeshAddActions = false;

  struct ParticleMeta
  {
    TEXTUREID textureId;
    MeshMetaAllocator::AllocId metaAllocId;
  };

  eastl::unordered_map<uint32_t, ParticleMeta> particleMeta;

  TerrainPatch terrainPatchTemplate;

  uint32_t animatedInstanceCount = 0;

  OSSpinlock meshMetaAllocatorLock;
  MeshMetaAllocator meshMetaAllocator;

  BindlessTextureAllocator bindlessTextureAllocator;
  BindlessCubeTextureAllocator bindlessCubeTextureAllocator;
  BindlessBufferAllocator bindlessBufferAllocator;

#if DAGOR_DBGLEVEL > 0
  eastl::unordered_map<void *, String> bindlessBufferAllocatorNames;
#endif

  WinCritSec cutdownTreeLock;
  WinCritSec purgeSkinBuffersLock;

  UniqueBuf cableVertices;
  UniqueBuf cableIndices;
  dag::Vector<UniqueBLAS> cableBLASes;

  struct BindlessTexHolder
  {
    TEXTUREID texId = BAD_TEXTUREID;
    d3d::SamplerHandle texSampler = d3d::INVALID_SAMPLER_HANDLE;
    uint32_t bindlessTexture = 0;
    uint32_t bindlessSampler = 0;

    void close(bvh::ContextId context_id)
    {
      if (texId != BAD_TEXTUREID)
      {
        context_id->releaseTexure(texId);
        texId = BAD_TEXTUREID;
        texSampler = d3d::INVALID_SAMPLER_HANDLE;
      }
    }
  };

  BindlessTexHolder paint_details_texBindless;
  uint32_t paintTexSize = 0;
  BindlessTexHolder grass_land_color_maskBindless;
  BindlessTexHolder dynamic_mfd_texBindless;
  BindlessTexHolder cache_tex0Bindless;
  BindlessTexHolder indirection_texBindless;
  BindlessTexHolder cache_tex1Bindless;
  BindlessTexHolder cache_tex2Bindless;
  BindlessTexHolder last_clip_texBindless;

  UniqueTex atmosphereTexture;
  int atmosphereCursor = 0;
  bool atmosphereDirty = true;

  Texture *stubTexture = nullptr;

  dag::Vector<HWInstance> instanceDescsCpu;
  dag::Vector<UPoint2> perInstanceDataCpu;

  static constexpr int atmDegreesPerSample = 4;
  static constexpr int atmDistanceSteps = 200;
  static constexpr float atmMaxDistance = 20000;
  static constexpr int atmTexWidth = 360 / atmDegreesPerSample;
  static constexpr int atmTexHeight = atmMaxDistance / atmDistanceSteps;

  struct AtmData
  {
    E3DCOLOR inscatterValues[atmTexWidth * atmTexHeight];
    E3DCOLOR lossValues[atmTexWidth * atmTexHeight];
  } atmData;

  void releaseAllBindlessTexHolders();

  void moveToDeathrow(UniqueBVHBufferWithOffset &&buf);
  void clearDeathrow();
  void processDeathrow();
  void getDeathRowStats(int &count, int64_t &size);

private:
  OSSpinlock deathrowLock;
  dag::Vector<UniqueBVHBuffer> deathrow DAG_TS_GUARDED_BY(deathrowLock);
};

inline int divide_up(int x, int y) { return (x + y - 1) / y; }

inline String ccn(ContextId context_id, const char *name)
{
  return String(context_id->name.length() + 64, "%s_%s", context_id->name.c_str(), name);
}

void bvh_yield();

// Helper functions because we can't pass the address of bitfields
inline Texture *MeshMeta::holdAlbedoTex(Context *context_id, TEXTUREID texture_id)
{
  uint32_t textureIndex, samplerIndex;
  auto tex = context_id->holdTexture(texture_id, textureIndex, samplerIndex);
  albedoTextureIndex = textureIndex;
  albedoAndNormalSamplerIndex = samplerIndex;
  return tex;
}
inline Texture *MeshMeta::holdNormalTex(Context *context_id, TEXTUREID texture_id)
{
  uint32_t textureIndex, samplerIndex;
  auto tex = context_id->holdTexture(texture_id, textureIndex, samplerIndex);
  normalTextureIndex = textureIndex;
  return tex;
}
inline Texture *MeshMeta::holdAlphaTex(Context *context_id, TEXTUREID texture_id)
{
  uint32_t textureIndex, samplerIndex;
  auto tex = context_id->holdTexture(texture_id, textureIndex, samplerIndex);
  alphaTextureIndex = textureIndex;
  alphaSamplerIndex = samplerIndex;
  return tex;
}
inline Texture *MeshMeta::holdExtraTex(Context *context_id, TEXTUREID texture_id)
{
  uint32_t textureIndex, samplerIndex;
  auto tex = context_id->holdTexture(texture_id, textureIndex, samplerIndex);
  extraTextureIndex = textureIndex;
  extraSamplerIndex = samplerIndex;
  return tex;
}

} // namespace bvh
