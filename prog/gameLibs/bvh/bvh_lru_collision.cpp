// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <render/lruCollision.h>
#include <drv/3d/rayTrace/dag_drvRayTrace.h>
#include <drv/3d/dag_buffers.h>
#include <3d/dag_lockSbuffer.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_shaderVar.h>
#include <memory/dag_linearHeapAllocator.h>
#include <shaders/dag_linearSbufferAllocator.h>
#include <util/dag_threadPool.h>
#include <util/dag_stlqsort.h>
#include <osApiWrappers/dag_atomic.h>
#include <memory/dag_framemem.h>
#include <perfMon/dag_statDrv.h>
#include <vecmath/dag_vecMath.h>
#include <EASTL/algorithm.h>
#include <EASTL/string.h>
#include <debug/dag_debug.h>

#include "bvh_context.h"

extern uint32_t lru_collision_get_type(rendinst::riex_handle_t h);
extern const CollisionResource *lru_collision_get_collres(uint32_t i);

namespace bvh
{

Sbuffer *alloc_scratch_buffer(uint32_t size, uint32_t &offset);

struct LruCollisionData
{
  struct GatherJob final : public cpujobs::IJob
  {
    LruCollisionData *self = nullptr;
    Point3 origin = {0, 0, 0};
    float range = 0;
    uint32_t rangeRevision = 0;
    dag::Vector<rendinst::riex_handle_t> handles;
    dag::Vector<mat43f> transforms;
    volatile int gatherDone = 0;
    void doJob() override;
    const char *getJobName(bool &) const override { return "BvhLruCollisionGather"; }
  } gatherJob;

  enum class ModelState : uint8_t
  {
    NotBuilt,
    Pending,
    Built,
    Empty
  };

  LruCollisionSettings settings;
  LRURendinstCollision *lru = nullptr;
  lru_collision_gather_fn gather;

  dag::Vector<rendinst::riex_handle_t> residentHandles;
  dag::Vector<mat43f> residentTransforms;
  dag::Vector<rendinst::riex_handle_t> pendingBuilds;
  dag::Vector<ModelState> modelState;
  dag::Vector<UniqueBLAS> modelBlas;
  dag::Vector<uint32_t> modelSize;
  dag::Vector<uint32_t> modelLastSeen;
  dag::Vector<NativeInstance> instanceDescs;

  // One octahedral face normal per triangle, the only hit attribute of the meta-less
  // TLAS. Written at BLAS build time from the heap regions; like the BLAS it is
  // derived data, so later updateLRU eviction cannot invalidate it.
  LinearHeapAllocatorSbuffer normalsPool{SbufferHeapManager("bvh_lru_collision_normals", sizeof(uint32_t), SBCF_UA_SR_STRUCTURED)};
  dag::Vector<LinearHeapAllocatorSbuffer::RegionId> modelNormals;
  UniqueBuf normalsIndex;
  bool normalsIndexDirty = false;
  // per type, the offset the bound table holds: a model is emitted only while its
  // live region matches, so a failed refresh suppresses exactly the stale models
  dag::Vector<uint32_t> tableOffsets;

  bool pendingStaleDrop = false;
  bool gatherContractViolated = false;
  uint32_t updateLRUFailStreak = 0;
  uint32_t blasFailStreak = 0;
  uint32_t normalsIndexFailStreak = 0;
  uint32_t normalsAllocFailStreak = 0;
  uint32_t allocRetryCooldown = 0;
  uint32_t gatherQueuePos = 0;
  Point3 lastGatherOrigin = {0, 0, 0};
  uint32_t gatherGeneration = 0;
  uint32_t rangeRevision = 0;
  bool windowValid = false;
  bool jobActive = false;
  uint32_t builtModels = 0;
  uint32_t revision = 0;
  uint64_t totalCachedSize = 0;
  uint64_t cacheLimit = 0;
};

void LruCollisionData::GatherJob::doJob()
{
  handles.clear();
  transforms.clear();
  bbox3f box;
  v_bbox3_init_by_bsph(box, v_ldu(&origin.x), v_splats(range));
  self->gather(box, handles, transforms);
  G_ASSERTF_ONCE(handles.size() == transforms.size(), "gather emitted %d transforms for %d handles", (int)transforms.size(),
    (int)handles.size());
  interlocked_release_store(gatherDone, 1);
}

} // namespace bvh

