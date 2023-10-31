#include "hmlPlugin.h"
#include "hmlSplineObject.h"
#include "hmlSplinePoint.h"
#include "recastNavMesh.h"
#include <recast.h>
#include <recastAlloc.h>
#include <detourCommon.h>
#include <detourNavMeshBuilder.h>
#include <detourNavMesh.h>
#include <detourNavMeshQuery.h>
#include <detourTileCache.h>
#include <detourTileCacheBuilder.h>
#include <dllPluginCore/core.h>
#include <de3_genObjUtil.h>
#include <de3_splineClassData.h>
#include <de3_groundHole.h>
#include <libTools/util/makeBindump.h>
#include <perfMon/dag_cpuFreq.h>
#include <math/dag_bezierPrec.h>
#include <image/dag_tga.h>
#include <image/dag_texPixel.h>
#include <ioSys/dag_lzmaIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_memIo.h>
#include <util/dag_hierBitMap2d.h>
#include <pathFinder/pathFinder.h>
#include <pathFinder/tileCache.h>
#include <pathFinder/tileCacheUtil.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_miscApi.h>
#include <util/dag_threadPool.h>
#include <math/dag_adjpow2.h>
#include <memory/dag_MspaceAlloc.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <recastTools/recastBuildEdges.h>
#include <recastTools/recastBuildCovers.h>
#include <recastTools/recastBuildJumpLinks.h>
#include <recastTools/recastNavMeshTile.h>
#include <recastTools/recastObstacleFlags.h>
#define ZDICT_STATIC_LINKING_ONLY 1
#include <arc/zstd-1.4.5/zstd.h>
#include <arc/zstd-1.4.5/dictBuilder/zdict.h>

#include <util/dag_lookup.h>

static Tab<covers::Cover> coversDebugList;
static Tab<eastl::pair<Point3, Point3>> edgesDebugList;
static Tab<NavMeshObstacle> obstaclesDebugList;
static scene::TiledScene coversDebugScene;
static bool exportedCoversDebugListDone = false;
static bool exportedEdgesDebugListDone = false;
static bool exportedNavMeshLoadDone = false;
static bool exportedObstaclesLoadDone = false;

static Tab<MspaceAlloc *> thread_alloc;
static int nextThreadId = 0;
thread_local static int threadId = -1;

static IMemAlloc *get_thread_alloc(int idx) { return idx >= 0 && idx < thread_alloc.size() ? thread_alloc[idx] : midmem; }

static IMemAlloc *get_thread_alloc()
{
  if (threadId < 0)
    threadId = interlocked_increment(nextThreadId);
  return get_thread_alloc(threadId - 1);
}

static void *alloc_for_recast(size_t size, rcAllocHint hint)
{
  void *ptr = get_thread_alloc()->alloc(size + 16);
  if (!ptr)
    return nullptr;
  *(int *)ptr = threadId - 1;
  return (char *)ptr + 16;
}

static void free_for_recast(void *ptr)
{
  if (!ptr)
    return;
  ptr = (char *)ptr - 16;
  int idx = *(int *)(ptr);
  get_thread_alloc(idx)->free(ptr);
}

static void *alloc_for_detour(size_t size, dtAllocHint hint) { return alloc_for_recast(size, RC_ALLOC_PERM); }

static void free_for_detour(void *ptr) { return free_for_recast(ptr); }

static void setupRecastDetourAllocators()
{
  // pathfinder sets these to framemem allocator, but we'll use recast/detour
  // in a multithreaded manner, thus, we need thread-safe allocators.
  threadId = 0; // Make sure that caller of this function will always use midmem allocator.
  rcAllocSetCustom(alloc_for_recast, free_for_recast);
  dtAllocSetCustom(alloc_for_detour, free_for_detour);
}

const float MIN_BOTTOM_DEPTH = -50.f;
static const int TILECACHE_MAX_OBSTACLES = 128 * 1024;
static const int TILECACHE_AVG_LAYERS_PER_TILE = 4;  // This is averaged number of layers (i.e. number of floors) per-tile.
static const int TILECACHE_MAX_LAYERS_PER_TILE = 32; // This is maximum number of layers per-tile.
// Note that there's also RC_MAX_LAYERS in recast itself, this is the maximum number of layers per-tile library can handle.

static dtNavMesh *navMesh = NULL;
static dtTileCache *tileCache = NULL;
static pathfinder::TileCacheMeshProcess tcMeshProc;

struct NavMeshParams
{
  NavmeshExportType navmeshExportType;
  float cellSize, cellHeight;
  float traceStep;
  float agentMaxSlope, agentHeight, agentMaxClimb, agentRadius, agentClimbAfterGluingMeshes;
  float edgeMaxLen, edgeMaxError;
  float regionMinSize, regionMergeSize;
  int vertsPerPoly;
  float detailSampleDist, detailSampleMaxError;
  pathfinder::NavMeshType navMeshType;
  bool usePrefabCollision;
  int tileSize;
  float bucketSize;
  float crossingWaterDepth;
  int jlkCovExtraCells;
  bool crossObstaclesWithJumplinks;
  recastbuild::JumpLinksParams jlkParams;
  recastbuild::MergeEdgeParams mergeParams;
};

static NavMeshParams navMeshParams[MAX_NAVMESHES];
static recastbuild::CoversParams covParams;

static const char *navmeshExportTypeNames[] = {"water", "splines", "height_from_above", "geometry", "water_and_geometry", "invalid"};
NavmeshExportType navmesh_export_type_name_to_enum(const char *name)
{
  return NavmeshExportType(lup(name, navmeshExportTypeNames, int(NavmeshExportType::COUNT), int(NavmeshExportType::INVALID)));
}

const char *navmesh_export_type_to_string(NavmeshExportType type) { return navmeshExportTypeNames[int(type)]; }

/// Recast build context.
class BuildContext : public rcContext
{
  int64_t m_startTime[RC_MAX_TIMERS];
  int m_accTime[RC_MAX_TIMERS];

public:
  BuildContext() {} //-V730
  virtual ~BuildContext() {}

protected:
  virtual void doResetLog() {}
  virtual void doLog(const rcLogCategory category, const char *msg, const int /*len*/)
  {
    if (category >= RC_LOG_ERROR)
      DAEDITOR3.conError("%s", msg);
    else if (category >= RC_LOG_PROGRESS)
      debug(msg);
  }
  virtual void doResetTimers()
  {
    for (int i = 0; i < RC_MAX_TIMERS; ++i)
      m_accTime[i] = -1;
  }
  virtual void doStartTimer(const rcTimerLabel label) { m_startTime[label] = ref_time_ticks(); }
  virtual void doStopTimer(const rcTimerLabel label)
  {
    const int deltaTime = get_time_usec(m_startTime[label]);
    if (m_accTime[label] == -1)
      m_accTime[label] = deltaTime;
    else
      m_accTime[label] += deltaTime;
  }
  virtual int doGetAccumulatedTime(const rcTimerLabel label) const { return m_accTime[label]; }
};
static BuildContext global_build_ctx;

static void logLine(rcContext &ctx, rcTimerLabel label, const char *name, const double pc)
{
  const int t = ctx.getAccumulatedTime(label);
  if (t >= 0)
    ctx.log(RC_LOG_PROGRESS, "%s:\t%.2fms\t(%.1f%%)", name, t / 1000.0f, t * pc);
}

static void duLogBuildTimes(rcContext &ctx, const int totalTimeUsec)
{
  const double pc = 100.0f / totalTimeUsec;

  ctx.log(RC_LOG_PROGRESS, "Build Times");
  logLine(ctx, RC_TIMER_RASTERIZE_TRIANGLES, "- Rasterize", pc);
  logLine(ctx, RC_TIMER_BUILD_COMPACTHEIGHTFIELD, "- Build Compact", pc);
  logLine(ctx, RC_TIMER_FILTER_BORDER, "- Filter Border", pc);
  logLine(ctx, RC_TIMER_FILTER_WALKABLE, "- Filter Walkable", pc);
  logLine(ctx, RC_TIMER_ERODE_AREA, "- Erode Area", pc);
  logLine(ctx, RC_TIMER_MEDIAN_AREA, "- Median Area", pc);
  logLine(ctx, RC_TIMER_MARK_BOX_AREA, "- Mark Box Area", pc);
  logLine(ctx, RC_TIMER_MARK_CONVEXPOLY_AREA, "- Mark Convex Area", pc);
  // logLine(ctx, RC_TIMER_MARK_CYLINDER_AREA,   "- Mark Cylinder Area", pc);
  logLine(ctx, RC_TIMER_BUILD_DISTANCEFIELD, "- Build Distance Field", pc);
  logLine(ctx, RC_TIMER_BUILD_DISTANCEFIELD_DIST, "    - Distance", pc);
  logLine(ctx, RC_TIMER_BUILD_DISTANCEFIELD_BLUR, "    - Blur", pc);
  logLine(ctx, RC_TIMER_BUILD_REGIONS, "- Build Regions", pc);
  logLine(ctx, RC_TIMER_BUILD_REGIONS_WATERSHED, "    - Watershed", pc);
  logLine(ctx, RC_TIMER_BUILD_REGIONS_EXPAND, "      - Expand", pc);
  logLine(ctx, RC_TIMER_BUILD_REGIONS_FLOOD, "      - Find Basins", pc);
  logLine(ctx, RC_TIMER_BUILD_REGIONS_FILTER, "    - Filter", pc);
  // logLine(ctx, RC_TIMER_BUILD_LAYERS,         "- Build Layers", pc);
  logLine(ctx, RC_TIMER_BUILD_CONTOURS, "- Build Contours", pc);
  logLine(ctx, RC_TIMER_BUILD_CONTOURS_TRACE, "    - Trace", pc);
  logLine(ctx, RC_TIMER_BUILD_CONTOURS_SIMPLIFY, "    - Simplify", pc);
  logLine(ctx, RC_TIMER_BUILD_POLYMESH, "- Build Polymesh", pc);
  logLine(ctx, RC_TIMER_BUILD_POLYMESHDETAIL, "- Build Polymesh Detail", pc);
  logLine(ctx, RC_TIMER_MERGE_POLYMESH, "- Merge Polymeshes", pc);
  logLine(ctx, RC_TIMER_MERGE_POLYMESHDETAIL, "- Merge Polymesh Details", pc);
  ctx.log(RC_LOG_PROGRESS, "=== TOTAL:\t%.2fms", totalTimeUsec / 1000.0f);
}

static void init(const DataBlock &blk, int nav_mesh_idx)
{
  setupRecastDetourAllocators();
  NavMeshParams &nmParams = navMeshParams[nav_mesh_idx];

  nmParams.navmeshExportType = navmesh_export_type_name_to_enum(blk.getStr("navmeshExportType", "invalid"));

  nmParams.cellSize = blk.getReal("cellSize", 2.0f);
  nmParams.traceStep = blk.getReal("traceStep", blk.getReal("cellSize", 2.f));
  nmParams.cellHeight = blk.getReal("cellHeight", 0.125f);
  nmParams.agentMaxSlope = blk.getReal("agentMaxSlope", 30.0f);
  nmParams.agentHeight = blk.getReal("agentHeight", 3.0f);
  nmParams.agentMaxClimb = blk.getReal("agentMaxClimb", 1.5f);
  nmParams.agentClimbAfterGluingMeshes = blk.getReal("agentClimbAfterGluingMeshes", 0.1f);
  nmParams.agentRadius = blk.getReal("agentRadius", 2.0f);
  nmParams.edgeMaxLen = blk.getReal("edgeMaxLen", 128.0f);
  nmParams.edgeMaxError = blk.getReal("edgeMaxError", 2.0f);
  nmParams.regionMinSize = blk.getReal("regionMinSize", 9.0f);
  nmParams.regionMergeSize = blk.getReal("regionMergeSize", 100.0f);
  nmParams.vertsPerPoly = blk.getInt("vertsPerPoly", 3);
  nmParams.detailSampleDist = blk.getReal("detailSampleDist", 3.0f);
  nmParams.detailSampleMaxError = blk.getReal("detailSampleMaxError", 0.25f);
  nmParams.usePrefabCollision = blk.getBool("usePrefabCollision", true);
  nmParams.crossingWaterDepth = blk.getReal("crossingWaterDepth", 0.f);
  nmParams.crossObstaclesWithJumplinks = blk.getBool("crossObstaclesWithJumplinks", false);

  nmParams.jlkCovExtraCells = blk.getInt("jlkExtraCells", 32);
  nmParams.jlkParams.enabled = blk.getBool("jumpLinksEnabled", false);
  nmParams.jlkParams.jumpHeight = blk.getReal("jumpLinksHeight", 2.0f) * 2.f; // up + down
  const float typoDefJumpLinksLength = blk.getReal("jumpLinksLenght", 2.5f);
  nmParams.jlkParams.jumpLength = blk.getReal("jumpLinksLength", typoDefJumpLinksLength);
  nmParams.jlkParams.width = blk.getReal("jumpLinksWidth", 1.0f);
  nmParams.jlkParams.deltaHeightThreshold = blk.getReal("jumpLinksDeltaHeightTreshold", 0.5f);
  nmParams.jlkParams.complexJumpTheshold = blk.getReal("complexJumpTheshold", 0.0f);
  nmParams.jlkParams.linkDegAngle = cosf(DegToRad(blk.getReal("jumpLinksMergeAngle", 15.0f)));
  nmParams.jlkParams.linkDegDist = cosf(DegToRad(blk.getReal("jumpLinksMergeDist", 5.0f)));
  nmParams.jlkParams.agentRadius = nmParams.agentRadius;
  nmParams.jlkParams.crossObstaclesWithJumplinks = blk.getBool("crossObstaclesWithJumplinks", false);

  covParams.enabled = nav_mesh_idx == 0 && blk.getBool("coversEnabled", false);
  if (covParams.enabled)
  {
    covParams.typeGen = blk.getInt("coversTypeGen", recastbuild::COVERS_TYPEGEN_BASIC);
    covParams.maxHeight = blk.getReal("coversMaxHeight", 2.0f);
    covParams.minHeight = blk.getReal("coversMinHeight", 1.0f);
    covParams.minWidth = blk.getReal("coversWidth", 1.0f);
    covParams.minDepth = blk.getReal("coversDepth", 1.0f);
    covParams.agentMaxClimb = nmParams.agentMaxClimb;
    covParams.shootWindowHeight = blk.getReal("coversShootWindow", 0.4f);
    covParams.minCollTransparent = blk.getReal("coversTransparent", 0.8f);
    covParams.deltaHeightThreshold = blk.getReal("coversDeltaHeightTreshold", 0.2f);
    covParams.shootDepth = blk.getReal("coversShootDepth", 3.0f);
    covParams.shootHeight = blk.getReal("coversShootHeight", 1.5f);
    covParams.mergeDist = blk.getReal("coversMergeDist", 0.3f);

    // covParams.maxVisibleBox == navarea;
    covParams.openingTreshold = blk.getReal("openingTreshold", 0.9f);
    covParams.boxOffset = blk.getReal("visibleBoxOffset", 1.f);

    covParams.testPos.x = blk.getReal("coversTestX", 0.0f);
    covParams.testPos.y = blk.getReal("coversTestY", 0.0f);
    covParams.testPos.z = blk.getReal("coversTestZ", 0.0f);
  }

  auto sqr = [](float val) -> float { return val * val; };
  nmParams.mergeParams.enabled = blk.getBool("simplificationEdgeEnabled", true);
  nmParams.mergeParams.walkPrecision = blk.getPoint2("walkPrecision", Point2(nmParams.agentRadius, nmParams.agentMaxClimb));
  nmParams.mergeParams.maxExtrudeErrorSq = sqr(blk.getReal("extrudeError", 0.4f));
  nmParams.mergeParams.extrudeLimitSq = sqr(blk.getReal("extrudeLimit", 0.3f));
  nmParams.mergeParams.safeCutLimitSq = sqr(blk.getReal("safeCutLimit", 0.5f));
  nmParams.mergeParams.unsafeCutLimitSq = sqr(blk.getReal("unsafeCutLimit", 0.2f));
  nmParams.mergeParams.unsafeMaxCutSpace = blk.getReal("maxUnsafeCutSpace", 1.f);

  nmParams.navMeshType =
    (pathfinder::NavMeshType)blk.getInt("navMeshType", blk.getBool("tiled", false) ? pathfinder::NMT_TILED : pathfinder::NMT_SIMPLE);
  nmParams.tileSize = blk.getInt("tileCells", 128);
  if (nmParams.navMeshType == pathfinder::NMT_TILECACHED)
    nmParams.tileSize = rcMin(nmParams.tileSize, 255);
  nmParams.bucketSize = blk.getReal("bucketSize", 200.0f);
}

