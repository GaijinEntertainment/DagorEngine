// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bvh_debug.h"
#include "shaders/dag_shaderVar.h"

// The memory report also builds in force-logs retail: its target is player-machine GPU OOM logs.
#if DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS

#include "bvh_context.h"
#include "bvh_omm.h"
#include <bvh/bvh_processors.h>
#include <perfMon/dag_statDrv.h>
#include <math/dag_mathBase.h>
#include <debug/dag_debug.h>
#include <string.h>
#include <stdio.h>

#if DAGOR_DBGLEVEL > 0
#include "bvh_tlas_debug.h"
#include <shaders/dag_computeShaders.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_color.h>
#include <imgui/imgui.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>
#include <util/dag_console.h>
#include <util/dag_convar.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_renderTarget.h>
#include <3d/dag_lockSbuffer.h>
#include <gui/dag_stdGuiRender.h>
#endif

namespace bvh
{
uint32_t get_scratch_buffers_memory_statistics();
uint32_t get_transform_buffers_memory_statistics();

extern float mip_range;
extern float mip_scale;
extern float max_water_distance;
extern float water_fade_power;
extern float max_water_depth;
extern float rtr_max_water_depth;

} // namespace bvh
namespace bvh::grass
{
void get_memory_statistics(ContextId context_id, int64_t &vb, int64_t &ib, int64_t &blas, int64_t &meta, int64_t &query);
} // namespace bvh::grass
namespace bvh::gobj
{
void get_memory_statistics(int64_t &meta, int64_t &query);
}
namespace bvh::gpugrass
{
void get_memory_statistics(ContextId context_id, int &gpuGrassCount, int64_t &gpuGrassMemory, int64_t &gpuGrassTexturesMemory);
} // namespace bvh::gpugrass
namespace bvh::smoke_tracers
{
void get_memory_statistics(int &count, int64_t &vb, int64_t &blas);
} // namespace bvh::smoke_tracers
namespace bvh::voxel_activity
{
extern bool freeze;
extern bool cull;
} // namespace bvh::voxel_activity

namespace bvh
{

namespace
{
// Hand-rolled timed acquire: the spin lock has no timed variant. Write mode, because
// worker jobs insert into the unique*/flag buffer maps under a SHARED objectsLock, so
// only exclusive ownership makes the gather safe. A self-held write lock counts as
// owned via ownsWrite, so the OOM report works from under its own write lock.
struct DAG_TS_SCOPED_CAPABILITY TryObjectWriteLock
{
  TryObjectWriteLock(WriteOwnerRwSpinLock &lock, int timeout_ms) DAG_TS_ACQUIRE(lock) :
    alreadyOwned(lock.ownsWrite()), savedLock(nullptr)
  {
    for (int i = 0; !alreadyOwned && i <= timeout_ms; sleep_msec(1), ++i)
      if (lock.tryLockWrite())
      {
        savedLock = &lock;
        break;
      }
  }
  ~TryObjectWriteLock() DAG_TS_RELEASE()
  {
    if (savedLock)
      savedLock->unlockWrite();
  }
  bool owns() const { return alreadyOwned || savedLock; }

  TryObjectWriteLock(const TryObjectWriteLock &) = delete;
  TryObjectWriteLock &operator=(const TryObjectWriteLock &) = delete;

private:
  const bool alreadyOwned;
  WriteOwnerRwSpinLock *savedLock;
};

struct TryAutoLock
{
  TryAutoLock(WinCritSec &lock, int timeout_ms) : savedLock(lock.timedLock(timeout_ms) ? &lock : nullptr) {}
  ~TryAutoLock()
  {
    if (savedLock)
      savedLock->unlock();
  }
  bool owns() const { return savedLock != nullptr; }

