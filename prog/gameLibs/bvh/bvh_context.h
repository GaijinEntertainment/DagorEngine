// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <atomic>
#include <3d/dag_resPtr.h>
#include <3d/dag_eventQueryHolder.h>
#include <drv/3d/dag_bindless.h>
#include <util/dag_multicastEvent.h>
#include <util/dag_threadPool.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_spinlock.h>
#include <dag/dag_vector.h>
#include <perfMon/dag_statDrv.h>
#include <generic/dag_enumerate.h>
#include <EASTL/unordered_map.h>
#include <EASTL/unordered_set.h>
#include <EASTL/optional.h>
#include <EASTL/string.h>
#include <bvh/bvh.h>

#define ENABLE_BLAS_COMPACTION 1

class LandMeshManager;

namespace bvh
{

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

template <typename HeapManager, int pool_size>
struct BVHHeapAllocator
{
  using Heap = typename HeapManager::Heap;
  using Elem = typename HeapManager::Elem;

  using AllocId = int;

  AllocId allocate()
  {
    if (freeIndices.empty())
    {
      int base = manager.size();
      manager.increaseHeap(pool_size);
      for (int i = 0; i < pool_size; ++i)
        freeIndices.push_back(base + i);
    }

    G_ASSERT(!freeIndices.empty());
    int index = freeIndices.back();
    freeIndices.pop_back();

    return index;
  }

  AllocId allocate(const Elem &e)
  {
    int index = allocate();
    manager.set(index, e);
    return index;
  }

  void free(AllocId index)
  {
    G_ASSERTF(index >= 0 && index < manager.size(), "Index is %d", index);
    manager.reset(index);
    freeIndices.push_back(index);
  }

  Elem &get(AllocId index) { return manager.get(index); }
  const Elem &get(AllocId index) const { return manager.get(index); }

  int size() const { return manager.size(); }

  int allocated() const { return manager.size() - freeIndices.size(); }

  Elem *data() { return manager.data(); }
  const Elem *data() const { return manager.data(); }

  Heap &getHeap() { return manager.getHeap(); }
  const Heap &getHeap() const { return manager.getHeap(); }

private:
  HeapManager manager;

  dag::Vector<int> freeIndices;
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
  BUFFER,
  COUNT
};

static constexpr uint32_t operator*(BindlessRangeType type) { return type == BindlessRangeType::BUFFER ? RES3D_SBUF : RES3D_TEX; }

struct TextureIdHash
{
  size_t operator()(TEXTUREID id) const { return (unsigned)id; }
};

template <BindlessRangeType type>
struct BindlessResourceHeap
{
  using ResourceType = eastl::type_select_t<type == BindlessRangeType::TEXTURE, Texture *, Sbuffer *>;

  void resize(int size)
  {
    if (size <= resources.size())
      return;

    resources.resize(size);
  }