static void saveBmTga(HierBitMap2d<HierConstSizeBitMap2d<4, ConstSizeBitMap2d<5>>> &bm, const char *name)
{
  int w = bm.getW(), h = bm.getH();
  if (!w || !h)
    return;
  Tab<TexPixel8a> pix;
  pix.resize(w * h);
  mem_set_ff(pix);

  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++)
      pix[y * w + x].l = bm.get(x, y) ? 128 : 0;

  save_tga8a(name, pix.data(), w, h, elem_size(pix) * w);
}

struct NavmeshTCTile
{
  NavmeshTCTile() = default;
  NavmeshTCTile(int idx, uint32_t uncompressedOffset, uint32_t uncompressedSize) :
    idx(idx), uncompressedOffset(uncompressedOffset), uncompressedSize(uncompressedSize)
  {}

  int idx;
  uint32_t uncompressedOffset;
  uint32_t uncompressedSize;
};

struct NavmeshBucket
{
  BBox2 bbox;
  Tab<int> tileIds;
  Tab<NavmeshTCTile> tcTiles;
  uint32_t dataSize = 0;
};

struct NavmeshBucketsKeyHasher
{
  size_t operator()(const eastl::pair<int, int> &key) const
  {
    return (size_t)(((uint32_t)key.second * 16777619) ^ (uint32_t)key.first);
  }
};

template <class T>
using SamplesTab = dag::Vector<T, dag::MemPtrAllocator, false, uint64_t>;

static void save_navmesh_bucket_format(const dtNavMesh *mesh, const NavMeshParams &nav_mesh_params, dtTileCache *tc,
  dag::ConstSpan<NavMeshObstacle> obstacles, BinDumpSaveCB &cwr)
{
  ska::flat_hash_map<eastl::pair<int, int>, NavmeshBucket, NavmeshBucketsKeyHasher> buckets;

  Point2 orig(mesh->getParams()->orig[0], mesh->getParams()->orig[2]);

  SamplesTab<char> samplesBuffer;
  Tab<size_t> samplesSizes;
  const float bucketSize = nav_mesh_params.bucketSize;

  for (int i = 0; i < mesh->getMaxTiles(); ++i)
  {
    const dtMeshTile *tile = mesh->getTile(i);
    if (!tile || !tile->header || !tile->dataSize)
      continue;
    Point2 lower(tile->header->bmin[0], tile->header->bmin[2]);
    Point2 upper(tile->header->bmax[0], tile->header->bmax[2]);
    Point2 pt = lower - orig;
    NavmeshBucket &bucket = buckets[eastl::make_pair(pt.x / bucketSize, pt.y / bucketSize)];
    bucket.tileIds.push_back(i);
    bucket.bbox += lower;
    bucket.bbox += upper;
    bucket.dataSize += sizeof(dtTileRef) + sizeof(int) + tile->dataSize;
  }
  size_t resv_samples_sz = 0;
  for (int i = 0; i < tc->getTileCount(); ++i)
  {
    const dtCompressedTile *tile = tc->getTile(i);
    if (tile && tile->header && tile->dataSize)
      resv_samples_sz += (int)tile->header->width * (int)tile->header->height * 4; //+maxUncompressedSize
  }
  samplesBuffer.reserve(eastl::max<size_t>(resv_samples_sz, 2u << 20u));
  samplesSizes.reserve(tc->getTileCount());
  for (int i = 0; i < tc->getTileCount(); ++i)
  {
    const dtCompressedTile *tile = tc->getTile(i);
    if (!tile || !tile->header || !tile->dataSize)
      continue;
    BBox3 aabb = pathfinder::tilecache_calc_tile_bounds(tc->getParams(), tile->header);
    Point2 lower = Point2::xz(aabb.lim[0]);
    Point2 upper = Point2::xz(aabb.lim[1]);
    Point2 pt = lower - orig;
    NavmeshBucket &bucket = buckets[eastl::make_pair(pt.x / bucketSize, pt.y / bucketSize)];

    uint32_t maxUncompressedSize = (int)tile->header->width * (int)tile->header->height * 4;
    size_t off = samplesBuffer.size();
    samplesBuffer.resize(off + maxUncompressedSize);
    int sz = 0;
    dtStatus status = tc->getCompressor()->decompress(tile->compressed, tile->compressedSize, (uint8_t *)(samplesBuffer.data() + off),
      maxUncompressedSize, &sz);
    G_ASSERT(status == DT_SUCCESS);
    G_ASSERT(sz <= maxUncompressedSize);
    samplesBuffer.resize(off + sz);
    samplesSizes.push_back(sz);

    bucket.tcTiles.push_back(NavmeshTCTile(i, (uint32_t)off, sz));
    bucket.bbox += lower;
    bucket.bbox += upper;
  }
  debug("samplesBuffer.size=%dK capacity=%uK resv_samples_sz=%uK", samplesBuffer.size() >> 10, samplesBuffer.capacity() >> 10,
    resv_samples_sz >> 10);

  Tab<char> dictBuffer;
  // 16K dict seems to be the best option for us.
  dictBuffer.resize(16 * 1024);

  ZDICT_fastCover_params_t params;
  memset(&params, 0, sizeof(params));
  params.d = 8;
  params.steps = 4;
  /* Default to level 6 since no compression level information is available */
  params.zParams.compressionLevel = -1;

  // Unfortunately we need ALL of our tiles uncompressed in order to build nice
  // dict, currently this bumps mem usage from 14G to 17G for 2048x2048 meter navmesh.
  // m.b. not an issue right now, since level builders have enough mem, but m.b. we'll want
  // to do something about it in the future.
  size_t dictSize = ZDICT_optimizeTrainFromBuffer_fastCover(dictBuffer.data(), dictBuffer.size(), samplesBuffer.data(),
    samplesSizes.data(), samplesSizes.size(), &params);
  G_ASSERT(!ZDICT_isError(dictSize));

  ZSTD_CDict *cDict = ZSTD_createCDict(dictBuffer.data(), dictSize, -1);
  ZSTD_CCtx *cctx = ZSTD_createCCtx();
  size_t res = ZSTD_CCtx_loadDictionary(cctx, dictBuffer.data(), dictSize);
  G_ASSERT(!ZSTD_isError(res));

  // 2 upper bits is coding format, 30th bit is already used for whole zstd packing specification, we'll use
  // 31th to indicate bucket format.
  cwr.writeInt32e(0 | 0x80000000);

  // Params.
  cwr.writeRaw(mesh->getParams(), sizeof(dtNavMeshParams));
  cwr.writeRaw(tc->getParams(), sizeof(dtTileCacheParams));

  // Write obstacles resource name hashes.
  ska::flat_hash_set<uint32_t> obsResNameHashes;
  for (const NavMeshObstacle &obs : obstacles)
    obsResNameHashes.insert(obs.resNameHash);
  cwr.writeInt32e((uint32_t)obsResNameHashes.size());
  for (uint32_t h : obsResNameHashes)
    cwr.writeInt32e(h);

  // Write zstd dict.
  cwr.writeInt32e((int)dictSize);
  cwr.writeRaw(dictBuffer.data(), (int)dictSize);

  Tab<char> tmpBuffer;

  // Write per-bucket data.
  cwr.writeInt32e((uint32_t)buckets.size());
  for (const auto &it : buckets)
  {
    const NavmeshBucket &bucket = it.second;

    // Write bucket header.
    cwr.writeFloat32e(bucket.bbox.lim[0].x);
    cwr.writeFloat32e(bucket.bbox.lim[0].y);
    cwr.writeFloat32e(bucket.bbox.lim[1].x);
    cwr.writeFloat32e(bucket.bbox.lim[1].y);

    cwr.beginBlock();

    // Write tile cache data, re-compress using dict.
    cwr.writeInt32e((uint32_t)bucket.tcTiles.size());
    for (const NavmeshTCTile &tct : bucket.tcTiles)
    {
      const dtCompressedTile *tile = tc->getTile(tct.idx);
      tmpBuffer.resize(ZSTD_compressBound(tct.uncompressedSize));
      size_t newCompressedSize = ZSTD_compress_usingCDict(cctx, tmpBuffer.data(), tmpBuffer.size(),
        samplesBuffer.data() + tct.uncompressedOffset, tct.uncompressedSize, cDict);
      G_ASSERT(!ZSTD_isError(newCompressedSize));
      cwr.writeInt32e(tile->dataSize - tile->compressedSize + (int)newCompressedSize);
      cwr.writeRaw(tile->data, tile->dataSize - tile->compressedSize);
      cwr.writeRaw(tmpBuffer.data(), (int)newCompressedSize);
    }

    // Write navmesh data, compress the old way with zstd stream.
    uint32_t dataSize = sizeof(int) + bucket.dataSize;
    cwr.writeInt32e(dataSize);
    BinDumpSaveCB dcwr(2 << 20, cwr.getTarget() ? cwr.getTarget() : _MAKE4C('PC'), cwr.WRITE_BE);
    dcwr.writeInt32e((uint32_t)bucket.tileIds.size());
    for (int i : bucket.tileIds)
    {
      const dtMeshTile *tile = mesh->getTile(i);
      dcwr.writeInt64e(mesh->getTileRef(tile));
      dcwr.writeInt32e(tile->dataSize);
      dcwr.writeRaw(tile->data, tile->dataSize);
    }
    DynamicMemGeneralSaveCB save(midmem);
    dcwr.copyDataTo(save);
    InPlaceMemLoadCB crd(save.data(), save.size());
    zstd_compress_data(cwr.getRawWriter(), crd, dataSize, 1 << 20, 19);

    cwr.endBlock();
  }

  ZSTD_freeCCtx(cctx);
  ZSTD_freeCDict(cDict);
}

static void save_navmesh(const dtNavMesh *mesh, const NavMeshParams &nav_mesh_params, dtTileCache *tc,
  dag::ConstSpan<NavMeshObstacle> obstacles, BinDumpSaveCB &cwr, const int compress_ratio)
{
  int64_t reftSave = ref_time_ticks_qpc();
  if (tc && (nav_mesh_params.bucketSize > 0.0f))
  {
    // TODO: Bucket format for tile cached navmesh should be the only one,
    // remove tile cached code path below later.
    save_navmesh_bucket_format(mesh, nav_mesh_params, tc, obstacles, cwr);
    return;
  }
  int numTiles = 0;
  int numTCTiles = 0;
  int numObsResNameHashes = 0;
  ska::flat_hash_set<uint32_t> obsResNameHashes;
  uint32_t dataSize = sizeof(int) + sizeof(dtNavMeshParams);
  for (int i = 0; i < mesh->getMaxTiles(); ++i)
  {
    const dtMeshTile *tile = mesh->getTile(i);
    if (!tile || !tile->header || !tile->dataSize)
      continue;
    dataSize += sizeof(dtTileRef) + sizeof(int) + tile->dataSize;
    numTiles++;
  }
  if (tc)
  {
    dataSize += sizeof(int) + sizeof(dtTileCacheParams);
    for (int i = 0; i < tc->getTileCount(); ++i)
    {
      const dtCompressedTile *tile = tc->getTile(i);
      if (!tile || !tile->header || !tile->dataSize)
        continue;
      dataSize += sizeof(int) + tile->dataSize;
      numTCTiles++;
    }
    for (const NavMeshObstacle &obs : obstacles)
      obsResNameHashes.insert(obs.resNameHash);
    numObsResNameHashes = (int)obsResNameHashes.size();
    dataSize += sizeof(int) + sizeof(uint32_t) * numObsResNameHashes;
  }

  BinDumpSaveCB dcwr(2 << 20, cwr.getTarget() ? cwr.getTarget() : _MAKE4C('PC'), cwr.WRITE_BE);
  dcwr.writeRaw(mesh->getParams(), sizeof(dtNavMeshParams));
  if (tc)
  {
    dcwr.writeRaw(tc->getParams(), sizeof(dtTileCacheParams));
    dcwr.writeInt32e(numTiles);
    dcwr.writeInt32e(numTCTiles);
    dcwr.writeInt32e(numObsResNameHashes);
    DEBUG_DUMP_VAR(numTiles);
    DEBUG_DUMP_VAR(numTCTiles);
    DEBUG_DUMP_VAR(numObsResNameHashes);
  }
  else
  {
    dcwr.writeInt32e(numTiles);
    DEBUG_DUMP_VAR(numTiles);
  }

  // Store tiles.
  for (int i = 0; i < mesh->getMaxTiles(); ++i)
  {
    const dtMeshTile *tile = mesh->getTile(i);
    if (!tile || !tile->header || !tile->dataSize)
      continue;

    dcwr.writeInt64e(mesh->getTileRef(tile));
    dcwr.writeInt32e(tile->dataSize);
    dcwr.writeRaw(tile->data, tile->dataSize);
  }
  if (tc)
  {
    for (int i = 0; i < tc->getTileCount(); ++i)
    {
      const dtCompressedTile *tile = tc->getTile(i);
      if (!tile || !tile->header || !tile->dataSize)
        continue;

      dcwr.writeInt32e(tile->dataSize);
      dcwr.writeRaw(tile->data, tile->dataSize);
    }
    for (uint32_t h : obsResNameHashes)
      dcwr.writeInt32e(h);
  }
  DEBUG_DUMP_VAR(dataSize);
  cwr.writeInt32e(dataSize | (HmapLandPlugin::preferZstdPacking ? 0x40000000 : 0));
  DynamicMemGeneralSaveCB save(midmem);
  dcwr.copyDataTo(save);
  InPlaceMemLoadCB crd(save.data(), save.size());
  int64_t reftCompress = ref_time_ticks_qpc();
  int compressedSize = 0;
  if (HmapLandPlugin::preferZstdPacking)
    compressedSize = zstd_compress_data(cwr.getRawWriter(), crd, dataSize, 1 << 20, compress_ratio);
  else
    compressedSize = lzma_compress_data(cwr.getRawWriter(), compress_ratio, crd, dataSize);
  DAEDITOR3.conNote("Saved navmesh for time %.2f sec, compressed by %.2f%% (%.2fkb/%.2fkb, for %.2f sec) with ratio %i",
    get_time_usec_qpc(reftSave) / 1000000.0, (float)compressedSize / dataSize * 100.f, compressedSize / 1024.f, dataSize / 1024.f,
    get_time_usec_qpc(reftCompress) / 1000000.0, compress_ratio);
}

static void save_covers(const Tab<covers::Cover> &covers, BinDumpSaveCB &cwr)
{
  BinDumpSaveCB dcwr(2 << 20, cwr.getTarget() ? cwr.getTarget() : _MAKE4C('PC'), cwr.WRITE_BE);

  int dataSize = (int)covers.size() * (int)sizeof(covers::Cover);
  dcwr.writeRaw(covers.data(), dataSize);

  cwr.writeInt32e(dataSize | (HmapLandPlugin::preferZstdPacking ? 0x40000000 : 0));
  DynamicMemGeneralSaveCB save(midmem);
  dcwr.copyDataTo(save);
  InPlaceMemLoadCB crd(save.data(), save.size());
  if (HmapLandPlugin::preferZstdPacking)
    zstd_compress_data(cwr.getRawWriter(), crd, dataSize, 1 << 20, 19);
  else
    lzma_compress_data(cwr.getRawWriter(), 9, crd, dataSize);
}

