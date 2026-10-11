// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <osApiWrappers/dag_atomic_types.h>
#include <osApiWrappers/dag_atomic.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_ringCPUQueryLock.h>
#include <3d/dag_eventQueryHolder.h>
#include <drv/3d/dag_bindless.h>
#include <util/dag_multicastEvent.h>
#include <util/dag_threadPool.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_spinlock.h>
#include <osApiWrappers/dag_rwSpinLock.h>
#include <osApiWrappers/dag_rwLock.h>
#include <osApiWrappers/dag_miscApi.h>
#include <dag/dag_vector.h>
#include <perfMon/dag_statDrv.h>
#include <generic/dag_enumerate.h>
#include <memory/dag_linearHeapAllocator.h>
#include <shaders/dag_linearSbufferAllocator.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/unordered_map.h>
#include <EASTL/unordered_set.h>
#include <EASTL/optional.h>
#include <EASTL/string.h>
#include <EASTL/vector_set.h>
#include <EASTL/numeric_limits.h>
#include <EASTL/array.h>
#include <EASTL/deque.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <bvh/bvh.h>
#include <math/dag_bits.h>
#include <generic/dag_span.h>
#include <math/dag_hlsl_floatx.h>
#include <vecmath/dag_vecMath.h>
#include "shaders/bvh_mesh_meta.hlsli"

#include <render/omm.h>

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

inline constexpr uint64_t GPU_ADDRESS_LOW_MASK = 0xFFFFFFFFu;
inline constexpr int GPU_ADDRESS_HIGH_SHIFT = 32;

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
  size_t hash() const { return eastl::hash<uint32_t>{}((uint32_t(slabIndex) << 16) | (uint32_t(offset) << 8) | size); }
};

struct BVHHeapAllocatorAllocIdHash
{
  size_t operator()(const BVHHeapAllocatorAllocId &id) const { return id.hash(); }
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

