// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bvh_debug.h"
#include "shaders/dag_shaderVar.h"

#if DAGOR_DBGLEVEL > 0

#include "bvh_context.h"
#include "bvh_tlas_debug.h"
#include <bvh/bvh_processors.h>
#include <shaders/dag_computeShaders.h>
#include <perfMon/dag_statDrv.h>
#include <math/integer/dag_IPoint2.h>
#include <imgui/imgui.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>
#include <util/dag_console.h>
#include <util/dag_convar.h>
#include <render/denoiser.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_renderTarget.h>
#include <3d/dag_lockSbuffer.h>
#include <gui/dag_stdGuiRender.h>
#include <string.h>
#include <stdio.h>

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

namespace bvh
{

RtMemoryOverhead get_rt_memory_overhead(ContextId context_id)
{
  RtMemoryOverhead o;
  if (context_id == bvh::InvalidContextId)
    return o;

  TIME_PROFILE(bvh::get_rt_memory_overhead);
  Context::BvhObjectReadLock objectsGuard(context_id->objectsLock);
  WinAutoLock lock(context_id->tidyUpTreesLock);
  WinAutoLock lock2(context_id->tidyUpSkinsLock);

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
  int ommCount = 0;
  int64_t ommAS = 0, ommArrayData = 0, ommDescArray = 0, ommIndexBuffer = 0, ommPending = 0;
  auto addOmmSlot = [&](const Mesh::OmmSlot &slot) {
    if (slot.omm)
    {
      ommCount++;
      ommAS += slot.omm.getASSize();
    }
    ommArrayData += bs(slot.bakeResult.arrayData);
    ommDescArray += bs(slot.bakeResult.descArray);
    ommIndexBuffer += bs(slot.bakeResult.indexBuffer);
  };
  auto addOmmMesh = [&](const Mesh &mesh) {
    for (const Mesh::OmmSlot &slot : mesh.ommSlots)
      addOmmSlot(slot);
  };
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
      const int64_t src = context_id->getSourceBufferSize(mesh.geometry.heapIndex, mesh.geometry.bufferRegion);
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
      addOmmMesh(mesh);
    }
  }
  for (auto &object : context_id->impostors)
  {
    impostorBlas += as(object.second.blas);
    impostorCnt++;
    for (auto &mesh : object.second.meshes)
    {
      staticProcGeom += bs(mesh.geometry.processedVertexBuffer);
      staticSrcGeom += context_id->getSourceBufferSize(mesh.geometry.heapIndex, mesh.geometry.bufferRegion);
      addOmmMesh(mesh);
    }
  }
  for (const render::omm::PendingBake &bake : context_id->ommContext.pendingBakes)
  {
    if (bake.state == render::omm::PendingBakeState::Free)
      continue;

    ommPending += bs(bake.outOmmArrayData);
    ommPending += bs(bake.outOmmDescArray);
    ommPending += bs(bake.outOmmDescArrayHistogram);
    ommPending += bs(bake.outOmmIndexBuffer);
    ommPending += bs(bake.outOmmIndexHistogram);
    ommPending += bs(bake.outPostDispatchInfo);
    ommPending += bs(bake.readbackOmmDescArrayHistogram);
    ommPending += bs(bake.readbackOmmIndexHistogram);
    ommPending += bs(bake.readbackPostDispatchInfo);
    for (const UniqueBuf &buffer : bake.transientPoolBuffers)
      ommPending += bs(buffer);
    for (const UniqueBuf &buffer : bake.constantBuffers)
      ommPending += bs(buffer);
  }
  for (auto &[tag, b] : staticBlasByTag)
    o.add("Static shared BLAS", tag, b, note("x%d vb %dM", staticCntByTag[tag], int(staticGeomByTag[tag] >> 20)));
  o.add("Static shared BLAS", "impostor", impostorBlas, note("x%d", impostorCnt));
  for (auto &[tag, b] : treeFlagBlasByTag)
    o.add("Tree/flag rendinst BLAS", tag, b, note("x%d vb %dM", treeFlagCntByTag[tag], int(treeFlagGeomByTag[tag] >> 20)));
  o.add("Dynamic model BLAS", "dyn model", dynModelBlas, note("x%d vb %dM", dynModelCnt, int(dynMeshGeom >> 20)));

  // 2) Unique (per-instance) BLAS + their geometry.
  int64_t skinBlas = 0, skinGeom = 0;
  int skinCnt = 0, splineCnt = 0, rigenTreeCnt = 0, riExTreeCnt = 0, flagCnt = 0, statTreeCnt = 0;
  for (auto &uu : context_id->uniqueSkinBuffers)
    for (auto &u : uu.second.elems)
    {
      skinBlas += as(u.second.blas);
      skinGeom += u.second.buffer.size;
      skinCnt++;
    }
  for (auto &uu : context_id->uniqueHeliRotorBuffers)
    for (auto &u : uu.second)
    {
      skinBlas += as(u.second.blas);
      skinGeom += u.second.buffer.size;
      skinCnt++;
    }
  for (auto &uu : context_id->uniqueDeformedBuffers)
    for (auto &u : uu.second)
    {
      skinBlas += as(u.second.blas);
      skinGeom += u.second.buffer.size;
      skinCnt++;
    }
  int64_t splineBlas = 0, splineGeom = 0;
  for (auto &u : context_id->uniqueSplinegenBuffers)
  {
    splineBlas += as(u.second.blas);
    splineGeom += u.second.buffer.size;
    splineCnt++;
  }
  int64_t rigenTreeBlas = 0, rigenTreeGeom = 0;
  for (auto &lod : context_id->uniqueTreeBuffers)
    for (auto &uu : lod)
      for (auto &u : uu.second.elems)
      {
        rigenTreeBlas += as(u.second.blas);
        rigenTreeGeom += u.second.buffer.size;
        rigenTreeCnt++;
      }
  int64_t riExTreeBlas = 0, riExTreeGeom = 0;
  for (auto &lod : context_id->uniqueRiExtraTreeBuffers)
    for (auto &uu : lod)
      for (auto &u : uu.second.elems)
      {
        riExTreeBlas += as(u.second.blas);
        riExTreeGeom += u.second.buffer.size;
        riExTreeCnt++;
      }
  int64_t flagBlas = 0, flagGeom = 0;
  for (auto &uu : context_id->uniqueRiExtraFlagBuffers)
    for (auto &u : uu.second)
    {
      flagBlas += as(u.second.blas);
      flagGeom += u.second.buffer.size;
      flagCnt++;
    }
  int64_t statTreeBlas = 0, statTreeGeom = 0;
  for (auto &[id, tree] : context_id->stationaryTreeBuffers)
  {
    statTreeBlas += as(tree.blas);
    statTreeGeom += tree.buffer.size;
    statTreeCnt++;
  }
  o.add("Unique BLAS", "skin", skinBlas, note("x%d vb %dM", skinCnt, int(skinGeom >> 20)));
  o.add("Unique BLAS", "splinegen", splineBlas, note("x%d vb %dM", splineCnt, int(splineGeom >> 20)));
  o.add("Unique BLAS", "RiGen tree", rigenTreeBlas, note("x%d vb %dM", rigenTreeCnt, int(rigenTreeGeom >> 20)));
  o.add("Unique BLAS", "RiEx tree", riExTreeBlas, note("x%d vb %dM", riExTreeCnt, int(riExTreeGeom >> 20)));
  o.add("Unique BLAS", "flag", flagBlas, note("x%d vb %dM", flagCnt, int(flagGeom >> 20)));
  o.add("Unique BLAS", "stationary tree", statTreeBlas, note("x%d vb %dM", statTreeCnt, int(statTreeGeom >> 20)));

  // 3) Unique BLAS caches (recycled free pool).
  int64_t skinCache = 0, rigenTreeCache = 0, riExTreeCache = 0;
  int skinCacheCnt = 0, rigenTreeCacheCnt = 0, riExTreeCacheCnt = 0;
  for (auto &uu : context_id->freeUniqueSkinBLASes)
    for (auto &blas : uu.second.blases)
    {
      skinCache += as(blas);
      skinCacheCnt++;
    }
  for (auto &uu : context_id->freeUniqueTreeBLASes)
    for (auto &blas : uu.second.blases)
    {
      rigenTreeCache += as(blas);
      rigenTreeCacheCnt++;
    }
  for (auto &uu : context_id->freeUniqueRiExtraTreeBLASes)
    for (auto &blas : uu.second.blases)
    {
      riExTreeCache += as(blas);
      riExTreeCacheCnt++;
    }
  o.add("Unique BLAS cache", "skin", skinCache, note("x%d", skinCacheCnt));
  o.add("Unique BLAS cache", "RiGen tree", rigenTreeCache, note("x%d", rigenTreeCacheCnt));
  o.add("Unique BLAS cache", "RiEx tree", riExTreeCache, note("x%d", riExTreeCacheCnt));

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
  o.add("Geometry buffers", "unique (skin/tree/...)", skinGeom + splineGeom + rigenTreeGeom + riExTreeGeom + flagGeom + statTreeGeom);
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

  return o;
}
} // namespace bvh

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
    case bvh::DebugMode::Lod: return "LOD (RI)";
    case bvh::DebugMode::LruCollision: return "LRU collision";
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
  constexpr bvh::DebugMode lastDebugMode = bvh::DebugMode::LruCollision;