static void save_debug_edges(const Tab<Tab<eastl::pair<Point3, Point3>>> &debugEdges, BinDumpSaveCB &cwr)
{
  BinDumpSaveCB dcwr(2 << 20, cwr.getTarget() ? cwr.getTarget() : _MAKE4C('PC'), cwr.WRITE_BE);

  int dataSize = 0;
  int pairSize = (int)sizeof(eastl::pair<Point3, Point3>);
  for (const auto &edgesList : debugEdges)
  {
    dataSize += (int)edgesList.size() * pairSize;
    for (const auto &edge : edgesList)
      dcwr.writeRaw(&edge, pairSize);
  }

  cwr.writeInt32e(dataSize | (HmapLandPlugin::preferZstdPacking ? 0x40000000 : 0));
  DynamicMemGeneralSaveCB save(midmem);
  dcwr.copyDataTo(save);
  InPlaceMemLoadCB crd(save.data(), save.size());
  if (HmapLandPlugin::preferZstdPacking)
    zstd_compress_data(cwr.getRawWriter(), crd, dataSize, 1 << 20, 19);
  else
    lzma_compress_data(cwr.getRawWriter(), 9, crd, dataSize);
}

static bool load_debug_edges(IGenLoad &crd, Tab<eastl::pair<Point3, Point3>> &debug_edges)
{
  clear_and_shrink(debug_edges);

  unsigned pairSize = (unsigned)sizeof(eastl::pair<Point3, Point3>);
  unsigned dataSize = crd.readInt();
  unsigned fmt = 0;
  if ((dataSize & 0xC0000000) == 0x40000000)
  {
    fmt = 1;
    dataSize &= ~0xC0000000;
  }

  if (dataSize % pairSize)
  {
    logerr_ctx("Could not load debug edges: broken data: data size = %d, debug edges size = %d", dataSize, pairSize);
    return false;
  }
  if (!dataSize)
    return false;

  IGenLoad *dcrd = (fmt == 1) ? (IGenLoad *)new (alloca(sizeof(ZstdLoadCB)), _NEW_INPLACE) ZstdLoadCB(crd, crd.getBlockRest())
                              : (IGenLoad *)new (alloca(sizeof(LzmaLoadCB)), _NEW_INPLACE) LzmaLoadCB(crd, crd.getBlockRest());

  debug_edges.resize(dataSize / pairSize);
  dcrd->read(debug_edges.data(), dataSize);
  dcrd->ceaseReading();
  dcrd->~IGenLoad();
  return true;
}

static void save_debug_obstacles(const Tab<NavMeshObstacle> &obstacles, BinDumpSaveCB &cwr)
{
  cwr.writeInt32e(obstacles.size());
  cwr.writeTabDataRaw(obstacles);
}

static void load_debug_obstacles(IGenLoad &crd, Tab<NavMeshObstacle> &obstacles)
{
  clear_and_shrink(obstacles);
  crd.readTab(obstacles);
}


static void load_pathfinder(const HmapLandPlugin *plugin, int nav_mesh_idx, const DataBlock &nav_mesh_props)
{
  String fileName;
  pathfinder::NavMeshType type{unsigned(
    nav_mesh_props.getInt("navMeshType", nav_mesh_props.getBool("tiled", false) ? pathfinder::NMT_TILED : pathfinder::NMT_SIMPLE))};

  switch (type)
  {
    case pathfinder::NMT_TILECACHED: fileName = "navMeshTiled3"; break;
    case pathfinder::NMT_TILED: fileName = "navMeshTiled2"; break;
    case pathfinder::NMT_SIMPLE: fileName = "navMesh2"; break;
    default: G_ASSERT(false); break;
  }

  fileName += (nav_mesh_idx == 0 ? String(".PC.bin") : String(50, "_%d.PC.bin", nav_mesh_idx + 1));

  FullFileLoadCB fcrd(DAGORED2->getPluginFilePath(plugin, fileName));

  if (fcrd.fileHandle)
  {
    fcrd.beginBlock();
    if (!pathfinder::loadNavMesh(fcrd, type))
      DAEDITOR3.conError("failed to load NavMesh from %s", fcrd.getTargetName());
    fcrd.endBlock();
  }
}

static bool finalize_navmesh_tile(const NavMeshParams &nav_mesh_params, BuildContext &ctx, const rcConfig &cfg,
  recastnavmesh::OffMeshConnectionsStorage &conn_storage, recastnavmesh::RecastTileContext &tile_ctx, int tx, int ty,
  Tab<recastnavmesh::BuildTileData> &tile_data)
{
  //
  // Step 1. Build polygons mesh from contours.
  //

  // Build polygon navmesh from the contours.
  tile_ctx.pmesh = rcAllocPolyMesh();
  if (!tile_ctx.pmesh)
  {
    ctx.log(RC_LOG_ERROR, "buildNavigation: Out of memory 'tile_ctx.pmesh'.");
    tile_ctx.clearIntermediate(&tile_data);
    return false;
  }
  if (!rcBuildPolyMesh(&ctx, *tile_ctx.cset, cfg.maxVertsPerPoly, *tile_ctx.pmesh))
  {
    ctx.log(RC_LOG_ERROR, "buildNavigation: Could not triangulate contours.");
    tile_ctx.clearIntermediate(&tile_data);
    return false;
  }

  //
  // Step 2. Create detail mesh which allows to access approximate height on each polygon.
  //

  tile_ctx.dmesh = rcAllocPolyMeshDetail();
  if (!tile_ctx.dmesh)
  {
    ctx.log(RC_LOG_ERROR, "buildNavigation: Out of memory 'pmdtl'.");
    tile_ctx.clearIntermediate(&tile_data);
    return false;
  }

  if (!rcBuildPolyMeshDetail(&ctx, *tile_ctx.pmesh, *tile_ctx.chf, cfg.detailSampleDist, cfg.detailSampleMaxError, *tile_ctx.dmesh))
  {
    ctx.log(RC_LOG_ERROR, "buildNavigation: Could not build detail mesh.");
    tile_ctx.clearIntermediate(&tile_data);
    return false;
  }

  rcFreeCompactHeightfield(tile_ctx.chf);
  tile_ctx.chf = NULL;
  rcFreeContourSet(tile_ctx.cset);
  tile_ctx.cset = NULL;

  //
  // (Optional) Step 3. Create Detour data from Recast poly mesh.
  //

  // The GUI may allow more max points per polygon than Detour can handle.
  // Only build the detour navmesh if we do not exceed the limit.
  if ((cfg.maxVertsPerPoly <= DT_VERTS_PER_POLYGON) && (tile_ctx.pmesh->npolys > 0))
  {
    for (int i = 0; i < tile_ctx.pmesh->npolys; ++i)
      tile_ctx.pmesh->flags[i] = 1;

    dtNavMeshCreateParams params;
    memset(&params, 0, sizeof(params));
    params.verts = tile_ctx.pmesh->verts;
    params.vertCount = tile_ctx.pmesh->nverts;
    params.polys = tile_ctx.pmesh->polys;
    params.polyAreas = tile_ctx.pmesh->areas;
    params.polyFlags = tile_ctx.pmesh->flags;
    params.polyCount = tile_ctx.pmesh->npolys;
    params.nvp = tile_ctx.pmesh->nvp;
    params.detailMeshes = tile_ctx.dmesh->meshes;
    params.detailVerts = tile_ctx.dmesh->verts;
    params.detailVertsCount = tile_ctx.dmesh->nverts;
    params.detailTris = tile_ctx.dmesh->tris;
    params.detailTriCount = tile_ctx.dmesh->ntris;
    if (nav_mesh_params.jlkParams.enabled)
    {
      params.offMeshConVerts = conn_storage.m_offMeshConVerts;
      params.offMeshConRad = conn_storage.m_offMeshConRads;
      params.offMeshConDir = conn_storage.m_offMeshConDirs;
      params.offMeshConAreas = conn_storage.m_offMeshConAreas;
      params.offMeshConFlags = conn_storage.m_offMeshConFlags;
      params.offMeshConUserID = conn_storage.m_offMeshConId;
      params.offMeshConCount = conn_storage.m_offMeshConCount;
    }
    params.walkableHeight = nav_mesh_params.agentHeight;
    params.walkableRadius = nav_mesh_params.agentRadius;
    params.walkableClimb = nav_mesh_params.agentClimbAfterGluingMeshes;
    params.tileX = tx;
    params.tileY = ty;
    params.tileLayer = 0;
    rcVcopy(params.bmin, tile_ctx.pmesh->bmin);
    rcVcopy(params.bmax, tile_ctx.pmesh->bmax);
    params.cs = cfg.cs;
    params.ch = cfg.ch;
    params.buildBvTree = false;

    unsigned char *navData = 0;
    int navDataSize = 0;
    if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
    {
      ctx.log(RC_LOG_ERROR, "Could not build Detour navmesh.");
      tile_ctx.clearIntermediate(&tile_data);
      return false;
    }
    tile_ctx.clearIntermediate(&tile_data);
    recastnavmesh::BuildTileData td;
    td.navMeshData = navData;
    td.navMeshDataSz = navDataSize;
    tile_data.push_back(td);
    return true;
  }

  tile_ctx.clearIntermediate(&tile_data);
  return true;
}

static bool finalize_navmesh_tilecached_tile(const NavMeshParams &nav_mesh_params, BuildContext &ctx, const rcConfig &cfg,
  recastnavmesh::OffMeshConnectionsStorage &conn_storage, recastnavmesh::RecastTileContext &tile_ctx,
  dag::ConstSpan<NavMeshObstacle> obstacles, int tx, int ty, Tab<recastnavmesh::BuildTileData> &tile_data)
{
  dtTileCacheAlloc tcAllocator;
  pathfinder::TileCacheCompressor tcComp;
  tcComp.reset(nav_mesh_params.bucketSize > 0.0f);
  float agentRadius = nav_mesh_params.agentRadius;
  float agentHeight = nav_mesh_params.agentHeight;
  float agentMaxClimb = nav_mesh_params.agentMaxClimb;

  auto fn = [&](dag::ConstSpan<NavMeshObstacle> obstacles, const rcConfig &cfg, dtTileCacheLayer &layer,
              const dtTileCacheLayerHeader &header) {
    for (const auto &obs : obstacles)
    {
      Point3 c = obs.box.center();
      Point3 ext = obs.box.width() * 0.5f;
      float coshalf = cosf(0.5f * obs.yAngle);
      float sinhalf = sinf(-0.5f * obs.yAngle);
      float rotAux[2] = {coshalf * sinhalf, coshalf * coshalf - 0.5f};
      pathfinder::tilecache_apply_obstacle_padding(cfg.cs, Point2(agentRadius, agentHeight), c, ext);
      dtMarkBoxArea(layer, header.bmin, cfg.cs, cfg.ch, &c.x, &ext.x, rotAux, 0);
    }
  };

  return recastnavmesh::finalize_navmesh_tilecached_tile(ctx, cfg, &tcAllocator, &tcComp,
    nav_mesh_params.jlkParams.enabled ? &conn_storage : nullptr, tile_ctx, tx, ty, agentMaxClimb, agentHeight, agentRadius, obstacles,
    tile_data, fn);
}

static bool prepare_tile_context(const NavMeshParams &nav_mesh_params, BuildContext &ctx, rcConfig &cfg,
  recastnavmesh::RecastTileContext &tile_ctx, dag::ConstSpan<Point3> vertices, dag::ConstSpan<int> indices,
  dag::ConstSpan<IPoint2> transparent, const BBox3 &box, int gw, int gh, int override_tile_size, bool build_cset)
{
  //
  // Step 1. Initialize build config.
  //
  memset(&cfg, 0, sizeof(cfg));
  cfg.cs = nav_mesh_params.cellSize;
  cfg.ch = nav_mesh_params.cellHeight;
  cfg.walkableSlopeAngle = nav_mesh_params.agentMaxSlope;
  cfg.walkableHeight = (int)ceilf(nav_mesh_params.agentHeight / cfg.ch);
  cfg.walkableClimb = (int)ceilf(nav_mesh_params.agentMaxClimb / cfg.ch);
  cfg.walkableRadius = (int)ceilf(nav_mesh_params.agentRadius / cfg.cs);
  cfg.maxEdgeLen = (int)(nav_mesh_params.edgeMaxLen / nav_mesh_params.cellSize);
  cfg.maxSimplificationError = clamp(nav_mesh_params.edgeMaxError / nav_mesh_params.cellSize, 1.1f, 1.5f);
  cfg.minRegionArea = int(nav_mesh_params.regionMinSize * sqr(safeinv(nav_mesh_params.cellSize)));
  cfg.mergeRegionArea = int(nav_mesh_params.regionMergeSize * sqr(safeinv(nav_mesh_params.cellSize)));
  cfg.maxVertsPerPoly = nav_mesh_params.vertsPerPoly;
  cfg.tileSize = (nav_mesh_params.navMeshType > pathfinder::NMT_SIMPLE) ? override_tile_size : 0;
  cfg.borderSize = (nav_mesh_params.navMeshType > pathfinder::NMT_SIMPLE) ? cfg.walkableRadius + 3 : 0;
  cfg.width = gw > 0 ? gw : cfg.tileSize + cfg.borderSize * 2;
  cfg.height = gh > 0 ? gh : cfg.tileSize + cfg.borderSize * 2;
  cfg.detailSampleDist = nav_mesh_params.detailSampleDist < 0.9f ? 0 : nav_mesh_params.cellSize * nav_mesh_params.detailSampleDist;
  cfg.detailSampleMaxError = nav_mesh_params.cellHeight * nav_mesh_params.detailSampleMaxError;

  BBox3 expandedBox(box);
  expandedBox.inflate(cfg.borderSize * cfg.cs);
  rcVcopy(cfg.bmin, &expandedBox.lim[0].x);
  rcVcopy(cfg.bmax, &expandedBox.lim[1].x);

  ctx.log(RC_LOG_PROGRESS, "Building navigation:");
  ctx.log(RC_LOG_PROGRESS, " - %d x %d cells", cfg.width, cfg.height);
  ctx.log(RC_LOG_PROGRESS, " - %.1fK verts, %.1fK tris", vertices.size() / 1000.0f, indices.size() / 3 / 1000.0f);

  return recastnavmesh::prepare_tile_context(ctx, cfg, tile_ctx, vertices, indices, transparent, build_cset);
}

static bool prepare_tile(const NavMeshParams &nav_mesh_params, BuildContext &ctx, rcConfig &cfg,
  recastnavmesh::RecastTileContext &tile_ctx, dag::ConstSpan<Point3> vertices, dag::ConstSpan<int> indices,
  dag::ConstSpan<IPoint2> transparent, const BBox3 &box, int gw, int gh, bool build_cset, bool with_extra_cells)
{
  tile_ctx.clearIntermediate(nullptr);

  if (!with_extra_cells)
    return prepare_tile_context(nav_mesh_params, ctx, cfg, tile_ctx, vertices, indices, transparent, box, gw, gh,
      nav_mesh_params.tileSize, build_cset);

  // By specifying 0.499 factor in SplitTileGeomJob we're making sure that
  // extra half of tile of geometry gets fetched for each tile on each side, thus, we can
  // add extra cells here and use this "extended" tile context for calculating offmesh links and
  // covers, this solves "no covers / offmesh links on tile boundaries" problem.
  // We can use resulting offmesh links right away when building "narrower" navmesh, since navmesh strips down all
  // offmesh links that don't belong to tile. For covers, we handle this manually down below after all tiles
  // are processed.

  const int extraCells = min((nav_mesh_params.tileSize - 1) / 2, nav_mesh_params.jlkCovExtraCells);
  BBox3 extBox = box;
  const float cellSize = nav_mesh_params.cellSize;
  extBox.lim[0] -= Point3(extraCells * cellSize, 0.0f, extraCells * cellSize);
  extBox.lim[1] += Point3(extraCells * cellSize, 0.0f, extraCells * cellSize);

  const int extTileSize = nav_mesh_params.tileSize + extraCells * 2;

  return prepare_tile_context(nav_mesh_params, ctx, cfg, tile_ctx, vertices, indices, transparent, extBox, gw, gh, extTileSize,
    build_cset);
}

static bool isCustomJumpLink(const SplineObject *spline)
{
  return !spline->points.empty() && spline->points.front()->getSplineClass() != nullptr &&
         spline->points.front()->getSplineClass()->isCustomJumplink;
}