  TryAutoLock(const TryAutoLock &) = delete;
  TryAutoLock &operator=(const TryAutoLock &) = delete;

private:
  WinCritSec *savedLock;
};
} // namespace

static void gather_rt_memory_overhead(ContextId context_id, RtMemoryOverhead &o) DAG_TS_REQUIRES_SHARED(context_id->objectsLock)
{
  int blasCount = 0;
  int64_t blasTotalBytes = 0;
  auto as = [&](auto &a) -> int64_t {
    if (!a)
      return (int64_t)0;
    blasCount++;
    const int64_t sz = (int64_t)d3d::get_raytrace_acceleration_structure_size(a.get());
    blasTotalBytes += sz;
    return sz;
  };
  auto bs = [](auto &b) -> int64_t { return b ? (int64_t)b->getSize() : (int64_t)0; };
  // From the cache, not a per-mesh walk: idle entries outlive the meshes that linked them. The idle
  // count lags the drops queued since the last eviction pass.
  int ommCount = 0, ommEntryCount = 0, ommIdleEntryCount = 0, ommRecycledCount = 0;
  int64_t ommAS = 0, ommArrayData = 0, ommDescArray = 0, ommIndexBuffer = 0, ommPending = 0, ommIdleBytes = 0, ommRecycled = 0;
  for (const auto &cached : context_id->ommCache)
  {
    const OmmCacheEntry &entry = cached.second;
    ommEntryCount++;
    if (entry.refCount == 0)
    {
      ommIdleEntryCount++;
      ommIdleBytes += omm_entry_bytes(entry);
    }
    if (entry.omm)
    {
      ommCount++;
      ommAS += entry.omm.getASSize();
    }
    ommArrayData += bs(entry.bakeResult.arrayData);
    ommDescArray += bs(entry.bakeResult.descArray);
    ommIndexBuffer += bs(entry.bakeResult.indexBuffer);
  }
  // TLAS size only, must not feed the BLAS accumulators above.
  auto tas = [](auto &a) -> int64_t { return a ? (int64_t)d3d::get_raytrace_acceleration_structure_size(a.get()) : (int64_t)0; };

  // Bracketed, non-additive annotation (counts, free headroom, sub-splits) shown next to a byte line.
  char noteBuf[96];
  const auto note = [&](const char *fmt, auto... a) -> const char * {
    snprintf(noteBuf, sizeof(noteBuf), fmt, a...);
    return noteBuf;
  };

  // 1) Per-object mesh BLAS + RT geometry copies, split three ways by BvhType:
  //  - RI:   plain static rendinst, shared one BLAS per asset (by LOD tag).
  //  - None: static rendinst that carries trees/flags (see bvh_tools.h); its shared prototype
  //          BLAS is kept separate from plain RI, since the animated foliage itself is built as
  //          per-instance unique BLAS (counted under "Unique BLAS").
  //  - Dyn:  dynamic model (dynrend).
  eastl::unordered_map<const char *, int64_t> staticBlasByTag;
  eastl::unordered_map<const char *, int64_t> treeFlagBlasByTag;
  eastl::unordered_map<const char *, int> staticCntByTag, treeFlagCntByTag;
  eastl::unordered_map<const char *, int64_t> staticGeomByTag, treeFlagGeomByTag;
  int64_t dynModelBlas = 0, treeFlagMeshGeom = 0, dynMeshGeom = 0, impostorBlas = 0;
  int dynModelCnt = 0, impostorCnt = 0;
  // Static-mesh geometry split by buffer kind: BVH-owned source copy (full IB+VB, the dominant
  // cost), processed/transformed VB, and any-hit-shader texcoord verts.
  int64_t staticSrcGeom = 0, staticProcGeom = 0, staticAhsGeom = 0;
  auto resourceIdOf = [](uint64_t objectId) { return uint32_t(objectId >> 32); };
  auto lodIndexOf = [](uint64_t objectId) { return uint32_t((objectId >> 28) & 0xF); };

  eastl::unordered_map<uint32_t, uint32_t> coarsestLoadedLodByResource;
  for (auto &[objectId, object] : context_id->objects)
    if (object.type == BvhType::RI || object.type == BvhType::None)
    {
      const uint32_t lod = lodIndexOf(objectId);
      auto [it, inserted] = coarsestLoadedLodByResource.emplace(resourceIdOf(objectId), lod);
      if (!inserted)
        it->second = eastl::max(it->second, lod);
    }
  int64_t lastLodBlasBytes = 0;
  int lastLodBlasCount = 0;
  for (auto &object : context_id->objects)
  {
    const char *tag = object.second.tag ? object.second.tag : "untagged";
    int64_t *combinedGeom = nullptr; // null => static, split into src/proc/ahs below
    int64_t *tagGeom = nullptr;      // per-tag VB total, shown in brackets next to the BLAS line
    const int64_t blasSize = as(object.second.blas);
    switch (object.second.type)
    {
      case BvhType::Dyn:
        dynModelBlas += blasSize;
        dynModelCnt++;
        combinedGeom = &dynMeshGeom;
        break;
      case BvhType::None:
        treeFlagBlasByTag[tag] += blasSize;
        treeFlagCntByTag[tag]++;
        combinedGeom = &treeFlagMeshGeom;
        tagGeom = &treeFlagGeomByTag[tag];
        break;
      default:
        staticBlasByTag[tag] += blasSize;
        staticCntByTag[tag]++;
        tagGeom = &staticGeomByTag[tag];
        break;
    }
    if (object.second.type == BvhType::RI || object.second.type == BvhType::None)
      if (lodIndexOf(object.first) == coarsestLoadedLodByResource[resourceIdOf(object.first)])
      {
        lastLodBlasBytes += blasSize;
        if (blasSize)
          lastLodBlasCount++;
      }
    for (auto &mesh : object.second.meshes)
    {
      const int64_t proc = bs(mesh.geometry.processedVertexBuffer);
      const int64_t src = context_id->getSourceGeometryRegion(mesh.geometry.heapIndex, mesh.geometry.bufferRegion).size;
      const int64_t ahs = bs(mesh.ahsVertices);
      if (combinedGeom)
        *combinedGeom += proc + src + ahs;
      else
      {
        staticProcGeom += proc;
        staticSrcGeom += src;
        staticAhsGeom += ahs;
      }
      if (tagGeom)
        *tagGeom += proc + src + ahs;
    }
  }
  for (auto &object : context_id->impostors)
  {
    impostorBlas += as(object.second.blas);
    impostorCnt++;
    for (auto &mesh : object.second.meshes)
    {
      staticProcGeom += bs(mesh.geometry.processedVertexBuffer);
      staticSrcGeom += context_id->getSourceGeometryRegion(mesh.geometry.heapIndex, mesh.geometry.bufferRegion).size;
    }
  }
  ommPending += bs(context_id->ommContext.globalConstantBuffer);
  ommPending += bs(context_id->ommContext.localConstantBuffer);
  for (const render::omm::PendingBake &bake : context_id->ommContext.pendingBakes)
  {
    // The pool survives the slot close, so a free slot can still hold VRAM.
    ommPending += bs(bake.pool.outOmmDescArrayHistogram);
    ommPending += bs(bake.pool.outOmmIndexHistogram);
    ommPending += bs(bake.pool.outPostDispatchInfo);
    ommPending += bs(bake.pool.readbackOmmDescArrayHistogram);
    ommPending += bs(bake.pool.readbackOmmIndexHistogram);
    ommPending += bs(bake.pool.readbackPostDispatchInfo);
    for (const UniqueBuf &buffer : bake.pool.transientPoolBuffers)
      ommPending += bs(buffer);

    if (bake.state == render::omm::PendingBakeState::Free)
      continue;

    ommPending += bs(bake.outOmmArrayData);
    ommPending += bs(bake.outOmmDescArray);
    ommPending += bs(bake.outOmmIndexBuffer);
  }
  for (const render::omm::BufferRecycleStore *store :
    {&context_id->ommContext.recycledArrayDataBuffers, &context_id->ommContext.recycledOutputBuffers})
    for (uint32_t i = 0; i < store->count; ++i)
    {
      ommRecycled += bs(store->buffers[i]);
      ommRecycledCount++;
    }
  for (auto &[tag, b] : staticBlasByTag)
    o.add("Static shared BLAS", tag, b, note("x%d vb %dM", staticCntByTag[tag], int(staticGeomByTag[tag] >> 20)));
  o.add("Static shared BLAS", "impostor", impostorBlas, note("x%d", impostorCnt));
  for (auto &[tag, b] : treeFlagBlasByTag)
    o.add("Tree/flag rendinst BLAS", tag, b, note("x%d vb %dM", treeFlagCntByTag[tag], int(treeFlagGeomByTag[tag] >> 20)));
  o.add("Dynamic model BLAS", "dyn model", dynModelBlas, note("x%d vb %dM", dynModelCnt, int(dynMeshGeom >> 20)));

  // 2) Unique (per-instance) BLAS + their geometry.
  struct BlasTotal
  {
    int64_t blas = 0, geom = 0;
    int count = 0;
    void add(int64_t blas_bytes, int64_t geom_bytes)
    {
      blas += blas_bytes;
      geom += geom_bytes;
      count++;
    }
  };
  auto addUnique = [&](BlasTotal &total, auto &data) { total.add(as(data.blas), data.buffer.size); };
  auto addInstances = [&](BlasTotal &total, auto &instances) {
    for (auto &uu : instances)
      for (auto &u : uu.second.elems)
        addUnique(total, u.second);
  };
  BlasTotal skin, spline, rigenTree, riExTree, flag, statTree;
  addInstances(skin, context_id->uniqueSkinBuffers);
  for (auto *buffers : {&context_id->uniqueHeliRotorBuffers, &context_id->uniqueDeformedBuffers})
    for (auto &uu : *buffers)
      for (auto &u : uu.second)
        addUnique(skin, u.second);
  for (auto &u : context_id->uniqueSplinegenBuffers)
    addUnique(spline, u.second);
  for (int lod = 0; lod < Context::maxUniqueLods; ++lod)
  {
    addInstances(rigenTree, context_id->uniqueTreeBuffers[lod]);
    addInstances(riExTree, context_id->uniqueRiExtraTreeBuffers[lod]);
    addInstances(flag, context_id->uniqueRiExtraFlagBuffers[lod]);
  }
  for (auto &u : context_id->stationaryTreeBuffers)
    addUnique(statTree, u.second);
  o.add("Unique BLAS", "skin", skin.blas, note("x%d vb %dM", skin.count, int(skin.geom >> 20)));
  o.add("Unique BLAS", "splinegen", spline.blas, note("x%d vb %dM", spline.count, int(spline.geom >> 20)));
  o.add("Unique BLAS", "RiGen tree", rigenTree.blas, note("x%d vb %dM", rigenTree.count, int(rigenTree.geom >> 20)));
  o.add("Unique BLAS", "RiEx tree", riExTree.blas, note("x%d vb %dM", riExTree.count, int(riExTree.geom >> 20)));
  o.add("Unique BLAS", "flag", flag.blas, note("x%d vb %dM", flag.count, int(flag.geom >> 20)));
  o.add("Unique BLAS", "stationary tree", statTree.blas, note("x%d vb %dM", statTree.count, int(statTree.geom >> 20)));

  // 3) Unique BLAS caches (recycled free pool).
  auto addPool = [&](BlasTotal &total, auto &pool) {
    for (auto &uu : pool)
      for (auto &pooled : uu.second.blases)
        total.add(as(pooled.blas), 0);
  };
  BlasTotal skinCache, rigenTreeCache, riExTreeCache, flagCache;
  addPool(skinCache, context_id->freeUniqueSkinBLASes);
  addPool(rigenTreeCache, context_id->freeUniqueTreeBLASes);
  addPool(riExTreeCache, context_id->freeUniqueRiExtraTreeBLASes);
  addPool(flagCache, context_id->freeUniqueRiExtraFlagBLASes);
  o.add("Unique BLAS cache", "skin", skinCache.blas, note("x%d", skinCache.count));
  o.add("Unique BLAS cache", "RiGen tree", rigenTreeCache.blas, note("x%d", rigenTreeCache.count));
  o.add("Unique BLAS cache", "RiEx tree", riExTreeCache.blas, note("x%d", riExTreeCache.count));
  o.add("Unique BLAS cache", "flag", flagCache.blas, note("x%d", flagCache.count));

  // 4) Landscape BLAS + geometry.
  int64_t terrainBlas = 0, terrainGeom = 0;
  for (auto &lod : context_id->terrainLods)
    for (auto &patch : lod.patches)
    {
      terrainBlas += as(patch.blas);
      terrainGeom += bs(patch.vertices);
    }
  int64_t cableBlas = 0;
  for (auto &blas : context_id->cableBLASes)
    cableBlas += as(blas);
  const int64_t cableVB = bs(context_id->cableVertices), cableIB = bs(context_id->cableIndices);
  int64_t waterBlas = 0, waterVB = 0;
  const int64_t waterIB =
    bs(context_id->waterFlatIb) + bs(context_id->waterHeightHighDetailIb) + bs(context_id->waterHeightLowDetailIb);
  int waterCnt = 0;
  for (auto &patch : context_id->water_patches)
  {
    waterBlas += as(patch.blas);
    waterVB += bs(patch.vertexBuffer);
    waterCnt += patch.instances.size();
  }
  const int64_t cableGeom = cableVB + cableIB;
  const int64_t waterGeom = waterVB + waterIB;
  int64_t grassVB = 0, grassIB = 0, grassBlas = 0, grassMeta = 0, grassQuery = 0;
  bvh::grass::get_memory_statistics(context_id, grassVB, grassIB, grassBlas, grassMeta, grassQuery);
  int smokeCount = 0;
  int64_t smokeVB = 0, smokeBlas = 0;
  bvh::smoke_tracers::get_memory_statistics(smokeCount, smokeVB, smokeBlas);
  o.add("Landscape BLAS", "terrain", terrainBlas, note("vb %dM", int(terrainGeom >> 20)));
  o.add("Landscape BLAS", "water", waterBlas, note("x%d vb %dM", waterCnt, int(waterGeom >> 20)));
  o.add("Landscape BLAS", "cable", cableBlas, note("vb %dM", int(cableGeom >> 20)));
  o.add("Landscape BLAS", "grass", grassBlas, note("vb %dM", int((grassVB + grassIB) >> 20)));
  o.add("Landscape BLAS", "smoke tracer", smokeBlas, note("x%d vb %dM", smokeCount, int(smokeVB >> 20)));
  const auto lruCollisionStats = bvh::get_lru_collision_stats(context_id);
  o.add("Landscape BLAS", "lru collision", int64_t(lruCollisionStats.cachedBytes),
    note("x%d limit %dM normals %dM", int(lruCollisionStats.builtModels), int(lruCollisionStats.cacheLimit >> 20),
      int(lruCollisionStats.normalsHeapBytes >> 20)));

  // 5) Opacity micromaps.
  o.add("Opacity micromaps", "AS", ommAS, note("x%d", ommCount));
  o.add("Opacity micromaps", "array data", ommArrayData);
  o.add("Opacity micromaps", "desc array", ommDescArray);
  o.add("Opacity micromaps", "index buffer", ommIndexBuffer);
  o.add("Opacity micromaps", "pending bake", ommPending);
  o.add("Opacity micromaps", "recycle stores", ommRecycled, note("x%d", ommRecycledCount));
  // A subset of the OMM buffer items, thus this item adds no bytes of its own.
  o.add("Opacity micromaps", "of which cache idle", 0,
    note("%dK in x%d idle of x%d entries", int(ommIdleBytes >> 10), ommIdleEntryCount, ommEntryCount));

  // 6) TLAS.
  o.add("TLAS", "main", tas(context_id->tlasMain));
  o.add("TLAS", "terrain", tas(context_id->tlasTerrain));
  o.add("TLAS", "particles", tas(context_id->tlasParticles));
  o.add("TLAS", "lru collision", tas(context_id->tlasLruCollision));
  o.add("TLAS", "upload",
    context_id->tlasUploadMain.totalSize() + context_id->tlasUploadTerrain.totalSize() + bs(context_id->tlasUploadParticles) +
      context_id->tlasUploadLruCollision.totalSize());

  // 7) Geometry buffers (RT-owned VB/IB).
  int64_t dynamicVB = 0, dynamicVBFree = 0;
  for (auto &[allocator, _] : context_id->processBufferAllocator)
  {
    dynamicVB += allocator.getHeapSize();
    dynamicVBFree += allocator.getHeapSize() - allocator.allocated();
  }
  o.add("Geometry buffers", "static mesh: source copy", staticSrcGeom);
  o.add("Geometry buffers", "static mesh: processed VB", staticProcGeom);
  o.add("Geometry buffers", "static mesh: AHS verts", staticAhsGeom);
  o.add("Geometry buffers", "tree/flag rendinst mesh", treeFlagMeshGeom);
  o.add("Geometry buffers", "dynamic model mesh", dynMeshGeom);
  o.add("Geometry buffers", "unique (skin/tree/...)",
    skin.geom + spline.geom + rigenTree.geom + riExTree.geom + flag.geom + statTree.geom);
  o.add("Geometry buffers", "landscape", terrainGeom + waterGeom + cableGeom + grassVB + grassIB + smokeVB,
    note("terr %dK grass %d/%dK cbl %d/%dK wtr %d/%dK", int(terrainGeom >> 10), int(grassVB >> 10), int(grassIB >> 10),
      int(cableVB >> 10), int(cableIB >> 10), int(waterVB >> 10), int(waterIB >> 10)));
  o.add("Geometry buffers", "dynamic VB allocator", dynamicVB, note("free %dK", int(dynamicVBFree >> 10)));

  // 8) Per-instance & transform.
  o.add("Per-instance & transform", "per-instance data", context_id->perInstanceData.totalSize());
  o.add("Per-instance & transform", "transform", bvh::get_transform_buffers_memory_statistics());

  // 9) Scratch.
  o.add("Scratch", "build/refit", bvh::get_scratch_buffers_memory_statistics());

  // 10) Meta & bookkeeping.
  o.add("Meta & bookkeeping", "mesh meta", context_id->meshMeta.totalSize());
  int64_t gobjMeta = 0, gobjQuery = 0;
  bvh::gobj::get_memory_statistics(gobjMeta, gobjQuery);
  o.add("Meta & bookkeeping", "GPU obj meta/query", gobjMeta + gobjQuery,
    note("meta %dK query %dK", int(gobjMeta >> 10), int(gobjQuery >> 10)));
  o.add("Meta & bookkeeping", "grass meta/query", grassMeta + grassQuery,
    note("meta %dK query %dK", int(grassMeta >> 10), int(grassQuery >> 10)));
  int gpuGrassCount = 0;
  int64_t gpuGrassMem = 0, gpuGrassTex = 0;
  bvh::gpugrass::get_memory_statistics(context_id, gpuGrassCount, gpuGrassMem, gpuGrassTex);
  o.add("Meta & bookkeeping", "GPU grass", gpuGrassMem + gpuGrassTex,
    note("x%d mem %dK tex %dK", gpuGrassCount, int(gpuGrassMem >> 10), int(gpuGrassTex >> 10)));
  int64_t compaction = 0;
  int compactionCnt = 0;
  for (auto &c : context_id->blasCompactions)
    if (c.has_value())
    {
      compaction += as(c->compactedBlas);
      compactionCnt++;
    }
  o.add("Meta & bookkeeping", "compaction", compaction, note("x%d active", compactionCnt));
  o.add("Meta & bookkeeping", "compaction size buffer", bs(context_id->compactedSizeBuffer));
  int deathRowCount = 0;
  int64_t deathRowSize = 0;
  context_id->getDeathRowStats(deathRowCount, deathRowSize);
  o.add("Meta & bookkeeping", "death row", deathRowSize, note("x%d", deathRowCount));
  int64_t idxProc = 0;
  int idxProcCnt = 0;
  auto &ip = ProcessorInstances::getIndexProcessor();
  for (auto &buffer : ip.outputs)
    if (buffer)
    {
      idxProc += buffer->getSize();
      idxProcCnt++;
    }
  o.add("Meta & bookkeeping", "index processor", idxProc, note("x%d", idxProcCnt));
  o.add("Meta & bookkeeping", "atmosphere LUT", context_id->atmosphereTexture ? context_id->atmosphereTexture->getSize() : 0);

  o.blasTotalBytes = blasTotalBytes;
  o.blasCount = blasCount;
  o.lastLodBlasBytes = lastLodBlasBytes;
  o.lastLodBlasCount = lastLodBlasCount;
}

RtMemoryOverhead get_rt_memory_overhead(ContextId context_id)
{
  RtMemoryOverhead o;
  if (context_id == bvh::InvalidContextId)
    return o;

  TIME_PROFILE(bvh::get_rt_memory_overhead);
  Context::BvhObjectReadLock objectsGuard(context_id->objectsLock);
  WinAutoLock riGuard(context_id->tidyUpRendinstsLock);
  WinAutoLock skinsGuard(context_id->tidyUpSkinsLock);

  gather_rt_memory_overhead(context_id, o);
  return o;
}

static void log_gathered_rt_memory_overhead(const RtMemoryOverhead &overhead)
{
  auto mb = [](int64_t v) { return int(v == 0 ? 0 : eastl::max((v + 1024 * 1024 - 1) / (1024 * 1024), (int64_t)1)); };
  logdbg("BVH RT memory overhead (real, RT-only)");
  overhead.forEachCategory([](const eastl::string &) {},
    [&](const RtMemoryOverhead::Item &it) {
      if (it.note.empty())
        logdbg("    %s / %s: %d MB", it.category.c_str(), it.sub.c_str(), mb(it.bytes));
      else
        logdbg("    %s / %s: %d MB  [%s]", it.category.c_str(), it.sub.c_str(), mb(it.bytes), it.note.c_str());
    },
    [&](const eastl::string &cat, int64_t sum) { logdbg("  = %s: %d MB", cat.c_str(), mb(sum)); });
  logdbg("-------------------------");
  logdbg("RT overhead total: %d MB", mb(overhead.total));
  logdbg("BLAS total: %d MB  (x%d)", mb(overhead.blasTotalBytes), overhead.blasCount);
  logdbg("Last-LOD BLAS (streaming floor): %d MB  (x%d)", mb(overhead.lastLodBlasBytes), overhead.lastLodBlasCount);
  logdbg("-------------------------");
}

void log_rt_memory_overhead(ContextId context_id)
{
  if (context_id == bvh::InvalidContextId)
    return;
  log_gathered_rt_memory_overhead(get_rt_memory_overhead(context_id));
}

void try_log_rt_memory_overhead(ContextId context_id)
{
  if (context_id == bvh::InvalidContextId)
    return;

  RtMemoryOverhead overhead;
  {
    TIME_PROFILE(bvh::try_gather_rt_memory_overhead);
    // One deadline shared by all acquisitions: the caller stalls the failing allocation's
    // thread, and closeBVH and device reset wait on it. At 0ms budget each guard still makes
    // one non-blocking attempt, so uncontended locks succeed and contended ones skip.
    constexpr int lockWaitMs = 100;
    const int deadlineMs = get_time_msec() + lockWaitMs;
    const auto remaining = [&] { return eastl::max(deadlineMs - get_time_msec(), 0); };
    TryObjectWriteLock objectsGuard(context_id->objectsLock, remaining());
    TryAutoLock riGuard(context_id->tidyUpRendinstsLock, remaining());
    TryAutoLock skinsGuard(context_id->tidyUpSkinsLock, remaining());
    TryAutoLock processBuffersGuard(context_id->processBufferAllocatorLock, remaining());
    // Holders mutate on the CPU, so reading without the locks is not safe; losing the report is.
    if (!objectsGuard.owns() || !riGuard.owns() || !skinsGuard.owns() || !processBuffersGuard.owns())
    {
      logdbg("BVH RT memory overhead: locks are held by other threads, skipping the report.");
      return;
    }
    gather_rt_memory_overhead(context_id, overhead);
  }
  log_gathered_rt_memory_overhead(overhead);
}
} // namespace bvh