#else
  constexpr bvh::DebugMode lastDebugMode = bvh::DebugMode::Lod;
#endif
  ImGuiDagor::EnumCombo("Debug mode", bvh::DebugMode::None, lastDebugMode, debug_mode, &operator!);

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
    console::command("render.pix_capture_n_frames");

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

  ImGui::Separator();
  bvh::debug::draw_tlas_debug_imgui();

  if (debug_mode != bvh::DebugMode::None && debugTex)
    ImGuiDagor::Image(debugTex.getTexId(), d3d::get_screen_aspect_ratio());
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
    WinAutoLock treesLock(context_id->tidyUpTreesLock);
    for (int lod = 0; lod < Context::maxUniqueLods; ++lod)
    {
      for (auto &uu : context_id->uniqueTreeBuffers[lod])
        for (auto &u : uu.second.elems)
          markMeta(u.second.metaAllocId, lod);
      for (auto &uu : context_id->uniqueRiExtraTreeBuffers[lod])
        for (auto &u : uu.second.elems)
          markMeta(u.second.metaAllocId, lod);
    }
    for (auto &[id, tree] : context_id->stationaryTreeBuffers)
      markMeta(tree.metaAllocId, int((id >> 28) & 0xF));
    for (auto &[id, flagElems] : context_id->uniqueRiExtraFlagBuffers)
      for (auto &u : flagElems)
        markMeta(u.second.metaAllocId, int((id >> 28) & 0xF));
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
  static int rtr_shadowVarId = get_shader_variable_id("rtr_shadow", true);
  static int bvh_debug_intersection_count_thresholdVarId = get_shader_variable_id("bvh_debug_intersection_count_threshold", true);
  static int bvh_debug_min_tVarId = get_shader_variable_id("bvh_debug_min_t", true);
  static int bvh_debug_back_viewVarId = get_shader_variable_id("bvh_debug_back_view", true);
  static int bvh_disable_ahs_with_ommVarId = get_shader_variable_id("bvh_disable_ahs_with_omm", true);

  ShaderGlobal::set_texture(bvh_debug_target, debug_mode == DebugMode::Lit ? intermediateDebugTex.getTexId() : debugTex.getTexId());
  ShaderGlobal::set_int(bvh_debug_mode, *debug_mode - *DebugMode::Lit);
  ShaderGlobal::set_int(bvh_debug_use_atmosphere, use_atmosphere ? 1 : 0);
  ShaderGlobal::set_int(rtr_shadowVarId, 1);
  ShaderGlobal::set_float(bvh_debug_intersection_count_thresholdVarId, intersection_count_threshold);
  ShaderGlobal::set_float(bvh_debug_min_tVarId, min_t);
  ShaderGlobal::set_int(bvh_debug_back_viewVarId, show_back_view ? 1 : 0);
  ShaderGlobal::set_int(bvh_disable_ahs_with_ommVarId, disable_ahs_with_omm ? 1 : 0);

  if (debug_mode == DebugMode::Lod)
    update_lod_debug_buffer(debugged_context_id);

  d3d::set_cs_constbuffer_register_count(192);

  debugShader->dispatchThreads(ti.w, ti.h, 1);

  d3d::set_cs_constbuffer_register_count(0);

  bvh::unbind_resources();

  if (debug_mode == DebugMode::Lit)
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

#else

#include <bvh/bvh.h>

namespace bvh
{
RtMemoryOverhead get_rt_memory_overhead(ContextId) { return RtMemoryOverhead{}; }
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
