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
#include <memory/dag_linearHeapAllocator.h>
#include <shaders/dag_linearSbufferAllocator.h>
#include <EASTL/unordered_map.h>
#include <EASTL/unordered_set.h>
#include <EASTL/optional.h>
#include <EASTL/string.h>
#include <EASTL/vector_set.h>
#include <EASTL/numeric_limits.h>
#include <EASTL/deque.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <bvh/bvh.h>
#include <math/dag_bits.h>
#include <generic/dag_span.h>
#include <math/dag_hlsl_floatx.h>
#include "shaders/bvh_mesh_meta.hlsli"

class LandMeshManager;

struct PerInstanceData
{
  uint32_t x;
  uint32_t y;
  uint32_t z;
  uint32_t w;

  static const PerInstanceData ZERO;
};

struct TextureHandle
{
  TextureHandle() = default;
  TextureHandle(TEXTUREID id) : id(id) {}
  TextureHandle(const TextureHandle &) = delete;
  TextureHandle(TextureHandle &&other)
  {
    texture = other.texture;
    id = other.id;
    other.texture = nullptr;
  }
  TextureHandle &operator=(const TextureHandle &) = delete;
  TextureHandle &operator=(TextureHandle &&other)
  {
    if (texture)
      release_managed_tex(id);

    texture = other.texture;
    id = other.id;
    other.texture = nullptr;
    return *this;
  }
  ~TextureHandle()
  {
    if (texture)
      release_managed_tex(id);
  }
  operator bool() const { return !!texture; }
  Texture *operator->() { return texture; }
  Texture *texture = nullptr;
  TEXTUREID id = BAD_TEXTUREID;
};

struct BVHBufferReference
{
  static inline constexpr LinearHeapAllocatorSbuffer::RegionId InvalidAllocId = {};

  uint32_t allocator = -1;
  LinearHeapAllocatorSbuffer::RegionId allocId = InvalidAllocId;

  operator bool() const { return allocId != InvalidAllocId; }
  bool operator!() const { return allocId == InvalidAllocId; }

  Sbuffer *buffer = nullptr;
  uint32_t size = 0;
  uint32_t offset = 0;
};

struct UniqueOrReferencedBVHBuffer
{
  UniqueBVHBuffer *unique = nullptr;
  BVHBufferReference *referenced = nullptr;

  UniqueOrReferencedBVHBuffer() = default;
  UniqueOrReferencedBVHBuffer(UniqueBVHBuffer &unique) : unique(&unique) {}
  UniqueOrReferencedBVHBuffer(BVHBufferReference &referenced) : referenced(&referenced) {}

  bool operator!() const { return !unique && !referenced; }
  operator bool() const { return unique || referenced; }

  Sbuffer *get() const { return unique ? unique->get() : referenced ? referenced->buffer : nullptr; }
  uint32_t getOffset() const { return referenced ? referenced->offset : 0; }

  bool needAllocation() const { return unique && !*unique || referenced && !*referenced; }
  bool isAllocated() const { return unique && *unique || referenced && *referenced; }
};

namespace bvh
{

#if _TARGET_C2




#elif _TARGET_APPLE
inline constexpr bool is_blas_compaction_enabled() { return false; }
inline constexpr bool is_blas_compaction_cheap() { return false; }
#else
inline constexpr bool is_blas_compaction_enabled() { return true; }
inline constexpr bool is_blas_compaction_cheap() { return false; }
#endif

// To be stored in InstanceContributionToHitGroupIndex
inline uint32_t pack_color8_to_color777(uint32_t color)
{
  return ((color & 0xFEu) << 2) | ((color & 0xFE00u) << 1) | ((color & 0xFE0000u) << 0);
}

inline constexpr ResourceBarrier bindlessSRVBarrier = ResourceBarrier::RB_RO_SRV | ResourceBarrier::RB_STAGE_ALL_SHADERS;
inline constexpr ResourceBarrier bindlessUAVBarrier = ResourceBarrier::RB_RW_UAV | ResourceBarrier::RB_STAGE_ALL_SHADERS;
inline constexpr ResourceBarrier bindlessUAVComputeBarrier = ResourceBarrier::RB_RW_UAV | ResourceBarrier::RB_STAGE_COMPUTE;

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
    if (allocation.slabIndex * SlabSize + allocation.offset + allocation.size > manager.size())
    {
      logerr("[BVH] Meta (index, offset, size) is (%d, %d, %d), which is invalid for free (manager size: %d)", allocation.slabIndex,
        allocation.offset, allocation.size, manager.size());
      return;
    }

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