namespace bvh::lru_collision
{

using ModelState = LruCollisionData::ModelState;

static constexpr uint64_t CACHE_SIZE_ALIGNMENT = 64 << 10;
static inline uint64_t align_up_cache_size(uint64_t v) { return (v + CACHE_SIZE_ALIGNMENT - 1) & ~(CACHE_SIZE_ALIGNMENT - 1); }

static const auto blas_flags = RaytraceBuildFlags::FAST_TRACE | RaytraceBuildFlags::LOW_MEMORY;

// VRAM pressure recovers on the driver's schedule, not the next frame: retrying
// an allocation every frame re-attempts a heap resize per retry
static constexpr uint32_t ALLOC_RETRY_COOLDOWN_FRAMES = 16;

// revision seeds across reconnects (settings change, device reset), so a
// consumer's cached revision from before the reconnect never reads current
static uint32_t last_revision = 0;

static void ensure_model(LruCollisionData &d, uint32_t type)
{
  if (d.modelState.size() <= type)
  {
    d.modelState.resize(type + 1, ModelState::NotBuilt);
    d.modelBlas.resize(type + 1);
    d.modelSize.resize(type + 1, 0);
    d.modelLastSeen.resize(type + 1, 0);
    d.modelNormals.resize(type + 1);
    d.tableOffsets.resize(type + 1, ~0u);
  }
}

static void account_cache_and_evict(LruCollisionData &d)
{
  uint64_t requiredSize = 0;
  d.totalCachedSize = 0;
  for (uint32_t m = 0; m < d.modelState.size(); ++m)
  {
    if (d.modelState[m] != ModelState::Built)
      continue;
    d.totalCachedSize += d.modelSize[m];
    if (d.modelLastSeen[m] == d.gatherGeneration)
      requiredSize += d.modelSize[m];
  }

  if (requiredSize > d.cacheLimit)
  {
    d.cacheLimit = align_up_cache_size(requiredSize + requiredSize / 4);
    debug("[BVH] lru collision cache limit grown to %dK", int(d.cacheLimit >> 10));
  }

  if (d.totalCachedSize <= d.cacheLimit)
    return;
  FRAMEMEM_REGION;
  dag::Vector<eastl::pair<uint32_t, uint32_t>, framemem_allocator> cached;
  for (uint32_t m = 0; m < d.modelState.size(); ++m)
    if (d.modelState[m] == ModelState::Built && d.modelLastSeen[m] != d.gatherGeneration)
      cached.push_back({d.modelLastSeen[m], m});
  stlsort::sort(cached.begin(), cached.end());
  for (auto [gen, m] : cached)
  {
    if (d.totalCachedSize <= d.cacheLimit)
      break;
    d.modelBlas[m].reset();
    if (d.modelNormals[m])
    {
      d.normalsPool.free(d.modelNormals[m]);
      d.normalsIndexDirty = true;
    }
    d.modelState[m] = ModelState::NotBuilt;
    d.totalCachedSize -= d.modelSize[m];
    d.modelSize[m] = 0;
    --d.builtModels;
    ++d.revision;
  }
}

static void apply_gather(LruCollisionData &d)
{
  const bool residentChanged =
    d.gatherJob.handles != d.residentHandles || d.gatherJob.transforms.size() != d.residentTransforms.size() ||
    (d.residentTransforms.size() &&
      memcmp(d.gatherJob.transforms.data(), d.residentTransforms.data(), d.residentTransforms.size() * sizeof(mat43f)) != 0);
  d.residentHandles = eastl::move(d.gatherJob.handles);
  d.residentTransforms = eastl::move(d.gatherJob.transforms);
  d.gatherContractViolated = d.residentTransforms.size() < d.residentHandles.size();
  ++d.gatherGeneration;

  for (auto h : d.residentHandles)
  {
    const uint32_t type = lru_collision_get_type(h);
    if (type == ~0u)
      continue;
    ensure_model(d, type);
    if (d.modelLastSeen[type] == d.gatherGeneration)
      continue;
    d.modelLastSeen[type] = d.gatherGeneration;
    if (d.modelState[type] == ModelState::NotBuilt) //-V1051
    {
      d.modelState[type] = ModelState::Pending;
      d.pendingBuilds.push_back(h);
    }
  }

  for (uint32_t m = 0; m < d.modelState.size(); ++m)
    if (d.modelState[m] == ModelState::Pending && d.modelLastSeen[m] != d.gatherGeneration)
    {
      d.modelState[m] = ModelState::NotBuilt;
      d.pendingStaleDrop = true;
    }

  account_cache_and_evict(d);

  d.lastGatherOrigin = d.gatherJob.origin;
  d.windowValid = d.gatherJob.rangeRevision == d.rangeRevision;
  if (residentChanged)
    ++d.revision;
}

static void kick_gather(LruCollisionData &d, const Point3 &origin)
{
  d.gatherJob.origin = origin;
  d.gatherJob.range = d.settings.radius + d.settings.border;
  d.gatherJob.rangeRevision = d.rangeRevision;
  interlocked_release_store(d.gatherJob.gatherDone, 0);
  threadpool::add(&d.gatherJob, threadpool::PRIO_LOW, d.gatherQueuePos, threadpool::AddFlags::WakeOnAdd);
  // stays set until the job is waited out, even when it completed inside add:
  // the threadpool epilogue writes the job object after doJob returns, and an
  // unwaited teardown would free that memory under it
  d.jobActive = true;
}

// waiting passively on a PRIO_LOW job can outlast the gather itself (queued
// behind higher priority work); the barrier wait drains the queue up to and
// including the job on the calling thread
static void wait_gather(LruCollisionData &d)
{
  threadpool::barrier_active_wait_for_job(&d.gatherJob, threadpool::PRIO_LOW, d.gatherQueuePos);
  d.jobActive = false;
}

static constexpr uint32_t NORMALS_POOL_PAGE_SIZE = 1 << 20;

struct NormalsFill
{
  uint32_t startIndex, baseVertex, triCount, type;
};

static eastl::unique_ptr<ComputeShaderElement> fill_normals_shader;

static bool fill_normals(LruCollisionData &d, Sbuffer *heap_vb, Sbuffer *heap_ib, dag::ConstSpan<NormalsFill> fills)
{
  if (fills.empty())
    return true;

  TIME_D3D_PROFILE(bvh_lru_collision_normals);

  if (!fill_normals_shader)
    fill_normals_shader.reset(new_compute_shader("bvh_lru_collision_fill_normals"));
  if (!fill_normals_shader)
  {
    // a shader list that enables the module but misses the fill shader: the
    // caller retires the batch, so Built still implies filled
    static bool logged = false;
    if (!logged)
      logerr("[BVH] bvh_lru_collision_fill_normals is missing from the shader dump");
    logged = true;
    return false;
  }

  static int start_indexVarId = get_shader_variable_id("bvh_lru_collision_normals_start_index");
  static int base_vertexVarId = get_shader_variable_id("bvh_lru_collision_normals_base_vertex");
  static int tri_countVarId = get_shader_variable_id("bvh_lru_collision_normals_tri_count");
  static int dest_offsetVarId = get_shader_variable_id("bvh_lru_collision_normals_dest_offset");
  static int vb_const_no = ShaderGlobal::get_slot_by_name("bvh_lru_collision_normals_vb_const_no");
  static int ib_const_no = ShaderGlobal::get_slot_by_name("bvh_lru_collision_normals_ib_const_no");
  static int uav_no = ShaderGlobal::get_slot_by_name("bvh_lru_collision_normals_uav_no");

  Sbuffer *pool = d.normalsPool.getHeap().getBuf();
  G_ASSERT_RETURN(pool, false);

  d3d::resource_barrier(ResourceBarrierDesc(pool, ResourceBarrier::RB_RW_UAV | ResourceBarrier::RB_STAGE_COMPUTE));
  d3d::set_buffer(STAGE_CS, vb_const_no, heap_vb);
  d3d::set_buffer(STAGE_CS, ib_const_no, heap_ib);
  d3d::set_rwbuffer(STAGE_CS, uav_no, pool);

  for (const NormalsFill &fill : fills)
  {
    // resolved here, not at registration: a later allocation in the batch may grow
    // the pool, and the grow compacts region offsets
    const uint32_t destOffset = d.normalsPool.get(d.modelNormals[fill.type]).offset / sizeof(uint32_t);
    // one dispatch is capped at 65535 thread groups of 64: chunk oversized models
    constexpr uint32_t maxTrisPerDispatch = 65535u * 64u;
    for (uint32_t done = 0; done < fill.triCount; done += maxTrisPerDispatch)
    {
      const uint32_t count = min(fill.triCount - done, maxTrisPerDispatch);
      ShaderGlobal::set_int(start_indexVarId, fill.startIndex + done * 3);
      ShaderGlobal::set_int(base_vertexVarId, fill.baseVertex);
      ShaderGlobal::set_int(tri_countVarId, count);
      ShaderGlobal::set_int(dest_offsetVarId, destOffset + done);
      fill_normals_shader->dispatchThreads(count, 1, 1);
    }
  }

  d3d::set_buffer(STAGE_CS, vb_const_no, nullptr);
  d3d::set_buffer(STAGE_CS, ib_const_no, nullptr);
  d3d::set_rwbuffer(STAGE_CS, uav_no, nullptr);
  d3d::resource_barrier(ResourceBarrierDesc(pool, bindlessSRVBarrier));
  return true;
}

// A failure keeps the last complete table bound and the dirty flag set: the flag
// holds settled false and the retry runs next update. Models whose regions moved
// meanwhile fail the get_instances match and are absent until the retry lands.
// On growth the table buffer is replaced: consumers pick it up through the
// regular bind-after-build order, the same contract as every bvh resource.
static void update_normals_index(LruCollisionData &d)
{
  if (!d.normalsIndexDirty)
    return;

  const uint32_t count = d.modelNormals.size();
  if (!count)
  {
    d.normalsIndexDirty = false;
    return;
  }

  const bool grow = !d.normalsIndex || d.normalsIndex->getNumElements() < count;
  UniqueBuf next;
  if (grow)
  {
    static uint32_t generation = 0; // unique name: the outgoing buffer is still registered while both are alive
    eastl::string name;
    name.sprintf("bvh_lru_collision_normals_index_%u", generation++);
    next = dag::create_sbuffer(sizeof(uint32_t), max(count, 256u), SBCF_DYNAMIC | SBCF_BIND_SHADER_RES | SBCF_MISC_STRUCTURED, 0,
      name.c_str());
    if (!next)
    {
      if (d.normalsIndexFailStreak++ % 64 == 0)
        logwarn("[BVH] lru collision normals index creation failed for %u models (%u tries), will retry", count,
          d.normalsIndexFailStreak);
      return;
    }
  }

  if (auto data = lock_sbuffer<uint32_t>(grow ? next.getBuf() : d.normalsIndex.getBuf(), 0, 0, VBLOCK_WRITEONLY | VBLOCK_DISCARD))
  {
    for (uint32_t m = 0; m < count; ++m)
      data[m] = d.tableOffsets[m] = d.modelNormals[m] ? uint32_t(d.normalsPool.get(d.modelNormals[m]).offset / sizeof(uint32_t)) : ~0u;
  }
  else
  {
    if (d.normalsIndexFailStreak++ % 64 == 0)
      logwarn("[BVH] lru collision normals index lock failed for %u models (%u tries), will retry", count, d.normalsIndexFailStreak);
    return;
  }

  if (grow)
    d.normalsIndex = eastl::move(next);
  d.normalsIndexFailStreak = 0;
  d.normalsIndexDirty = false;
}

static void process_pending_builds(LruCollisionData &d, uint32_t max_builds)
{
  if (d.pendingBuilds.empty())
    return;
  if (d.allocRetryCooldown)
  {
    --d.allocRetryCooldown;
    return;
  }
  TIME_D3D_PROFILE(bvh_lru_collision_builds);
  FRAMEMEM_REGION;

  if (d.pendingStaleDrop)
  {
    d.pendingStaleDrop = false;
    d.pendingBuilds.erase(eastl::remove_if(d.pendingBuilds.begin(), d.pendingBuilds.end(),
                            [&](rendinst::riex_handle_t h) {
                              const uint32_t type = lru_collision_get_type(h);
                              return type >= d.modelState.size() || d.modelState[type] != ModelState::Pending;
                            }),
      d.pendingBuilds.end());
  }

  const uint32_t batchSize = min<uint32_t>(max_builds, d.pendingBuilds.size());
  if (!batchSize)
    return;

  // requeued entries go to the back of the queue: models waiting for their
  // resource or a freed-up allocation must not monopolize the per-frame batch
  // and starve ready models queued behind them
  dag::Vector<rendinst::riex_handle_t, framemem_allocator> requeue, batch;
  requeue.reserve(batchSize);
  batch.reserve(batchSize);
  for (uint32_t i = 0; i < batchSize; ++i)
  {
    const rendinst::riex_handle_t h = d.pendingBuilds[i];
    if (!lru_collision_get_collres(lru_collision_get_type(h)))
      requeue.push_back(h);
    else
      batch.push_back(h);
  }

  const uint32_t poolGeneration = d.normalsPool.offsetsGeneration();
  if (batch.size())
  {
    if (!d.lru->updateLRU(dag::ConstSpan<rendinst::riex_handle_t>(batch.data(), batch.size())))
    {
      d.allocRetryCooldown = ALLOC_RETRY_COOLDOWN_FRAMES;
      if (d.updateLRUFailStreak++ % 8 == 0)
        logwarn("[BVH] lru collision updateLRU failed for %d models (%u tries), will retry", (int)batch.size(), d.updateLRUFailStreak);
      return;
    }
    d.updateLRUFailStreak = 0;

    dag::Vector<RaytraceGeometryDescription, framemem_allocator> descs;
    dag::Vector<::raytrace::BatchedBottomAccelerationStructureBuildInfo, framemem_allocator> builds;
    dag::Vector<NormalsFill, framemem_allocator> normalsFills;
    descs.reserve(batch.size());
    builds.reserve(batch.size());
    normalsFills.reserve(batch.size());
    Sbuffer *heapVb = nullptr, *heapIb = nullptr;

    // the BLAS builds and the normals fill both run after the loop: a device-lost
    // return in between would leave this batch's models Built with neither ever
    // running. Revert them to Pending; the interrupted return also skips the
    // pendingBuilds erase, so the batch handles stay queued and rebuild after
    // recovery. A permanent failure (missing fill shader) retires the models as
    // Empty instead: only invalidate re-queries them, so a misconfiguration
    // degrades to a logged error, not per-frame rebuild churn.
    auto revertBatch = [&](bool permanent) {
      for (const NormalsFill &fill : normalsFills)
      {
        d.modelBlas[fill.type].reset();
        if (d.modelNormals[fill.type])
          d.normalsPool.free(d.modelNormals[fill.type]);
        d.totalCachedSize -= d.modelSize[fill.type];
        d.modelSize[fill.type] = 0;
        d.modelState[fill.type] = permanent ? ModelState::Empty : ModelState::Pending;
        --d.builtModels;
        ++d.revision;
        d.normalsIndexDirty = true;
      }
    };

    for (auto h : batch)
    {
      const uint32_t type = lru_collision_get_type(h);
      if (d.modelState[type] != ModelState::Pending)
        continue;
      auto data = d.lru->getModelData(type);
      if (!data.has_value() || data->vertexCount == 0 || data->indexCount == 0)
      {
        d.modelState[type] = ModelState::Empty;
        continue;
      }

      // the fill shader hardcodes this layout while the BLAS desc consumes it
      // generically: a mismatch must fail loudly in release builds too, or a
      // format change corrupts the stored normals silently on the shipping path
      if (DAGOR_UNLIKELY(
            data->vertexStride != 8 || data->positionFormat != VSDT_HALF4 || data->positionOffset != 0 || data->indexStride != 4))
      {
        static bool logged = false;
        if (!logged)
          logerr("[BVH] collision heap layout changed: stride %d format %d offset %d index stride %d, lru collision normals "
                 "cannot be filled",
            data->vertexStride, data->positionFormat, data->positionOffset, data->indexStride);
        logged = true;
        d.modelState[type] = ModelState::Empty; // only invalidate re-queries: no per-frame retry for a permanent mismatch
        continue;
      }

      const uint32_t triCount = uint32_t(data->indexCount) / 3;
      auto normalsRegion = d.normalsPool.allocate(triCount * sizeof(uint32_t), NORMALS_POOL_PAGE_SIZE);
      // checked at the allocation, not the batch end: a heap resize compacts every
      // region offset, and the later failure returns (device lost) must not skip
      // the repair
      if (d.normalsPool.offsetsGeneration() != poolGeneration)
        d.normalsIndexDirty = true;
      if (!normalsRegion)
      {
        if (is_in_lost_device_state || d3d::device_lost(nullptr))
        {
          revertBatch(false);
          return;
        }
        d.allocRetryCooldown = ALLOC_RETRY_COOLDOWN_FRAMES;
        if (d.normalsAllocFailStreak++ % 8 == 0)
          logwarn("[BVH] lru collision normals pool allocation failed for model %u (%u tries), will retry", type,
            d.normalsAllocFailStreak);
        requeue.push_back(h);
        continue;
      }

      RaytraceGeometryDescription desc;
      memset(&desc, 0, sizeof(desc));
      desc.type = RaytraceGeometryDescription::Type::TRIANGLES;
      desc.data.triangles.vertexBuffer = data->vertices;
      desc.data.triangles.indexBuffer = data->indices;
      desc.data.triangles.vertexCount = data->vertexCount;
      desc.data.triangles.vertexStride = data->vertexStride;
      desc.data.triangles.vertexOffset = data->baseVertex;
      desc.data.triangles.vertexFormat = data->positionFormat;
      desc.data.triangles.indexCount = data->indexCount;
      desc.data.triangles.indexOffset = data->startIndex;
      desc.data.triangles.indexFormat = RaytraceGeometryDescription::IndexFormat::U32;
      desc.data.triangles.flags = RaytraceGeometryDescription::Flags::IS_OPAQUE;

      auto blas = UniqueBLAS::create(&desc, 1, blas_flags);
      if (!blas)
      {
        d.normalsPool.free(normalsRegion);
        if (is_in_lost_device_state || d3d::device_lost(nullptr))
        {
          revertBatch(false);
          return;
        }
        d.allocRetryCooldown = ALLOC_RETRY_COOLDOWN_FRAMES;
        if (d.blasFailStreak++ % 8 == 0)
          logwarn("[BVH] lru collision BLAS creation failed for model %u (%u tries), will retry", type, d.blasFailStreak);
        requeue.push_back(h);
        continue;
      }

      auto &build = builds.push_back();
      build.as = blas.get();
      build.basbi.geometryDescCount = 1;
      build.basbi.flags = blas_flags;
      build.basbi.doUpdate = false;
      build.basbi.scratchSpaceBufferSizeInBytes = blas.getBuildScratchSize();
      build.basbi.scratchSpaceBuffer = alloc_scratch_buffer(blas.getBuildScratchSize(), build.basbi.scratchSpaceBufferOffsetInBytes);
      if (!build.basbi.scratchSpaceBuffer)
      {
        builds.pop_back();
        d.normalsPool.free(normalsRegion);
        if (is_in_lost_device_state || d3d::device_lost(nullptr))
        {
          revertBatch(false);
          return;
        }
        d.allocRetryCooldown = ALLOC_RETRY_COOLDOWN_FRAMES;
        requeue.push_back(h);
        continue;
      }
      descs.push_back(desc);
      heapVb = data->vertices;
      heapIb = data->indices;

      normalsFills.push_back({uint32_t(data->startIndex), uint32_t(data->baseVertex), triCount, type});
      d.modelNormals[type] = normalsRegion;
      d.normalsIndexDirty = true;

      d.modelSize[type] = blas.getASSize() + triCount * sizeof(uint32_t);
      d.modelBlas[type] = eastl::move(blas);
      d.modelState[type] = ModelState::Built;
      d.totalCachedSize += d.modelSize[type];
      ++d.builtModels;
      ++d.revision;
      d.blasFailStreak = 0;
      d.normalsAllocFailStreak = 0;
    }

    if (!builds.empty())
    {
      for (uint32_t i = 0; i < builds.size(); ++i)
        builds[i].basbi.geometryDesc = &descs[i];
      d3d::resource_barrier(ResourceBarrierDesc(heapVb, bindlessSRVBarrier));
      d3d::resource_barrier(ResourceBarrierDesc(heapIb, bindlessSRVBarrier));
      d3d::build_bottom_acceleration_structures(builds.data(), builds.size());
      if (!fill_normals(d, heapVb, heapIb, normalsFills))
        revertBatch(true); // Built implies filled: an unfillable batch retires as Empty
      else if (is_in_lost_device_state || d3d::device_lost(nullptr))
      {
        // the just-enqueued builds and fills die with the device: revert and
        // return before the erase, so the batch handles stay queued for recovery
        revertBatch(false);
        return;
      }
      account_cache_and_evict(d);
    }
  }
  d.pendingBuilds.erase(d.pendingBuilds.begin(), d.pendingBuilds.begin() + batchSize);
  for (auto h : requeue)
    d.pendingBuilds.push_back(h);
}

void update(ContextId context_id, const Point3 &camera_pos)
{
  auto d = context_id->lruCollision;
  if (!d)
    return;
  CHECK_LOST_DEVICE_STATE();
  TIME_D3D_PROFILE(bvh_lru_collision_update);

  const bool teleported = d->windowValid && (camera_pos - d->lastGatherOrigin).lengthSq() > d->settings.radius * d->settings.radius;
  const bool forceSyncFill = teleported || !d->windowValid;

  if (d->jobActive && (interlocked_acquire_load(d->gatherJob.gatherDone) || forceSyncFill))
  {
    wait_gather(*d);
    apply_gather(*d);
  }

  if (forceSyncFill && !d->jobActive)
  {
    d->gatherJob.origin = camera_pos;
    d->gatherJob.range = d->settings.radius;
    d->gatherJob.rangeRevision = d->rangeRevision;
    d->gatherJob.doJob();
    apply_gather(*d);
    d->allocRetryCooldown = 0; // the fill must attempt the builds now, pressure or not
    process_pending_builds(*d, ~0u);
    kick_gather(*d, camera_pos);
    update_normals_index(*d);
    return;
  }

  process_pending_builds(*d, d->settings.maxModelBuildsPerFrame);

  const float refreshDist = max(d->settings.border * 0.5f, 1.f);
  if (!d->jobActive && (camera_pos - d->lastGatherOrigin).lengthSq() > refreshDist * refreshDist)
    kick_gather(*d, camera_pos);

  update_normals_index(*d);
}

const dag::Vector<NativeInstance> &get_instances(ContextId context_id, const Point3 &camera_pos)
{
  static const dag::Vector<NativeInstance> empty;
  auto d = context_id->lruCollision;
  if (!d)
    return empty;
  TIME_PROFILE(bvh_lru_collision_instances);
  d->instanceDescs.clear();
  if (DAGOR_UNLIKELY(d->residentTransforms.size() < d->residentHandles.size()))
  {
    static bool logged = false;
    if (!logged)
      logerr("[BVH] lru collision gather emitted %d transforms for %d handles", d->residentTransforms.size(),
        d->residentHandles.size());
    logged = true;
  }
  d->instanceDescs.reserve(d->residentHandles.size());
  for (uint32_t i = 0, e = min(d->residentHandles.size(), d->residentTransforms.size()); i < e; ++i)
  {
    const uint32_t type = lru_collision_get_type(d->residentHandles[i]);
    if (type >= d->modelState.size() || d->modelState[type] != ModelState::Built)
      continue;
    // emitted only while the bound table's entry matches the live region: a hit
    // can then never decode another region's data, whatever refresh failed or
    // compaction happened. Mismatched models are absent and resolve to sky.
    if (d->tableOffsets[type] != uint32_t(d->normalsPool.get(d->modelNormals[type]).offset / sizeof(uint32_t)))
      continue;
    HWInstance desc;
    desc.transform.row0 = v_sub(d->residentTransforms[i].row0, v_make_vec4f(0, 0, 0, camera_pos.x));
    desc.transform.row1 = v_sub(d->residentTransforms[i].row1, v_make_vec4f(0, 0, 0, camera_pos.y));
    desc.transform.row2 = v_sub(d->residentTransforms[i].row2, v_make_vec4f(0, 0, 0, camera_pos.z));
    desc.instanceID = type & 0xFFFFFFu;
    desc.instanceMask = 0xFF;
    desc.instanceContributionToHitGroupIndex = 0;
    desc.flags = RaytraceGeometryInstanceDescription::TRIANGLE_CULL_DISABLE;
    desc.blasGpuAddress = d->modelBlas[type].getGPUAddress();
    d->instanceDescs.push_back(convert_instance(desc));
  }
  return d->instanceDescs;
}

void bind_resources(ContextId context_id)
{
  static int face_normalsVarId = get_shader_variable_id("bvh_lru_collision_face_normals", true);
  static int normals_indexVarId = get_shader_variable_id("bvh_lru_collision_normals_index", true);

  auto d = context_id->lruCollision;
  ShaderGlobal::set_buffer(face_normalsVarId, d ? d->normalsPool.getHeap().getBufId() : BAD_D3DRESID);
  ShaderGlobal::set_buffer(normals_indexVarId, d ? d->normalsIndex.getBufId() : BAD_D3DRESID);
}

void teardown(ContextId context_id)
{
  auto d = context_id->lruCollision;
  if (!d)
    return;
  if (d->jobActive)
    wait_gather(*d);
  last_revision = d->revision;
  delete d;
  context_id->lruCollision = nullptr;
  context_id->tlasLruCollision.reset();
  context_id->tlasUploadLruCollision.close();
  context_id->tlasLruCollisionValid = false;
}

void on_unload_scene(ContextId context_id) { invalidate_lru_collision(context_id); }

} // namespace bvh::lru_collision