  void updateBindings()
  {
    TIME_PROFILE(updateBindless);

    bool allDirty = false;

    if (resources.size() != range.size)
    {
      if (range.size == 0)
        range.range = d3d::allocate_bindless_resource_range(*type, resources.size());
      else
        range.range = d3d::resize_bindless_resource_range(*type, range.range, range.size, resources.size());

      range.size = resources.size();
      allDirty = true;
    }

    for (auto [index, resource] : enumerate(resources))
      if (allDirty || resource.dirty)
      {
        if (resource.res)
          d3d::update_bindless_resource(range.range + index, resource.res);
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

  Elem &get(int index) { return heap.resources[index]; }
  const Elem &get(int index) const { return heap.resources[index]; }

  int size() const { return heap.size(); }

  Elem *data() { return heap.resources.data(); }
  const Elem *data() const { return heap.resources.data(); }

  Heap &getHeap() { return heap; }
  const Heap &getHeap() const { return heap; }

private:
  Heap heap;
};

using BindlessTextureAllocator = BVHHeapAllocator<BindlessResourceHeapManager<BindlessRangeType::TEXTURE>, 32>;
using BindlessBufferAllocator = BVHHeapAllocator<BindlessResourceHeapManager<BindlessRangeType::BUFFER>, 32>;

struct BindlessTexture
{
  BindlessTextureAllocator::AllocId allocId = 0;
  uint32_t samplerIndex = 0;
  uint32_t referenceCount = 0;
  Texture *texture = nullptr;
};

struct BindlessBuffer
{
  BindlessBufferAllocator::AllocId allocId = 0;
  uint32_t referenceCount = 0;
};

struct MeshMeta
{
  static constexpr uint32_t bvhMaterialTerrain = 0;
  static constexpr uint32_t bvhMaterialRendinst = 1;
  static constexpr uint32_t bvhMaterialInterior = 2;
  static constexpr uint32_t bvhMaterialParticle = 3;

  static constexpr uint32_t bvhMaterialAlphaTest = 1 << 16;
  static constexpr uint32_t bvhMaterialPainted = 1 << 17;
  static constexpr uint32_t bvhMaterialImpostor = 1 << 18;
  static constexpr uint32_t bvhMaterialAtlas = 1 << 19;
  static constexpr uint32_t bvhInstanceColor = 1 << 20;
  static constexpr uint32_t bvhMaterialCamo = 1 << 21;
  static constexpr uint32_t bvhMaterialLayered = 1 << 22;
  static constexpr uint32_t bvhMaterialGrass = 1 << 23;
  static constexpr uint32_t bvhMaterialEmissive = 1 << 24;

  Point4 materialData1;
  Point4 materialData2;
  uint32_t layerData[4] = {};

  uint32_t materialType = 0;
  uint32_t indexBitAndTexcoordFormat = ~0U;
  uint32_t texcoordNormalColorOffsetAndVertexStride = ~0U;
  uint32_t indexAndVertexBufferIndex = ~0U;
  uint32_t albedoTextureAndSamplerIndex = ~0U;
  uint32_t alphaTextureAndSamplerIndex = ~0U;
  uint32_t normalTextureAndSamplerIndex = ~0U;
  uint32_t extraTextureAndSamplerIndex = ~0U;
  uint32_t startIndex = 0;
  uint32_t startVertex = 0;
  float texcoordScale = 1.f;

  uint32_t atlasTileSize = 0;
  uint32_t atlasFirstLastTile = 0;

  uint32_t vertexOffset = 0;

  uint32_t pad1 = 0;
  uint32_t pad2 = 0;

  bool isInitialized() const { return materialType >> 31; }
  void markInitialized() { materialType |= 1 << 31; }

  void setIndexBit(uint32_t index_format)
  {
    G_ASSERT(index_format == 2 || index_format == 4);
    indexBitAndTexcoordFormat &= 0x7FFFFFFFU;
    indexBitAndTexcoordFormat |= index_format == 4 ? (1 << 31) : 0;
  }
  void setTexcoordFormat(uint32_t texcoord_format)
  {
    G_ASSERT((texcoord_format >> 31) == 0 || texcoord_format == 0xFFFFFFFFU);
    indexBitAndTexcoordFormat &= 1 << 31;
    indexBitAndTexcoordFormat |= texcoord_format;
  }
  void setIndexBitAndTexcoordFormat(uint32_t index_format, uint32_t texcoord_format)
  {
    indexAndVertexBufferIndex = 0;
    setIndexBit(index_format);
    setTexcoordFormat(texcoord_format);
  }
};

struct MeshMetaHeapManager
{
public:
  using Elem = MeshMeta;
  using Heap = dag::Vector<Elem>;

  void increaseHeap(int pool_size) { heap.resize(heap.size() + pool_size); }

  void set(int index, const Elem &e) { heap[index] = e; }
  void reset(int index) { G_UNUSED(index); }

  Elem &get(int index) { return heap[index]; }
  const Elem &get(int index) const { return heap[index]; }

  int size() const { return heap.size(); }

  Elem *data() { return heap.data(); }
  const Elem *data() const { return heap.data(); }

  const Heap &getHeap() const { return heap; }

private:
  Heap heap;
};

using MeshMetaAllocator = BVHHeapAllocator<MeshMetaHeapManager, 32>;

struct Mesh
{
  ~Mesh()
  {
    G_ASSERT(albedoTextureId == BAD_TEXTUREID && alphaTextureId == BAD_TEXTUREID && normalTextureId == BAD_TEXTUREID &&
             extraTextureId == BAD_TEXTUREID);
  }

  void teardown(ContextId context_id);

  MeshMeta &createAndGetMeta(ContextId context_id);

  // This portion is used extensively, so they need to be kept together for cache efficiency.
  UniqueBLAS blas;
  MeshMetaAllocator::AllocId metaAllocId = -1;
  bool enableCulling = true;
  const BufferProcessor *vertexProcessor = nullptr;
  // ~This portion is used extensively, so they need to be kept together for cache efficiency.

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

  uint32_t piBindlessIndex = -1;
  uint32_t pvBindlessIndex = -1;

  uint32_t albedoTextureLevel = 0;

  Point4 posMul;
  Point4 posAdd;

  bool isHeliRotor = false;

  float impostorHeightOffset = 0;
  Point4 impostorScale;
  Point4 impostorSliceTm1;
  Point4 impostorSliceTm2;
  Point4 impostorSliceClippingLines1;
  Point4 impostorSliceClippingLines2;
  Point4 impostorOffsets[4];
};

struct TerrainPatch
{
  Point2 position;
  UniqueBVHBuffer vertices;
  UniqueBLAS blas;
  MeshMetaAllocator::AllocId metaAllocId = -1;

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
    other.metaAllocId = -1;
  }
  TerrainPatch &operator=(TerrainPatch &&other)
  {
    position = other.position;
    vertices = eastl::move(other.vertices);
    blas = eastl::move(other.blas);
    metaAllocId = other.metaAllocId;
    other.metaAllocId = -1;
    return *this;
  }
  ~TerrainPatch() { G_ASSERT(metaAllocId < 0); }

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
  MeshMetaAllocator::AllocId metaAllocId = -1;
};

struct ReferencedTransformDataWithAge
{
  int age = 0;
  eastl::unordered_map<uint64_t, ReferencedTransformData> elems;
};

struct BLASesWithAtomicCursor
{
  std::atomic<int> cursor = 0;
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

using MeshMap = eastl::unordered_map<uint64_t, Mesh>;

static constexpr int ri_gen_thread_count = 8;
static constexpr int ri_extra_thread_count = 8;

inline int get_ri_gen_worker_count() { return min(ri_gen_thread_count, threadpool::get_num_workers()); }
inline int get_ri_extra_worker_count() { return min(ri_extra_thread_count, threadpool::get_num_workers()); }

struct Context
{
  struct Instance
  {
    mat43f transform;
    uint64_t meshId;