#if DAGOR_DBGLEVEL > 0

static eastl::unordered_set<bvh::ContextId> context_ids;
static bvh::ContextId debugged_context_id = bvh::InvalidContextId;

static bvh::DebugMode debug_mode = bvh::DebugMode::Unknown;

static UniqueTex debugTex;
static UniqueTex intermediateDebugTex;
static eastl::unique_ptr<ComputeShaderElement> debugShader;
static eastl::unique_ptr<ComputeShaderElement> postfxShader;

static int last_available_width = 1;
static int target_width = 1;
static int resolution_change_cooldown = 0;

static bool do_super_sampling = true;
static bool use_atmosphere = true;
static bool show_back_view = false;
static bool disable_ahs_with_omm = false;
static bool preview_open = true;
static float camera_yaw_offset = 0;
static float camera_pitch_offset = 0;

static UniqueBuf lod_by_meta_buf;
static eastl::vector<uint32_t> lod_by_meta_cpu;

bool bvh_ri_extra_range_enable = false;
float bvh_ri_extra_range = 100;

bool bvh_ri_gen_range_enable = false;
float bvh_ri_gen_range = 100;

bool bvh_dyn_range_enable = false;
float bvh_dyn_range = 100;

bool bvh_gpuobject_enable = true;

bool bvh_grass_enable = true;