  int add(dag::Span<ResourceType> resource_list)
  {
    int rangeBase = d3d::allocate_bindless_resource_range(*type, resource_list.size());
    ranges[rangeBase] = resource_list.size();

    for (auto [index, resource] : enumerate(resource_list))
    {
      G_ASSERT(resource);

      if constexpr (type == BindlessRangeType::BUFFER)
        G_ASSERT(resource->getFlags() & SBCF_BIND_SHADER_RES);

      d3d::update_bindless_resource(*type, rangeBase + index, resource);

      resources[rangeBase + index] = resource;
    }

    return rangeBase;
  }

  int add(ResourceType resource) { return add(make_span(&resource, 1)); }

  void update(int range, int offset, ResourceType resource)
  {
    auto iter = ranges.find(range);
    G_ASSERT_RETURN(iter != ranges.end(), );

    d3d::update_bindless_resource(*type, range + offset, resource);
    resources[range + offset] = resource;
  }

  void remove(int range)
  {
    auto iter = ranges.find(range);
    G_ASSERT_RETURN(iter != ranges.end(), );

    d3d::free_bindless_resource_range(*type, range, iter->second);
    for (int i = 0; i < iter->second; ++i)
      resources.erase(range + i);
    ranges.erase(iter);
  }

  ResourceType get_resource(int slot_index) const
  {
    auto iter = resources.find(slot_index);
    return iter == resources.end() ? nullptr : iter->second;
  }

  ~BindlessResourceHeap()
  {
    for (auto [range, size] : ranges)
      d3d::free_bindless_resource_range(*type, range, size);
  }

private:
  ska::flat_hash_map<int, int> ranges;
  ska::flat_hash_map<int, ResourceType> resources;
};

using BindlessTextureAllocator = BindlessResourceHeap<BindlessRangeType::TEXTURE>;
using BindlessCubeTextureAllocator = BindlessResourceHeap<BindlessRangeType::CUBE_TEXTURE>;
using BindlessBufferAllocator = BindlessResourceHeap<BindlessRangeType::BUFFER>;

struct BindlessTexture
{
  uint32_t rangeBase = 0xFFFFU;
  uint32_t slotIndex = 0;
  uint32_t referenceCount = 0;
  eastl::optional<D3DResourceType> resourceType;
};

struct BindlessBuffer
{
  uint32_t rangeBase = 0;
  uint32_t slotIndex = 0;
  uint32_t referenceCount = 0;
};

struct MeshMeta : public BVHMeta
{
  static constexpr uint32_t bvhMaterialTerrain = 0;
  static constexpr uint32_t bvhMaterialRendinst = 1;
  static constexpr uint32_t bvhMaterialInterior = 2;
  static constexpr uint32_t bvhMaterialParticle = 3;
  static constexpr uint32_t bvhMaterialCable = 4;
  static constexpr uint32_t bvhMaterialWater = 5;
  static constexpr uint32_t bvhMaterialLandclass = 6;
  static constexpr uint32_t bvhMaterialMonochrome = 7;

  static constexpr uint32_t bvhMaterialAnimcharDecals = 1 << 15;
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