  static constexpr uint32_t get_occupancy_mask(AllocId allocation)
  {
    return uint32_t(((uint64_t(1) << allocation.size) - 1) << allocation.offset);
  }
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
    if (!is_valid(allocation) || allocation.slabIndex * SlabSize + allocation.offset + allocation.size > manager.size())
    {
      if (allocation != INVALID_ALLOC_ID)
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
  uint32_t rangeBase = 0;
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
  static constexpr uint32_t bvhMaterialSmokeTracer = 8;

  static constexpr uint32_t bvhMaterialPaintedByMask = 1 << 13;
  static constexpr uint32_t bvhMaterialDynrend = 1 << 14;
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
    secondaryMaskTextureIndex = INVALID_TEXTURE;
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
    G_ASSERT(index_format == 2);
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
  TextureHandle holdSecondaryMaskTex(Context *context_id, TEXTUREID texture_id);
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

struct DAG_TS_SCOPED_CAPABILITY LockedMetaAccess
{
  MeshMetaAllocator::AllocId allocId;

  MeshMeta &operator[](int i) { return metas[i]; }
  const MeshMeta &operator[](int i) const { return metas[i]; }
  dag::Span<MeshMeta> span() const { return metas; }

  LockedMetaAccess(Context &ctx, MeshMetaAllocator::AllocId id);

  ~LockedMetaAccess() DAG_TS_RELEASE()
  {
    if (lock)
      lock->unlock();
  }

  LockedMetaAccess(LockedMetaAccess &&) = delete;
  LockedMetaAccess(const LockedMetaAccess &) = delete;
  LockedMetaAccess &operator=(const LockedMetaAccess &) = delete;
  LockedMetaAccess &operator=(LockedMetaAccess &&) = delete;

private:
  OSSpinlock *lock = nullptr;
  dag::Span<MeshMeta> metas;
};

enum class OmmState : uint8_t
{
  None,
  Baking,
  Ready,
  Built,
  Failed
};

enum class OmmFailure : uint8_t
{
  None,
  UnsupportedTexcoordFormat,
  NoAlphaSource,
  AlphaTextureNeverLoaded,
  BakeStartFailed,
  ReadbackInvalid,
  NoOutputBuffers,
  NoDescriptors,
  ZeroArraySize,
  // Applies to one instance, thus it never fails the shared slot; here only to share the text table.
  InstanceAlphaSourceOverride,
  // Not an asset problem: the mesh has no cutout, thus it enters the BVH as opaque geometry.
  AllTrianglesOpaque,
  // Not an asset problem outside strict checks: the raster draws nothing for it, thus the BVH skips it.
  AllTrianglesTransparent,
};

struct OmmCacheEntry;

// Move-only owner of one OMM cache reference. The entry itself lives in the context's OMM cache and
// outlives the ref; only the refcount, and thus the eviction age, moves with this type, so no holder can
// leak a reference or drop one twice.
class OmmEntryRef
{
public:
  OmmEntryRef() = default;
  // Render thread only: it counts up in place, where reset() only queues the drop.
  OmmEntryRef(Context *context, OmmCacheEntry *entry);
  OmmEntryRef(const OmmEntryRef &) = delete;
  OmmEntryRef &operator=(const OmmEntryRef &) = delete;
  OmmEntryRef(OmmEntryRef &&other) : context(other.context), entry(other.entry)
  {
    other.context = nullptr;
    other.entry = nullptr;
  }
  OmmEntryRef &operator=(OmmEntryRef &&other)
  {
    if (this != &other)
    {
      reset();
      context = other.context;
      entry = other.entry;
      other.context = nullptr;
      other.entry = nullptr;
    }
    return *this;
  }
  ~OmmEntryRef() { reset(); }

  // Callable off the render thread: the drop only queues the entry, it touches no accounting.
  void reset();

  OmmCacheEntry *get() const { return entry; }
  OmmCacheEntry *operator->() const { return entry; }
  OmmCacheEntry &operator*() const { return *entry; }
  explicit operator bool() const { return entry != nullptr; }

private:
  Context *context = nullptr;
  OmmCacheEntry *entry = nullptr;
};

struct Mesh
{
  void teardown(ContextId context_id);

  const BufferProcessor *vertexProcessor = nullptr;

  TEXTUREID albedoTextureId = BAD_TEXTUREID;
  TEXTUREID alphaTextureId = BAD_TEXTUREID;
  TEXTUREID normalTextureId = BAD_TEXTUREID;
  TEXTUREID extraTextureId = BAD_TEXTUREID;
  TEXTUREID secondaryMaskTextureId = BAD_TEXTUREID;
  TEXTUREID ppPositionTextureId = BAD_TEXTUREID;
  TEXTUREID ppDirectionTextureId = BAD_TEXTUREID;
  TEXTUREID clothNoiseCombinedTexTextureId = BAD_TEXTUREID;
  TEXTUREID faceMorphAtlasTextureId = BAD_TEXTUREID;
  uint32_t ppPositionBindless = MeshMeta::INVALID_TEXTURE;
  uint32_t ppDirectionBindless = MeshMeta::INVALID_TEXTURE;
  uint32_t clothNoiseCombinedTexBindless = MeshMeta::INVALID_TEXTURE;
  uint32_t faceMorphAtlasBindless = MeshMeta::INVALID_TEXTURE;
  uint32_t faceMorphUvOffset = MeshInfo::invalidOffset;
  uint32_t faceMorphUvSize = 0;
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
  uint32_t corOffset = MeshInfo::invalidOffset;
  uint32_t vertexStride = 0;
  uint32_t baseVertex = 0;
  uint32_t startVertex = 0;
  uint32_t startIndex = 0;
  uint32_t materialType = 0;

  BSphere3 boundingSphere;

  BVHGeometryBufferWithOffset geometry;
  UniqueBVHBufferWithOffset ahsVertices;

  // An empty slot means no bake ran for it.
  OmmEntryRef ommEntries[2];
  // Mesh-level: these conditions have no cache entry to live on, or must not touch the shared entry.
  bool noAlphaSourceLogged = false;
  bool alphaSourceOverrideLogged = false;
  bool notOmmCandidateLogged = false;

  uint32_t piBindlessIndex = -1;
  uint32_t pvBindlessIndex = -1;

  uint32_t albedoTextureLevel = 0;

  Point4 posMul;
  Point4 posAdd;

  bool isHeliRotor = false;
  bool isGunBarrel = false;
  bool isPaintedHeightLocked = false;
  bool isCamoNet = false;
  bool hasColorMod = false;
  // Captured at mesh setup because vertexProcessor is nulled after the first build pass, while
  // the BLAS descriptor layout must stay stable across the frames an OMM bake keeps the mesh waiting.
  bool hasSecondaryGeometry = false;
  // Index of the mesh's first geometry desc in its object's BLAS; a mesh with secondary geometry takes two.
  uint32_t firstGeometryIndex = 0;
  // Hash of the mesh fields that shape the bake, part of this mesh's OMM cache keys. Vdata offsets stay
  // out of it: a re-add lands at a new offset over the same triangles.
  uint32_t ommLayoutHash = 0;

  // Progress of a half-baked mesh through process_meshes. Each stage runs exactly once; a mesh can
  // linger in ResolvingOmm for several frames while its bake completes. The OmmCacheEntry bake state is
  // the authoritative record of OMM progress; this only tracks which processing loop owns the mesh.
  enum class BuildStage : uint8_t
  {
    NeedsProcessing,
    ResolvingOmm,
    NeedsBlasBuild
  };
  BuildStage buildStage = BuildStage::NeedsProcessing;

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
  BvhType type = BvhType::None;
  bool isAnimated = false;
  bool hasVertexProcessor = false;
  const char *tag = nullptr;
  AssetNameRef assetName;
  // Bumped on every create or re-create of the object: the previous build inputs are freed then.
  uint32_t buildInputsGeneration = 0;

  void teardown(ContextId context_id, uint64_t object_id);

  void ensureMetaAllocated(ContextId context_id, int size);
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

struct OmmCacheKey
{
  uint64_t objectId = 0;
  uint32_t slotId = 0;
  TEXTUREID bakeTexId = BAD_TEXTUREID;
  // Hash of the mesh fields that shape the bake, so a content change under a reused object id resolves
  // to a new entry.
  uint32_t layoutHash = 0;

  bool operator==(const OmmCacheKey &) const = default;
};

struct OmmCacheKeyHash
{
  size_t operator()(const OmmCacheKey &key) const
  {
    uint64_t hash = key.objectId * 0x9E3779B97F4A7C15ull;
    hash = (hash ^ key.slotId) * 0x100000001B3ull;
    hash = (hash ^ unsigned(key.bakeTexId)) * 0x100000001B3ull;
    hash = (hash ^ key.layoutHash) * 0x100000001B3ull;
    return size_t(hash ^ (hash >> 32));
  }
};

// Bake state is unsynchronized: every reader and writer runs on the render thread.
//
// A bake walks None -> Baking -> Ready -> Built, or ends in Failed. A transient Failed goes back to
// None on the next object add, so a retry costs one add and not one frame.
// These fields track lifetimes that the state does not, each with its own owner:
//   refCount - the meshes and the BLASes that link the entry;
//   lastPollFrame with bakeObjectId - the object that waits for an in-flight bake, judged against
//     the half-baked object list;
//   zeroRefFrame - the age a zero-ref entry is evicted by.
struct OmmCacheEntry
{
  static constexpr uint8_t NO_BAKE_STARTED = 0xFFu;

  OmmState state = OmmState::None;
  uint32_t lastPollFrame = 0;
  render::omm::BakeHandle bakeHandle;
  render::omm::BakeResult bakeResult;
  UniqueOMM omm;

  OmmFailure failure = OmmFailure::None;
  // Frame the failure was recorded in; the override resolve times its retry of a transient failure from it.
  uint32_t failFrame = 0;
  bool failureLogged = false;
  // The override resolve publishes to the debug viewer itself, because no Mesh owns its entry; this
  // keeps each bake attempt published once.
  bool debugPublished = false;
  uint32_t textureWaitAttempts = 0;
  uint32_t textureWaitFrame = 0;

  // Bake parameters for the diagnostics. The format is the bake source's, not always mesh.texcoordFormat.
  uint32_t bakeTexcoordFormat = 0;
  uint8_t bakeSubdivisionLevel = NO_BAKE_STARTED;
  bool bakeUvCutout = false;
  render::omm::BakeStats bakeStats;

  uint32_t refCount = 0;
  uint32_t zeroRefFrame = 0;
  uint64_t bakeObjectId = 0;

#if DAGOR_DBGLEVEL > 0
  render::omm::DebugBakeSource debugBakeSource;
#endif

  // Clears the wait budget and the diagnostics with the state: the next user of the entry must not
  // inherit a spent budget or a suppressed log. The result buffers are the caller's to clear or recycle.
  void resetBakeState()
  {
    state = OmmState::None;
    bakeHandle = {};
    omm.reset();
    failure = OmmFailure::None;
    failureLogged = false;
    debugPublished = false;
    textureWaitAttempts = 0;
    textureWaitFrame = 0;
    bakeTexcoordFormat = 0;
    bakeSubdivisionLevel = NO_BAKE_STARTED;
    bakeUvCutout = false;
    bakeStats = {};
#if DAGOR_DBGLEVEL > 0
    debugBakeSource = {};
#endif
  }
};

using OmmCache = eastl::unordered_map<OmmCacheKey, OmmCacheEntry, OmmCacheKeyHash>;

// One reference for each mesh of a blas, in mesh order; an empty slot links no OMM. The ownership is all
// OmmEntryRef's, thus the references travel with the blas through the recycle pools as moves and exactly
// one holder ever gives them back.
struct OmmEntryLinks
{
  void assign(Context *context, dag::ConstSpan<OmmCacheEntry *> desired)
  {
    refs.clear();
    for (OmmCacheEntry *entry : desired)
      refs.push_back(OmmEntryRef(context, entry));
  }

  void reset() { refs.clear(); }

  void swap(OmmEntryLinks &other) { refs.swap(other.refs); }

  bool operator==(dag::ConstSpan<OmmCacheEntry *> desired) const
  {
    if (refs.size() != desired.size())
      return false;
    for (uint32_t i = 0; i < refs.size(); ++i)
      if (refs[i].get() != desired[i])
        return false;
    return true;
  }

  // Two inline: one of these is retained per pooled blas, thus the resident cost outweighs the spill.
  eastl::fixed_vector<OmmEntryRef, 2, true> refs;
};

struct ReferencedTransformData
{
  BVHBufferReference buffer;
  UniqueBLAS blas;
  MeshMetaAllocator::AllocId metaAllocId = MeshMetaAllocator::INVALID_ALLOC_ID;
  int age = 0;
  // Object generation this blas was built from; a mismatch means those inputs are freed: rebuild, never refit.
  uint32_t builtGeneration = 0;
  OmmEntryLinks linkedOmms;
};

struct ReferencedTransformDatasForInstance
{
  eastl::unordered_map<uint64_t, ReferencedTransformData> elems;
  int animIndex = 0;
};

struct PooledBLAS
{
  UniqueBLAS blas;
  uint32_t builtGeneration = 0;
  OmmEntryLinks linkedOmms;
};

// The blas, its generation and its OMM references travel as one group: a partial transfer would leave
// references behind, and eviction could then free an OMM a pooled blas still links.
inline void swap_blas_with_pool(ReferencedTransformData &data, PooledBLAS &pooled)
{
  data.blas.swap(pooled.blas);
  eastl::swap(data.builtGeneration, pooled.builtGeneration);
  data.linkedOmms.swap(pooled.linkedOmms);
}

inline void take_blas_from_pool(ReferencedTransformData &data, PooledBLAS &pooled) { swap_blas_with_pool(data, pooled); }

inline void give_blas_to_pool(PooledBLAS &pooled, ReferencedTransformData &data) { swap_blas_with_pool(data, pooled); }

struct BLASesWithAtomicCursor
{
  dag::AtomicInteger<int> cursor = 0;
  dag::Vector<PooledBLAS> blases;
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

__forceinline bool VECTORCALL need_winding_flip(mat43f transform)
{
  /* So this need explanation. From the DXR specification:
   * Since these winding direction rules are defined in object space, they are unaffected by instance
   * transforms. For example, an instance transform matrix with negative determinant (e.g. mirroring
   * some geometry) does not change the facing of the triangles within the instance. Per-geometry
   * transforms, by contrast, (defined in D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC), get combined with
   * the associated vertex data in object space, so a negative determinant matrix there does flip triangle
   * winding.
   *
   * BUT. Some of our models are modelled in a way that the actual triangles are flipped, and expected to be
   * flipped during rasterization by the model matrix, which has 1 or 3 axes flipped. So as the instance
   * transform is not flipping the winding, we need to do it here to match the rasterized output.
   */

  mat33f transposedBasis = {transform.row0, transform.row1, transform.row2};
  return v_test_vec_x_lt_0(v_mat33_det(transposedBasis));
}

#if _TARGET_C2

















































#else
using NativeInstance = HWInstance;

inline NativeInstance convert_instance(const HWInstance &src)
{
  NativeInstance out = src;
  if (need_winding_flip(src.transform))
    out.flags |= RaytraceGeometryInstanceDescription::Flags::TRIANGLE_CULL_FLIP_WINDING;
  return out;
}
#endif


struct LruCollisionData;

using ObjectMap = ska::flat_hash_map<uint64_t, Object>;

static constexpr int ri_gen_thread_count = 16;
static constexpr int ri_extra_thread_count = 8;

inline int get_ri_gen_worker_count() { return max(min(ri_gen_thread_count, threadpool::get_num_workers()), 1); }
inline int get_ri_extra_worker_count() { return max(min(ri_extra_thread_count, threadpool::get_num_workers()), 1); }

// BVH allocates GPU memory under objectsLock, and the lock is non-reentrant; the OOM
// report probes ownsWrite instead of re-acquiring. ownsWrite is not a reentrancy mechanism.
// Write acquisitions must go through this type, or ownsWrite misses them and the report skips.
struct WriteOwnerRwSpinLock : NoWritersSpinLockReadWriteLock
{
  void lockWrite(::da_profiler::desc_id_t profile_token = ::da_profiler::DescWriteLock) DAG_TS_ACQUIRE()
  {
    NoWritersSpinLockReadWriteLock::lockWrite(profile_token);
    interlocked_relaxed_store(writerTid, get_current_thread_id());
  }
  void unlockWrite() DAG_TS_RELEASE()
  {
    interlocked_relaxed_store(writerTid, 0);
    NoWritersSpinLockReadWriteLock::unlockWrite();
  }
  bool tryLockWrite() DAG_TS_TRY_ACQUIRE(true)
  {
    if (!NoWritersSpinLockReadWriteLock::tryLockWrite())
      return false;
    interlocked_relaxed_store(writerTid, get_current_thread_id());
    return true;
  }

  bool ownsWrite() const { return interlocked_relaxed_load(writerTid) == get_current_thread_id(); }

private:
  volatile int64_t writerTid = 0;
};

struct Context
{
  friend struct DeathrowJob;

  // TODO: split it up, use partially data oriented design + unions for mutually exclusive features
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
    ReferencedTransformData *uniqueData;
    AnimationUpdateMode animationUpdateMode;
    MeshMetaAllocator::AllocId metaAllocId;
    eastl::optional<PerInstanceData> perInstanceData;
    int animIndex;

    TreeData tree;
    FlagData flag;
    SkinData skin;

    bool uniqueIsRecycled;
    bool uniqueIsStationary;
    bool noShadow;
    bool hasInstanceColor;
    bool forceEnableBackfaceCulling;

    bool needsBlasBuild;
    bool alreadyProcessed;
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
      G_ASSERT(!other.blasCreateJob);
      objectId = other.objectId;
      compactedSizeValue = other.compactedSizeValue;
      compactedSizeOffset = other.compactedSizeOffset;
      compactedBlas = eastl::move(other.compactedBlas);
      other.compactedSizeValue = -1;
      other.compactedSizeOffset = 0;
      other.objectId = 0;
      query = eastl::move(other.query);
      stage = other.stage;
      type = other.type;
      other.stage = Stage::MovedFrom; // For debugging
    }
    BLASCompaction &operator=(BLASCompaction &&other)
    {
      if (this == &other)
        return *this;

      G_ASSERT(!blasCreateJob && !other.blasCreateJob);
      objectId = other.objectId;
      compactedSizeValue = other.compactedSizeValue;
      compactedSizeOffset = other.compactedSizeOffset;
      compactedBlas = eastl::move(other.compactedBlas);
      other.compactedSizeValue = -1;
      other.compactedSizeOffset = 0;
      other.objectId = 0;
      query = eastl::move(other.query);
      stage = other.stage;
      type = other.type;
      other.stage = Stage::MovedFrom; // For debugging
      return *this;
    }
    ~BLASCompaction()
    {
      if (auto job = interlocked_acquire_load_ptr(blasCreateJob))
        threadpool::wait(job);
    }

    uint64_t objectId = 0;
    uint32_t compactedSizeValue = -1;
    uint32_t compactedSizeOffset = 0;
    UniqueBLAS compactedBlas;
    EventQueryHolder query;
    cpujobs::IJob *volatile blasCreateJob = nullptr;
    BvhType type = BvhType::None;

    Stage stage = Stage::Created;
  };

  using InstanceMap = dag::Vector<Instance>;
  using HWInstanceMap = dag::Vector<HWInstance>;
  using NativeInstanceMap = dag::Vector<NativeInstance>;

  Context() = default;
  ~Context() { teardown(); }

  void teardown();

  MeshMetaAllocator::AllocId allocateMetaRegion(int size, const char *origin);
  void freeMetaRegion(MeshMetaAllocator::AllocId &id);

  TextureHandle holdTexture(TEXTUREID id, uint32_t &texture_bindless_index, bool forceRefreshSrvsWhenLoaded = false);
  bool releaseTexture(TEXTUREID id);
  bool releaseTexture(uint32_t texture_and_sampler_bindless_indices);
  void releaseUnavailableTextures();
  void markChangedTextures();

  void holdBuffer(Sbuffer *buffer, uint32_t &bindless_index);
  bool releaseBuffer(Sbuffer *buffer);

  bool hasAny(uint32_t featureBits) const { return (features & featureBits) != 0; }
  bool hasAll(uint32_t featureBits) const { return (features & featureBits) == featureBits; }

  BLASCompaction *beginBLASCompaction(uint64_t object_id, BvhType type);
  void cancelCompaction(uint64_t object_id);

  String name;

  Features features = static_cast<Features>(0);
  Features designatedDynFeatures = static_cast<Features>(0);

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
    // MUST stay >= the driver's frames-in-flight (FRAME_FRAME_BACKLOG_LENGTH in DX12)
    // TODO: use assert or common consts with the drivers
#if _TARGET_PC_WIN
    static constexpr uint32_t ringSize = 4;
#else
    static constexpr uint32_t ringSize = 3;
#endif

    int ringIndex = 0;
    eastl::array<UniqueBuf, ringSize> buffers = {};

    void step() { ringIndex = (ringIndex + 1) % ringSize; }

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
  RingBuffers tlasUploadLruCollision;
  RingBuffers meshMeta;
  RingBuffers perInstanceData;

  UniqueBuf tlasUploadParticles;

  using BvhObjectReadLock = ScopedLockReadTemplate<WriteOwnerRwSpinLock>;
  using BvhObjectWriteLock = ScopedLockWriteTemplate<WriteOwnerRwSpinLock>;

  WriteOwnerRwSpinLock objectsLock;
  ObjectMap objects DAG_TS_GUARDED_BY(objectsLock);
  ObjectMap impostors DAG_TS_GUARDED_BY(objectsLock);
  // Starts at 1: builtGeneration 0 means "never built for any live object".
  uint32_t nextBuildInputsGeneration DAG_TS_GUARDED_BY(objectsLock) = 1;
  UniqueTLAS tlasMain;
  UniqueTLAS tlasTerrain;
  UniqueTLAS tlasParticles;
  UniqueTLAS tlasLruCollision;

  eastl::unordered_map<uint32_t, uint32_t> camoTextures;

  bool tlasMainValid = false;
  bool tlasTerrainValid = false;
  bool tlasParticlesValid = false;
  bool tlasLruCollisionValid = false;

  LruCollisionData *lruCollision = nullptr;

  eastl::unordered_set<TEXTUREID, TextureIdHash> texturesWaitingForLoad;

  render::omm::Context ommContext;
  bool ommEnabled = false;
  eastl::unordered_map<TEXTUREID, uint32_t, TextureIdHash> ommTextureWaitRefs;
  struct OmmTextureWaits
  {
    eastl::vector<TEXTUREID> textures;
    uint32_t attempts = 0;
  };
  eastl::unordered_map<uint64_t, OmmTextureWaits> ommTextureWaitsByObject;
  // Keyed by bake inputs, so a re-added object resolves to its old entry. Node-based: entry pointers
  // stay valid across a rehash, which every holder of an entry pointer relies on.
  // Render thread only: objectsLock read mode does not make an off-thread read of it safe.
  OmmCache ommCache;
  // Every member is in OmmState::Baking and owns a pending bake slot, and every such entry is a member.
  // A state change out of Baking must remove the entry in the same step. Render thread only, under the
  // same contract as ommCache.
  dag::Vector<OmmCacheEntry *> bakingOmmEntries;
  // Every dropped reference queues here: the refcount stays render thread only, thus a drop from a tidy
  // job is legal; the render thread drains the queue before it evicts.
  OSSpinlock deferredOmmReleaseLock;
  dag::Vector<OmmCacheEntry *> deferredOmmReleases DAG_TS_GUARDED_BY(deferredOmmReleaseLock);
  eastl::unordered_map<TEXTUREID, BindlessTexture, TextureIdHash> usedTextures;
  eastl::unordered_map<Sbuffer *, BindlessBuffer> usedBuffers;

  eastl::unordered_set<uint64_t> halfBakedObjects DAG_TS_GUARDED_BY(objectsLock);

  using CompQueue = eastl::deque<eastl::optional<BLASCompaction>>;
  CompQueue blasCompactions;
  eastl::unordered_map<uint64_t, CompQueue::iterator> blasCompactionsAccel;
  eastl::vector<eastl::unique_ptr<cpujobs::IJob>> createCompactedBLASJobQueue;
  int numCompactionBlasesInFlight = 0;
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
  eastl::array<uint64_t, compactedSizeBufferSize> compactedSizeBufferValues = {};

  struct VoxelActivity
  {
    // Everything here is render thread only, except the atomic job counters and the cpu
    // snapshot below, which state their own rules.
    // Ping-pong pair: a scroll copies current into the other one and swaps; decay and the ray
    // shaders' marks write current in place.
    eastl::array<UniqueTex, 2> tex = {};
    int current = 0;
    IPoint3 dims = IPoint3::ZERO;
    float voxelSize = 16;
    int activeValue = 0;                 // 0 while the feature is off for this context
    IPoint3 originVoxel = IPoint3::ZERO; // world voxel coordinate of texel (0,0,0)
    Point3 gridOriginRel = Point3::ZERO; // grid origin (texel 0,0,0 corner) relative to the build camera, for rebinds
    bool needsFill = true;

    // The decay kernel packs the aged volume into this ring, four voxels per uint. The ring
    // keeps at most readbackDepth frames in flight, so the origins need as many slots.
    static constexpr uint32_t readbackDepth = 4;
    RingCPUBufferLock readback;
    eastl::array<IPoint3, readbackDepth> pendingOrigin = {}; // readback frame -> grid origin at dispatch time

    // Snapshot for keep_instance, read by the placement jobs; the spawn/wait frame ordering is
    // the synchronization. Written in update() after the frame's jobs are waited, and by
    // set_voxel_activity / close_textures, which must stay outside the update_instances..build
    // job window.
    struct Cpu
    {
      dag::Vector<uint8_t> ages;     // x-major voxel lives
      vec4i originVoxel = v_zeroi(); // world voxel coordinate of texel (0,0,0); w = 0
      vec4i dimsMinus1 = v_zeroi();  // per axis voxel count - 1; w = INT_MAX, so it never fails the bounds test
      vec4f invVoxelSize = v_zero(); // splatted
      int rowStride = 0;             // dims.x
      int sliceStride = 0;           // dims.x * dims.y
      uint32_t frameSalt = 0;        // reshuffles the dead voxel keep set every frame
      uint32_t keepThreshold = 0;    // dead voxel keep chance in 1/65536 units
      bool cull = false;             // per frame copy of the debug kill switch
      bool valid = false;
    } cpu;

    // Placement statistics: the placement jobs count locally and add their totals once per job.
    dag::AtomicInteger<uint32_t> riGenConsidered = 0;
    dag::AtomicInteger<uint32_t> riGenCulled = 0;
    dag::AtomicInteger<uint32_t> riExConsidered = 0;
    dag::AtomicInteger<uint32_t> riExCulled = 0;
    dag::AtomicInteger<uint32_t> dynConsidered = 0;
    dag::AtomicInteger<uint32_t> dynCulled = 0;
    uint32_t statRiGenConsidered = 0, statRiGenCulled = 0;
    uint32_t statRiExConsidered = 0, statRiExCulled = 0;
    uint32_t statDynConsidered = 0, statDynCulled = 0;
  };
  VoxelActivity voxelActivity;

  HeightProvider *heightProvider = nullptr;
  dag::Vector<TerrainLOD> terrainLods;
  Point2 terrainMiddlePoint = Point2(-1000000, -1000000);
  // The terrain TLAS transforms are relative to this anchor, not the camera, so the TLAS only
  // needs a rebuild when a patch BLAS changes. The bvh_terrain_offset shader var moves the rays.
  Point2 terrainAnchorPoint = Point2(-1000000, -1000000);
  bool terrainDirty = false;

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
  eastl::unordered_map<uint64_t, ReferencedTransformDatasForInstance> uniqueRiExtraTreeBuffers[maxUniqueLods];
  eastl::unordered_map<uint64_t, ReferencedTransformDatasForInstance> uniqueRiExtraFlagBuffers[maxUniqueLods];
  eastl::unordered_map<uint64_t, ReferencedTransformData> uniqueSplinegenBuffers;
  eastl::unordered_map<uint64_t, ReferencedTransformDatasForInstance> uniqueTreeBuffers[maxUniqueLods];
  eastl::unordered_map<uint32_t, ReferencedTransformDatasForInstance> uniqueSkinBuffers;

  eastl::unordered_map<uint64_t, BLASesWithAtomicCursor> freeUniqueTreeBLASes;
  eastl::unordered_map<uint64_t, BLASesWithAtomicCursor> freeUniqueRiExtraTreeBLASes;
  eastl::unordered_map<uint64_t, BLASesWithAtomicCursor> freeUniqueRiExtraFlagBLASes;
  eastl::unordered_map<uint64_t, BLASesWithAtomicCursor> freeUniqueSkinBLASes;

  WinCritSec processBufferAllocatorLock;
  eastl::vector<eastl::pair<LinearHeapAllocatorSbuffer, uint32_t>> processBufferAllocator;

  eastl::unordered_map<uint64_t, ReferencedTransformData> stationaryTreeBuffers;

  static constexpr int MaxTreeAnimIndices = 10;
  int treeAnimIndexCount[MaxTreeAnimIndices] DAG_TS_GUARDED_BY(treeAnimIndexCountLock) = {};
  OSSpinlock treeAnimIndexCountLock;

  OSSpinlock pendingObjectActionsLock;
  eastl::unordered_map<uint64_t, eastl::pair<uint32_t, ObjectInfo>> pendingObjectAddActions DAG_TS_GUARDED_BY(
    pendingObjectActionsLock);
  eastl::unordered_map<uint64_t, uint32_t> pendingObjectRemoveActions DAG_TS_GUARDED_BY(pendingObjectActionsLock);
  eastl::unordered_map<uint64_t, uint32_t> pendingObjectPreChangeActions DAG_TS_GUARDED_BY(pendingObjectActionsLock);
  eastl::vector_set<const RenderableInstanceLodsResource *> pendingStaticBLASRequestActions;
  dag::AtomicInteger<bool> hasPendingObjectAddActions = false;
  dag::AtomicInteger<uint32_t> pendingObjectActionOrderCounter = 0;

  struct DynModelUsage
  {
    int lastUsedMsec = 0;
    eastl::vector_set<uint64_t> objectIds;
  };
  OSSpinlock dynrendObjectsWithTimeoutLock;
  eastl::unordered_map<uint32_t, DynModelUsage> dynrendObjectsWithTimeout DAG_TS_GUARDED_BY(dynrendObjectsWithTimeoutLock);

  struct ParticleMeta
  {
    TEXTUREID textureId;
    MeshMetaAllocator::AllocId metaAllocId;
  };

  eastl::unordered_map<uint32_t, ParticleMeta> particleMeta;

  TerrainPatch terrainPatchTemplate;

  OSSpinlock meshMetaAllocatorLock;
  MeshMetaAllocator meshMetaAllocator DAG_TS_GUARDED_BY(meshMetaAllocatorLock);

  WinCritSec bindlessTextureLock;
  BindlessTextureAllocator bindlessTextureAllocator;
  BindlessCubeTextureAllocator bindlessCubeTextureAllocator;
  BindlessBufferAllocator bindlessBufferAllocator;

#if DAGOR_DBGLEVEL > 0
  eastl::unordered_map<void *, String> bindlessBufferAllocatorNames;
#endif
  eastl::unordered_map<MeshMetaAllocator::AllocId, const char *, BVHHeapAllocatorAllocIdHash> metaOriginTracker;

  WinCritSec tidyUpRendinstsLock;
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
  UniqueBuf waterHeightHighDetailIb;
  UniqueBuf waterHeightLowDetailIb;

  struct BindlessTexHolder
  {
    TEXTUREID texId = BAD_TEXTUREID;
    uint32_t bindlessTexture = MeshMeta::INVALID_TEXTURE;

    void close(bvh::ContextId context_id)
    {
      if (texId != BAD_TEXTUREID)
      {
        G_VERIFY(context_id->releaseTexture(texId));
        texId = BAD_TEXTUREID;
      }
      bindlessTexture = MeshMeta::INVALID_TEXTURE;
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
    // Shared by every grass texture, but only without OMM: a micromap is baked from one alpha texture,
    // thus with OMM each texture needs the BLAS in TextureSlot below.
    UniqueBLAS blas;
    uint32_t vertexBufferBindless = BVH_BINDLESS_BUFFER_MAX;
    uint32_t indexBufferBindless = BVH_BINDLESS_BUFFER_MAX;
    uint32_t ahsBufferBindless = BVH_BINDLESS_BUFFER_MAX;

    // Only the render thread uses the bake state, as with OmmCacheEntry.
    struct TextureSlot
    {
      TEXTUREID alphaTexId = BAD_TEXTUREID;
      TEXTUREID diffuseTexId = BAD_TEXTUREID; // for the bake diagnostics
      UniqueBLAS blas;
      OmmState ommState = OmmState::None;
      uint32_t ommWaitAttempts = 0;
      render::omm::BakeHandle ommBakeHandle;
      render::omm::BakeResult ommBakeResult;
      render::omm::BakeStats ommBakeStats;
      render::omm::DebugBakeSource ommDebugBakeSource;
      UniqueOMM omm;
    };
    dag::Vector<TextureSlot> textureSlots;
    // The position where the entries of this orientation start in the shared mapping buffer.
    int mappingBase = 0;

    // Neither blas nor ahsBuffer is tested: with OMM the per-texture BLASes replace the shared one, and
    // the any-hit vertices are not needed at all.
    bool hasGeometry() const { return indexBuffer && vertexBuffer && metaAllocId != MeshMetaAllocator::INVALID_ALLOC_ID; }
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

  void releaseGameTextureHolds();
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
  eastl::array<bool, MaxTreeAnimIndices> riGenUpdateSlots = {}; // intentionally not bitset, size is tiny and fast lookup is preferred

  void rebuildUpdateSlots()
  {
    riGenUpdateSlots.fill(false);
    float step = float(MaxTreeAnimIndices) / riGenIndexTypePerFrame;
    for (int ix = 0; ix < riGenIndexTypePerFrame; ++ix)
    {
      int ui = (riGenStartIndexType + int(step * ix + 0.5)) % MaxTreeAnimIndices;
      riGenUpdateSlots[ui] = true;
    }
  }

private:
  bool releaseTextureNoLock(TEXTUREID id);

  OSSpinlock deathrowLock;
  dag::Vector<UniqueBVHBuffer> deathrow DAG_TS_GUARDED_BY(deathrowLock);
};

inline LockedMetaAccess::LockedMetaAccess(Context &ctx, MeshMetaAllocator::AllocId id) DAG_TS_ACQUIRE(ctx.meshMetaAllocatorLock) :
  lock(&ctx.meshMetaAllocatorLock), allocId(id)
{
  ctx.meshMetaAllocatorLock.lock();
  metas = ctx.meshMetaAllocator.get(id);
}

inline int divide_up(int x, int y) { return (x + y - 1) / y; }

inline String ccn(ContextId context_id, const char *name)
{
  return String(context_id->name.length() + 64, "%s_%s", context_id->name.c_str(), name);
}

Object *find_half_baked_object(ContextId context_id, uint64_t object_id) DAG_TS_REQUIRES_SHARED(context_id->objectsLock);

Sbuffer *alloc_scratch_buffer(uint32_t size, uint32_t &offset);

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
inline TextureHandle MeshMeta::holdSecondaryMaskTex(Context *context_id, TEXTUREID texture_id)
{
  uint32_t textureIndex;
  auto tex = context_id->holdTexture(texture_id, textureIndex);
  secondaryMaskTextureIndex = textureIndex;
  return tex;
}

} // namespace bvh