static Tab<recastbuild::CustomJumpLink> get_relevant_custom_jumplinks(const BBox3 &box, const HmapLandObjectEditor *objEd)
{
  Tab<recastbuild::CustomJumpLink> relevantJumplinks;
  const int splinesCount = objEd->splinesCount();
  for (int i = 0; i < splinesCount; ++i)
  {
    const auto &spline = objEd->getSpline(i);
    if (!isCustomJumpLink(spline))
      continue;

    const auto &points = spline->points;
    if (points.size() != 2)
    {
      logerr("Custom spline jumplink should consist out of two points, but has %d points. Skipping", int(points.size()));
      continue;
    }

    const auto &splineSegments = spline->getBezierSpline().segs;
    if (splineSegments.size() != 1)
    {
      logerr("Custom spline jumplink should consist out of two points and one segment, but has %d segments. Skipping",
        int(splineSegments.size()));
      continue;
    }

    const Point3 &jumpLinkStart = splineSegments.front().sk[0];
    const Point3 jumpLinkEnd = splineSegments.front().point(1.0f);
    if (box & jumpLinkStart)
      relevantJumplinks.push_back({jumpLinkStart, jumpLinkEnd});
    else if (box & jumpLinkEnd)
      relevantJumplinks.push_back({jumpLinkEnd, jumpLinkStart});
  }
  return relevantJumplinks;
}

static bool rasterize_and_build_navmesh_tile(const NavMeshParams &nav_mesh_params, recastnavmesh::RecastTileContext &tile_ctx,
  dag::ConstSpan<Point3> vertices, dag::ConstSpan<int> indices, dag::ConstSpan<IPoint2> transparent,
  dag::ConstSpan<NavMeshObstacle> obstacles, const ska::flat_hash_map<uint32_t, uint32_t> &obstacle_flags_by_res_name_hash,
  const BBox3 &box, Tab<covers::Cover> &covers, Tab<eastl::pair<Point3, Point3>> &edges_out, int tx, int ty,
  Tab<recastnavmesh::BuildTileData> &tileData, const HmapLandObjectEditor *objEd, int gw = -1, int gh = -1)
{
  const bool needCov = covParams.enabled;
  const bool needJlkCov = nav_mesh_params.jlkParams.enabled || needCov;
  const bool useExtraCells =
    (nav_mesh_params.navMeshType > pathfinder::NMT_SIMPLE) && needJlkCov && (nav_mesh_params.jlkCovExtraCells > 0);
  const bool useTransparent = needCov && !transparent.empty();

  rcConfig cfg;
  BuildContext ctx;
  bool preparedOKForNavMesh = false;
  const Tab<IPoint2> noTransparent;

  //
  // Build covers and off mesh connections (jump links) from edges contours.
  //
  recastnavmesh::OffMeshConnectionsStorage connStorage;
  if (needJlkCov)
  {
    if (!prepare_tile(nav_mesh_params, ctx, cfg, tile_ctx, vertices, indices, noTransparent, box, gw, gh, true, useExtraCells))
      return false;
    preparedOKForNavMesh = !useExtraCells;

    Tab<recastbuild::Edge> edges;
    recastbuild::build_edges(edges, tile_ctx.cset, tile_ctx.chf, nav_mesh_params.mergeParams, &edges_out);

    if (nav_mesh_params.jlkParams.enabled)
      recastbuild::build_jumplinks_connstorage(connStorage, edges, nav_mesh_params.jlkParams, tile_ctx.chf, tile_ctx.solid);

    const Tab<recastbuild::CustomJumpLink> customJumplinks = get_relevant_custom_jumplinks(box, objEd);
    add_custom_jumplinks(connStorage, edges, customJumplinks);

    if (nav_mesh_params.jlkParams.crossObstaclesWithJumplinks && obstacle_flags_by_res_name_hash.size() > 0)
    {
      Tab<recastbuild::JumpLinkObstacle> crossObstacles;
      for (auto &o : obstacles)
      {
        if (obstacle_flags_by_res_name_hash.find(o.resNameHash) != obstacle_flags_by_res_name_hash.end())
        {
          uint32_t f = obstacle_flags_by_res_name_hash.find(o.resNameHash)->second;
          if (f & ObstacleFlags::CROSS_WITH_JL)
            crossObstacles.push_back({o.box, o.yAngle});
        }
      }
      cross_obstacles_with_jumplinks(connStorage, edges, box, nav_mesh_params.jlkParams, crossObstacles);
    }
    if (obstacle_flags_by_res_name_hash.size() > 0)
    {
      Tab<recastbuild::JumpLinkObstacle> disableJLObstacles;
      for (auto &o : obstacles)
      {
        if (obstacle_flags_by_res_name_hash.find(o.resNameHash) != obstacle_flags_by_res_name_hash.end())
        {
          uint32_t f = obstacle_flags_by_res_name_hash.find(o.resNameHash)->second;
          if (f & ObstacleFlags::DISABLE_JL_AROUND)
            disableJLObstacles.push_back({o.box, o.yAngle});
        }
      }
      disable_jumplinks_around_obstacle(connStorage, disableJLObstacles);
    }

    if (needCov)
    {
      if (useTransparent)
      {
        rcConfig cfg2;
        recastnavmesh::RecastTileContext tile_ctx2;
        if (!prepare_tile(nav_mesh_params, ctx, cfg2, tile_ctx2, vertices, indices, transparent, box, gw, gh, true, useExtraCells))
        {
          tile_ctx2.clearIntermediate(nullptr);
          return false;
        }

        recastbuild::build_covers(covers, box, edges, covParams, tile_ctx.chf, tile_ctx.solid, tile_ctx2.chf, tile_ctx2.solid,
          &edges_out);
        tile_ctx2.clearIntermediate(nullptr);
      }
      else
      {
        recastbuild::build_covers(covers, box, edges, covParams, tile_ctx.chf, tile_ctx.solid, tile_ctx.chf, tile_ctx.solid,
          &edges_out);
      }
    }
  }

  if (nav_mesh_params.navMeshType == pathfinder::NMT_TILECACHED)
  {
    Tab<int> cutIndices(get_thread_alloc());
    cutIndices.reserve(indices.size());
    for (int idx : indices)
    {
      bool ign = false;
      for (const NavMeshObstacle &obs : obstacles)
      {
        if (obs.vtxStart == -1)
          continue;
        if ((idx >= obs.vtxStart) && (idx < obs.vtxStart + obs.vtxCount))
        {
          ign = true;
          break;
        }
      }
      if (!ign)
        cutIndices.push_back(idx);
    }

    // Calculate tilecached navmesh with obstacles stripped out.
    tile_ctx.clearIntermediate(nullptr);
    if (!prepare_tile_context(nav_mesh_params, ctx, cfg, tile_ctx, vertices, cutIndices, noTransparent, box, gw, gh,
          nav_mesh_params.tileSize, false))
      return false;

    return finalize_navmesh_tilecached_tile(nav_mesh_params, ctx, cfg, connStorage, tile_ctx, obstacles, tx, ty, tileData);
  }

  if (!preparedOKForNavMesh)
  {
    if (!prepare_tile(nav_mesh_params, ctx, cfg, tile_ctx, vertices, indices, noTransparent, box, gw, gh, true, false))
      return false;
  }
  else
  {
    rcFreeHeightField(tile_ctx.solid);
    tile_ctx.solid = NULL;
  }

  return finalize_navmesh_tile(nav_mesh_params, ctx, cfg, connStorage, tile_ctx, tx, ty, tileData);
}

static WinCritSec buildJobCc;
class SplitTileGeomJob : public cpujobs::IJob
{
public:
  BBox3 landBBox;
  int th = 1;
  int tw = 1;
  Tab<Tab<int>> &outIndices;
  Tab<Tab<IPoint2>> &outTransparent;
  Tab<Tab<NavMeshObstacle>> &outTileObstacles;
  dag::ConstSpan<IPoint2> inTransparent;
  dag::ConstSpan<Point3> inVertices;
  dag::ConstSpan<int> inIndices;
  IPoint2 indexRange;
  float tcs = 1.f;
  dag::ConstSpan<NavMeshObstacle> inObstacles;

  SplitTileGeomJob(const BBox3 land_box, int in_tw, int in_th, Tab<Tab<int>> &out_ind, Tab<Tab<IPoint2>> &out_transparent,
    Tab<Tab<NavMeshObstacle>> &out_obstacles, dag::ConstSpan<Point3> in_vertices, dag::ConstSpan<int> in_indices,
    dag::ConstSpan<IPoint2> in_transparent, const IPoint2 &index_range, float in_tcs, dag::ConstSpan<NavMeshObstacle> in_obstacles) :
    landBBox(land_box),
    th(in_th),
    tw(in_tw),
    outIndices(out_ind),
    outTransparent(out_transparent),
    outTileObstacles(out_obstacles),
    inTransparent(in_transparent),
    inVertices(in_vertices),
    inIndices(in_indices),
    indexRange(index_range),
    tcs(in_tcs),
    inObstacles(in_obstacles)
  {}

  struct TileIndex
  {
    Point2 tilePos;
    int idx;
  };

  virtual void doJob()
  {
    G_ASSERTF(outIndices.size() == th * tw, "Incorrect arrays %d expected, got %d", th * tw, outIndices.size());

    float invStep = safeinv(tcs);
    Point2 landBase = Point2::xz(landBBox.lim[0]);
    auto getPosTileIdx = [this](const Point2 &pos_v2) -> int {
      return clamp(int(pos_v2.x), 0, this->tw - 1) + clamp(int(pos_v2.y), 0, this->th - 1) * this->tw;
    };
    auto getTileIdx = [&landBase, invStep, this](const Point3 &pos) -> TileIndex {
      Point2 vp2 = invStep * (Point2::xz(pos) - landBase);
      return {vp2, clamp(int(vp2.x), 0, this->tw - 1) + clamp(int(vp2.y), 0, this->th - 1) * this->tw};
    };
    Tab<int> tileIndices(midmem);
    tileIndices.reserve((indexRange.y - indexRange.x) * 4);
    Tab<int> triCount;
    triCount.resize(outIndices.size());
    mem_set_0(triCount);
    tileIndices.resize((indexRange.y - indexRange.x) * 4);
    mem_set_ff(tileIndices);
    for (int idx = indexRange.x; idx < indexRange.y; idx += 3)
    {
      int tileIndicesBase = (idx - indexRange.x) * 4;
      int tileIndexLast = tileIndicesBase;
      for (int i = 0; i < 3; ++i)
      {
        TileIndex tileIdx = getTileIdx(inVertices[inIndices[idx + i]]);
        for (int y = -1; y <= +1; ++y)
          for (int x = -1; x <= +1; ++x)
          {
            int tidx = getPosTileIdx(tileIdx.tilePos + Point2(0.499f * float(x), 0.499f * float(y)));
            bool found = false;
            for (int ti = tileIndicesBase; ti < tileIndexLast && !found; ++ti)
              found |= tileIndices[ti] == tidx;

            if (!found)
            {
              tileIndices[tileIndexLast] = tidx;
              ++tileIndexLast;
            }
          }
      }
      for (int i = tileIndicesBase; i < tileIndexLast; ++i)
        triCount[tileIndices[i]]++;
    }
    Tab<Tab<int>> indices(midmem);
    Tab<Tab<IPoint2>> transparent(midmem);
    indices.reserve(outIndices.size());
    indices.resize(outIndices.size());
    transparent.reserve(outTransparent.size());
    transparent.resize(outTransparent.size());

    for (int i = 0; i < indices.size(); ++i)
      indices[i].reserve(triCount[i] * 3);
    auto pushToTile = [&indices, &transparent, this](int tidx, int v0, int v1, int v2, bool transp) {
      auto &ind = indices[tidx];
      if (transp)
      {
        const int indNext = (int)ind.size();
        auto &tr = transparent[tidx];
        if (!tr.empty() && indNext == tr.back().x + tr.back().y)
          tr.back().y += 3;
        else
          tr.push_back({indNext, 3});
      }
      ind.push_back(v0);
      ind.push_back(v1);
      ind.push_back(v2);
    };
    auto checkTransparent = [&](int idx, int &tr_idx, int tr_num) {
      while (tr_idx < tr_num)
      {
        IPoint2 tr = inTransparent[tr_idx];
        const int ti = idx - tr.x;
        if (ti < 0)
          return false;
        if (ti < tr.y)
          return true;
        ++tr_idx;
      }
      return false;
    };
    int trIdx = 0;
    const int trNum = (int)inTransparent.size();
    auto tidxToCoord = [this](int idx) { return IPoint2(idx % this->tw, idx / this->tw); };
    auto coordToTidx = [this](int x, int y) { return x + y * this->tw; };
    for (int idx = indexRange.x; idx < indexRange.y; idx += 3)
    {
      bool transp = checkTransparent(idx, trIdx, trNum);
      int vert0 = inIndices[idx + 0];
      int vert1 = inIndices[idx + 1];
      int vert2 = inIndices[idx + 2];
      IPoint2 minC = IPoint2(INT_MAX, INT_MAX);
      IPoint2 maxC = IPoint2(INT_MIN, INT_MIN);
      for (int ti = (idx - indexRange.x) * 4; ti < (idx + 3 - indexRange.x) * 4; ++ti)
        if (tileIndices[ti] >= 0)
        {
          IPoint2 coord = tidxToCoord(tileIndices[ti]);
          minC.x = min(minC.x, coord.x);
          minC.y = min(minC.y, coord.y);
          maxC.x = max(maxC.x, coord.x);
          maxC.y = max(maxC.y, coord.y);
        }
      for (int x = minC.x; x <= maxC.x; ++x)
        for (int y = minC.y; y <= maxC.y; ++y)
          pushToTile(coordToTidx(x, y), vert0, vert1, vert2, transp);
    }

    Tab<Tab<NavMeshObstacle>> tileObstacles(midmem);
    tileObstacles.resize(outTileObstacles.size());

    for (const NavMeshObstacle &obs : inObstacles)
    {
      int tileIdx = 0;
      for (int i = 0; i < th; ++i)
        for (int j = 0; j < tw; ++j)
        {
          Point2 p1 = landBase + Point2(j, i) * tcs;
          Point2 p2 = p1 + Point2(tcs, tcs);
          BBox2 tileBox(p1, p2);
          BBox2 obsBox(Point2::xz(obs.aabb.lim[0]), Point2::xz(obs.aabb.lim[1]));
          if (tileBox & obsBox)
            tileObstacles[tileIdx].push_back(obs);
          ++tileIdx;
        }
    }

    WinAutoLock lock(buildJobCc);
    for (int i = 0; i < transparent.size(); ++i)
    {
      if (transparent[i].empty())
        continue;
      const int indRebase = (int)outIndices[i].size();
      outTransparent[i].reserve(outTransparent[i].size() + transparent[i].size());
      for (int j = 0; j < transparent[i].size(); ++j)
      {
        IPoint2 tr = transparent[i][j];
        tr.x += indRebase;
        outTransparent[i].push_back(tr);
      }
    }
    for (int i = 0; i < indices.size(); ++i)
    {
      if (indices[i].empty())
        continue;
      outIndices[i].reserve(outIndices[i].size() + indices[i].size());
      for (int j = 0; j < indices[i].size(); ++j)
        outIndices[i].push_back(indices[i][j]);
    }
    for (int i = 0; i < tileObstacles.size(); ++i)
    {
      if (tileObstacles[i].empty())
        continue;
      outTileObstacles[i].reserve(outTileObstacles[i].size() + tileObstacles[i].size());
      for (int j = 0; j < tileObstacles[i].size(); ++j)
        outTileObstacles[i].push_back(tileObstacles[i][j]);
    }
  }
  virtual void releaseJob() { delete this; }
};

class DropDownTileJob : public cpujobs::IJob
{
public:
  const int pointDimensionsCount = 3;
  const dtMeshTile *tile;
  Point3 startPointOffset, traceDir;
  float traceLen, traceOffset;

  int vertexProcessed, detailVertexProcessed;

  DropDownTileJob(const dtMeshTile *tile, const Point3 &start_point_offset, const Point3 &trace_dir, float trace_len,
    float trace_offset) :
    tile(tile),
    startPointOffset(start_point_offset),
    traceDir(trace_dir),
    traceLen(trace_len),
    traceOffset(trace_offset),
    vertexProcessed(0),
    detailVertexProcessed(0)
  {}