bool bvh_particles_enable = true;

bool bvh_cables_enable = true;


bool bvh_splinegen_enable = true;

bool bvh_tracers_enable = true;

float intersection_count_threshold = 16.f;

extern int bvh_terrain_lod_count;
extern bool bvh_terrain_lock;

inline const char *operator!(bvh::DebugMode mode)
{
  switch (mode)
  {
    case bvh::DebugMode::None: return "None";
    case bvh::DebugMode::Lit: return "Lit";
    case bvh::DebugMode::DiffuseColor: return "Diffuse color";
    case bvh::DebugMode::Normal: return "Normal";
    case bvh::DebugMode::Texcoord: return "Texcoord";
    case bvh::DebugMode::SecTexcoord: return "SecTexcoord";
    case bvh::DebugMode::CamoTexcoord: return "CamoTexcoord";
    case bvh::DebugMode::VertexColor: return "Vertex color";
    case bvh::DebugMode::GI: return "GI";
    case bvh::DebugMode::Paint: return "Paint";
    case bvh::DebugMode::IntersectionCount: return "Intersection count";
    case bvh::DebugMode::Instances: return "Instances";
    case bvh::DebugMode::NaN: return "NaN";
    case bvh::DebugMode::Emission: return "Emission";
    case bvh::DebugMode::Lod: return "LOD (RI)";
    case bvh::DebugMode::LruCollision: return "LRU collision";
    case bvh::DebugMode::VoxelActivity: return "Voxel activity";
    default: return "Unknown";
  }
}