  MeshMeta()
  {
    materialData1 = {};
    materialData2 = {};
    layerData = {};
    initialized = 0;
    materialType = 0;
    alphaTextureIndex = INVALID_TEXTURE;
    padding1 = 0;
    ahsVertexBufferIndex = BVH_BINDLESS_BUFFER_MAX;
    padding2 = 0;
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
    normalTextureIndex = INVALID_TEXTURE;
    extraTextureIndex = INVALID_TEXTURE;
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
  TextureHandle holdAlbedoTex(Context *context_id, TEXTUREID texture_id);
  TextureHandle holdNormalTex(Context *context_id, TEXTUREID texture_id);
  TextureHandle holdAlphaTex(Context *context_id, TEXTUREID texture_id);
  TextureHandle holdExtraTex(Context *context_id, TEXTUREID texture_id);
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
  void reset(int index) { get(index).initialized = 0; }

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
  TEXTUREID ppPositionTextureId = BAD_TEXTUREID;
  TEXTUREID ppDirectionTextureId = BAD_TEXTUREID;
  uint32_t ppPositionBindless = 0xFFFFFFFFU;
  uint32_t ppDirectionBindless = 0xFFFFFFFFU;
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

  BVHGeometryBufferWithOffset geometry;
  UniqueBVHBufferWithOffset ahsVertices;

  uint32_t piBindlessIndex = -1;
  uint32_t pvBindlessIndex = -1;

  uint32_t albedoTextureLevel = 0;

  Point4 posMul;
  Point4 posAdd;

  bool isHeliRotor = false;
  bool isPaintedHeightLocked = false;
  bool hasColorMod = false;

  eastl::optional<bool> needWindingFlip;

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
  bool hasVertexProcessor = false;
  const char *tag = nullptr;

  void teardown(ContextId context_id, uint64_t object_id);

  dag::Span<MeshMeta> createAndGetMeta(ContextId context_id, int size);
};

struct PhysTrackData
{
  int number_id = -1;
  int number_t_id = -1;
  int number_no = -1;
  int number_t_no = -1;
  int number_f = -1;
  int number_t_f = -1;
};

struct TerrainPatch
{
  Point2 position;
  UniqueBVHBuffer vertices;
  UniqueBLAS blas;
  MeshMetaAllocator::AllocId metaAllocId = MeshMetaAllocator::INVALID_ALLOC_ID;

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
  int animIndex = 0;
};

struct BLASesWithAtomicCursor
{
  dag::AtomicInteger<int> cursor = 0;
  dag::Vector<UniqueBLAS> blases;
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

#if _TARGET_C2

















































#else
using NativeInstance = HWInstance;

inline NativeInstance convert_instance(const HWInstance &src) { return src; }
#endif


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
    eastl::function<Sbuffer *(uint32_t &)> getSplineDataFn;
    BVHBufferReference *uniqueTransformedBuffer;
    UniqueBLAS *uniqueBlas;
    bool uniqueIsRecycled;
    bool uniqueIsStationary;
    bool noShadow;
    AnimationUpdateMode animationUpdateMode;
    MeshMetaAllocator::AllocId metaAllocId;
    bool hasInstanceColor;
    eastl::optional<PerInstanceData> perInstanceData;
    int animIndex;

    TreeData tree;
    FlagData flag;
  };

  struct BLASCompaction
  {
    enum class Stage
    {
      Created,
      SizeQueried,
      SizeBeingRead,
      SizeReceived,
      WaitingGPUTime,
      WaitingCompaction,
      MovedFrom,
    };

    BLASCompaction() = default;
    BLASCompaction(BLASCompaction &&other)
    {
      objectId = other.objectId;
      compactedSizeValue = other.compactedSizeValue;
      compactedSizeOffset = other.compactedSizeOffset;
      compactedBlas = eastl::move(other.compactedBlas);
      other.compactedSizeValue = -1;
      other.compactedSizeOffset = 0;
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
      compactedSizeValue = other.compactedSizeValue;
      compactedSizeOffset = other.compactedSizeOffset;
      compactedBlas = eastl::move(other.compactedBlas);
      other.compactedSizeValue = -1;
      other.compactedSizeOffset = 0;
      other.objectId = 0;
      query = eastl::move(other.query);
      stage = other.stage;
      other.stage = Stage::MovedFrom; // For debugging
      return *this;
    }
    ~BLASCompaction()
    {
      if (blasCreateJob)
        threadpool::wait(blasCreateJob);
    }

    uint64_t objectId = 0;
    uint32_t compactedSizeValue = -1;
    uint32_t compactedSizeOffset = 0;
    UniqueBLAS compactedBlas;
    EventQueryHolder query;
    cpujobs::IJob *blasCreateJob = nullptr;

    Stage stage = Stage::Created;
  };

  using InstanceMap = dag::Vector<Instance>;
  using HWInstanceMap = dag::Vector<HWInstance>;
  using NativeInstanceMap = dag::Vector<NativeInstance>;

  Context();
  ~Context() { teardown(); }

  void teardown();

  MeshMetaAllocator::AllocId allocateMetaRegion(int size);
  void freeMetaRegion(MeshMetaAllocator::AllocId &id);