  void dropPointOnCollision(float *vertex)
  {
    float t = traceLen + traceOffset;
    Point3 point(vertex, Point3::CTOR_FROM_PTR);
    if (IEditorCoreEngine::get()->traceRay(point + startPointOffset, traceDir, t, nullptr, false))
    {
      point += traceDir * (t - traceOffset);
      memcpy(vertex, &point[0], sizeof(float) * pointDimensionsCount);
    }
  }

  virtual void doJob()
  {
    const int tileVertsCount = tile->header->vertCount;
    vertexProcessed += tileVertsCount;
    for (int j = 0; j < tileVertsCount; ++j)
      dropPointOnCollision(&tile->verts[j * pointDimensionsCount]);
    const int detailTileVertsCount = tile->header->detailVertCount;
    detailVertexProcessed += detailTileVertsCount;
    for (int j = 0; j < detailTileVertsCount; ++j)
      dropPointOnCollision(&tile->detailVerts[j * pointDimensionsCount]);
  }

  virtual void releaseJob() { delete this; }
};

class BuildTileJob : public cpujobs::IJob
{
  recastnavmesh::RecastTileContext tileCtx;
  BBox3 tileBox;
  int nmIdx = 0;
  const NavMeshParams &nmParams;
  dag::ConstSpan<Point3> vertices;
  dag::ConstSpan<int> indices;
  dag::ConstSpan<IPoint2> transparent;
  dag::ConstSpan<NavMeshObstacle> obstacles;
  Tab<recastnavmesh::BuildTileData> tileData;
  const ska::flat_hash_map<uint32_t, uint32_t> obstacleFlags;
  const HmapLandObjectEditor *objEd;

public:
  bool haveProblems = false;
  int x = 0;
  int y = 0;
  Tab<BuildTileJob *> &finishedJobs;
  Tab<covers::Cover> &covers;
  Tab<eastl::pair<Point3, Point3>> &edges_out;

  BuildTileJob(int in_x, int in_y, const BBox3 &box, int in_nav_mesh_idx, dag::ConstSpan<Point3> in_vertices,
    dag::ConstSpan<int> in_indices, dag::ConstSpan<IPoint2> in_transparent, dag::ConstSpan<NavMeshObstacle> in_obstacles,
    const ska::flat_hash_map<uint32_t, uint32_t> &obstacle_flags_by_res_name_hash, Tab<BuildTileJob *> &finished_jobs,
    Tab<covers::Cover> &in_covers, Tab<eastl::pair<Point3, Point3>> &in_edges_out, const HmapLandObjectEditor *object_editor) :
    x(in_x),
    y(in_y),
    tileBox(box),
    nmParams(navMeshParams[in_nav_mesh_idx]),
    vertices(in_vertices),
    indices(in_indices),
    transparent(in_transparent),
    obstacles(in_obstacles),
    finishedJobs(finished_jobs),
    covers(in_covers),
    edges_out(in_edges_out),
    obstacleFlags(obstacle_flags_by_res_name_hash),
    objEd(object_editor)
  {}

  virtual void doJob()
  {
    haveProblems = !rasterize_and_build_navmesh_tile(nmParams, tileCtx, vertices, indices, transparent, obstacles, obstacleFlags,
      tileBox, covers, edges_out, x, y, tileData, objEd);
    WinAutoLock lock(buildJobCc);
    finishedJobs.push_back(this);
  }
  virtual void releaseJob()
  {
    for (const recastnavmesh::BuildTileData &td : tileData)
    {
      navMesh->removeTile(navMesh->getTileRefAt(x, y, td.layer), NULL, NULL);
      if (td.tileCacheData)
        if (dtStatusFailed(tileCache->addTile(td.tileCacheData, td.tileCacheDataSz, DT_COMPRESSEDTILE_FREE_DATA, 0)))
          dtFree(td.tileCacheData);
      if (td.navMeshData)
        if (dtStatusFailed(navMesh->addTile(td.navMeshData, td.navMeshDataSz, DT_TILE_FREE_DATA, 0, nullptr)))
          dtFree(td.navMeshData);
    }
    delete this;
  }
};

class CheckCoversJob : public cpujobs::IJob
{
public:
  IPoint2 range;
  Tab<covers::Cover> &covers;
  Tab<int> &unavailableCovers;
  dtNavMeshQuery *navQuery;

  static constexpr float maxRadiusThreshold = 500.0f;
  static constexpr int aroundCountTreshold = 50;
  static constexpr int countTreshold = 20;

  CheckCoversJob(IPoint2 range_in, Tab<covers::Cover> &covers_in, Tab<int> &unavailable_covers_in) :
    range(range_in), covers(covers_in), unavailableCovers(unavailable_covers_in), navQuery(nullptr)
  {}

  virtual void doJob()
  {
    if (!pathfinder::isLoaded())
      return;

    navQuery = dtAllocNavMeshQuery();
    if (!navQuery)
    {
      logerr_ctx("Could not create NavMeshQuery");
      return;
    }

    if (dtStatusFailed(navQuery->init(pathfinder::getNavMeshPtr(), 2048)))
    {
      logerr_ctx("Could not init Detour navmesh query");
      return;
    }

    for (int i = range[0]; i < range[1]; i++)
    {
      int checkedCov = 0, totalCov = 0;
      Point3 posi = lerp(covers[i].groundLeft, covers[i].groundRight, 0.5f);

      float radTreshold = 100.0f / 2.0f;
      float radTresholdSq = sqr(radTreshold);

      int aroundCount = 0;
      while (aroundCount < aroundCountTreshold && radTreshold < maxRadiusThreshold)
      {
        radTreshold *= 2.0f;
        radTresholdSq = sqr(radTreshold);

        aroundCount = 0;
        for (int j = 0; j < covers.size(); j++)
        {
          if (j == i)
            continue;
          Point3 posj = lerp(covers[j].groundLeft, covers[j].groundRight, 0.5f);
          if (lengthSq(posi - posj) > radTresholdSq)
            continue;
          ++aroundCount;
        }
      }

      pathfinder::NavParams params;
      params.enable_jumps = true;
      for (int j = 0; j < covers.size(); j++)
      {
        if (j == i)
          continue;

        Point3 posj = lerp(covers[j].groundLeft, covers[j].groundRight, 0.5f);
        if (lengthSq(posi - posj) > radTresholdSq)
          continue;
        ++totalCov;

        if (pathfinder::check_path(navQuery, posi, posj, Point3(0.5f, FLT_MAX, 0.5f), params, 0.5f, 1.f))
          if (++checkedCov > countTreshold)
            break;
      }

      if (checkedCov < countTreshold && float(checkedCov) / float(totalCov) < 0.5f)
        unavailableCovers.push_back(i);
    }

    dtFreeNavMeshQuery(navQuery);
  }
  virtual void releaseJob()
  {
    navQuery = NULL;
    delete this;
  }
};

static void gather_collider_plugins(const NavMeshParams &nav_mesh_params, Tab<IDagorEdCustomCollider *> &coll)
{
  for (int i = 0, coll_cnt = DAGORED2->getCustomCollidersCount(); i < coll_cnt; ++i)
    if (IDagorEdCustomCollider *p = DAGORED2->getCustomCollider(i))
      if (strcmp(p->getColliderName(), "(srv) Render instance entities") == 0 ||
          (nav_mesh_params.usePrefabCollision && strcmp(p->getColliderName(), "(srv) Prefab entities") == 0) ||
          strcmp(p->getColliderName(), "SGeometry") == 0)
        coll.push_back(p);
}

static void prepare_collider_plugins(const Tab<IDagorEdCustomCollider *> &coll)
{
  for (IDagorEdCustomCollider *c : coll)
    c->prepareCollider();
}

static void setup_collider_plugins(const Tab<IDagorEdCustomCollider *> &coll, const BBox3 &box)
{
  DAGORED2->setColliders(coll, 0xFFFFFFFF);
  // pregen all RI to get proper cuts for navigation mesh
  for (int i = 0; i < coll.size(); ++i)
    coll[i]->setupColliderParams(1, box);
}

static void gather_static_coll(StaticGeometryContainer &cont)
{
  for (int i = 0, plugin_cnt = IEditorCoreEngine::get()->getPluginCount(); i < plugin_cnt; ++i)
  {
    IGenEditorPlugin *plugin = DAGORED2->getPlugin(i);
    IGatherStaticGeometry *geom = plugin->queryInterface<IGatherStaticGeometry>();
    IGatherCollision *gatherColl = plugin->queryInterface<IGatherCollision>();
    if (geom)
      geom->gatherStaticCollisionGeomEditor(cont);
  }
}

static void calc_colliders_buffer_size(int &vert, int &ind, const StaticGeometryContainer &cont)
{
  for (int i = 0; i < cont.nodes.size(); ++i)
  {
    vert += cont.nodes[i]->mesh->mesh.vert.size();
    ind += cont.nodes[i]->mesh->mesh.face.size() * 3;
  }
}

static void gather_colliders_meshes(Tab<Point3> &vertices, Tab<int> &indices, Tab<IPoint2> &transparent,
  Tab<NavMeshObstacle> &obstacles, const ska::flat_hash_map<uint32_t, uint32_t> *&obstacle_flags_by_res_name_hash,
  const StaticGeometryContainer &cont, const BBox3 &box)
{
  for (int i = 0; i < cont.nodes.size(); ++i)
  {
    StaticGeometryNode &n = *cont.nodes[i];
    Mesh mesh = n.mesh->mesh;
    const TMatrix &wtm = n.wtm;
    int idxBase = vertices.size();
    for (int j = 0; j < mesh.vert.size(); ++j)
      vertices.push_back(wtm * mesh.vert[j]);
    for (int j = 0; j < mesh.face.size(); ++j)
      for (int k = 0; k < 3; ++k)
        indices.push_back(mesh.face[j].v[k] + idxBase);
  }

  for (int i = 0, plugin_cnt = IEditorCoreEngine::get()->getPluginCount(); i < plugin_cnt; ++i)
  {
    IGenEditorPlugin *plugin = DAGORED2->getPlugin(i);
    IGatherCollision *gatherColl = plugin->queryInterface<IGatherCollision>();
    if (!gatherColl)
      continue;
    int64_t refdet = ref_time_ticks_qpc();
    gatherColl->gatherCollision(box, vertices, indices, transparent, obstacles);
    DAEDITOR3.conNote("Gathered geom for %.2f sec", get_time_usec_qpc(refdet) / 1000000.0);
    gatherColl->getObstaclesFlags(obstacle_flags_by_res_name_hash);
  }
}