static void imguiWindow()
{
  if (debugged_context_id == bvh::InvalidContextId)
    return;

  if (debug_mode == bvh::DebugMode::Unknown)
  {
    debug_mode = debugged_context_id->name == "GI" ? bvh::DebugMode::GI : bvh::DebugMode::Lit;
#if _TARGET_PC_WIN || _TARGET_PC_LINUX
    // a collision only context keeps an empty main TLAS: its own view is the
    // only one with content, every other mode starts on a black image
    if (debugged_context_id->hasAny(bvh::Features::LruCollision) &&
        !debugged_context_id->hasAny(~static_cast<uint32_t>(bvh::Features::LruCollision)))
      debug_mode = bvh::DebugMode::LruCollision;
#endif
  }

  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
  if (ImGui::BeginCombo("##debugged_context_id", debugged_context_id->name.data(), 0))
  {
    for (auto &context_id : context_ids)
      if (ImGui::Selectable(context_id->name.data(), debugged_context_id == context_id))
      {
        // recompute the per context default; the explicit mode choice resets
        // with it, which beats keeping a mode the new context cannot show
        if (debugged_context_id != context_id)
          debug_mode = bvh::DebugMode::Unknown;
        debugged_context_id = context_id;
      }
    ImGui::EndCombo();
  }

  if (ImGui::CollapsingHeader("Memory statistics"))
  {
    auto overhead = bvh::get_rt_memory_overhead(debugged_context_id);
    auto fmtMb = [](int64_t v) { return double(v) / (1024.0 * 1024.0); };

    overhead.forEachCategory(
      [](const eastl::string &category) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%s", category.c_str());
      },
      [&](const bvh::RtMemoryOverhead::Item &item) {
        if (item.note.empty())
          ImGui::Text("    %s: %.2f MB", item.sub.c_str(), fmtMb(item.bytes));
        else
          ImGui::Text("    %s: %.2f MB  [%s]", item.sub.c_str(), fmtMb(item.bytes), item.note.c_str());
      },
      [&](const eastl::string &category, int64_t sum) { ImGui::Text("  %s subtotal: %.2f MB", category.c_str(), fmtMb(sum)); });
    ImGui::Separator();
    ImGui::Text("RT-only total: %.2f MB", fmtMb(overhead.total));
    ImGui::Text("BLAS total: %.2f MB  (x%d)", fmtMb(overhead.blasTotalBytes), overhead.blasCount);
    ImGui::Text("Last-LOD BLAS (streaming floor): %.2f MB  (x%d)", fmtMb(overhead.lastLodBlasBytes), overhead.lastLodBlasCount);
  }

  ImGui::Text("riGen index type per frame: %d", debugged_context_id->riGenIndexTypePerFrame);
  ImGui::Text("riGen process time: %dus", debugged_context_id->lastRiGenProcessTimeUs);

  ImGui::Checkbox("Enable riExtra range", &bvh_ri_extra_range_enable);
  if (bvh_ri_extra_range_enable)
    ImGui::SliderFloat("riExtra range", &bvh_ri_extra_range, 0, 200);

  ImGui::Checkbox("Enable riGen range", &bvh_ri_gen_range_enable);
  if (bvh_ri_gen_range_enable)
    ImGui::SliderFloat("riGen range", &bvh_ri_gen_range, 0, 200);

  ImGui::Checkbox("Enable dynrend range", &bvh_dyn_range_enable);
  if (bvh_dyn_range_enable)
    ImGui::SliderFloat("Dynrend range", &bvh_dyn_range, 0, 200);

  ImGui::Separator();

  ImGui::SliderFloat("Mip range", &bvh::mip_range, 10, 2000);
  ImGui::SliderFloat("Mip scale", &bvh::mip_scale, 1, 20);

  ImGui::Separator();

  ImGui::Checkbox("Enable GPU objects", &bvh_gpuobject_enable);
  ImGui::Checkbox("Enable grass", &bvh_grass_enable);
  ImGui::Checkbox("Enable particles", &bvh_particles_enable);
  ImGui::Checkbox("Enable cables", &bvh_cables_enable);
  ImGui::Checkbox("Enable splinegen", &bvh_splinegen_enable);
  ImGui::Checkbox("Enable tracers", &bvh_tracers_enable);


  ImGui::Separator();

  ImGui::Checkbox("Lock terrain", &bvh_terrain_lock);
  ImGui::SliderInt("Terrain lods", &bvh_terrain_lod_count, 1, 6);

  ImGui::Separator();

  ImGui::SliderFloat("Max water distance", &bvh::max_water_distance, 0.1, 50);
  ImGui::SliderFloat("Water fade power", &bvh::water_fade_power, 0, 5);
  ImGui::SliderFloat("Max water depth", &bvh::max_water_depth, 0, 10);
  ImGui::SliderFloat("RTR max water depth", &bvh::rtr_max_water_depth, 0, 5);

  ImGui::Separator();

  // the LRU collision view needs the pc-only inline ray query path in bvh_debug.dshl