    TMatrix4 invWorldTm;
    eastl::function<void()> setTransformsFn;
    eastl::function<void(Point4 &, Point4 &)> getHeliParamsFn;
    eastl::function<void(float &, Point2 &)> getDeformParamsFn;
    BVHBufferReference *uniqueTransformBuffer;
    UniqueBLAS *uniqueBlas;
    bool uniqueIsRecycled;
    bool noShadow;
    MeshMetaAllocator::AllocId metaAllocId;
    bool hasInstanceColor;
    eastl::optional<Point4> perInstanceData;

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
    };

    BLASCompaction() = default;
    BLASCompaction(BLASCompaction &&other)
    {
      meshId = other.meshId;
      compactedBlas = eastl::move(other.compactedBlas);
      compactedSize = other.compactedSize;
      compactedSizeValue = other.compactedSizeValue;
      other.compactedSize = nullptr;
      other.compactedSizeValue = 0;
      other.meshId = 0;
      query = eastl::move(other.query);
      stage = other.stage;
    }
    BLASCompaction &operator=(BLASCompaction &&other)
    {
      meshId = other.meshId;
      compactedBlas = eastl::move(other.compactedBlas);
      compactedSize = other.compactedSize;
      compactedSizeValue = other.compactedSizeValue;
      other.compactedSize = nullptr;
      other.compactedSizeValue = 0;
      other.meshId = 0;
      query = eastl::move(other.query);
      stage = other.stage;
      return *this;
    }
    ~BLASCompaction()
    {
      if (compactedSize)
        del_d3dres(compactedSize);
    }

    void beginSizeQuery()
    {
      if (compactedSize->lock(0, 0, static_cast<void **>(nullptr), VBLOCK_READONLY))
        compactedSize->unlock();
      d3d::issue_event_query(query.get());
      stage = Stage::WaitingSize;
    }

    uint64_t meshId = 0;
    uint32_t compactedSizeValue = -1;
    UniqueBLAS compactedBlas;
    Sbuffer *compactedSize = nullptr;
    EventQueryHolder query;

    Stage stage = Stage::Prepared;
  };

  using InstanceMap = dag::Vector<Instance>;
  using HWInstanceMap = dag::Vector<HWInstance>;

  Context();
  ~Context() { teardown(); }

  void teardown();

  MeshMetaAllocator::AllocId allocateMetaRegion();
  void freeMetaRegion(MeshMetaAllocator::AllocId &id);

  Texture *holdTexture(TEXTUREID id, uint32_t &texture_and_sampler_bindless_index,
    d3d::SamplerHandle sampler = d3d::INVALID_SAMPLER_HANDLE, bool forceRefreshSrvsWhenLoaded = false);
  bool releaseTexure(TEXTUREID id);
  void markChangedTextures();

  void holdBuffer(Sbuffer *buffer, uint32_t &bindless_index);
  bool releaseBuffer(Sbuffer *buffer);

  bool has(uint32_t feature) const { return (features & feature) != 0; }

  BLASCompaction *beginBLASCompaction(uint64_t mesh_id);
  void removeFromCompaction(uint64_t mesh_id);

  String name;

  Features features = static_cast<Features>(0);

  float grassRange = 100;