  TextureHandle holdTexture(TEXTUREID id, uint32_t &texture_bindless_index, bool forceRefreshSrvsWhenLoaded = false);
  bool releaseTexture(TEXTUREID id);
  bool releaseTexture(uint32_t texture_and_sampler_bindless_indices);
  void markChangedTextures();

  void holdBuffer(Sbuffer *buffer, uint32_t &bindless_index);
  bool releaseBuffer(Sbuffer *buffer);

  bool has(uint32_t feature) const { return (features & feature) != 0; }

  BLASCompaction *beginBLASCompaction(uint64_t object_id);
  void cancelCompaction(uint64_t object_id);

  String name;

  Features features = static_cast<Features>(0);

  float grassRange = 100;
  float grassFraction = 1;

  InstanceMap genericInstances;
  Padded<InstanceMap> riGenInstances[ri_gen_thread_count];
  Padded<NativeInstanceMap> riExtraInstances[ri_extra_thread_count];
  Padded<dag::Vector<PerInstanceData>> riExtraInstanceData[ri_extra_thread_count];
  Padded<InstanceMap> riExtraTreeInstances[ri_extra_thread_count];
  Padded<InstanceMap> riExtraFlagInstances[ri_extra_thread_count];
  Padded<eastl::unordered_map<dynrend::ContextId, InstanceMap>> dynrendInstances;
  Padded<NativeInstanceMap> impostorInstances[ri_gen_thread_count];
  Padded<dag::Vector<PerInstanceData>> impostorInstanceData[ri_gen_thread_count];
  InstanceMap splineGenInstances;


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
    Sbuffer *getNextBuf() const { return buffers[(ringIndex + 1) % ringSize].getBuf(); }

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
  eastl::vector<eastl::unique_ptr<cpujobs::IJob>> createCompactedBLASJobQueue;
  eastl::atomic<int> numCompactionBlasesBeingCreated = 0;
  eastl::atomic<int> numCompactionBlasesWaitingBuild = 0;
  struct PendingCompactSizeBuffer
  {
    UniqueBVHBuffer buf;
    EventQueryHolder query;
  };
  UniqueBVHBuffer compactedSizeBuffer; // not used on PS5
  UniqueBVHBuffer compactedSizeBufferReadback;
  static constexpr uint32_t compactedSizeBufferSize = 512; // 4KB
  uint32_t compactedSizeWritesInQueue = 0;
  uint32_t compactedSizeBufferCursor = 0;
  EventQueryHolder compactedSizeQuery;
  bool compactedSizeQueryRunning = false;
  eastl::array<uint64_t, compactedSizeBufferSize> compactedSizeBufferValues;

  HeightProvider *heightProvider = nullptr;
  dag::Vector<TerrainLOD> terrainLods;
  Point2 terrainMiddlePoint = Point2(-1000000, -1000000);

  eastl::vector<eastl::pair<eastl::optional<LinearHeapAllocatorSbuffer>, uint32_t>> sourceGeometryAllocators;

  struct SourceGeometryAllocation
  {
    uint32_t heapIx;
    LinearHeapAllocatorSbuffer::RegionId region;
    uint32_t bindlessId;
  };
  SourceGeometryAllocation allocateSourceGeometry(uint32_t dwordCount, bool force_unique = false);
  void freeSourceGeometry(int &heapix, LinearHeapAllocatorSbuffer::RegionId region);
  uint32_t getSourceBufferOffset(int heapix, LinearHeapAllocatorSbuffer::RegionId region);
  uint32_t getSourceBufferSize(int heapix, LinearHeapAllocatorSbuffer::RegionId region);

  static constexpr int maxUniqueLods = 8;

  eastl::unordered_map<uint32_t, eastl::unordered_map<uint64_t, ReferencedTransformData>> uniqueHeliRotorBuffers;
  eastl::unordered_map<uint32_t, eastl::unordered_map<uint64_t, ReferencedTransformData>> uniqueDeformedBuffers;
  eastl::unordered_map<uint64_t, ReferencedTransformDataWithAge> uniqueRiExtraTreeBuffers[maxUniqueLods];
  eastl::unordered_map<uint64_t, eastl::unordered_map<uint64_t, ReferencedTransformData>> uniqueRiExtraFlagBuffers;
  eastl::unordered_map<uint64_t, ReferencedTransformData> uniqueSplinegenBuffers;
  eastl::unordered_map<uint64_t, ReferencedTransformDataWithAge> uniqueTreeBuffers[maxUniqueLods];
  eastl::unordered_map<uint32_t, ReferencedTransformDataWithAge> uniqueSkinBuffers;