#if _TARGET_PC_WIN || _TARGET_PC_LINUX
  constexpr bvh::DebugMode lastDebugMode = bvh::DebugMode::VoxelActivity;
#else
  constexpr bvh::DebugMode lastDebugMode = bvh::DebugMode::Lod;
#endif
  ImGuiDagor::EnumCombo("Debug mode", bvh::DebugMode::None, lastDebugMode, debug_mode, &operator!);

  if (const auto &va = debugged_context_id->voxelActivity; va.activeValue > 0)
  {
    ImGui::Text("Voxel activity: %dx%dx%d voxels of %.1fm, active value %d, origin (%d, %d, %d)", va.dims.x, va.dims.y, va.dims.z,
      va.voxelSize, va.activeValue, va.originVoxel.x, va.originVoxel.y, va.originVoxel.z);
    ImGui::Checkbox("Freeze voxel activity", &bvh::voxel_activity::freeze);
    if (bvh::voxel_activity::freeze)
    {
      ImGui::SameLine();
      ImGui::TextDisabled("(no decay, marks or scrolling)");
    }
    ImGui::Checkbox("Cull placement in dead voxels", &bvh::voxel_activity::cull);
    if (!bvh::voxel_activity::cull)
      ImGui::TextDisabled("Placement culling: off, everything is placed");
    else if (!va.cpu.valid)
      ImGui::TextDisabled("Placement culling: waiting for the first readback");
    else
    {
      const uint32_t considered = va.statRiGenConsidered + va.statRiExConsidered + va.statDynConsidered;
      const uint32_t culled = va.statRiGenCulled + va.statRiExCulled + va.statDynCulled;
      ImGui::Text("Placement: %u of %u instances in the TLAS (%.1f%% culled)", considered - culled, considered,
        considered ? 100.f * culled / considered : 0.f);
      ImGui::Text("  riGen/impostor: %u of %u, riEx: %u of %u, dyn: %u of %u", va.statRiGenConsidered - va.statRiGenCulled,
        va.statRiGenConsidered, va.statRiExConsidered - va.statRiExCulled, va.statRiExConsidered,
        va.statDynConsidered - va.statDynCulled, va.statDynConsidered);
    }
  }

  ImGui::Separator();
  bvh::debug::draw_tlas_debug_imgui();

  preview_open = ImGui::CollapsingHeader("Preview", ImGuiTreeNodeFlags_DefaultOpen);
  if (preview_open)
  {
    ImGui::Checkbox("Super sampling", &do_super_sampling);
    ImGui::SameLine();
    ImGui::Checkbox("Use atmosphere", &use_atmosphere);
    ImGui::SameLine();
    ImGui::Checkbox("Back view", &show_back_view);
    if (d3d::get_driver_desc().caps.hasRayTraceOpacityMicroMapTriangleArrays ||
        d3d::get_driver_desc().caps.hasNvidiaRayTraceOpacityMicroMapTriangleArrays)
    {
      ImGui::SameLine();
      ImGui::Checkbox("OMM for AHS", &disable_ahs_with_omm);
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::Button("Make capture"))
      console::command("render.capture");

    int availableWidth = max(ImGui::GetContentRegionAvail().x * (do_super_sampling ? 2 : 1), 10.0f);

    if (availableWidth != last_available_width)
      resolution_change_cooldown = debugTex ? 50 : 0;

    if (resolution_change_cooldown > 0)
      resolution_change_cooldown--;
    else
      target_width = availableWidth;

    last_available_width = availableWidth;

    if (debug_mode == bvh::DebugMode::IntersectionCount)
    {
      ImGui::Separator();
      ImGui::SliderFloat("Intersection count threshold", &intersection_count_threshold, 0.f, 256.f);
    }

    if (debug_mode != bvh::DebugMode::None && debugTex)
    {
      ImGui::TextDisabled("Left drag rotates the view, double click resets it. Yaw: %.0f deg, pitch: %.0f deg",
        RadToDeg(camera_yaw_offset), RadToDeg(camera_pitch_offset));

      const float aspect = d3d::get_screen_aspect_ratio();
      const int imageWidth = max(int(ImGui::GetContentRegionAvail().x), 1);
      const int imageHeight = max(int(imageWidth / aspect), 1);
      const ImVec2 imagePos = ImGui::GetCursorScreenPos();

      // an invisible button over the image captures the drag, so it does not move the window
      ImGui::InvisibleButton("bvh_preview_rotate", ImVec2(imageWidth, imageHeight));
      if (ImGui::IsItemActive())
      {
        constexpr float rotateSpeed = 0.005f; // radians per pixel
        camera_yaw_offset += ImGui::GetIO().MouseDelta.x * rotateSpeed;
        camera_pitch_offset += ImGui::GetIO().MouseDelta.y * rotateSpeed;
      }
      if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        camera_yaw_offset = camera_pitch_offset = 0;

      // re-clamped every frame, base pitch included: at the poles the yaw axis is
      // parallel to the view and the control degenerates into roll. A zero offset
      // is exempt: an untouched preview must match the base view exactly
      if (camera_pitch_offset != 0)
      {
        static int viewVecLTVarId = get_shader_variable_id("view_vecLT", true);
        static int viewVecRTVarId = get_shader_variable_id("view_vecRT", true);
        static int viewVecLBVarId = get_shader_variable_id("view_vecLB", true);
        static int viewVecRBVarId = get_shader_variable_id("view_vecRB", true);
        const Color4 forward = ShaderGlobal::get_float4(viewVecLTVarId) + ShaderGlobal::get_float4(viewVecRTVarId) +
                               ShaderGlobal::get_float4(viewVecLBVarId) + ShaderGlobal::get_float4(viewVecRBVarId);
        const float len = sqrtf(forward.r * forward.r + forward.g * forward.g + forward.b * forward.b);
        if (len > 1e-6f)
        {
          constexpr float pitchLimit = HALFPI - 0.02f;
          const float basePitch = asinf(clamp(forward.g / len, -1.f, 1.f)); // positive looks up
          // a positive offset pitches down, so the total pitch is basePitch - offset
          camera_pitch_offset = clamp(camera_pitch_offset, basePitch - pitchLimit, basePitch + pitchLimit);
        }
      }

      ImGui::SetCursorScreenPos(imagePos);
      ImGuiDagor::Image(debugTex.getTexId(), imageWidth, imageHeight);
    }
  }
}

REGISTER_IMGUI_WINDOW("Render", "BVH", imguiWindow);