namespace bvh
{

bool connect_lru_collision(ContextId context_id, LRURendinstCollision *lru_coll, lru_collision_gather_fn gather,
  const LruCollisionSettings &settings)
{
  if (context_id == InvalidContextId)
    return false;
  if (!context_id->hasAny(Features::LruCollision))
  {
    logerr("[BVH] connect_lru_collision needs Features::LruCollision on context %s", context_id->name.c_str());
    return false;
  }
  if (!lru_coll || settings.radius <= 0.f || settings.border < 0.f || settings.maxModelBuildsPerFrame == 0)
  {
    logerr("[BVH] connect_lru_collision rejected: lru_coll %p, radius %f, border %f, maxModelBuildsPerFrame %u", lru_coll,
      settings.radius, settings.border, settings.maxModelBuildsPerFrame);
    return false;
  }

  lru_collision::teardown(context_id);

  auto d = new LruCollisionData();
  d->lru = lru_coll;
  d->gather = eastl::move(gather);
  d->settings = settings;
  d->cacheLimit = lru_collision::align_up_cache_size(settings.initialCacheSize);
  d->revision = lru_collision::last_revision + 1;
  d->gatherJob.self = d;
  context_id->lruCollision = d;
  return true;
}

void remove_lru_collision(ContextId context_id)
{
  if (context_id == InvalidContextId)
    return;
  lru_collision::teardown(context_id);
}

void invalidate_lru_collision(ContextId context_id)
{
  if (context_id == InvalidContextId)
    return;
  auto d = context_id->lruCollision;
  if (!d)
    return;
  if (d->jobActive)
    lru_collision::wait_gather(*d);
  // the released BLASes must not stay reachable through the bound TLAS; the
  // next build() re-validates it
  context_id->tlasLruCollisionValid = false;
  d->modelState.clear();
  d->modelBlas.clear();
  d->modelSize.clear();
  d->modelLastSeen.clear();
  d->modelNormals.clear();
  d->tableOffsets.clear();
  d->normalsPool.clear();
  d->normalsIndexDirty = true;
  d->residentHandles.clear();
  d->residentTransforms.clear();
  d->pendingBuilds.clear();
  d->pendingStaleDrop = false;
  d->gatherContractViolated = false;
  d->allocRetryCooldown = 0;
  d->builtModels = 0;
  d->totalCachedSize = 0;
  d->gatherGeneration = 0;
  d->windowValid = false;
  ++d->revision;
}

void set_lru_collision_range(ContextId context_id, float radius, float border)
{
  auto d = context_id != InvalidContextId ? context_id->lruCollision : nullptr;
  if (!d)
    return;
  if (radius <= 0.f || border < 0.f)
  {
    logerr("[BVH] set_lru_collision_range rejected: radius %f, border %f", radius, border);
    return;
  }
  if (d->settings.radius == radius && d->settings.border == border)
    return;
  d->settings.radius = radius;
  d->settings.border = border;
  ++d->rangeRevision;
  d->windowValid = false;
}

LruCollisionStats get_lru_collision_stats(ContextId context_id)
{
  LruCollisionStats stats;
  auto d = context_id != InvalidContextId ? context_id->lruCollision : nullptr;
  if (!d)
    return stats;
  stats.residentInstances = d->residentHandles.size();
  stats.builtModels = d->builtModels;
  stats.cachedBytes = d->totalCachedSize;
  stats.cacheLimit = d->cacheLimit;
  stats.normalsHeapBytes = d->normalsPool.getHeapSize();
  stats.revision = d->revision;
  stats.settled = d->windowValid && !d->jobActive && d->pendingBuilds.empty() && !d->gatherContractViolated && !d->normalsIndexDirty;
  return stats;
}

} // namespace bvh