bool HmapLandPlugin::buildAndWriteSingleNavMesh(BinDumpSaveCB &cwr, int nav_mesh_idx)
{
  NavMeshParams &nmParams = navMeshParams[nav_mesh_idx];
  DataBlock &nmProps = navMeshProps[nav_mesh_idx];
  const bool needsDebugEdges = nav_mesh_idx == 0;

  if (!nmProps.getBool("export", false) && cwr.getTarget())
    return true;

  init(nmProps, nav_mesh_idx);
  if (nmParams.navmeshExportType == NavmeshExportType::INVALID)
  {
    DAEDITOR3.conError("Invalid navmesh export type, please choose appropriate type of navmesh export");
    return false;
  }
  clearNavMesh();

  dtTileCacheAlloc tcAllocator;
  pathfinder::TileCacheCompressor tcComp;
  tcComp.reset(nmParams.bucketSize > 0.0f);

  float water_lev = hasWaterSurface ? waterSurfaceLevel : (nmProps.getBool("hasWater", false) ? nmProps.getReal("waterLev", 0) : -1e6);
  float min_h = 0, max_h = water_lev;

  if (nmParams.navmeshExportType == NavmeshExportType::WATER || nmParams.navmeshExportType == NavmeshExportType::WATER_AND_GEOMETRY)
    water_lev = nmProps.getReal("waterLev", 0);

  int nav_area_type = nmProps.getInt("navArea", 0);
  if (nav_area_type < 0 || nav_area_type > 2)
  {
    DAEDITOR3.conWarning("Incorrect navArea=%d, changing to 1", nav_area_type);
    nmProps.setInt("navArea", nav_area_type = 1);
  }

  BBox2 nav_area;
  if (nav_area_type == 1 && detDivisor)
    nav_area = detRect;
  else if (nav_area_type == 2)
  {
    nav_area[0] = nmProps.getPoint2("rect0", Point2(0, 0));
    nav_area[1] = nmProps.getPoint2("rect1", Point2(1000, 1000));
  }
  else
  {
    nav_area[0].set_xz(landBox[0]);
    nav_area[1].set_xz(landBox[1]);
  }

  if (covParams.enabled)
  {
    covParams.maxVisibleBox[0] = Point3(nav_area[0].x, -500.f, nav_area[0].y);
    covParams.maxVisibleBox[1] = Point3(nav_area[1].x, 500.f, nav_area[1].y);
  }

  int width = nav_area.width().x / nmParams.traceStep;
  int length = nav_area.width().y / nmParams.traceStep;
  if (width <= 0 || width >= 4096 || length <= 0 || length >= 4096)
  {
    DAEDITOR3.conError("Incorrect grid size %dx%d (for area %@ with traceStep=%.2f)", width, length, nav_area, nmParams.traceStep);
    return false;
  }

  int64_t reft = ref_time_ticks_qpc();

  Tab<IDagorEdCustomCollider *> coll(tmpmem);
  gather_collider_plugins(nmParams, coll);
  BBox3 collidersBox(Point3::xVy(nav_area[0], -FLT_MAX), Point3::xVy(nav_area[1], FLT_MAX));
  setup_collider_plugins(coll, collidersBox);
  prepare_collider_plugins(coll);
  Tab<Point3> vertices;
  Tab<int> indices;
  Tab<IPoint2> transparent;
  Tab<NavMeshObstacle> obstacles;
  const ska::flat_hash_map<uint32_t, uint32_t> obstacleFlagsDefault;
  const ska::flat_hash_map<uint32_t, uint32_t> *obstacleFlags = nullptr;
  const bool useDetailGeometry =
    nmParams.navmeshExportType == NavmeshExportType::GEOMETRY || nmParams.navmeshExportType == NavmeshExportType::WATER_AND_GEOMETRY;
  if (useDetailGeometry)
  {
    int64_t refdet = ref_time_ticks_qpc();
    setup_collider_plugins(coll, collidersBox);
    int vertCount = 0;
    int indCount = 0;
    StaticGeometryContainer geoCont;
    gather_static_coll(geoCont);
    calc_colliders_buffer_size(vertCount, indCount, geoCont);
    vertices.reserve(vertCount);
    indices.reserve(indCount);

    gather_colliders_meshes(vertices, indices, transparent, obstacles, obstacleFlags, geoCont, collidersBox);

    for (int i = 0; i < coll.size(); ++i)
      coll[i]->setupColliderParams(0, BBox3());
    DAGORED2->restoreEditorColliders();
    for (int i = 0; i < vertices.size(); ++i)
    {
      inplace_min(min_h, vertices[i].y);
      inplace_max(max_h, vertices[i].y);
    }
    DAEDITOR3.conNote("Built detail geom for %.2f sec, %d vertices", get_time_usec_qpc(refdet) / 1000000.0, vertices.size());
  }

  if (!obstacleFlags)
    obstacleFlags = &obstacleFlagsDefault;

  {
    // Leave only obstacles that are higher than 'agentMaxClimb'.
    // 'agentMaxClimb' is somewhat confusing name, it describes how high
    // ledges can be before the character can no longer step up on them.
    // So if the character CAN step up on them, there's no point in making an obstacle.
    Tab<NavMeshObstacle> filteredObstacles;
    filteredObstacles.reserve(obstacles.size());
    for (const NavMeshObstacle &obstacle : obstacles)
      if (!pathfinder::tilecache_ri_obstacle_too_low(obstacle.aabb.width().y, nmParams.agentMaxClimb))
        filteredObstacles.push_back(obstacle);
    filteredObstacles.swap(obstacles);
  }

  {
    Point2 worldPosOfs = nav_area[0];

    float startXfloat = worldPosOfs.x, startZfloat = worldPosOfs.y;

    HierBitMap2d<HierConstSizeBitMap2d<4, ConstSizeBitMap2d<5>>> splBm;
    BezierSpline2d s2d_src;
    splBm.resize(width, length);

    int64_t refsplines = ref_time_ticks_qpc();
    for (int i = 0; i < objEd.splinesCount(); i++)
    {
      SplineObject *sobj = objEd.getSpline(i);
      float def_hw = sobj->getProps().useForNavMesh ? sobj->getProps().navMeshStripeWidth / 2 : -1;
      if (sobj->isPoly())
        continue;

      int w = width, h = length;
      float sx = 1.0f / nmParams.traceStep;
      sobj->getSplineXZ(s2d_src);
      BezierSplinePrec2d &s2d = toPrecSpline(s2d_src);
      float splineLen = s2d.getLength(), st;

      const splineclass::AssetData *a = sobj->points[0]->getSplineClass();
      int seg = 0, hw0 = (a && a->navMeshStripeWidth > 0) ? int(a->navMeshStripeWidth / 2 * sx) : def_hw * sx;

      for (float s = 0.0; s < splineLen; s += 0.5 / sx) //-V1034
      {
        int n_seg = s2d.findSegment(s, st);
        if (seg != n_seg)
        {
          a = sobj->points[n_seg]->getSplineClass();
          hw0 = (a && a->navMeshStripeWidth > 0) ? int(a->navMeshStripeWidth / 2 * sx) : def_hw * sx;
          seg = n_seg;
        }
        if (hw0 <= 0)
          continue;

        Point2 p = s2d.segs[n_seg].point(st);
        Point2 t = normalize(s2d.segs[n_seg].tang(st));
        Point2 n(t.y / sx, -t.x / sx);
        for (int i = -hw0 + 1; i <= hw0 - 1; i++)
        {
          float px = p.x + n.x * i - startXfloat, pz = p.y + n.y * i - startZfloat;
          int ix = int(px * sx) - 1, ixe = ix + 3 <= w ? ix + 3 : w;
          int iy = int(pz * sx) - 1, iye = iy + 3 <= h ? iy + 3 : h;
          for (iy = iy < 0 ? 0 : iy, ix = ix < 0 ? 0 : ix; iy < iye; iy++)
            for (int x = ix; x < ixe; x++)
              splBm.set(x, iy);
        }
      }
    }
    DAEDITOR3.conNote("Built splines geom for %.2f sec", get_time_usec_qpc(refsplines) / 1000000.0);
    // if (use_splines)
    //   saveBmTga(splBm, "splMask.tga");

    DEBUG_DUMP_VAR(landMeshMap.getEditorLandRayTracer());
    DEBUG_DUMP_VAR(landMeshMap.getGameLandRayTracer());
    DEBUG_DUMP_VAR(water_lev);

    int idxBase = vertices.size();
    vertices.resize(vertices.size() + length * width);
    const int indicesBase = indices.size();
    indices.resize(indicesBase + (length - 1) * (width - 1) * 6);

    float *v = &vertices[idxBase].x;

    int64_t refhmap = ref_time_ticks_qpc();
    for (int z = 0; z < length; z++)
      for (int x = 0; x < width; x++, v += 3)
      {
        v[0] = startXfloat + nmParams.traceStep * x;
        v[1] = 8192;
        v[2] = startZfloat + nmParams.traceStep * z;
        if ((nmParams.navmeshExportType == NavmeshExportType::SPLINES && !splBm.get(x, z)) ||
            (!getHeight(Point2(v[0], v[2]), v[1], NULL) && !landMeshMap.getHeight(Point2(v[0], v[2]), v[1], NULL)))
          v[1] = water_lev - 1 - nmParams.crossingWaterDepth;
        else
        {
          const float aboveHt = 200;
          real dist = aboveHt;
          if ((nmParams.navmeshExportType == NavmeshExportType::WATER ||
                nmParams.navmeshExportType == NavmeshExportType::WATER_AND_GEOMETRY) &&
              v[1] < MIN_BOTTOM_DEPTH)
            v[1] = MIN_BOTTOM_DEPTH;
          if (!useDetailGeometry && DAGORED2->traceRay(Point3(v[0], v[1] + aboveHt, v[2]), Point3(0, -1, 0), dist, NULL, false))
            v[1] += aboveHt - dist;
          if (nmParams.navmeshExportType == NavmeshExportType::WATER ||
              nmParams.navmeshExportType == NavmeshExportType::WATER_AND_GEOMETRY)
            v[1] = max(v[1], water_lev);
          inplace_max(max_h, v[1]);
          inplace_min(min_h, v[1]);
        }
      }

    v = &vertices[idxBase].y;
    int *indicesPtr = &indices[indicesBase];
    for (int z = 0, zwidth = idxBase; z + 1 < length; z++, v += 3, zwidth += width)
      for (int x = 0, zwidthplusx = zwidth; x + 1 < width; x++, v += 3, zwidthplusx++)
      {
        if (nmParams.navmeshExportType == NavmeshExportType::WATER ||
            nmParams.navmeshExportType == NavmeshExportType::WATER_AND_GEOMETRY)
        {
          if (v[0] <= water_lev && v[width * 3 + 3] <= water_lev && v[3] <= water_lev)
          {
            *(indicesPtr++) = zwidthplusx;
            *(indicesPtr++) = zwidthplusx + width + 1;
            *(indicesPtr++) = zwidthplusx + 1;
          }
          if (v[0] <= water_lev && v[width * 3] <= water_lev && v[width * 3 + 3] <= water_lev)
          {
            *(indicesPtr++) = zwidthplusx;
            *(indicesPtr++) = zwidthplusx + width;
            *(indicesPtr++) = zwidthplusx + width + 1;
          }
        }
        else
        {
          if (v[0] > water_lev - nmParams.crossingWaterDepth && v[width * 3 + 3] > water_lev - nmParams.crossingWaterDepth &&
              v[3] > water_lev - nmParams.crossingWaterDepth)
          {
            *(indicesPtr++) = zwidthplusx;
            *(indicesPtr++) = zwidthplusx + width + 1;
            *(indicesPtr++) = zwidthplusx + 1;
          }
          if (v[0] > water_lev - nmParams.crossingWaterDepth && v[width * 3] > water_lev - nmParams.crossingWaterDepth &&
              v[width * 3 + 3] > water_lev - nmParams.crossingWaterDepth)
          {
            *(indicesPtr++) = zwidthplusx;
            *(indicesPtr++) = zwidthplusx + width;
            *(indicesPtr++) = zwidthplusx + width + 1;
          }
        }
      }
    G_ASSERT(indices.capacity() >= indicesBase + (indicesPtr - &indices[indicesBase]));
    indices.resize(indicesBase + (indicesPtr - &indices[indicesBase]));
    DAEDITOR3.conNote("Built hmap geom for %.2f sec", get_time_usec_qpc(refhmap) / 1000000.0);

    for (int i = 0, plugin_cnt = IEditorCoreEngine::get()->getPluginCount(); i < plugin_cnt; ++i)
    {
      IGenEditorPlugin *plugin = DAGORED2->getPlugin(i);
      IGatherGroundHoles *holesMgr = plugin->queryInterface<IGatherGroundHoles>();
      if (!holesMgr)
        continue;
      Tab<GroundHole> holes;
      holesMgr->gatherGroundHoles(holes);
      DAEDITOR3.conNote("Ground holes: %d", holes.size());
      int64_t refholes = ref_time_ticks_qpc();
      int *indicesEnd = indices.end();
      for (int *indPtr = &indices[indicesBase]; indPtr < indicesEnd; indPtr += 3)
        for (const GroundHole &hole : holes)
          if (hole.check(Point2::xz(vertices[indPtr[0]])) || hole.check(Point2::xz(vertices[indPtr[1]])) ||
              hole.check(Point2::xz(vertices[indPtr[2]])))
          {
            indicesEnd -= 3;
            indPtr[0] = indicesEnd[0];
            indPtr[1] = indicesEnd[1];
            indPtr[2] = indicesEnd[2];
            break;
          }
      indices.resize(indicesEnd - &indices[0]);
      DAEDITOR3.conNote("Cut ground holes for %.2f sec", get_time_usec_qpc(refholes) / 1000000.0);
    }
  }
  if (!useDetailGeometry)
  {
    for (int i = 0; i < coll.size(); ++i)
      coll[i]->setupColliderParams(0, BBox3());
    DAGORED2->restoreEditorColliders();
  }

  DAEDITOR3.conNote("Built collision %dx%d for %.2f sec (minH/maxH/waterH/crossing=%.1f/%.1f/%.1f/%.1f) nTri=%d vVert=%d", width,
    length, get_time_usec_qpc(reft) / 1000000.0, min_h, max_h, water_lev, nmParams.crossingWaterDepth, indices.size() / 3,
    vertices.size());

  BBox3 landBBox;
  if (nmParams.navmeshExportType == NavmeshExportType::WATER)
  {
    landBBox[0].set_xVy(nav_area[0], min_h - 1);
    landBBox[1].set_xVy(nav_area[1], min(max_h, water_lev) + 1);
  }
  else
  {
    landBBox[0].set_xVy(nav_area[0], max(min_h, water_lev) - 1);
    landBBox[1].set_xVy(nav_area[1], max_h + 1);
  }
  DEBUG_DUMP_VAR(landBBox);


  // Set the area where the navigation will be build.
  // Here the bounds of the input mesh are used, but the
  // area could be specified by an user defined box, etc.
  int gw, gh;
  rcCalcGridSize(&landBBox.lim[0].x, &landBBox.lim[1].x, nmParams.cellSize, &gw, &gh);

  // Reset build times gathering.
  global_build_ctx.resetTimers();

  // Start the build process.
  global_build_ctx.startTimer(RC_TIMER_TOTAL);

  Tab<Tab<covers::Cover>> coversLists(midmem);
  coversLists.reserve(1);
  coversLists.push_back().reserve(100);

  Tab<Tab<eastl::pair<Point3, Point3>>> debugEdgesLists(midmem);
  debugEdgesLists.reserve(1);
  debugEdgesLists.push_back().reserve(100);

  navMesh = dtAllocNavMesh();
  if (!navMesh)
  {
    global_build_ctx.log(RC_LOG_ERROR, "Could not create Detour navmesh");
    return false;
  }
  if (nmParams.navMeshType > pathfinder::NMT_SIMPLE)
  {
    // Tiled calculation
    const int ts = nmParams.tileSize;
    const int tw = (gw + ts - 1) / ts;
    const int th = (gh + ts - 1) / ts;
    const float tcs = nmParams.tileSize * nmParams.cellSize;

    DEBUG_DUMP_VAR(ts);
    DEBUG_DUMP_VAR(tw);
    DEBUG_DUMP_VAR(th);
    DEBUG_DUMP_VAR(tcs);

    dtNavMeshParams params;
    rcVcopy(params.orig, &landBBox.lim[0].x);
    params.tileWidth = tcs;
    params.tileHeight = tcs;
    if (nmParams.navMeshType == pathfinder::NMT_TILED)
    {
      params.maxTiles = th * tw;
      params.maxPolys = 1 << (32 - 16 - 2);
    }
    else
    {
      G_ASSERT(nmParams.navMeshType == pathfinder::NMT_TILECACHED);
      params.maxTiles = th * tw * TILECACHE_AVG_LAYERS_PER_TILE;
      params.maxPolys = 1 << (32 - 16 - 2);
    }
    DEBUG_DUMP_VAR(params.maxPolys);
    if (!navMesh->init(&params))
    {
      global_build_ctx.log(RC_LOG_ERROR, "Could not init Detour navmesh");
      clearNavMesh();
      return false;
    }
    int cores_count = cpujobs::get_core_count();
    DAEDITOR3.conNote("Building nav mesh with Recast/Detour with %d/%d cores", cores_count, cpujobs::get_core_count());
    int numSplits = cores_count * 2;
    Tab<Tab<int>> tileIndices;
    Tab<Tab<IPoint2>> tileTransparent;
    Tab<Tab<NavMeshObstacle>> tileObstacles;
    tileIndices.resize(th * tw);
    tileTransparent.resize(th * tw);
    tileObstacles.resize(th * tw);
    int64_t refsplit = ref_time_ticks_qpc();
    DAEDITOR3.conNote("vert data size %.2f MB, indices data size %.2f MB", data_size(vertices) * 0.000001f,
      data_size(indices) * 0.000001f);
    if (cores_count)
    {
      threadpool::init(cores_count, get_bigger_pow2(max(th * tw, numSplits)), 128 << 10);
      thread_alloc.resize(cores_count);
      for (int i = 0; i < cores_count; ++i)
        thread_alloc[i] = MspaceAlloc::create(nullptr, 32 << 20, false);
    }
    Tab<cpujobs::IJob *> splitJobs(tmpmem);
    for (int i = 0; i < numSplits; ++i)
    {
      int triCount = indices.size() / 3;
      IPoint2 indexRange(i * (triCount / (numSplits - 1)) * 3, (i + 1) * (triCount / (numSplits - 1)) * 3);
      if (indexRange.y > indices.size())
        indexRange.y = indices.size();
      IPoint2 obstaclesRange(i * ((float)obstacles.size() / (float)(numSplits - 1)),
        (i + 1) * ((float)obstacles.size() / (float)(numSplits - 1)));
      obstaclesRange.x = min((int)obstacles.size(), max(i, obstaclesRange.x));
      obstaclesRange.y = min((int)obstacles.size(), max(obstaclesRange.x + 1, obstaclesRange.y));

      SplitTileGeomJob *job = new SplitTileGeomJob(landBBox, tw, th, tileIndices, tileTransparent, tileObstacles, vertices, indices,
        transparent, indexRange, tcs, make_span_const(&obstacles[obstaclesRange.x], obstaclesRange.y - obstaclesRange.x));
      threadpool::add(job);
      splitJobs.push_back(job);
    }
    if (!cores_count)
    {
      SplitTileGeomJob job(landBBox, tw, th, tileIndices, tileTransparent, tileObstacles, vertices, indices, transparent,
        IPoint2(0, indices.size()), tcs, obstacles);
      job.doJob();
    }
    else
    {
      threadpool::wake_up_all();
      for (int i = 0; i < splitJobs.size(); ++i)
        threadpool::wait(splitJobs[i]);
      for (int i = 0; i < splitJobs.size(); ++i)
        splitJobs[i]->releaseJob();
    }
    int64_t dataSize = 0;
    for (int i = 0; i < tileIndices.size(); ++i)
      dataSize += data_size(tileIndices[i]);
    DAEDITOR3.conNote("total data size for tiles indices %.2f MB", dataSize * 0.000001f);

    DAEDITOR3.conNote("Split tiles in %.2f sec", get_time_usec_qpc(refsplit) / 1000000.0);
    clear_and_shrink(indices); // we don't need those indices anymore

    if (cores_count)
    {
      int threadCount = th * tw;
      coversLists.reserve(threadCount);
      debugEdgesLists.reserve(threadCount);
      for (int i = 0; i < threadCount - 1; ++i)
      {
        coversLists.push_back().reserve(100);
        debugEdgesLists.push_back().reserve(100);
        ;
      }
    }

    if (nmParams.navMeshType == pathfinder::NMT_TILECACHED)
    {
      tileCache = dtAllocTileCache();
      if (!tileCache)
      {
        global_build_ctx.log(RC_LOG_ERROR, "Could not create Detour tile cache");
        clearNavMesh();
        return false;
      }

      dtTileCacheParams tcparams;
      memset(&tcparams, 0, sizeof(tcparams));
      rcVcopy(tcparams.orig, &landBBox.lim[0].x);
      tcparams.cs = nmParams.cellSize;
      tcparams.ch = nmParams.cellHeight;
      tcparams.width = nmParams.tileSize;
      tcparams.height = nmParams.tileSize;
      tcparams.walkableHeight = nmParams.agentHeight;
      tcparams.walkableRadius = nmParams.agentRadius;
      tcparams.walkableClimb = nmParams.agentMaxClimb;
      tcparams.maxSimplificationError = clamp(nmParams.edgeMaxError / nmParams.cellSize, 1.1f, 1.5f);
      tcparams.maxTiles = tw * th * TILECACHE_AVG_LAYERS_PER_TILE;
      tcparams.maxObstacles = TILECACHE_MAX_OBSTACLES;

      tcMeshProc.setNavMesh(navMesh);
      tcMeshProc.setNavMeshQuery(nullptr);
      tcMeshProc.setTileCache(tileCache);
      tcMeshProc.forceStaticLinks();
      dtStatus status = tileCache->init(&tcparams, &tcAllocator, &tcComp, &tcMeshProc);
      if (dtStatusFailed(status))
      {
        global_build_ctx.log(RC_LOG_ERROR, "Could not init Detour tile cache");
        clearNavMesh();
        return false;
      }

      DAEDITOR3.conNote("Number of stationary obstacles %d, max - %d", (int)obstacles.size(), TILECACHE_MAX_OBSTACLES);

      if ((int)obstacles.size() > (TILECACHE_MAX_OBSTACLES * 4 / 5))
        global_build_ctx.log(RC_LOG_WARNING,
          "Way too many stationary obstacles (%d), it would make sense to raise TILECACHE_MAX_OBSTACLES value", (int)obstacles.size());
    }
    Tab<BuildTileJob *> tileJobs(tmpmem);
    Tab<BuildTileJob *> finishedJobs(tmpmem);
    bool haveProblems = false;
    for (int y = 0; y < th; ++y)
      for (int x = 0; x < tw; ++x)
      {
        int threadId = y * tw + x;
        BBox3 tileBox = BBox3(landBBox.lim[0] + Point3(x * tcs, 0.f, y * tcs),
          Point3::xVz(landBBox.lim[0] + Point3((x + 1) * tcs, 0.f, (y + 1) * tcs), landBBox.lim[1].y));
        if (!cores_count)
        {
          BuildTileJob job(x, y, tileBox, nav_mesh_idx, vertices, tileIndices[threadId], tileTransparent[threadId],
            tileObstacles[threadId], *obstacleFlags, finishedJobs, coversLists[0], debugEdgesLists[0], &objEd);
          job.doJob();
          haveProblems = job.haveProblems;
          job.releaseJob();
          continue;
        }
        BuildTileJob *job = new BuildTileJob(x, y, tileBox, nav_mesh_idx, vertices, tileIndices[threadId], tileTransparent[threadId],
          tileObstacles[threadId], *obstacleFlags, finishedJobs, coversLists[threadId], debugEdgesLists[threadId], &objEd);
        tileJobs.push_back(job);
        threadpool::add(job);
      }
    if (cores_count)
    {
      threadpool::wake_up_all();
      while (!tileJobs.empty())
      {
        Tab<BuildTileJob *> jobsToFinalize(tmpmem);
        {
          WinAutoLock lock(buildJobCc);
          jobsToFinalize = eastl::move(finishedJobs);
        }
        for (BuildTileJob *j : jobsToFinalize)
        {
          threadpool::wait(j); // should be already finished, but additional protection, so we'll not try to release job which is not
                               // done yet.
          haveProblems |= j->haveProblems;
          j->releaseJob();
          erase_item_by_value(tileJobs, j);
        }
        if (finishedJobs.empty())
          sleep_msec(10); // give some space for threadpool to complete it's job
      }
    }

    if (haveProblems)
      global_build_ctx.log(RC_LOG_ERROR, "Navmesh was built with problems, check text error log!");

    reft = ref_time_ticks_qpc();
    int pos = cwr.tell();

    BinDumpSaveCB dcwr(2 << 20, cwr.getTarget() ? cwr.getTarget() : _MAKE4C('PC'), cwr.WRITE_BE);

    if (nmProps.getBool("dropNavmeshOnCollision", false))
    {
      reft = ref_time_ticks_qpc();

      for (int i = 0, plugin_cnt = IEditorCoreEngine::get()->getPluginCount(); i < plugin_cnt; ++i)
        if (IDagorEdCustomCollider *p = DAGORED2->getCustomCollider(i))
          if (strcmp(p->getColliderName(), "HeightMap") == 0)
            coll.push_back(p);

      BBox3 collidersBox(Point3::xVy(nav_area[0], -FLT_MAX), Point3::xVy(nav_area[1], FLT_MAX));
      setup_collider_plugins(coll, collidersBox);

      constexpr int pointDimensionsCount = 3;
      const float traceOffset = nmProps.getReal("dropNavmeshTraceOffset", 0.3f);
      const float traceLen = nmProps.getReal("dropNavmeshTraceLen", 0.3f);
      const Point3 downDir(0.f, -1.f, 0.f);
      const Point3 startPointOffset = -downDir * traceOffset;
      const int maxTiles = navMesh->getMaxTiles();
      Tab<DropDownTileJob *> dropDownTileJobs(tmpmem);

      int tilesProcessed = 0;
      int vertexProcessed = 0;
      int detailVertexProcessed = 0;
      for (int i = 0; i < maxTiles; ++i)
      {
        const dtMeshTile *tile = ((const dtNavMesh *)navMesh)->getTile(i);
        if (tile && tile->header)
        {
          ++tilesProcessed;
          DropDownTileJob *job = new DropDownTileJob(tile, startPointOffset, downDir, traceLen, traceOffset);
          dropDownTileJobs.push_back(job);
          if (cores_count)
            threadpool::add(job);
        }
      }
      if (cores_count)
        threadpool::wake_up_all();
      for (DropDownTileJob *j : dropDownTileJobs)
      {
        if (cores_count)
          threadpool::wait(j); // should be already finished, but additional protection, so we'll not try to release job which is not
                               // done yet.
        else
          j->doJob();
        vertexProcessed += j->vertexProcessed;
        detailVertexProcessed += j->detailVertexProcessed;
        j->releaseJob();
      }
      clear_and_shrink(dropDownTileJobs);
      DAEDITOR3.conNote("Dropping navmesh points on collision done in %.2f sec", get_time_usec_qpc(reft) / 1000000.0);
      DAEDITOR3.conNote("Dropping navmesh points: tiles processed: %i", tilesProcessed);
      DAEDITOR3.conNote("Dropping navmesh points: vertex processed: %i", vertexProcessed);
      DAEDITOR3.conNote("Dropping navmesh points: detail vertex processed: %i", detailVertexProcessed);

      for (int i = 0; i < coll.size(); ++i)
        coll[i]->setupColliderParams(0, BBox3());
      DAGORED2->restoreEditorColliders();
      coll.pop_back();
    }

    if (nmParams.jlkParams.enabled && tileCache && nmProps.getBool("generateOverLinks", true))
    {
      bool haveDataForOverLinks = false;
      eastl::unique_ptr<scene::TiledScene> ladders;

      if (nmProps.getBool("generateOverLinksForLadders", true))
      {
        ladders.reset(new scene::TiledScene());

        int numLadders = 0;
        int64_t refTime = ref_time_ticks_qpc();
        for (int i = 0, plugin_cnt = IEditorCoreEngine::get()->getPluginCount(); i < plugin_cnt; ++i)
        {
          IGenEditorPlugin *plugin = DAGORED2->getPlugin(i);
          IGatherGameLadders *laddersMgr = plugin->queryInterface<IGatherGameLadders>();
          if (!laddersMgr)
            continue;
          Tab<GameLadder> laddersArr;
          laddersMgr->gatherGameLadders(laddersArr);
          for (GameLadder &ladder : laddersArr)
          {
            const int numLadderSteps = ladder.ladderStepsCount;
            mat44f m;
            v_mat44_make_from_43cu_unsafe(m, ladder.tm[0]);
            ladders->allocate(m, /*pool*/ 0, /*flags*/ numLadderSteps);
            ++numLadders;
          }
        }
        if (numLadders > 0)
          haveDataForOverLinks = true;
        DAEDITOR3.conNote("Gathered %d ladders in %.2f sec", numLadders, get_time_usec_qpc(refTime) / 1000000.0);
      }

      int numTilesNotEmpty = 0;
      int numTilesWithData = 0;
      int numTilesProcessed = 0;
      int numTilesProblems = 0;
      int numObstacleProblems = 0;

      const int64_t overLinksRefTime = ref_time_ticks();
      if (haveDataForOverLinks)
      {
        dtNavMeshQuery *navQuery = dtAllocNavMeshQuery();
        if (navQuery && dtStatusFailed(navQuery->init(navMesh, 4096)))
        {
          dtFreeNavMeshQuery(navQuery);
          navQuery = nullptr;
        }

        for (const auto &obs : obstacles)
        {
          Point3 c = obs.box.center();
          Point3 ext = obs.box.width() * 0.5f;
          pathfinder::tilecache_apply_obstacle_padding(nmParams.cellSize, Point2(nmParams.agentRadius, nmParams.agentHeight), c, ext);
          dtObstacleRef ref;
          dtStatus status = tileCache->addBoxObstacle(&c.x, &ext.x, obs.yAngle, &ref);
          if (dtStatusFailed(status))
            ++numObstacleProblems;
          else
          {
            bool upToDate = false;
            while (!upToDate)
            {
              // NB: Pass nullptr as navmesh to avoid real tiles rebuilding here
              status = tileCache->update(0.f, nullptr, &upToDate);
              if (dtStatusFailed(status))
                ++numObstacleProblems;
            }
          }
        }
        int tcObstacles = 0;
        if (numObstacleProblems > 0)
          global_build_ctx.log(RC_LOG_ERROR, "Problems with adding Detour obstacles for overlinks!");

        tcMeshProc.setLadders(ladders.get());
        tcMeshProc.setNavMeshQuery(navQuery);

        const int maxTiles = navMesh->getMaxTiles();
        for (int i = 0; i < maxTiles; ++i)
        {
          const dtMeshTile *tile = ((const dtNavMesh *)navMesh)->getTile(i);
          if (!tile || !tile->header)
            continue;

          ++numTilesNotEmpty;

          bool shouldRebuild = false;
          if (ladders.get())
          {
            BBox3 box;
            pathfinder::TileCacheMeshProcess::calcQueryLaddersBBox(box, tile->header->bmin, tile->header->bmax);
            bbox3f bbox = v_ldu_bbox3(box);
            ladders->boxCull<false, true>(bbox, 0, 0, [&](scene::node_index idx, mat44f_cref tm) { shouldRebuild = true; });
          }
          if (!shouldRebuild)
            continue;

          ++numTilesWithData;

          const int tileX = tile->header->x;
          const int tileY = tile->header->y;
          const int tileLayer = tile->header->layer;
          const dtCompressedTile *ctile = tileCache->getTileAt(tileX, tileY, tileLayer);
          if (!ctile)
            ++numTilesProblems;
          else
          {
            const dtCompressedTileRef ctileRef = tileCache->getTileRef(ctile);
            dtStatus status = tileCache->buildNavMeshTile(ctileRef, navMesh);
            if (dtStatusFailed(status))
              ++numTilesProblems;
            ++numTilesProcessed;
          }
        }
        if (numTilesProblems > 0)
          global_build_ctx.log(RC_LOG_ERROR, "Problems with rebuilding Detour tiles for overlinks!");

        tcMeshProc.setNavMeshQuery(nullptr);
        tcMeshProc.setLadders(nullptr);

        if (navQuery)
        {
          dtFreeNavMeshQuery(navQuery);
          navQuery = nullptr;
        }
      }

      if (numTilesProcessed > 0 || numTilesProblems > 0 || numObstacleProblems > 0)
      {
        const int overLinksTimeUsec = get_time_usec(overLinksRefTime);
        debug("NavMesh: rebuilded %d tiles for overlinks in %.2f sec (%d problems, %d non-empty tiles, %d tiles with data)",
          numTilesProcessed, overLinksTimeUsec / 1000000.0, numTilesProblems, numTilesNotEmpty, numTilesWithData);
      }
    }

    save_navmesh(navMesh, nmParams, tileCache, obstacles, dcwr, nmProps.getInt("navmeshCompressRatio", 19));

    String fileName(tileCache ? "navMeshTiled3" : "navMeshTiled2");
    fileName += (nav_mesh_idx == 0 ? String(".") : String(50, "_%d.", nav_mesh_idx + 1));
    fileName += mkbindump::get_target_str(dcwr.getTarget());
    fileName += ".bin";
    FullFileSaveCB fcwr(DAGORED2->getPluginFilePath(this, fileName));
    fcwr.beginBlock();
    dcwr.copyDataTo(fcwr);
    fcwr.endBlock();
    fcwr.close();

    if (cwr.getTarget())
    {
      cwr.beginTaggedBlock(tileCache ? _MAKE4C('Lnv3') : _MAKE4C('Lnv2'));
      dcwr.copyDataTo(cwr.getRawWriter());
      cwr.align8();
      cwr.endBlock();
      DAEDITOR3.conNote("Packed nav data: %dK (for %.2f sec)", (cwr.tell() - pos) >> 10, get_time_usec_qpc(reft) / 1000000.0);
    }
  }
  else
  {
    Tab<recastnavmesh::BuildTileData> tileData;
    recastnavmesh::RecastTileContext tctx;
    rasterize_and_build_navmesh_tile(nmParams, tctx, vertices, indices, transparent, obstacles, *obstacleFlags, landBBox,
      coversLists[0], debugEdgesLists[0], 0, 0, tileData, &objEd, gw, gh);
    if (!tileData.empty())
    {
      bool dtStatus = navMesh->init(tileData[0].navMeshData, tileData[0].navMeshDataSz, DT_TILE_FREE_DATA);
      if (!dtStatus)
      {
        dtFree(tileData[0].navMeshData);
        dtFreeNavMesh(navMesh);
        global_build_ctx.log(RC_LOG_ERROR, "Could not init Detour navmesh");
        return false;
      }
      reft = ref_time_ticks_qpc();
      int pos = cwr.tell();

      BinDumpSaveCB dcwr(2 << 20, cwr.getTarget() ? cwr.getTarget() : _MAKE4C('PC'), cwr.WRITE_BE);
      dcwr.writeInt32e(tileData[0].navMeshDataSz | (preferZstdPacking ? 0x40000000 : 0));
      InPlaceMemLoadCB crd(tileData[0].navMeshData, tileData[0].navMeshDataSz);
      if (preferZstdPacking)
        zstd_compress_data(dcwr.getRawWriter(), crd, tileData[0].navMeshDataSz, 1 << 20, 19);
      else
        lzma_compress_data(dcwr.getRawWriter(), 9, crd, tileData[0].navMeshDataSz);

      String fileName("navMesh2");
      fileName += (nav_mesh_idx == 0 ? String(".") : String(50, "_%d.", nav_mesh_idx + 1));
      fileName += mkbindump::get_target_str(dcwr.getTarget());
      fileName += ".bin";
      FullFileSaveCB fcwr(DAGORED2->getPluginFilePath(this, fileName));
      fcwr.beginBlock();
      dcwr.copyDataTo(fcwr);
      fcwr.endBlock();
      fcwr.close();

      if (cwr.getTarget())
      {
        cwr.beginTaggedBlock(_MAKE4C('Lnav'));
        dcwr.copyDataTo(cwr.getRawWriter());
        cwr.align8();
        cwr.endBlock();

        DAEDITOR3.conNote("Packed nav data: %dK (for %.2f sec)", (cwr.tell() - pos) >> 10, get_time_usec_qpc(reft) / 1000000.0);
      }
    }
    else
    {
      clearNavMesh();
      return false;
    }
  }

  if (needsDebugEdges)
  {
    BinDumpSaveCB dcwr(2 << 20, cwr.getTarget() ? cwr.getTarget() : _MAKE4C('PC'), cwr.WRITE_BE);
    save_debug_edges(debugEdgesLists, dcwr);
    String fileName("debug_edges");
    fileName += (nav_mesh_idx == 0 ? String(".") : String(50, "_%d.", nav_mesh_idx + 1)); // -V547
    fileName += mkbindump::get_target_str(dcwr.getTarget());
    fileName += ".bin";

    FullFileSaveCB fcwr(DAGORED2->getPluginFilePath(this, fileName));
    fcwr.beginBlock();
    dcwr.copyDataTo(fcwr);
    fcwr.endBlock();
    fcwr.close();
  }

  {
    BinDumpSaveCB dcwr(2 << 20, cwr.getTarget() ? cwr.getTarget() : _MAKE4C('PC'), cwr.WRITE_BE);
    save_debug_obstacles(obstacles, dcwr);
    String fileName("debug_obstacles");
    fileName += (nav_mesh_idx == 0 ? String(".") : String(50, "_%d.", nav_mesh_idx + 1));
    fileName += mkbindump::get_target_str(dcwr.getTarget());
    fileName += ".bin";

    FullFileSaveCB fcwr(DAGORED2->getPluginFilePath(this, fileName));
    fcwr.beginBlock();
    dcwr.copyDataTo(fcwr);
    fcwr.endBlock();
    fcwr.close();
  }

  if (covParams.enabled)
  {
    load_pathfinder(this, nav_mesh_idx, nmProps);

    const int64_t coverRefTime = ref_time_ticks();
    int totalSize = 0;
    for (const auto &coversList : coversLists)
      totalSize += (int)coversList.size();

    Tab<covers::Cover> covers(tmpmem);
    covers.reserve(totalSize);
    for (auto &coversStrip : coversLists)
      for (auto &cover : coversStrip)
        covers.push_back(cover);

    recastbuild::fit_intersecting_covers(covers, 0, nav_area, covParams);

    const int coverTimeUsec = get_time_usec(coverRefTime);
    DAEDITOR3.conNote("Simplified covers for %.2f sec", coverTimeUsec / 1000000.0);

    int cores_count = cpujobs::get_core_count();
    Tab<Tab<int>> unavailableCovers(tmpmem);

    if ((nmParams.navMeshType > pathfinder::NMT_SIMPLE) && cores_count)
    {
      int numSplits = cores_count * 2;
      unavailableCovers.resize(numSplits);

      int checkCount = (int)covers.size() / numSplits;
      int checkIt = 0;
      Tab<cpujobs::IJob *> checkCoversJobs(tmpmem);
      for (int i = 0; i < numSplits; ++i)
      {
        if (i == numSplits - 1)
          checkCount = (int)covers.size() - ((numSplits - 1) * checkCount);

        int nextCheckIt = checkIt + checkCount;
        unavailableCovers[i] = Tab<int>(tmpmem);
        unavailableCovers[i].reserve(checkCount);
        CheckCoversJob *job = new CheckCoversJob({checkIt, nextCheckIt}, covers, unavailableCovers[i]);
        threadpool::add(job);
        checkCoversJobs.push_back(job);

        checkIt = nextCheckIt;
      }

      threadpool::wake_up_all();
      for (int i = 0; i < checkCoversJobs.size(); ++i)
        threadpool::wait(checkCoversJobs[i]);
      for (int i = 0; i < checkCoversJobs.size(); ++i)
        checkCoversJobs[i]->releaseJob();
    }
    else
    {
      unavailableCovers.push_back(Tab<int>(tmpmem)).reserve(covers.size());
      CheckCoversJob job({0, (int)covers.size()}, covers, unavailableCovers[0]);
      job.doJob();
    }

    int numRemoved = 0;
    for (auto &unavCoversList : unavailableCovers)
      for (int idx : unavCoversList)
        if (idx >= 0 && idx < covers.size())
          covers[idx].hLeft = -1.0f;
    for (int i = 0; i < (int)covers.size(); ++i)
      if (covers[i].hLeft < 0.0f)
      {
        covers[i] = covers.back();
        covers.pop_back();
        --i;
        ++numRemoved;
      }

    debug("Covers: final count %d, removed %d in %.2f sec", (int)covers.size(), numRemoved, coverTimeUsec / 1000000.0);

    BinDumpSaveCB dcwr(2 << 20, cwr.getTarget() ? cwr.getTarget() : _MAKE4C('PC'), cwr.WRITE_BE);
    save_covers(covers, dcwr);

    FullFileSaveCB fcwr(DAGORED2->getPluginFilePath(this, "covers.") + mkbindump::get_target_str(dcwr.getTarget()) + ".bin");
    fcwr.beginBlock();
    dcwr.copyDataTo(fcwr);
    fcwr.endBlock();
    fcwr.close();

    if (cwr.getTarget())
    {
      cwr.beginTaggedBlock(_MAKE4C('CVRS'));
      dcwr.copyDataTo(cwr.getRawWriter());
      cwr.align8();
      cwr.endBlock();
    }
  }

  global_build_ctx.stopTimer(RC_TIMER_TOTAL);

  // Show performance stats.
  duLogBuildTimes(global_build_ctx, global_build_ctx.getAccumulatedTime(RC_TIMER_TOTAL));

  DAEDITOR3.conNote("Built nav mesh with Recast/Detour for %.2f sec", global_build_ctx.getAccumulatedTime(RC_TIMER_TOTAL) / 1000000.0);

  clearNavMesh();

  if (cpujobs::get_core_count() && (nmParams.navMeshType > pathfinder::NMT_SIMPLE))
  {
    for (MspaceAlloc *ta : thread_alloc)
      ta->destroy();
    clear_and_shrink(thread_alloc);
    nextThreadId = 0;
    threadpool::shutdown();
  }

  return true;
}