namespace bvh::debug
{

inline int operator*(bvh::DebugMode mode) { return static_cast<int>(mode); }

void init(ContextId id)
{
  context_ids.insert(id);
  if (context_ids.size() == 1)
    debugged_context_id = id;
}

void teardown(ContextId id)
{
  context_ids.erase(id);

  if (debugged_context_id == id)
  {
    if (context_ids.empty())
      debugged_context_id = bvh::InvalidContextId;
    else
      debugged_context_id = *context_ids.begin();
    // the replacement context recomputes its own default, same as the combo
    debug_mode = bvh::DebugMode::Unknown;
  }
}

void teardown()
{
  debugShader.reset();
  postfxShader.reset();
  debugTex.close();
  intermediateDebugTex.close();
  lod_by_meta_buf.close();
}

// Builds the meta-index -> (RI LOD + 1) table the LOD debug view samples, so the LOD never has to
// live in the production BVHMeta. The TLAS instanceID is the meta slot, which is the table key.
// Static RI carries the LOD in the object tag ("ri_lod0".."ri_lod4+"); trees/flags use per-instance
// unique BLAS with their own meta, where the LOD is the per-lod array index or meshId bits 28-31.
static void update_lod_debug_buffer(ContextId context_id)
{
  int metaCount;
  {
    OSSpinlockScopedLock metaGuard(context_id->meshMetaAllocatorLock);
    metaCount = context_id->meshMetaAllocator.size();
  }
  if (metaCount <= 0)
    return;

  if (!lod_by_meta_buf || (int)lod_by_meta_buf->getNumElements() < metaCount)
    lod_by_meta_buf = dag::buffers::create_one_frame_sr_structured(sizeof(uint32_t), metaCount, "bvh_debug_lod_by_meta", RESTAG_BVH);
  if (!lod_by_meta_buf)
    return;

  lod_by_meta_cpu.assign(metaCount, 0);
  auto markMeta = [&](MeshMetaAllocator::AllocId allocId, int lod) {
    const int base = MeshMetaAllocator::decode(allocId);
    if (base >= 0 && base < metaCount)
      lod_by_meta_cpu[base] = uint32_t(lod + 1); // +1 so 0 stays the "not a rendinst" sentinel
  };
  {
    Context::BvhObjectReadLock objectsGuard(context_id->objectsLock);
    for (auto &[id, object] : context_id->objects)
    {
      if (!object.tag || strncmp(object.tag, "ri_lod", 6) != 0)
        continue;
      const int lod = object.tag[6] - '0'; // "ri_lod4+" maps to lod 4
      const int base = MeshMetaAllocator::decode(object.metaAllocId);
      for (int k = 0, cnt = (int)object.meshes.size(); base >= 0 && k < cnt && base + k < metaCount; ++k)
        lod_by_meta_cpu[base + k] = uint32_t(lod + 1);
    }
  }
  {
    WinAutoLock riLock(context_id->tidyUpRendinstsLock);
    for (int lod = 0; lod < Context::maxUniqueLods; ++lod)
    {
      for (auto &uu : context_id->uniqueTreeBuffers[lod])
        for (auto &u : uu.second.elems)
          markMeta(u.second.metaAllocId, lod);
      for (auto &uu : context_id->uniqueRiExtraTreeBuffers[lod])
        for (auto &u : uu.second.elems)
          markMeta(u.second.metaAllocId, lod);
      for (auto &uu : context_id->uniqueRiExtraFlagBuffers[lod])
        for (auto &u : uu.second.elems)
          markMeta(u.second.metaAllocId, lod);
    }
    for (auto &[id, tree] : context_id->stationaryTreeBuffers)
      markMeta(tree.metaAllocId, int((id >> 28) & 0xF));
  }

  if (auto upload = lock_sbuffer<uint32_t>(lod_by_meta_buf.getBuf(), 0, metaCount, VBLOCK_WRITEONLY | VBLOCK_DISCARD))
    memcpy(upload.get(), lod_by_meta_cpu.data(), metaCount * sizeof(uint32_t));

  static int bvh_debug_lod_by_metaVarId = get_shader_variable_id("bvh_debug_lod_by_meta");
  ShaderGlobal::set_buffer(bvh_debug_lod_by_metaVarId, lod_by_meta_buf.getBufId());
}

void render_debug_context(ContextId context_id, float min_t)
{
  if (context_id != debugged_context_id)
    return;

  if (!debugged_context_id->tlasMain)
    return;

  if (debug_mode == DebugMode::Unknown || debug_mode == DebugMode::None)
    return;

  if (imgui_get_state() == ImGuiState::OFF)
    return;

  if (!preview_open)
    return;

  TIME_D3D_PROFILE(bvh_debug);

  if (debugTex)
  {
    TextureInfo ti;
    debugTex->getinfo(ti);
    if (ti.w != target_width)
    {
      debugTex.close();
      intermediateDebugTex.close();
    }
  }

  if (!debugShader)
    debugShader.reset(new_compute_shader("bvh_debug"));

  if (!postfxShader)
    postfxShader.reset(new_compute_shader("bvh_debug_postfx"));

  auto createTargetTex = [](const char *name) {
    float aspect = d3d::get_screen_aspect_ratio();
    UniqueTex tex = dag::create_tex(nullptr, target_width, max(int(target_width / aspect), 1), TEXCF_UNORDERED | TEXFMT_A16B16G16R16F,
      1, name, RESTAG_BVH);
    return tex;
  };

  if (!debugTex)
    debugTex = createTargetTex("bvh_debug_tex");

  if (!intermediateDebugTex)
    intermediateDebugTex = createTargetTex("bvh_intermediate_tex");

  bvh::bind_resources(debugged_context_id, target_width);

  TextureInfo ti;
  debugTex->getinfo(ti);

  static int bvh_debug_target = ::get_shader_variable_id("bvh_debug_target");
  static int bvh_postx_source = ::get_shader_variable_id("bvh_postfx_source");
  static int bvh_debug_mode = ::get_shader_variable_id("bvh_debug_mode");
  static int bvh_debug_use_atmosphere = ::get_shader_variable_id("bvh_debug_use_atmosphere");
  static int bvh_debug_intersection_count_thresholdVarId = get_shader_variable_id("bvh_debug_intersection_count_threshold", true);
  static int bvh_debug_min_tVarId = get_shader_variable_id("bvh_debug_min_t", true);
  static int bvh_debug_back_viewVarId = get_shader_variable_id("bvh_debug_back_view", true);
  static int bvh_debug_view_rotationVarId = get_shader_variable_id("bvh_debug_view_rotation", true);
  static int bvh_disable_ahs_with_ommVarId = get_shader_variable_id("bvh_disable_ahs_with_omm", true);
  static int bvh_debug_voxel_activity_active_valueVarId = get_shader_variable_id("bvh_debug_voxel_activity_active_value", true);

  const bool needsTonemap = debug_mode == DebugMode::Lit || debug_mode == DebugMode::Emission;

  ShaderGlobal::set_texture(bvh_debug_target, needsTonemap ? intermediateDebugTex.getTexId() : debugTex.getTexId());
  ShaderGlobal::set_int(bvh_debug_mode, *debug_mode - *DebugMode::Lit);
  ShaderGlobal::set_int(bvh_debug_use_atmosphere, use_atmosphere ? 1 : 0);
  ShaderGlobal::set_float(bvh_debug_intersection_count_thresholdVarId, intersection_count_threshold);
  ShaderGlobal::set_float(bvh_debug_min_tVarId, min_t);
  ShaderGlobal::set_int(bvh_debug_back_viewVarId, show_back_view ? 1 : 0);
  ShaderGlobal::set_float4(bvh_debug_view_rotationVarId, camera_yaw_offset, camera_pitch_offset);
  ShaderGlobal::set_int(bvh_disable_ahs_with_ommVarId, disable_ahs_with_omm ? 1 : 0);
  ShaderGlobal::set_int(bvh_debug_voxel_activity_active_valueVarId, debugged_context_id->voxelActivity.activeValue);

  if (debug_mode == DebugMode::Lod)
    update_lod_debug_buffer(debugged_context_id);

  debugShader->dispatchThreads(ti.w, ti.h, 1);

  bvh::unbind_resources();

  if (needsTonemap)
  {
    ShaderGlobal::set_texture(bvh_postx_source, intermediateDebugTex.getTexId());
    ShaderGlobal::set_texture(bvh_debug_target, debugTex.getTexId());

    postfxShader->dispatchThreads(ti.w, ti.h, 1);
  }
}

} // namespace bvh::debug