  InstanceMap genericInstances;
  InstanceMap riGenInstances[ri_gen_thread_count];
  HWInstanceMap riExtraInstances[ri_extra_thread_count];
  dag::Vector<Point4> riExtraInstanceData[ri_extra_thread_count];
  InstanceMap riExtraTreeInstances[ri_extra_thread_count];
  eastl::unordered_map<dynrend::ContextId, InstanceMap> dynrendInstances;
  HWInstanceMap impostorInstances[ri_gen_thread_count];
  dag::Vector<Point4> impostorInstanceData[ri_gen_thread_count];
  UniqueBuf tlasUploadMain;
  UniqueBuf tlasUploadTerrain;
  UniqueBuf tlasUploadParticles;
  UniqueBuf meshMeta;
  UniqueBuf perInstanceData;
  MeshMap meshes;
  MeshMap impostors;
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

  eastl::unordered_set<uint64_t> halfBakedMeshes;

  dag::Vector<BLASCompaction> blasCompactions;

  HeightProvider *heightProvider = nullptr;
  dag::Vector<TerrainLOD> terrainLods;
  Point2 terrainMiddlePoint = Point2(-1000000, -1000000);

  eastl::unordered_map<uint32_t, eastl::unordered_map<uint64_t, ReferencedTransformData>> uniqueSkinBuffers;
  eastl::unordered_map<uint32_t, eastl::unordered_map<uint64_t, ReferencedTransformData>> uniqueHeliRotorBuffers;
  eastl::unordered_map<uint32_t, eastl::unordered_map<uint64_t, ReferencedTransformData>> uniqueDeformedBuffers;
  eastl::unordered_map<uint64_t, eastl::unordered_map<uint64_t, ReferencedTransformData>> uniqueRiExtraTreeBuffers;
  eastl::unordered_map<uint64_t, eastl::unordered_map<uint64_t, ReferencedTransformData>> uniqueRiExtraFlagBuffers;
  eastl::unordered_map<uint64_t, ReferencedTransformDataWithAge> uniqueTreeBuffers;

  eastl::unordered_map<uint64_t, BLASesWithAtomicCursor> freeUniqueTreeBLASes;

  eastl::unordered_map<uint64_t, ProcessBufferAllocator> processBufferAllocators;

  OSSpinlock pendingMeshActionsLock;
  eastl::unordered_map<uint64_t, MeshInfo> pendingMeshActions DAG_TS_GUARDED_BY(pendingMeshActionsLock);
  std::atomic_bool hasPendingMeshActions = false;

  struct ParticleMeta
  {
    TEXTUREID textureId;
    int metaAllocId;
  };

  eastl::unordered_map<uint32_t, ParticleMeta> particleMeta;

  TerrainPatch terrainPatchTemplate;

  uint32_t animatedInstanceCount = 0;

  OSSpinlock meshMetaAllocatorLock;
  MeshMetaAllocator meshMetaAllocator;

  BindlessTextureAllocator bindlessTextureAllocator;
  BindlessBufferAllocator bindlessBufferAllocator;

  WinCritSec cutdownTreeLock;

  float riExtraRange = -1;

  UniqueBuf cableVertices;
  UniqueBuf cableIndices;
  dag::Vector<UniqueBLAS> cableBLASes;

  TEXTUREID registeredPaintTexId = BAD_TEXTUREID;
  d3d::SamplerHandle registeredPaintTexSampler = d3d::INVALID_SAMPLER_HANDLE;
  uint32_t paintTexBindlessIds = 0;
  uint32_t paintTexSize = 0;

  TEXTUREID registeredLandColorTexId = BAD_TEXTUREID;
  d3d::SamplerHandle registeredLandColorTexSampler = d3d::INVALID_SAMPLER_HANDLE;
  uint32_t landColorTexBindlessIds = 0;

  UniqueTex atmosphereTexture;
  int atmosphereCursor = 0;
  bool atmosphereDirty = true;

  Texture *stubTexture = nullptr;

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

  void releasePaintTex();
  void releaseLandColorTex();

  void moveToDeathrow(UniqueBVHBufferWithOffset &&buf);
  void clearDeathrow();
  void processDeathrow();
  void getDeathRowStats(int &count, int &size);

private:
  BLASCompaction *beginBLASCompaction();

  OSSpinlock deathrowLock;
  dag::Vector<UniqueBVHBuffer> deathrow DAG_TS_GUARDED_BY(deathrowLock);
};

inline int divide_up(int x, int y) { return (x + y - 1) / y; }

inline String ccn(ContextId context_id, const char *name)
{
  return String(context_id->name.length() + 64, "%s_%s", context_id->name.c_str(), name);
}

} // namespace bvh