bool HmapLandPlugin::buildAndWriteNavMesh(BinDumpSaveCB &cwr)
{
  if (!cwr.getTarget())
    return true;

  bool success = true;

  if (navMeshProps[0].getBool("export", false))
    success = success && buildAndWriteSingleNavMesh(cwr, 0); // exporting main nav mesh separately

  unsigned int exportMask = 0;
  int exportedCount = 0;

  for (int idx = 1; idx < MAX_NAVMESHES; ++idx) // -V1008
  {
    if (!navMeshProps[idx].getBool("export", false))
      continue;
    exportMask |= (1 << idx);
    ++exportedCount;
  }

  if (exportedCount == 0)
    return success;

  cwr.beginTaggedBlock(_MAKE4C('Lnvs'));
  const int fmt = 1;
  cwr.writeInt32e(fmt);
  cwr.writeInt32e(exportMask);

  for (int idx = 1; idx < MAX_NAVMESHES; ++idx) // -V1008
  {
    if (exportMask & (1 << idx))
    {
      cwr.writeDwString(navMeshProps[idx].getStr("kind", ""));
      if (!buildAndWriteSingleNavMesh(cwr, idx))
      {
        cwr.beginTaggedBlock(0);
        cwr.endBlock();
      }
    }
  }

  cwr.endBlock();

  return success;
}