namespace bvh
{
void render_rt_mem_overlay(ContextId context_id)
{
  if (context_id == bvh::InvalidContextId)
    return;

  auto overhead = bvh::get_rt_memory_overhead(context_id);

  StdGuiRender::ScopeStarterOptional strt;
  StdGuiRender::reset_textures(); // otherwise render_box samples the bound font atlas and the panel is invisible
  StdGuiRender::set_font(0);

  int w = 0, h = 0;
  d3d::get_screen_size(w, h);

  // Small font + wrap into columns so the whole breakdown always fits regardless of line count.
  const float scale = 0.7f;
  // Glyph extents relative to the pen (goto_xy is baseline-anchored, so textTop is negative).
  // Used to place the backing panel exactly over the drawn text.
  const auto fbb = StdGuiRender::get_str_bbox("Wg", 2);
  const float textTop = fbb[0].y * scale;
  const float textBot = fbb[1].y * scale;
  const float lineH = (fbb[1].y - fbb[0].y) * scale * 1.15f;
  const float topY = h * 0.05f;
  const float bottomY = h * 0.95f;
  const float colW = w * 0.24f;
  const float startX = w * 0.012f;

  // Build the formatted lines first, so each column's panel can be sized to its actual widest
  // line (the title/total line is far wider than the per-entry lines).
  struct OverlayLine
  {
    E3DCOLOR color;
    char text[128];
  };
  eastl::vector<OverlayLine> lines;
  lines.reserve(overhead.items.size() * 2 + 8);
  auto mb = [](int64_t v) { return double(v) / (1024.0 * 1024.0); };
  char tmp[256];
  auto emit = [&](E3DCOLOR c, const char *s) {
    OverlayLine l;
    l.color = c;
    strncpy(l.text, s, sizeof(l.text) - 1);
    l.text[sizeof(l.text) - 1] = 0;
    lines.push_back(l);
  };

  _snprintf(tmp, sizeof(tmp), "RT memory overhead: %.1f MB", mb(overhead.total));
  emit(E3DCOLOR(255, 230, 120), tmp);
  _snprintf(tmp, sizeof(tmp), "BLAS total: %.1f MB  (x%d)", mb(overhead.blasTotalBytes), overhead.blasCount);
  emit(E3DCOLOR(255, 230, 120), tmp);
  _snprintf(tmp, sizeof(tmp), "Last-LOD BLAS (streaming floor): %.1f MB  (x%d)", mb(overhead.lastLodBlasBytes),
    overhead.lastLodBlasCount);
  emit(E3DCOLOR(255, 230, 120), tmp);

  overhead.forEachCategory([&](const eastl::string &category) { emit(E3DCOLOR(180, 230, 130), category.c_str()); },
    [&](const RtMemoryOverhead::Item &item) {
      if (item.note.empty())
        _snprintf(tmp, sizeof(tmp), "      %s: %.2f MB", item.sub.c_str(), mb(item.bytes));
      else
        _snprintf(tmp, sizeof(tmp), "      %s: %.2f MB  [%s]", item.sub.c_str(), mb(item.bytes), item.note.c_str());
      emit(E3DCOLOR(210, 220, 230), tmp);
    },
    [&](const eastl::string &category, int64_t sum) {
      _snprintf(tmp, sizeof(tmp), "    %s subtotal: %.2f MB", category.c_str(), mb(sum));
      emit(E3DCOLOR(150, 200, 255), tmp);
    });

  const int total = (int)lines.size();
  const int linesPerCol = eastl::max(1, int((bottomY - topY) / lineH));
  const int nCols = (total + linesPerCol - 1) / linesPerCol;

  // Translucent black backing panel per column, sized to the widest line in it. Drawn first; text
  // goes on top. Black is safe under premultiplied alpha (RGB stays 0).
  StdGuiRender::set_ablend(true);
  StdGuiRender::set_color(E3DCOLOR(0, 0, 0, 170));
  for (int c = 0; c < nCols; c++)
  {
    const int begin = c * linesPerCol;
    const int end = (c + 1) * linesPerCol < total ? (c + 1) * linesPerCol : total;
    float maxW = 0;
    for (int i = begin; i < end; i++)
      maxW = eastl::max(maxW, StdGuiRender::get_str_bbox(lines[i].text).width().x * scale);
    const float x0 = startX + c * colW - 4;
    const float yTop = topY + textTop - 3.f;                             // first line sits at topY
    const float yBot = topY + (end - begin - 1) * lineH + textBot + 3.f; // last line in this column
    StdGuiRender::render_box(x0, yTop, x0 + maxW + 12, yBot);
  }

  // Text on top.
  for (int i = 0; i < total; i++)
  {
    StdGuiRender::goto_xy(startX + (i / linesPerCol) * colW, topY + (i % linesPerCol) * lineH);
    StdGuiRender::set_color(lines[i].color);
    StdGuiRender::draw_str_scaled(scale, lines[i].text);
  }
}

} // namespace bvh

#endif // DAGOR_DBGLEVEL > 0

#else

namespace bvh
{
RtMemoryOverhead get_rt_memory_overhead(ContextId) { return RtMemoryOverhead{}; }
void log_rt_memory_overhead(ContextId) {}
void try_log_rt_memory_overhead(ContextId) {}
} // namespace bvh

#endif

#if DAGOR_DBGLEVEL <= 0

namespace bvh
{
void render_rt_mem_overlay(ContextId) {}
} // namespace bvh

namespace bvh::debug
{
void init(ContextId) {}
void teardown(ContextId) {}
void render_debug_context(ContextId, float) {}
void teardown() {}
} // namespace bvh::debug

#endif