  eastl::unordered_map<uint64_t, BLASesWithAtomicCursor> freeUniqueTreeBLASes;
  eastl::unordered_map<uint64_t, BLASesWithAtomicCursor> freeUniqueRiExtraTreeBLASes;
  eastl::unordered_map<uint64_t, BLASesWithAtomicCursor> freeUniqueSkinBLASes;

  WinCritSec processBufferAllocatorLock;
  eastl::vector<eastl::pair<LinearHeapAllocatorSbuffer, uint32_t>> processBufferAllocator;

  eastl::unordered_map<uint64_t, ReferencedTransformData> stationaryTreeBuffers;

  static constexpr int MaxTreeAnimIndices = 10;
  int treeAnimIndexCount[MaxTreeAnimIndices] = {};

  OSSpinlock pendingObjectActionsLock;
  eastl::unordered_map<uint64_t, eastl::pair<uint32_t, ObjectInfo>> pendingObjectAddActions DAG_TS_GUARDED_BY(
    pendingObjectActionsLock);
  eastl::unordered_map<uint64_t, uint32_t> pendingObjectRemoveActions DAG_TS_GUARDED_BY(pendingObjectActionsLock);
  eastl::unordered_map<uint64_t, uint32_t> pendingObjectPreChangeActions DAG_TS_GUARDED_BY(pendingObjectActionsLock);
  eastl::vector_set<const RenderableInstanceLodsResource *> pendingStaticBLASRequestActions;
  dag::AtomicInteger<bool> hasPendingObjectAddActions = false;
  dag::AtomicInteger<uint32_t> pendingObjectActionOrderCounter = 0;

  struct ParticleMeta
  {
    TEXTUREID textureId;
    MeshMetaAllocator::AllocId metaAllocId;
  };

  eastl::unordered_map<uint32_t, ParticleMeta> particleMeta;

  TerrainPatch terrainPatchTemplate;

  OSSpinlock meshMetaAllocatorLock;
  MeshMetaAllocator meshMetaAllocator;

  WinCritSec bindlessTextureLock;
  BindlessTextureAllocator bindlessTextureAllocator;
  BindlessCubeTextureAllocator bindlessCubeTextureAllocator;
  BindlessBufferAllocator bindlessBufferAllocator;

#if DAGOR_DBGLEVEL > 0
  eastl::unordered_map<void *, String> bindlessBufferAllocatorNames;
#endif

  WinCritSec tidyUpTreesLock;
  WinCritSec tidyUpSkinsLock;

  UniqueBuf cableVertices;
  UniqueBuf cableIndices;
  dag::Vector<UniqueBLAS> cableBLASes;

  dag::Vector<uint64_t> binSceneObjectIds;
  dag::Vector<uint64_t> splineGenObjectIds;

  struct WaterPatches
  {
    int triangleCount = 0;
    int vertexCount = 0;
    ManagedBufView indexBuffer;
    UniqueBuf vertexBuffer;
    MeshMetaAllocator::AllocId metaAllocId = MeshMetaAllocator::INVALID_ALLOC_ID;
    UniqueBLAS blas;
    uint32_t indexBufferBindless = BVH_BINDLESS_BUFFER_MAX;
    uint32_t vertexBufferBindless = BVH_BINDLESS_BUFFER_MAX;
    struct InstanceDesc
    {
      Point2 position;
      Point2 scale;
    };
    dag::Vector<InstanceDesc> instances;
  };
  dag::Vector<WaterPatches> water_patches;
  UniqueBuf waterFlatIb;
  UniqueBuf waterHeightIb;

  struct BindlessTexHolder
  {
    TEXTUREID texId = BAD_TEXTUREID;
    uint32_t bindlessTexture = 0;


    void close(bvh::ContextId context_id)
    {
      if (texId != BAD_TEXTUREID)
      {
        G_VERIFY(context_id->releaseTexture(texId));
        texId = BAD_TEXTUREID;
      }
    }
  };