void HmapLandPlugin::clearNavMesh()
{
  dtFreeNavMesh(navMesh);
  navMesh = NULL;
  dtFreeTileCache(tileCache);
  tileCache = NULL;
  pathfinder::clear();

  clear_and_shrink(coversDebugList);
  clear_and_shrink(edgesDebugList);
  clear_and_shrink(obstaclesDebugList);

  exportedEdgesDebugListDone = false;
  exportedCoversDebugListDone = false;
  exportedNavMeshLoadDone = false;
  exportedObstaclesLoadDone = false;

  dagRender->invalidateDebugPrimitivesVbuffer(*navMeshBuf);
  dagRender->invalidateDebugPrimitivesVbuffer(*coversBuf);
  dagRender->invalidateDebugPrimitivesVbuffer(*contoursBuf);
  dagRender->invalidateDebugPrimitivesVbuffer(*obstaclesBuf);
}


static DebugPrimitivesVbuffer *active_debug_lines_vbuf = nullptr;


static void beginDebugPrimitivesVbufferCache(DebugPrimitivesVbuffer *vbuf)
{
  dagRender->beginDebugLinesCacheToVbuffer(*vbuf);
  active_debug_lines_vbuf = vbuf;
}


static void endDebugPrimitivesVbufferCache(DebugPrimitivesVbuffer *vbuf)
{
  dagRender->endDebugLinesCacheToVbuffer(*vbuf);
  active_debug_lines_vbuf = nullptr;
}


void HmapLandPlugin::renderNavMeshDebug()
{
  const bool showCovers = shownExportedNavMeshIdx == 0 && showExportedCovers;
  const bool showContours = shownExportedNavMeshIdx == 0 && showExportedNavMeshContours;

  setupRecastDetourAllocators();
  auto renderDebugPrimitives = [disableZtest = disableZtestForDebugNavMesh](DebugPrimitivesVbuffer *vbuf) {
    // We anyway perform ztest, but with CMPF_LESS function to draw hidden geometry with different color
    if (disableZtest)
      dagRender->renderLinesFromVbuffer(*vbuf, /*z_test*/ true, /*z_write*/ false,
        /*z_func_less*/ true, Color4(0.5, 0.5, 0.5, 1.0));
    dagRender->renderLinesFromVbuffer(*vbuf);
  };

  if (pathfinder::isLoaded() || coversDebugList.size() || edgesDebugList.size())
  {
    if (pathfinder::isLoaded() && showExportedNavMesh)
    {
      if (dagRender->isLinesVbufferValid(*navMeshBuf))
      {
        renderDebugPrimitives(navMeshBuf);
      }
      else
      {
        beginDebugPrimitivesVbufferCache(navMeshBuf);
        pathfinder::renderDebug();
        endDebugPrimitivesVbufferCache(navMeshBuf);
      }
    }

    if (showCovers && coversDebugList.size() && showExportedCovers)
    {
      if (dagRender->isLinesVbufferValid(*coversBuf))
      {
        renderDebugPrimitives(coversBuf);
      }
      else
      {
        beginDebugPrimitivesVbufferCache(coversBuf);
        covers::draw_debug(coversDebugList);
        covers::draw_debug(coversDebugScene);
        endDebugPrimitivesVbufferCache(coversBuf);
      }
    }

    if (showContours && edgesDebugList.size() && showExportedNavMeshContours)
    {
      if (dagRender->isLinesVbufferValid(*contoursBuf))
      {
        renderDebugPrimitives(contoursBuf);
      }
      else
      {
        beginDebugPrimitivesVbufferCache(contoursBuf);
        for (const auto &edge : edgesDebugList)
        {
          if (edge.first.y == -1024.5f)
          {
            const float ph = edge.first.x;
            const float sz = edge.first.z;
            const Point3 pt1 = edge.second;
            const Point3 pt2 = edge.second + Point3(sz, 0.0f, 0.0f);
            const Point3 pt3 = edge.second + Point3(sz, 0.0f, sz);
            const Point3 pt4 = edge.second + Point3(0.0f, 0.0f, sz);
            const Point3 ofs = Point3(0.0f, ph, 0.0f);

            dagRender->addLineToVbuffer(*contoursBuf, pt1, pt2, E3DCOLOR(200, 0, 0, 255));
            dagRender->addLineToVbuffer(*contoursBuf, pt2, pt3, E3DCOLOR(200, 0, 0, 255));
            dagRender->addLineToVbuffer(*contoursBuf, pt3, pt4, E3DCOLOR(200, 0, 0, 255));
            dagRender->addLineToVbuffer(*contoursBuf, pt4, pt1, E3DCOLOR(200, 0, 0, 255));

            dagRender->addLineToVbuffer(*contoursBuf, pt1 + ofs, pt2 + ofs, E3DCOLOR(200, 0, 0, 255));
            dagRender->addLineToVbuffer(*contoursBuf, pt2 + ofs, pt3 + ofs, E3DCOLOR(200, 0, 0, 255));
            dagRender->addLineToVbuffer(*contoursBuf, pt3 + ofs, pt4 + ofs, E3DCOLOR(200, 0, 0, 255));
            dagRender->addLineToVbuffer(*contoursBuf, pt4 + ofs, pt1 + ofs, E3DCOLOR(200, 0, 0, 255));

            dagRender->addLineToVbuffer(*contoursBuf, pt1, pt1 + ofs, E3DCOLOR(200, 0, 0, 255));
            dagRender->addLineToVbuffer(*contoursBuf, pt2, pt2 + ofs, E3DCOLOR(200, 0, 0, 255));
            dagRender->addLineToVbuffer(*contoursBuf, pt3, pt3 + ofs, E3DCOLOR(200, 0, 0, 255));
            dagRender->addLineToVbuffer(*contoursBuf, pt4, pt4 + ofs, E3DCOLOR(200, 0, 0, 255));

            continue;
          }
          dagRender->addLineToVbuffer(*contoursBuf, edge.first, edge.second, E3DCOLOR(200, 200, 0, 255));
        }
        endDebugPrimitivesVbufferCache(contoursBuf);
      }
    }
  }

  if (showExpotedObstacles && obstaclesDebugList.size())
  {
    if (dagRender->isLinesVbufferValid(*obstaclesBuf))
    {
      renderDebugPrimitives(obstaclesBuf);
    }
    else
    {
      beginDebugPrimitivesVbufferCache(obstaclesBuf);
      for (const NavMeshObstacle &obstacle : obstaclesDebugList)
      {
        Point3 boxWidth = obstacle.box.width();
        // Beware that yAngle is incorrect here, it's mirrored, because matrix to euler function does it in that way
        // (non-mathemathical)
        TMatrix boxBasis;
        Point3 ax = Point3(cosf(obstacle.yAngle), 0, -sinf(obstacle.yAngle)) * boxWidth.x;
        Point3 ay = Point3(0, boxWidth.y, 0);
        Point3 az = Point3(sinf(obstacle.yAngle), 0, cosf(obstacle.yAngle)) * boxWidth.z;
        Point3 pos = obstacle.box.center() - 0.5f * (ax + ay + az);
        dagRender->addSolidBoxToVbuffer(*obstaclesBuf, pos, ax, ay, az, E3DCOLOR_MAKE(255, 0, 255, 50));
        dagRender->addBoxToVbuffer(*obstaclesBuf, pos, ax, ay, az, E3DCOLOR_MAKE(255, 0, 255, 255));
      }
      endDebugPrimitivesVbufferCache(obstaclesBuf);
    }
  }

  if (!exportedNavMeshLoadDone && !pathfinder::isLoaded())
  {
    load_pathfinder(this, shownExportedNavMeshIdx, navMeshProps[shownExportedNavMeshIdx]);
    exportedNavMeshLoadDone = true;
  }

  if (!exportedCoversDebugListDone && !coversDebugList.size())
  {
    FullFileLoadCB fcrd(DAGORED2->getPluginFilePath(this, "covers.PC.bin"));
    if (fcrd.fileHandle)
    {
      fcrd.beginBlock();
      if (!covers::load(fcrd, coversDebugList))
        DAEDITOR3.conError("failed to load covers from %s", fcrd.getTargetName());
      else
        covers::build(128, 8, coversDebugList, coversDebugScene);
      fcrd.endBlock();
    }
    exportedCoversDebugListDone = true;
  }

  const String filePostfix =
    (shownExportedNavMeshIdx == 0 ? String(".PC.bin") : String(50, "_%d.PC.bin", shownExportedNavMeshIdx + 1));
  if (!exportedEdgesDebugListDone && !edgesDebugList.size())
  {
    FullFileLoadCB fcrd(DAGORED2->getPluginFilePath(this, String("debug_edges") + filePostfix));
    if (fcrd.fileHandle)
    {
      fcrd.beginBlock();
      load_debug_edges(fcrd, edgesDebugList);
      fcrd.endBlock();
    }
    exportedEdgesDebugListDone = true;
  }

  if (!exportedObstaclesLoadDone && obstaclesDebugList.empty())
  {
    FullFileLoadCB fcrd(DAGORED2->getPluginFilePath(this, String("debug_obstacles") + filePostfix));
    if (fcrd.fileHandle)
    {
      fcrd.beginBlock();
      load_debug_obstacles(fcrd, obstaclesDebugList);
      fcrd.endBlock();
    }
    exportedObstaclesLoadDone = true;
  }
}

// wrapper for functions used in gameLibs/pathFinder
void draw_cached_debug_line(const Point3 &p0, const Point3 &p1, E3DCOLOR c)
{
  dagRender->addLineToVbuffer(*active_debug_lines_vbuf, p0, p1, c);
}


void set_cached_debug_lines_wtm(const TMatrix &tm) { dagRender->setVbufferTm(*active_debug_lines_vbuf, tm); }

void draw_cached_debug_box(const Point3 &p, const Point3 &ax, const Point3 &ay, const Point3 &az, E3DCOLOR color)
{
  dagRender->addBoxToVbuffer(*active_debug_lines_vbuf, p, ax, ay, az, color);
}

void draw_cached_debug_box(const BBox3 &box, E3DCOLOR color) { dagRender->addBoxToVbuffer(*active_debug_lines_vbuf, box, color); }

void draw_cached_debug_solid_triangle(const Point3 p[3], E3DCOLOR color)
{
  dagRender->addTriangleToVbuffer(*active_debug_lines_vbuf, p, color);
}