  dag::Vector<SharedTex> gpuGrassTextures;
  struct GPUGrassBillboard
  {
    static constexpr int VERTEX_COUNT = 4;
    static constexpr int INDEX_COUNT = 6;
    UniqueBuf indexBuffer;
    UniqueBuf vertexBuffer;
    UniqueBuf ahsBuffer;
    MeshMetaAllocator::AllocId metaAllocId = MeshMetaAllocator::INVALID_ALLOC_ID;
    int metaSize = 0;
    UniqueBLAS blas;
    uint32_t vertexBufferBindless = BVH_BINDLESS_BUFFER_MAX;
    uint32_t indexBufferBindless = BVH_BINDLESS_BUFFER_MAX;
    uint32_t ahsBufferBindless = BVH_BINDLESS_BUFFER_MAX;
    bool isValid() const
    {
      return indexBuffer && vertexBuffer && ahsBuffer && blas && metaAllocId != MeshMetaAllocator::INVALID_ALLOC_ID;
    }
  };
  GPUGrassBillboard gpuGrassBillboard, gpuGrassHorizontal;

  BindlessTexHolder paint_details_texBindless;
  uint32_t paintTexSize = 0;
  BindlessTexHolder grass_land_color_maskBindless;
  BindlessTexHolder dynamic_mfd_texBindless;
  BindlessTexHolder cache_tex0Bindless;
  BindlessTexHolder indirection_texBindless;
  BindlessTexHolder cache_tex1Bindless;
  BindlessTexHolder cache_tex2Bindless;
  BindlessTexHolder last_clip_texBindless;
  BindlessTexHolder dynamic_decals_atlasBindless;

  int gbufferBindlessRange = -1;
  int fomShadowsBindlessRange = -1;

  UniqueTex atmosphereTexture;
  int atmosphereCursor = 0;
  bool atmosphereDirty = true;

  Texture *stubTexture = nullptr;

  dag::Vector<NativeInstance> instanceDescsCpu;
  dag::Vector<PerInstanceData> perInstanceDataCpu;

  UniqueBVHBuffer decalDataHolder;
  eastl::unordered_map<void *, int> decalDataHolderMap;
  int decalDataHolderCursor = 0;
  int decalDataHolderBindlessSlot = 0;

  eastl::vector<mat43f> initialNodes;
  UniqueBVHBuffer initialNodesHolder;
  int initialNodesHolderBindlessSlot = 0;

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

  void moveToDeathrow(BVHGeometryBufferWithOffset &&buf);
  void moveToDeathrow(UniqueBVHBufferWithOffset &&buf);
  void clearDeathrow();
  void processDeathrow();
  void getDeathRowStats(int &count, int64_t &size);

  static constexpr size_t MAX_GEOMS_PER_OBJ = 32;
  dag::Vector<::raytrace::BatchedBottomAccelerationStructureBuildInfo> blasUpdates;
  dag::Vector<eastl::fixed_vector<RaytraceGeometryDescription, MAX_GEOMS_PER_OBJ>> updateGeoms;

  int riGenIndexTypePerFrame = Context::MaxTreeAnimIndices;
  int riGenStartIndexType = 0;
  int lastRiGenProcessTimeUs = 0;

private:
  bool releaseTextureNoLock(TEXTUREID id);

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
inline TextureHandle MeshMeta::holdAlbedoTex(Context *context_id, TEXTUREID texture_id)
{
  uint32_t textureIndex;
  auto tex = context_id->holdTexture(texture_id, textureIndex);
  albedoTextureIndex = textureIndex;
  return tex;
}
inline TextureHandle MeshMeta::holdNormalTex(Context *context_id, TEXTUREID texture_id)
{
  uint32_t textureIndex;
  auto tex = context_id->holdTexture(texture_id, textureIndex);
  normalTextureIndex = textureIndex;
  return tex;
}
inline TextureHandle MeshMeta::holdAlphaTex(Context *context_id, TEXTUREID texture_id)
{
  uint32_t textureIndex;
  auto tex = context_id->holdTexture(texture_id, textureIndex);
  alphaTextureIndex = textureIndex;
  return tex;
}
inline TextureHandle MeshMeta::holdExtraTex(Context *context_id, TEXTUREID texture_id)
{
  uint32_t textureIndex;
  auto tex = context_id->holdTexture(texture_id, textureIndex);
  extraTextureIndex = textureIndex;
  return tex;
}

} // namespace bvh
