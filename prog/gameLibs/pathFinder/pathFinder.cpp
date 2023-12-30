#include <pathFinder/pathFinder.h>
#include <pathFinder/customNav.h>
#include <pathFinder/tileCache.h>
#include <detourCommon.h>
#include <detourNavMeshBuilder.h>
#include <detourNavMesh.h>
#include <detourNavMeshQuery.h>
#include <string.h>
#include <startup/dag_globalSettings.h>
#include <perfMon/dag_cpuFreq.h>
#include <math/dag_Point3.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_lzmaIo.h>
#include <ioSys/dag_zstdIo.h>
#include <gameMath/traceUtils.h>
#include <gameMath/mathUtils.h>
#include <rendInst/rendInstGen.h>
#include <landMesh/lmeshManager.h>
#include <heightmap/heightmapHandler.h>
#include <util/dag_hierBitMap2d.h>
#include <math/dag_bezierPrec.h>
#include <math/dag_mathUtils.h>
#include <image/dag_tga.h>
#include <image/dag_texPixel.h>
#include <debug/dag_debug3d.h>
#include <memory/dag_framemem.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/tuple.h>
#include <math/random/dag_random.h>

#include <detourPathCorridor.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_relocatableFixedVector.h>
#include <generic/dag_span.h>

#if _TARGET_C1 | _TARGET_C2






#endif

namespace pathfinder
{
struct DtFreer
{
  void operator()(void *ptr) { dtFree(ptr); }
};

static NavParams navMeshNavParams[NMS_COUNT];
static String navMeshKinds[NMS_COUNT];

static NavParams &get_nav_params(int nav_mesh_idx)
{
  G_ASSERT((unsigned int)(nav_mesh_idx) < NMS_COUNT);
  return navMeshNavParams[nav_mesh_idx];
}
const char *get_nav_mesh_kind(int nav_mesh_idx)
{
  G_ASSERT((unsigned int)(nav_mesh_idx) < NMS_COUNT);
  return navMeshKinds[nav_mesh_idx].c_str();
}

static const int max_path_size = 256;
static const float INVALID_PATH_DISTANCE = -1.0f;

struct NavMeshData
{
#if _TARGET_C1 | _TARGET_C2

#else
  using NavDataPtr = eastl::unique_ptr<uint8_t, DtFreer>;
#endif

  dtTileCacheAlloc tcAllocator;
  TileCacheCompressor tcComp;
  TileCacheMeshProcess tcMeshProc;
  dtNavMesh *navMesh = nullptr;
  dtNavMeshQuery *navQuery = nullptr;
  NavDataPtr navData;
};

static NavMeshData navMeshData[NMS_COUNT];
static dtNavMesh *&navMesh = navMeshData[NM_MAIN].navMesh;
static dtNavMeshQuery *&navQuery = navMeshData[NM_MAIN].navQuery;

static Tab<Point3> pathForDebug;
static Tab<BBox3> navDebugBoxes;
static int lastNavMeshDebugIdx = NM_MAIN;

static NavMeshData &get_nav_mesh_data(int nav_mesh_idx)
{
  G_ASSERT((unsigned int)(nav_mesh_idx) < NMS_COUNT);
  return navMeshData[nav_mesh_idx];
}

dtNavMesh *get_nav_mesh_ptr(int nav_mesh_idx) { return get_nav_mesh_data(nav_mesh_idx).navMesh; }
dtNavMesh *getNavMeshPtr() { return navMesh; }

static void *dt_alloc(size_t size, dtAllocHint hint)
{
  return hint == DT_ALLOC_TEMP ? framemem_ptr()->alloc(size) : memalloc(size, tmpmem);
}
static void dt_free(void *ptr) { framemem_ptr()->free(ptr); } // framemem correctly frees global memory
static struct DtSetDagorMem
{
  DtSetDagorMem() { dtAllocSetCustom(dt_alloc, dt_free); }
} dt_set_dagormem;

class NavQueryFilter : public dtQueryFilter
{
public:
  explicit NavQueryFilter(const dtNavMesh *nav_mesh, const NavParams &nav_params, const CustomNav *custom_nav,
    float max_jump_up_height = FLT_MAX, bool for_optimize = false, const ska::flat_hash_map<dtPolyRef, float> &cost_addition = {}) :
    customNav(custom_nav),
    navMesh(nav_mesh),
    navParams(nav_params),
    maxJumpUpHeight(max_jump_up_height),
    forOptimize(for_optimize),
    costAddition(cost_addition)
  {
    // We can't just cut out obstacles from navmesh, we must use separate areaId
    // with very high area cost. The reason is that obstacle may be dynamically created / moved / removed
    // and it may turn out that a starting point of a nav path is now inside obstacle box and thus
    // updated path will go straight through an obstacle. But what we really want is for nav path to
    // leave obstacle box in a shortest path possible and detour it afterwards. The easiest way to achieve this
    // is making obstacle area walkable, but with extremely high cost.
    setAreaCost(POLYAREA_OBSTACLE, 10000.f);
  }

  explicit NavQueryFilter(const CustomNav *custom_nav, float max_jump_up_height = FLT_MAX, bool for_optimize = false,
    const ska::flat_hash_map<dtPolyRef, float> &cost_addition = {}) :
    NavQueryFilter(get_nav_mesh_ptr(NM_MAIN), get_nav_params(NM_MAIN), custom_nav, max_jump_up_height, for_optimize, cost_addition)
  {}

  template <typename AreaCosts>
  void setAreasCost(const AreaCosts &areaCost)
  {
    for (const auto &pair : areaCost)
      setAreaCost(uint8_t(pair.x), pair.y);
  }

  float getCost(const float *pa, const float *pb, const dtPolyRef /*prevRef*/, const dtMeshTile * /*prevTile*/,
    const dtPoly * /*prevPoly*/, const dtPolyRef curRef, const dtMeshTile *curTile, const dtPoly *curPoly, const dtPolyRef nextRef,
    const dtMeshTile * /*nextTile*/, const dtPoly * /*nextPoly*/) const override
  {
    Point3 tmpVerts[2];
    uint8_t areaId = curPoly->getArea();
    float weight = getAreaCost(areaId);
    float len = 0;
    if (areaId == POLYAREA_JUMP)
    {
      if (!navParams.enable_jumps)
        return 1e6f;
      // This is a jumplink, we don't want to use original edge length, since
      // it doesn't reflect true distance in this case, we want to use jump link's
      // length (and points) instead (projected onto XZ plane).
      if (curPoly->vertCount == (sizeof(tmpVerts) / sizeof(tmpVerts[0])))
      {
        for (int k = 0; k < curPoly->vertCount; ++k)
          dtVcopy(&tmpVerts[k].x, &curTile->verts[curPoly->verts[k] * 3]);
        float minY = min(tmpVerts[0].y, tmpVerts[1].y);
        tmpVerts[0].y = minY;
        tmpVerts[1].y = minY;
        // Check if original edge goes way down, if so, this is a "jump down"
        // link, i.e. a cheaper one.
        bool isJumpDown = pa[1] - pb[1] > navParams.jump_down_threshold;
        bool canJumpUp = pb[1] - pa[1] < maxJumpUpHeight;
        pa = &tmpVerts[0].x;
        pb = &tmpVerts[1].x;
        len = dtVdist(pa, pb);
        weight = isJumpDown ? navParams.jump_down_weight : canJumpUp ? navParams.jump_weight : 10000.f * safeinv(len);
      }
      else
        logerr_ctx("getCost for poly at (%f, %f, %f) - bad jump link", pa[0], pa[1], pa[2]);
    }
    else
    {
      len = dtVdist(pa, pb);
    }

    if (customNav && (len > 1e-3))
    {
      float invLen = fastinv(len);
      Point3 p1(pa, Point3::CTOR_FROM_PTR);
      Point3 p2(pb, Point3::CTOR_FROM_PTR);
      Point3 normDir = (p2 - p1) * invLen;
      uint32_t curTileId = navMesh->decodePolyIdTile(curRef);
      uint32_t nextTileId = navMesh->decodePolyIdTile(nextRef);
      bool optimize = forOptimize;
      float w = customNav->getWeight(curTileId, p1, p2, normDir, len, invLen, optimize);
      if ((curTileId != nextTileId) && nextTileId)
        w = max(w, customNav->getWeight(nextTileId, p1, p2, normDir, len, invLen, optimize));
      // When using nav meshes with not-all-1 weights with corridor.optimizePathTopology
      // it's important to understand that local area search of optimizePathTopology
      // is basically A* that is limited by the number of iterations and collected polygons,
      // thus, it can mistakenly find suboptimal path that lies through areas with higher cost.
      // The only way to avoid this is to exclude areas that have weight above certain threshold
      // while calling optimizePathTopology. It seems natural to exclude all custom nav areas
      // that explicitly disallow optimizing through them.
      if (forOptimize && !optimize)
        return -1.0f; // 'detour' had been patched for this!
      weight = max(weight, w);
    }

    if (auto it = costAddition.find(nextRef); it != costAddition.end())
      weight += it->second;

    return len * weight;
  }

private:
  const CustomNav *customNav;
  const dtNavMesh *navMesh;
  const NavParams &navParams;
  float maxJumpUpHeight;
  bool forOptimize;
  ska::flat_hash_map<dtPolyRef, float> costAddition;
};

class CheckPathFilter : public dtQueryFilter
{
public:
  explicit CheckPathFilter(const NavParams &nav_params, float max_jump_up_height = FLT_MAX) :
    navParams(nav_params), maxJumpUpHeight(max_jump_up_height)
  {
    setAreaCost(POLYAREA_GROUND, 1.f);
    setAreaCost(POLYAREA_OBSTACLE, 1.f);
    setAreaCost(POLYAREA_JUMP, 1.f);
  }

  float getCost(const float *pa, const float *pb, const dtPolyRef /*prevRef*/, const dtMeshTile * /*prevTile*/,
    const dtPoly * /*prevPoly*/, const dtPolyRef /*curRef*/, const dtMeshTile * /*curTile*/, const dtPoly *curPoly,
    const dtPolyRef /*nextRef*/, const dtMeshTile * /*nextTile*/, const dtPoly * /*nextPoly*/) const override
  {
    uint8_t areaId = curPoly->getArea();
    float weight = getAreaCost(areaId);
    if (areaId == POLYAREA_JUMP)
    {
      if (!navParams.enable_jumps)
        return 1e6f;
      if (curPoly->vertCount == 2)
      {
        // Check if original edge goes way down, if so, this is a "jump down"
        // link, i.e. a cheaper one.
        bool canJumpUp = pb[1] - pa[1] < maxJumpUpHeight;
        weight = canJumpUp ? weight : 10000.f;
      }
      else
        logerr_ctx("getCost for poly at (%f, %f, %f) - bad jump link", pa[0], pa[1], pa[2]);
    }
    return weight;
  }

private:
  const NavParams &navParams;
  float maxJumpUpHeight;
};

void clear_nav_mesh(int nav_mesh_idx, bool clear_nav_data)
{
  NavMeshData &nmData = get_nav_mesh_data(nav_mesh_idx);
  nmData.tcMeshProc.setNavMesh(nullptr);
  nmData.tcMeshProc.setNavMeshQuery(nullptr);
  nmData.tcMeshProc.setTileCache(nullptr);
  nmData.tcMeshProc.setLadders(nullptr);
  if (nmData.navMesh)
    dtFreeNavMesh(nmData.navMesh);
  nmData.navMesh = NULL;
  if (clear_nav_data)
    nmData.navData.reset();
  if (nmData.navQuery)
    dtFreeNavMeshQuery(nmData.navQuery);
  nmData.navQuery = NULL;
  if (nav_mesh_idx == NM_MAIN)
    tilecache_cleanup();
}

void clear(bool clear_nav_data)
{
  for (int i = 0; i < NMS_COUNT; ++i)
    clear_nav_mesh(i, clear_nav_data);
  clear_and_shrink(pathForDebug);
  clear_and_shrink(navDebugBoxes);
}

static bool loadNavMeshChunked(IGenLoad &crd, NavMeshType type, tile_check_cb_t tile_check_cb, Tab<uint8_t> &out_buff)
{
  G_ASSERT(type == NMT_TILECACHED);
  G_UNUSED(type);
  // Reserve large buffer to avoid frequent re-allocations. 15 megs is pretty good.
  out_buff.reserve(15 * 1024 * 1024);
  uint32_t curOff;
#define READ_TYPE(typ, name)                \
  curOff = out_buff.size();                 \
  out_buff.resize(curOff + sizeof(typ));    \
  crd.read(&out_buff[curOff], sizeof(typ)); \
  typ name;                                 \
  memcpy(&name, &out_buff[curOff], sizeof(typ));
#define READ_BYTES(cnt)            \
  curOff = out_buff.size();        \
  out_buff.resize(curOff + (cnt)); \
  crd.read(&out_buff[curOff], (cnt));
  READ_TYPE(dtNavMeshParams, nmParams);
  READ_TYPE(dtTileCacheParams, tcParams);
  G_UNUSED(nmParams);
  READ_TYPE(int, numTiles);
  uint32_t numTilesOff = curOff;
  READ_TYPE(int, numTCTiles);
  uint32_t numTCTilesOff = curOff;
  READ_TYPE(int, numObsResNameHashes);
  int actualNumTiles = 0;
  int actualNumTCTiles = 0;
  for (int i = 0; i < numTiles; ++i)
  {
    uint32_t tileOff = out_buff.size();
    READ_TYPE(dtTileRef, tileRef);
    READ_TYPE(int, tileDataSize);
    if (!tileRef || !tileDataSize)
      break;
    uint32_t tileDataOff = out_buff.size();
    READ_BYTES(sizeof(dtMeshHeader));
#if _TARGET_BE
    if (!dtNavMeshHeaderSwapEndian(&out_buff[tileDataOff], sizeof(dtMeshHeader)))
    {
      logerr_ctx("dtNavMeshHeaderSwapEndian fails");
      return false;
    }
    if (!dtNavMeshDataSwapEndian(&out_buff[tileDataOff], sizeof(dtMeshHeader)))
    {
      logerr_ctx("dtNavMeshDataSwapEndian fails");
      return false;
    }
#endif
    const dtMeshHeader *header = (const dtMeshHeader *)&out_buff[tileDataOff];
    if (!tile_check_cb || tile_check_cb(BBox3(Point3(header->bmin[0], header->bmin[1], header->bmin[2]),
                            Point3(header->bmax[0], header->bmax[1], header->bmax[2]))))
    {
      READ_BYTES(tileDataSize - sizeof(dtMeshHeader));
      ++actualNumTiles;
    }
    else
    {
      crd.seekrel(tileDataSize - sizeof(dtMeshHeader));
      out_buff.resize(tileOff);
    }
  }
  for (int i = 0; i < numTCTiles; ++i)
  {
    uint32_t tileOff = out_buff.size();
    READ_TYPE(int, tileDataSize);
    if (!tileDataSize)
      break;
    uint32_t tileDataOff = out_buff.size();
    READ_BYTES(sizeof(dtTileCacheLayerHeader));
#if _TARGET_BE
    if (!dtNavMeshHeaderSwapEndian(&out_buff[tileDataOff], sizeof(dtTileCacheLayerHeader)))
    {
      logerr_ctx("dtNavMeshHeaderSwapEndian fails");
      return false;
    }
    if (!dtNavMeshDataSwapEndian(&out_buff[tileDataOff], sizeof(dtTileCacheLayerHeader)))
    {
      logerr_ctx("dtNavMeshDataSwapEndian fails");
      return false;
    }
#endif
    const dtTileCacheLayerHeader *header = (const dtTileCacheLayerHeader *)&out_buff[tileDataOff];
    BBox3 aabb = tilecache_calc_tile_bounds(&tcParams, header);
    if (!tile_check_cb || tile_check_cb(aabb))
    {
      READ_BYTES(tileDataSize - sizeof(dtTileCacheLayerHeader));
      ++actualNumTCTiles;
    }
    else
    {
      crd.seekrel(tileDataSize - sizeof(dtTileCacheLayerHeader));
      out_buff.resize(tileOff);
    }
  }
  READ_BYTES(numObsResNameHashes * sizeof(uint32_t));
#undef READ_TYPE
#undef READ_BYTES
  *(int *)&out_buff[numTilesOff] = actualNumTiles;
  *(int *)&out_buff[numTCTilesOff] = actualNumTCTiles;
  return true;
}

static void loadNavMeshBucketFormat(TileCacheCompressor &tc_comp, IGenLoad &crd, NavMeshType type, tile_check_cb_t tile_check_cb,
  Tab<uint8_t> &out_buff)
{
  G_ASSERT(type == NMT_TILECACHED);
  G_UNUSED(type);
  dtNavMeshParams nmParams;
  dtTileCacheParams tcParams;

  using BlockType = StaticTab<uint8_t, 8 * 1024 * 1024>;
  dag::RelocatableFixedVector<BlockType *, 64, true /*overflow*/> tcTilesBuff;
  dag::RelocatableFixedVector<BlockType *, 64, true /*overflow*/> tilesBuff;
  tilesBuff.push_back(new BlockType);
  tcTilesBuff.push_back(new BlockType);
  out_buff.reserve(4 * 1024 * 1024);

  auto writeToBuf = [](decltype(tilesBuff) &buf, const void *data, int size) {
    do
    {
      BlockType *last = buf.back();
      int curSize = last->size();
      int freeSize = last->capacity() - curSize;
      int writeSize = min(freeSize, size);
      last->resize(curSize + writeSize);
      memcpy(last->data() + curSize, data, writeSize);
      (ptrdiff_t &)data += writeSize;
      size -= writeSize;
      if (last->size() == last->capacity())
        buf.push_back(new BlockType);
    } while (size);
  };

  auto getWriteSpace = [](decltype(tilesBuff) &buf, int size) {
    BlockType *last = buf.back();
    int curSize = last->size();
    int freeSize = last->capacity() - curSize;
    int writeSize = min(freeSize, size);
    last->resize(curSize + writeSize);
    if (last->size() == last->capacity())
      buf.push_back(new BlockType);
    return dag::Span<uint8_t>(last->data() + curSize, writeSize);
  };

  crd.read(&nmParams, sizeof(nmParams));
  crd.read(&tcParams, sizeof(tcParams));

  int totalNumTiles = 0;
  int totalNumTCTiles = 0;
  int numObsResNameHashes = crd.readInt();

  Tab<uint8_t> obsResNameHashesBuff(numObsResNameHashes * sizeof(uint32_t));
  crd.read(obsResNameHashesBuff.data(), obsResNameHashesBuff.size());

  int dictSize = crd.readInt();
  Tab<char> dictBuff(dictSize);
  crd.read(dictBuff.data(), dictBuff.size());

  tc_comp.reset(true, dictBuff);

  int numBuckets = crd.readInt();
  const int64_t refTime = ref_time_ticks();
  while (numBuckets-- > 0)
  {
    float x1 = crd.readReal();
    float y1 = crd.readReal();
    float x2 = crd.readReal();
    float y2 = crd.readReal();
    BBox3 aabb(Point3(x1, -VERY_BIG_NUMBER, y1), Point3(x2, VERY_BIG_NUMBER, y2));

    crd.beginBlock();

    if (tile_check_cb && !tile_check_cb(aabb))
    {
      // Skip an entire bucket.
      crd.endBlock();
      continue;
    }

    int numTCTiles = crd.readInt();
    while (numTCTiles-- > 0)
    {
      uint32_t tileDataSz = crd.readInt();
      dtTileCacheLayerHeader header;
      crd.read(&header, sizeof header);

      if (tile_check_cb && !tile_check_cb(tilecache_calc_tile_bounds(&tcParams, &header)))
      {
        crd.seekrel(tileDataSz - sizeof(header));
        continue;
      }

      writeToBuf(tcTilesBuff, &tileDataSz, sizeof(tileDataSz));
      writeToBuf(tcTilesBuff, &header, sizeof(header));
      tileDataSz -= sizeof(header);
      while (tileDataSz)
      {
        dag::Span<uint8_t> sp = getWriteSpace(tcTilesBuff, tileDataSz);
        crd.read(sp.data(), sp.size());
        tileDataSz -= sp.size();
      }
      ++totalNumTCTiles;
    }

    out_buff.resize(0);
    out_buff.resize(crd.readInt());
    ZstdLoadCB loader(crd, crd.getBlockRest());
    loader.read(out_buff.data(), out_buff.size());
    loader.ceaseReading();

    unsigned char *curPtr = out_buff.data();
#define INIT_BY_PTR(typ, name) \
  typ *name = (typ *)curPtr;   \
  curPtr += sizeof(typ);
    INIT_BY_PTR(int, numTiles);
    for (int i = 0; i < *numTiles; ++i)
    {
      unsigned char *tmpPtr = curPtr;
      INIT_BY_PTR(dtTileRef, tileRef);
      INIT_BY_PTR(int, tileDataSize);
      G_UNUSED(tileRef);
      const dtMeshHeader *header = (const dtMeshHeader *)curPtr;
      curPtr += *tileDataSize;
      if (!tile_check_cb || tile_check_cb(BBox3(Point3(header->bmin[0], header->bmin[1], header->bmin[2]),
                              Point3(header->bmax[0], header->bmax[1], header->bmax[2]))))
      {
        writeToBuf(tilesBuff, tmpPtr, curPtr - tmpPtr);
        ++totalNumTiles;
      }
    }
#undef INIT_BY_PTR

    crd.endBlock();
  }
  const int timeUsec = get_time_usec(refTime);
  const uint32_t blockSize = BlockType::static_size / (1024 * 1024);
  debug("perf: navmesh load time: %.2f sec", timeUsec / 1000000.0);
  debug("tiles loaded: %d; used blocks: %u of %umb", totalNumTiles, tilesBuff.size() + tcTilesBuff.size(), blockSize);

  // Write out tightly packed navmesh buffer.
  out_buff.resize(0);
  uint64_t reqCap =
    sizeof(nmParams) + sizeof(tcParams) + sizeof(totalNumTiles) + sizeof(totalNumTCTiles) + sizeof(numObsResNameHashes);
  for (const BlockType *block : tilesBuff)
    reqCap += block->size();
  for (const BlockType *block : tcTilesBuff)
    reqCap += block->size();
  reqCap += obsResNameHashesBuff.size();
  out_buff.reserve(reqCap);
  append_items(out_buff, sizeof(nmParams), (const uint8_t *)&nmParams);
  append_items(out_buff, sizeof(tcParams), (const uint8_t *)&tcParams);
  append_items(out_buff, sizeof(totalNumTiles), (const uint8_t *)&totalNumTiles);
  append_items(out_buff, sizeof(totalNumTCTiles), (const uint8_t *)&totalNumTCTiles);
  append_items(out_buff, sizeof(numObsResNameHashes), (const uint8_t *)&numObsResNameHashes);
  for (const BlockType *block : tilesBuff)
  {
    append_items(out_buff, block->size(), block->data());
    delete block;
  }
  for (const BlockType *block : tcTilesBuff)
  {
    append_items(out_buff, block->size(), block->data());
    delete block;
  }
  append_items(out_buff, obsResNameHashesBuff.size(), obsResNameHashesBuff.data());
  G_ASSERT(reqCap == out_buff.size());
}

void initWeights(const DataBlock *navQueryFilterWeightsBlk) { init_weights_ex(NM_MAIN, navQueryFilterWeightsBlk); }

void init_weights_ex(int nav_mesh_idx, const DataBlock *navQueryFilterWeightsBlk)
{
  NavParams &navParams = get_nav_params(nav_mesh_idx);
  navParams.enable_jumps = (get_nav_mesh_kind(nav_mesh_idx)[0] == '\0');
  if (navQueryFilterWeightsBlk)
  {
    navParams.enable_jumps = navQueryFilterWeightsBlk->getBool("enableJumps", navParams.enable_jumps);
    navParams.max_path_weight = navQueryFilterWeightsBlk->getReal("maxPathWeight", FLT_MAX);
    navParams.jump_weight = navQueryFilterWeightsBlk->getReal("jumpWeight", 40.f);
    navParams.jump_down_weight = navQueryFilterWeightsBlk->getReal("jumpDownWeight", 20.f);
    navParams.jump_down_threshold = navQueryFilterWeightsBlk->getReal("jumpDownThreshold", 0.4f);
  }
}

template <class T>
struct PlacementFreer
{
  void operator()(void *ptr)
  {
    if (ptr)
    {
      ((T *)ptr)->~T();
    }
  }
};

void tilecache_start_ladders(const scene::TiledScene *ladders) { get_nav_mesh_data(NM_MAIN).tcMeshProc.setLadders(ladders); }

static bool load_nav_mesh_orig(int nav_mesh_idx, unsigned char *curPtr, int patchedTilesSize, const char *patchNavMeshFileName,
  int extra_tiles)
{
  NavMeshData &nmData = get_nav_mesh_data(nav_mesh_idx);
  const bool tileCacheNeeded = nav_mesh_idx == NM_MAIN;
#define INIT_BY_PTR(typ, name) \
  typ *name = (typ *)curPtr;   \
  curPtr += sizeof(typ);
  INIT_BY_PTR(dtNavMeshParams, nmParams);
  INIT_BY_PTR(dtTileCacheParams, tcParams);
  INIT_BY_PTR(int, numTiles);
  INIT_BY_PTR(int, numTCTiles);
  INIT_BY_PTR(int, numObsResNameHashes);
  nmParams->maxTiles += extra_tiles;
  if (dtStatusFailed(nmData.navMesh->init(nmParams, false)))
  {
    logerr_ctx("Could not init tiled Detour navmesh");
    return false;
  }
  dtTileCache *tc = nullptr;
  if (tileCacheNeeded)
  {
    tc = dtAllocTileCache();
    if (!tc)
    {
      logerr_ctx("dtAllocTileCache fails");
      return false;
    }
    nmData.tcMeshProc.setNavMesh(nmData.navMesh);
    nmData.tcMeshProc.setNavMeshQuery(nullptr);
    nmData.tcMeshProc.setTileCache(tc);
    tcParams->maxTiles += extra_tiles;
    if (dtStatusFailed(tc->init(tcParams, &nmData.tcAllocator, &nmData.tcComp, &nmData.tcMeshProc)))
    {
      logerr_ctx("Could not init tiled Detour tile cache");
      dtFreeTileCache(tc);
      return false;
    }
  }
  for (int i = 0; i < *numTiles; ++i)
  {
    INIT_BY_PTR(dtTileRef, tileRef);
    INIT_BY_PTR(int, tileDataSize);
    if (!*tileRef || !*tileDataSize)
      break;
    if (dtStatusFailed(nmData.navMesh->addTile(curPtr, *tileDataSize, 0, *tileRef, nullptr)))
    {
      logerr_ctx("navMesh->addTile fails");
      return false;
    }
    curPtr += *tileDataSize;
  }
  ska::flat_hash_set<uint32_t> obstacleResNameHashes;
  if (tileCacheNeeded && tc)
  {
    for (int i = 0; i < *numTCTiles; ++i)
    {
      INIT_BY_PTR(int, tileDataSize);
      if (!*tileDataSize)
        break;
      dtCompressedTileRef res = 0;
      if (dtStatusFailed(tc->addTile(curPtr, *tileDataSize, 0, &res)))
      {
        logerr_ctx("tc->addTile fails");
        dtFreeTileCache(tc);
        return false;
      }
      curPtr += *tileDataSize;
    }
    for (int i = 0; i < *numObsResNameHashes; ++i)
    {
      INIT_BY_PTR(uint32_t, hash);
      if (!*hash)
        break;
      obstacleResNameHashes.insert(*hash);
    }
#undef INIT_BY_PTR
  }
  if (patchedTilesSize > 0)
  {
    nmData.navMesh->reconstructFreeList();
    patchedNavMesh_loadFromFile(patchNavMeshFileName, tc, curPtr, obstacleResNameHashes);
  }
  nmData.navMesh->reconstructFreeList();
  tilecache_init(tc, obstacleResNameHashes);

  return true;
}

static bool init_nav_query(int nav_mesh_idx)
{
  NavMeshData &nmData = get_nav_mesh_data(nav_mesh_idx);
  nmData.navQuery = dtAllocNavMeshQuery();
  if (!nmData.navQuery)
  {
    logerr_ctx("Could not create NavMeshQuery");
    return false;
  }

  if (dtStatusFailed(nmData.navQuery->init(nmData.navMesh, 4096)))
  {
    logerr_ctx("Could not init Detour navmesh query");
    return false;
  }

  nmData.tcMeshProc.setNavMeshQuery(nmData.navQuery);
  return true;
}

static bool create_nav_mesh(int nav_mesh_idx)
{
  NavMeshData &nmData = get_nav_mesh_data(nav_mesh_idx);
  nmData.tcComp.reset(false);

  nmData.navMesh = dtAllocNavMesh();
  if (!nmData.navMesh)
  {
    logerr_ctx("Could not create Detour navmesh");
    return false;
  }

  return true;
}

bool reload_nav_mesh(int extra_tiles)
{
  clear_nav_mesh(NM_MAIN, false);

  if (!create_nav_mesh(NM_MAIN))
    return false;

  if (!load_nav_mesh_orig(NM_MAIN, get_nav_mesh_data(NM_MAIN).navData.get(), 0, "", extra_tiles))
    return false;

  if (!init_nav_query(NM_MAIN))
    return false;

  tilecache_restart();

  return true;
}

bool loadNavMesh(IGenLoad &_crd, NavMeshType type, tile_check_cb_t tile_check_cb, const char *patchNavMeshFileName)
{
  return load_nav_mesh_ex(NM_MAIN, get_nav_mesh_kind(NM_MAIN), _crd, type, tile_check_cb, patchNavMeshFileName);
}

bool load_nav_mesh_ex(int nav_mesh_idx, const char *kind, IGenLoad &_crd, NavMeshType type, tile_check_cb_t tile_check_cb,
  const char *patchNavMeshFileName)
{
  navMeshKinds[nav_mesh_idx] = String(kind);
  NavMeshData &nmData = get_nav_mesh_data(nav_mesh_idx);

  clear_nav_mesh(nav_mesh_idx);

  if (!create_nav_mesh(nav_mesh_idx))
    return false;

  unsigned navDataSize = _crd.readInt();
  bool isBucketFormat = (type == NMT_TILECACHED) && (navDataSize == 0x80000000);

  // Tile cached navmesh and we have tile check callback, probably we can cut out a lot of
  // the navmesh, this means we don't need to make huge allocations, just read out tiles as needed,
  // check them, copy them to temp storage and finally use dtAlloc to create final navmesh.
  bool useChunkedRead = ((type == NMT_TILECACHED) && tile_check_cb) || isBucketFormat;

  Tab<uint8_t> chunkedReadBuff;
  eastl::unique_ptr<IGenLoad, PlacementFreer<IGenLoad>> crd;
  if (isBucketFormat)
  {
    // TODO: Bucket format for tile cached navmesh should be the only one,
    // remove loadNavMeshChunked later.
    loadNavMeshBucketFormat(nmData.tcComp, _crd, type, tile_check_cb, chunkedReadBuff);
    navDataSize = chunkedReadBuff.size();
  }
  else
  {
    unsigned fmt = 0;
    if ((navDataSize & 0xC0000000) == 0x40000000)
    {
      fmt = 1;
      navDataSize &= ~0xC0000000;
    }
    crd.reset((fmt == 1) ? (IGenLoad *)new (alloca(sizeof(ZstdLoadCB)), _NEW_INPLACE) ZstdLoadCB(_crd, _crd.getBlockRest())
                         : (IGenLoad *)new (alloca(sizeof(LzmaLoadCB)), _NEW_INPLACE) LzmaLoadCB(_crd, _crd.getBlockRest()));
    if (useChunkedRead)
    {
      bool res = loadNavMeshChunked(*crd, type, tile_check_cb, chunkedReadBuff);
      crd->ceaseReading();
      if (!res)
        return false;
      navDataSize = chunkedReadBuff.size();
    }
  }

  int patchedExtraTiles = 0;
  uint32_t patchedTilesSize = 0;
  if (type == NMT_TILECACHED && patchNavMeshFileName && *patchNavMeshFileName)
    patchedTilesSize = patchedNavMesh_getFileSizeAndNumTiles(patchNavMeshFileName, patchedExtraTiles);

#if _TARGET_C1 | _TARGET_C2


#else
  eastl::unique_ptr<uint8_t, DtFreer> navDataLocal((uint8_t *)dtAlloc(navDataSize + patchedTilesSize, DT_ALLOC_PERM));
#endif
  if (useChunkedRead)
  {
    memcpy(navDataLocal.get(), chunkedReadBuff.data(), navDataSize);
    clear_and_shrink(chunkedReadBuff);
  }
  else
  {
    crd->read(navDataLocal.get(), navDataSize);
    crd->ceaseReading();
  }
  crd.reset();

  if (type == NMT_SIMPLE)
  {
#if _TARGET_BE
    if (!dtNavMeshHeaderSwapEndian(navDataLocal.get(), navDataSize))
    {
      logerr_ctx("dtNavMeshHeaderSwapEndian fails");
      return false;
    }
    if (!dtNavMeshDataSwapEndian(navDataLocal.get(), navDataSize))
    {
      logerr_ctx("dtNavMeshDataSwapEndian fails");
      return false;
    }
#endif

    if (dtStatusFailed(nmData.navMesh->init(navDataLocal.get(), navDataSize, 0))) // we can remove DT_TILE_FREE_DATA,
                                                                                  // because we keep reference to navData
    {
      logerr_ctx("Could not init Detour navmesh");
      return false;
    }
  }
  else if (type == NMT_TILED)
  {
    unsigned char *curPtr = navDataLocal.get();
#define INIT_BY_PTR(typ, name) \
  typ *name = (typ *)curPtr;   \
  curPtr += sizeof(typ);
    INIT_BY_PTR(dtNavMeshParams, params);
    INIT_BY_PTR(int, numTiles);
    if (dtStatusFailed(nmData.navMesh->init(params, false)))
    {
      logerr_ctx("Could not init tiled Detour navmesh");
      return false;
    }
    for (int i = 0; i < *numTiles; ++i)
    {
      INIT_BY_PTR(dtTileRef, tileRef);
      INIT_BY_PTR(int, tileDataSize);
      if (!*tileRef || !*tileDataSize)
        break;
#if _TARGET_BE
      if (!dtNavMeshHeaderSwapEndian(curPtr, *tileDataSize))
      {
        logerr_ctx("dtNavMeshHeaderSwapEndian fails");
        return false;
      }
      if (!dtNavMeshDataSwapEndian(curPtr, *tileDataSize))
      {
        logerr_ctx("dtNavMeshDataSwapEndian fails");
        return false;
      }
#endif
      const dtMeshHeader *header = (const dtMeshHeader *)curPtr;
      if (!tile_check_cb || tile_check_cb(BBox3(Point3(header->bmin[0], header->bmin[1], header->bmin[2]),
                              Point3(header->bmax[0], header->bmax[1], header->bmax[2]))))
      {
        if (dtStatusFailed(nmData.navMesh->addTile(curPtr, *tileDataSize, 0, *tileRef, nullptr)))
        {
          logerr_ctx("navMesh->addTile fails");
          return false;
        }
      }
      curPtr += *tileDataSize;
    }
    nmData.navMesh->reconstructFreeList();
#undef INIT_BY_PTR
  }
  else
  {
    G_ASSERT(type == NMT_TILECACHED);
    unsigned char *curPtr = navDataLocal.get();
    load_nav_mesh_orig(nav_mesh_idx, curPtr, patchedTilesSize, patchNavMeshFileName, patchedExtraTiles);
  }

  init_nav_query(nav_mesh_idx);

  nmData.navData = eastl::move(navDataLocal);

  return true;
}

inline bool inRange(const float *v1, const float *v2, const float r, const float h)
{
  const float dx = v2[0] - v1[0];
  const float dy = v2[1] - v1[1];
  const float dz = v2[2] - v1[2];
  return (dx * dx + dz * dz) < r * r && fabsf(dy) < h;
}

static int fixupCorridor(dtPolyRef *path, const int npath, const int maxPath, const dtPolyRef *visited, const int nvisited)
{
  int furthestPath = -1;
  int furthestVisited = -1;

  // Find furthest common polygon.
  for (int i = npath - 1; i >= 0; --i)
  {
    bool found = false;
    for (int j = nvisited - 1; j >= 0; --j)
    {
      if (path[i] == visited[j])
      {
        furthestPath = i;
        furthestVisited = j;
        found = true;
      }
    }
    if (found)
      break;
  }

  // If no intersection found just return current path.
  if (furthestPath == -1 || furthestVisited == -1)
    return npath;

  // Concatenate paths.

  // Adjust beginning of the buffer to include the visited.
  const int req = nvisited - furthestVisited;
  const int orig = min(furthestPath + 1, npath);
  int size = max(0, npath - orig);
  if (req + size > maxPath)
    size = maxPath - req;
  if (size)
    memmove(path + req, path + orig, size * sizeof(dtPolyRef));

  // Store visited
  for (int i = 0; i < req; ++i)
    path[i] = visited[(nvisited - 1) - i];

  return req + size;
}

// This function checks if the path has a small U-turn, that is,
// a polygon further in the path is adjacent to the first polygon
// in the path. If that happens, a shortcut is taken.
// This can happen if the target (T) location is at tile boundary,
// and we're (S) approaching it parallel to the tile edge.
// The choice at the vertex can be arbitrary,
//  +---+---+
//  |:::|:::|
//  +-S-+-T-+
//  |:::|   | <-- the step can end up in here, resulting U-turn path.
//  +---+---+
static int fixupShortcuts(dtPolyRef *path, int npath, dtNavMeshQuery *nav_query)
{
  if (npath < 3)
    return npath;

  // Get connected polygons
  static const int maxNeis = 16;
  dtPolyRef neis[maxNeis];
  int nneis = 0;

  const dtMeshTile *tile = 0;
  const dtPoly *poly = 0;
  if (dtStatusFailed(nav_query->getAttachedNavMesh()->getTileAndPolyByRef(path[0], &tile, &poly)))
    return npath;

  for (unsigned int k = poly->firstLink; k != DT_NULL_LINK; k = tile->links[k].next)
  {
    const dtLink *link = &tile->links[k];
    if (link->ref != 0)
    {
      if (nneis < maxNeis)
        neis[nneis++] = link->ref;
    }
  }

  // If any of the neighbour polygons is within the next few polygons
  // in the path, short cut to that polygon directly.
  static const int maxLookAhead = 6;
  int cut = 0;
  for (int i = dtMin(maxLookAhead, npath) - 1; i > 1 && cut == 0; i--)
  {
    for (int j = 0; j < nneis; j++)
    {
      if (path[i] == neis[j])
      {
        cut = i;
        break;
      }
    }
  }
  if (cut > 1)
  {
    int offset = cut - 1;
    npath -= offset;
    for (int i = 1; i < npath; i++)
      path[i] = path[i + offset];
  }

  return npath;
}

bool get_offmesh_connection_end_points(int nav_mesh_idx, dtPolyRef poly, const float *next_pos, Point3 &start, Point3 &end);

static bool getSteerTarget(int nav_mesh_idx, const float *startPos, const float *endPos, const float minTargetDist,
  const dtPolyRef *path, const int pathSize, float *steerPos, unsigned char &steerPosFlag, dtPolyRef &steerPosRef)
{
  NavMeshData &nmData = get_nav_mesh_data(nav_mesh_idx);
  // Find steer target.
  static const int MAX_STEER_POINTS = 3;
  float steerPath[MAX_STEER_POINTS * 3];
  unsigned char steerPathFlags[MAX_STEER_POINTS];
  dtPolyRef steerPathPolys[MAX_STEER_POINTS];
  int nsteerPath = 0;
  if (dtStatusFailed(nmData.navQuery->findStraightPath(startPos, endPos, path, pathSize, steerPath, steerPathFlags, steerPathPolys,
        &nsteerPath, MAX_STEER_POINTS)))
    return false;

  // Find vertex far enough to steer to.
  int ns = 0;
  while (ns < nsteerPath)
  {
    // Stop at Off-Mesh link or when point is further than slop away.
    if ((steerPathFlags[ns] & DT_STRAIGHTPATH_OFFMESH_CONNECTION) || !inRange(&steerPath[ns * 3], startPos, minTargetDist, 1000.0f))
      break;
    uint16_t polyFlags = 0;
    // cut-off almost passed offmesh connections
    if (ns + 1 < nsteerPath && dtStatusSucceed(nmData.navMesh->getPolyFlags(steerPathPolys[ns], &polyFlags)) &&
        (polyFlags & (POLYFLAG_JUMP | POLYFLAG_LADDER)))
    {
      Point3 start, end;
      if (get_offmesh_connection_end_points(nav_mesh_idx, steerPathPolys[ns], &steerPath[(ns + 1) * 3], start, end) &&
          dtVdistSqr(startPos, &end.x) > dtSqr(0.01f))
      {
        steerPathFlags[ns] = steerPathFlags[ns] | DT_STRAIGHTPATH_OFFMESH_CONNECTION;
        break;
      }
    }
    ns++;
  }
  // Failed to find good point to steer to.
  if (ns >= nsteerPath)
    return false;

  dtVcopy(steerPos, &steerPath[ns * 3]);
  steerPos[1] = startPos[1];
  steerPosFlag = steerPathFlags[ns];
  steerPosRef = steerPathPolys[ns];

  return true;
}

bool is_loaded_ex(int nav_mesh_idx) { return get_nav_mesh_data(nav_mesh_idx).navQuery != NULL; }

bool isLoaded() { return navQuery != NULL; }

inline bool find_polyref(int nav_mesh_idx, const Point3 &pos, float horz_dist, int incl_flags, int excl_flags, dtPolyRef &res,
  const CustomNav *custom_nav, float max_vert_dist = FLT_MAX)
{
  const Point3 extents(horz_dist, max_vert_dist, horz_dist);
  NavQueryFilter filter(custom_nav);
  filter.setIncludeFlags(incl_flags);
  filter.setExcludeFlags(excl_flags);
  return dtStatusSucceed(get_nav_mesh_data(nav_mesh_idx).navQuery->findNearestPoly(&pos.x, &extents.x, &filter, &res, nullptr)) &&
         res != 0;
}

inline bool find_end_poly(FindRequest &req, const CustomNav *custom_nav)
{
  NavQueryFilter filter(custom_nav);
  filter.setIncludeFlags(req.includeFlags);
  filter.setExcludeFlags(req.excludeFlags);
  filter.setAreasCost(req.areasCost);
  return dtStatusSucceed(navQuery->findNearestPoly(&req.end.x, &req.extents.x, &filter, &req.endPoly, nullptr)) && req.endPoly != 0;
}

inline FindPathResult find_poly_path(dtNavMeshQuery *nav_query, FindRequest &req, const NavParams &nav_params, dtPolyRef *polys,
  const dtQueryFilter &filter, int max_path)
{
  if (!nav_query->isValidPolyRef(req.startPoly, &filter) &&
      dtStatusFailed(nav_query->findNearestPoly(&req.start.x, &req.extents.x, &filter, &req.startPoly, nullptr)))
    return FPR_FAILED;
  if (!nav_query->isValidPolyRef(req.endPoly, &filter) &&
      dtStatusFailed(nav_query->findNearestPoly(&req.end.x, &req.extents.x, &filter, &req.endPoly, nullptr)))
    return FPR_FAILED;

  dtStatus status = nav_query->findPath(req.startPoly, req.endPoly, &req.start.x, &req.end.x, &filter, polys, &req.numPolys, max_path,
    nav_params.max_path_weight);
  if (!dtStatusSucceed(status))
    return FPR_FAILED;
  return dtStatusDetail(status, DT_PARTIAL_RESULT) ? FPR_PARTIAL : FPR_FULL;
}

static Point3 get_poly_center(const dtMeshTile *tile, const dtPoly *poly);

// gets polygonal line of two "straght" lines polys_first & polys_second
// -> returns smoother polygonal line of three "straght" lines (left part of polys_first + new intermediate part + right part of
// polys_second)
//
// req_first & req_second -- info via which polys_first & polys_second were calculated
// req_first.end ougth to be equal req_second.start
// resulting path is stored in polys_first
inline FindPathResult smooth_concatenate_paths(dtNavMeshQuery *nav_query, FindRequest &req_first, FindRequest req_second,
  const NavParams &nav_params, dtPolyRef *polys_first, const dtPolyRef *polys_second, int offset_first, int offset_second,
  const dtQueryFilter &filter, int max_path)
{
  if (offset_first >= req_first.numPolys || offset_second >= req_second.numPolys)
    return FPR_FAILED;
  if (req_first.numPolys > 0 && req_second.numPolys > 0)
  {
    int numPolysTmp = req_first.numPolys - offset_first - 1;
    FindRequest auxReq = {Point3(), Point3(), req_first.includeFlags, req_first.excludeFlags, req_first.extents,
      req_first.maxJumpUpHeight, 0, polys_first[numPolysTmp], polys_second[offset_second], req_first.areasCost};

    const dtMeshTile *tile = nullptr;
    const dtPoly *poly = nullptr;
    if (dtStatusFailed(nav_query->getAttachedNavMesh()->getTileAndPolyByRef(auxReq.startPoly, &tile, &poly)))
      return FPR_PARTIAL;
    auxReq.start = get_poly_center(tile, poly);
    if (dtStatusFailed(nav_query->getAttachedNavMesh()->getTileAndPolyByRef(auxReq.endPoly, &tile, &poly)))
      return FPR_PARTIAL;
    auxReq.end = get_poly_center(tile, poly);

    dtStatus status = nav_query->findPath(auxReq.startPoly, auxReq.endPoly, &auxReq.start.x, &auxReq.end.x, &filter,
      polys_first + numPolysTmp, &auxReq.numPolys, max_path - numPolysTmp, nav_params.max_path_weight);
    // return polys_first
    if (dtStatusFailed(status))
      return FPR_PARTIAL;
    req_first.numPolys = numPolysTmp + auxReq.numPolys;
    // if curve is partial, it hasn't reached start of second path -> return polys_first + curve
    if (dtStatusDetail(status, DT_PARTIAL_RESULT))
      return FPR_PARTIAL;
    // if curve isn't partial, add polys_second
    for (size_t i = req_first.numPolys, j = offset_second + 1; i < max_path && j < req_second.numPolys; ++i, ++j)
      polys_first[i] = polys_second[j];
    req_first.numPolys = min(req_first.numPolys + req_second.numPolys - offset_second - 1, max_path);
  }
  else
  {
    if (req_second.numPolys > 0)
    {
      int len = min(req_second.numPolys, max_path);
      for (size_t i = 0; i < len; ++i)
        polys_first[i] = polys_second[i];
    }
  }
  return FPR_FULL;
}

inline FindPathResult find_poly_path_curved(dtNavMeshQuery *nav_query, FindRequest &req, const NavParams &nav_params, dtPolyRef *polys,
  dtPolyRef *auxPolys, const dtQueryFilter &filter, int max_path, float max_deflection_angle)
{
  const Point3 pos = req.start;
  const Point3 wishPos = req.end;
  Point3 dir = normalize(wishPos - pos);
  float dirAngle = safe_atan2(dir.z, dir.x);
  float deflectionAngle = rnd_float(-max_deflection_angle, max_deflection_angle);
  const float normalizationCoeff = sqrtf(1 - dir.y * dir.y);
  Point3 offset(cosf(dirAngle + deflectionAngle) * normalizationCoeff, dir.y, sinf(dirAngle + deflectionAngle) * normalizationCoeff);
  const float pathLen = length(wishPos - pos);
  float lenMin = pathLen * 0.125;
  float lenMax = pathLen * 0.333; // should be less than pathLen/2 to avoid overlapping of path's parts
  float deflectionOffsetLen = rnd_float(lenMin, lenMax);
  offset *= deflectionOffsetLen;
  const Point3 start = pos + offset;
  dir = -dir;
  dirAngle += PI;
  offset = Point3(cosf(dirAngle - deflectionAngle) * normalizationCoeff, dir.y, sinf(dirAngle - deflectionAngle) * normalizationCoeff);
  offset *= deflectionOffsetLen;
  const Point3 end = wishPos + offset;

  FindRequest initialSegment = {
    req.start,
    start,
    req.includeFlags,
    req.excludeFlags,
    req.extents,
    req.maxJumpUpHeight,
    0,
    dtPolyRef(),
    dtPolyRef(),
    req.areasCost,
  };
  FindRequest movementSegment = {
    start, end, req.includeFlags, req.excludeFlags, req.extents, req.maxJumpUpHeight, 0, dtPolyRef(), dtPolyRef(), req.areasCost};
  FindRequest finalSegment = {
    end, req.end, req.includeFlags, req.excludeFlags, req.extents, req.maxJumpUpHeight, 0, dtPolyRef(), dtPolyRef(), req.areasCost};

  if (
    dtStatusFailed(nav_query->findNearestPoly(&initialSegment.start.x, &req.extents.x, &filter, &initialSegment.startPoly, nullptr)) ||
    dtStatusFailed(nav_query->findNearestPoly(&initialSegment.end.x, &req.extents.x, &filter, &initialSegment.endPoly, nullptr)))
    return FPR_FAILED;
  dtStatus status = nav_query->findPath(initialSegment.startPoly, initialSegment.endPoly, &initialSegment.start.x,
    &initialSegment.end.x, &filter, polys, &initialSegment.numPolys, max_path, nav_params.max_path_weight);
  if (dtStatusFailed(status))
    return FPR_FAILED;
  req.numPolys = initialSegment.numPolys;
  if (dtStatusDetail(status, DT_PARTIAL_RESULT))
    return FPR_PARTIAL;

  movementSegment.startPoly = initialSegment.endPoly;
  if (dtStatusFailed(
        nav_query->findNearestPoly(&movementSegment.end.x, &req.extents.x, &filter, &movementSegment.endPoly, nullptr))) // -V1051
    return FPR_PARTIAL;

  int offset_start = initialSegment.numPolys / 2;
  dtStatus statusNext =
    nav_query->findPath(movementSegment.startPoly, movementSegment.endPoly, &movementSegment.start.x, &movementSegment.end.x, &filter,
      auxPolys, &movementSegment.numPolys, max_path - (initialSegment.numPolys - offset_start), nav_params.max_path_weight);
  if (dtStatusFailed(statusNext))
    return FPR_PARTIAL;

  int offset_end = movementSegment.numPolys / 8;
  status = smooth_concatenate_paths(nav_query, initialSegment, movementSegment, nav_params, polys, auxPolys, offset_start, offset_end,
    filter, max_path);
  req.numPolys = initialSegment.numPolys;

  if (dtStatusFailed(status) || dtStatusDetail(status, DT_PARTIAL_RESULT) || dtStatusDetail(statusNext, DT_PARTIAL_RESULT))
    return FPR_PARTIAL;

  finalSegment.startPoly = movementSegment.endPoly;
  if (
    dtStatusFailed(nav_query->findNearestPoly(&finalSegment.end.x, &req.extents.x, &filter, &finalSegment.endPoly, nullptr))) // -V1051
    return FPR_PARTIAL;

  offset_start = offset_end;
  statusNext = nav_query->findPath(finalSegment.startPoly, finalSegment.endPoly, &finalSegment.start.x, &finalSegment.end.x, &filter,
    auxPolys, &finalSegment.numPolys, max_path - (initialSegment.numPolys - offset_start), nav_params.max_path_weight);
  if (dtStatusFailed(statusNext))
    return FPR_PARTIAL;

  offset_end = finalSegment.numPolys / 2;
  movementSegment.numPolys = initialSegment.numPolys;
  status = smooth_concatenate_paths(nav_query, movementSegment, finalSegment, nav_params, polys, auxPolys, offset_start, offset_end,
    filter, max_path);
  req.numPolys = movementSegment.numPolys;
  if (dtStatusFailed(status) || dtStatusDetail(status, DT_PARTIAL_RESULT) || dtStatusDetail(statusNext, DT_PARTIAL_RESULT))
    return FPR_PARTIAL;

  return FPR_FULL;
}

FindPathResult find_path_ex(int nav_mesh_idx, Tab<Point3> &path, FindRequest &req, float step_size, float slop,
  const CustomNav *custom_nav)
{
  NavMeshData &nmData = get_nav_mesh_data(nav_mesh_idx);
  const NavParams &navParams = get_nav_params(nav_mesh_idx);
  FindPathResult res = FPR_FAILED;
  clear_and_shrink(path);
  if (nmData.navQuery)
  {
    dtPolyRef polys[max_path_size];
    NavQueryFilter filter(nmData.navMesh, navParams, custom_nav, req.maxJumpUpHeight);
    filter.setIncludeFlags(req.includeFlags);
    filter.setExcludeFlags(req.excludeFlags);
    filter.setAreasCost(req.areasCost);
    res = find_poly_path(nmData.navQuery, req, navParams, polys, filter, max_path_size);
    if (res > FPR_FAILED)
    {
      NavQueryFilter filter(nmData.navMesh, navParams, custom_nav);
      filter.setIncludeFlags(POLYFLAGS_WALK | POLYFLAG_JUMP | POLYFLAG_LADDER);
      filter.setExcludeFlags(0);
      filter.setAreasCost(req.areasCost);
      // Iterate over the path to find smooth path on the detail mesh surface.
      dtPolyRef polys_c[max_path_size];
      memcpy(polys_c, polys, sizeof(dtPolyRef) * req.numPolys);
      int npolys = req.numPolys;

      float iterPos[3], targetPos[3];
      nmData.navQuery->closestPointOnPoly(req.startPoly, &req.start.x, iterPos, nullptr);
      nmData.navQuery->closestPointOnPoly(polys_c[npolys - 1], &req.end.x, targetPos, nullptr);

      static const int MAX_SMOOTH = 2048;
      static float smoothPath[MAX_SMOOTH * 3];
      int nsmoothPath = 0;

      dtVcopy(&smoothPath[nsmoothPath * 3], iterPos);
      nsmoothPath++;

      // Move towards target a small advancement at a time until target reached or
      // when ran out of memory to store the path.
      while (npolys && nsmoothPath < MAX_SMOOTH)
      {
        // Find location to steer towards.
        float steerPos[3];
        unsigned char steerPosFlag;
        dtPolyRef steerPosRef;

        if (!getSteerTarget(nav_mesh_idx, iterPos, targetPos, slop, polys_c, npolys, steerPos, steerPosFlag, steerPosRef))
          break;

        bool endOfPath = (steerPosFlag & DT_STRAIGHTPATH_END) ? true : false;
        bool offMeshConnection = (steerPosFlag & DT_STRAIGHTPATH_OFFMESH_CONNECTION) ? true : false;

        // Find movement delta.
        float delta[3], len;
        dtVsub(delta, steerPos, iterPos);
        len = sqrtf(dtVdot(delta, delta));
        // If the steer target is end of path or off-mesh link, do not move past the location.
        if ((endOfPath || offMeshConnection) && len < step_size)
          len = 1;
        else
          len = step_size / len;
        float moveTgt[3];
        dtVmad(moveTgt, iterPos, delta, len);

        // Move
        float result[3];
        dtPolyRef visited[16];
        int nvisited = 0;
        nmData.navQuery->moveAlongSurface(polys_c[0], iterPos, moveTgt, &filter, result, visited, &nvisited, 16);

        if (nvisited > 0)
          npolys = fixupCorridor(polys_c, npolys, max_path_size, visited, nvisited);
        npolys = fixupShortcuts(polys_c, npolys, nmData.navQuery);

        float h = result[1];
        nmData.navQuery->getPolyHeight(polys_c[0], result, &h);
        result[1] = h;
        dtVcopy(iterPos, result);

        // Handle end of path and off-mesh links when close enough.
        if (endOfPath && inRange(iterPos, steerPos, slop, 1.0f))
        {
          // Reached end of path.
          dtVcopy(iterPos, targetPos);
          dtVcopy(&smoothPath[nsmoothPath * 3], iterPos);
          nsmoothPath++;
          break;
        }
        else if (offMeshConnection && inRange(iterPos, steerPos, slop, 1.0f))
        {
          // Reached off-mesh connection.
          float startPos[3], endPos[3];

          // Advance the path up to and over the off-mesh connection.
          dtPolyRef prevRef = 0, polyRef = polys_c[0];
          int npos = 0;
          while (npos < npolys && polyRef != steerPosRef)
          {
            prevRef = polyRef;
            polyRef = polys_c[npos];
            npos++;
          }
          for (int i = npos; i < npolys; ++i)
            polys_c[i - npos] = polys_c[i];
          npolys -= npos;

          // Handle the connection.
          dtStatus status = nmData.navMesh->getOffMeshConnectionPolyEndPoints(prevRef, polyRef, startPos, endPos);
          if (dtStatusSucceed(status))
          {
            dtVcopy(&smoothPath[nsmoothPath * 3], startPos);
            nsmoothPath++;
            // Hack to make the dotted path not visible during off-mesh connection.
            if (nsmoothPath & 1)
            {
              dtVcopy(&smoothPath[nsmoothPath * 3], startPos);
              nsmoothPath++;
            }
            // Move position at the other side of the off-mesh link.
            dtVcopy(iterPos, endPos);
            float eh = iterPos[1];
            nmData.navQuery->getPolyHeight(polys[0], iterPos, &eh);
            iterPos[1] = eh;
          }
        }

        // Store results.
        if (nsmoothPath < MAX_SMOOTH)
        {
          dtVcopy(&smoothPath[nsmoothPath * 3], iterPos);
          nsmoothPath++;
        }
      }

      for (int i = 0; i < nsmoothPath; ++i)
      {
        path.push_back(Point3(&smoothPath[i * 3], Point3::CTOR_FROM_PTR));
      }
    }
  }
  return path.size() > 0 ? res : FPR_FAILED;
}

FindPathResult find_path_ex(int nav_mesh_idx, const Point3 &start_pos, const Point3 &end_pos, Tab<Point3> &path, float dist_to_path,
  float step_size, float slop, const CustomNav *custom_nav)
{
  const Point3 extents(dist_to_path, FLT_MAX, dist_to_path);
  FindRequest req = {
    start_pos, end_pos, POLYFLAGS_WALK | POLYFLAG_JUMP | POLYFLAG_LADDER, 0, extents, FLT_MAX, 0, dtPolyRef(), dtPolyRef()};
  return find_path_ex(nav_mesh_idx, path, req, step_size, slop, custom_nav);
}

FindPathResult find_path_ex(int nav_mesh_idx, const Point3 &start_pos, const Point3 &end_pos, Tab<Point3> &path, const Point3 &extents,
  float step_size, float slop, const CustomNav *custom_nav)
{
  FindRequest req = {
    start_pos, end_pos, POLYFLAGS_WALK | POLYFLAG_JUMP | POLYFLAG_LADDER, 0, extents, FLT_MAX, 0, dtPolyRef(), dtPolyRef()};
  return find_path_ex(nav_mesh_idx, path, req, step_size, slop, custom_nav);
}

FindPathResult findPath(Tab<Point3> &path, FindRequest &req, float step_size, float slop, const CustomNav *custom_nav)
{
  return find_path_ex(NM_MAIN, path, req, step_size, slop, custom_nav);
}

FindPathResult findPath(const Point3 &start_pos, const Point3 &end_pos, Tab<Point3> &path, float dist_to_path, float step_size,
  float slop, const CustomNav *custom_nav, const dag::Vector<Point2> &areasCost)
{
  const Point3 extents(dist_to_path, FLT_MAX, dist_to_path);
  FindRequest req = {
    start_pos, end_pos, POLYFLAGS_WALK | POLYFLAG_JUMP | POLYFLAG_LADDER, 0, extents, FLT_MAX, 0, dtPolyRef(), dtPolyRef(), areasCost};
  return findPath(path, req, step_size, slop, custom_nav);
}

FindPathResult findPath(const Point3 &start_pos, const Point3 &end_pos, Tab<Point3> &path, const Point3 &extents, float step_size,
  float slop, const CustomNav *custom_nav)
{
  FindRequest req = {
    start_pos, end_pos, POLYFLAGS_WALK | POLYFLAG_JUMP | POLYFLAG_LADDER, 0, extents, FLT_MAX, 0, dtPolyRef(), dtPolyRef()};
  return findPath(path, req, step_size, slop, custom_nav);
}

static Point3 get_poly_top(const dtMeshTile *tile, const dtPoly *poly, const dtPolyDetail *pd)
{
  Point3 top(-1e8f, -1e8f, -1e8f);
  for (int k = 0; k < pd->triCount; ++k)
  {
    const unsigned char *t = &tile->detailTris[(pd->triBase + k) * 4];
    Point3 posA(
      t[0] < poly->vertCount ? &tile->verts[poly->verts[t[0]] * 3] : &tile->detailVerts[(pd->vertBase + t[0] - poly->vertCount) * 3],
      Point3::CTOR_FROM_PTR);
    Point3 posB(
      t[1] < poly->vertCount ? &tile->verts[poly->verts[t[1]] * 3] : &tile->detailVerts[(pd->vertBase + t[1] - poly->vertCount) * 3],
      Point3::CTOR_FROM_PTR);
    Point3 posC(
      t[2] < poly->vertCount ? &tile->verts[poly->verts[t[2]] * 3] : &tile->detailVerts[(pd->vertBase + t[2] - poly->vertCount) * 3],
      Point3::CTOR_FROM_PTR);
    if (posA.y > top.y)
      top = posA;
    if (posB.y > top.y)
      top = posB;
    if (posC.y > top.y)
      top = posC;
  }
  return top;
}

static Point3 get_poly_center(const dtMeshTile *tile, const dtPoly *poly)
{
  Point3 midVert(0.f, 0.f, 0.f);
  for (int i = 0; i < poly->vertCount; ++i)
  {
    const Point3 *vert = (const Point3 *)&tile->verts[poly->verts[i] * 3];
    midVert += *vert;
  }
  return midVert * safeinv((float)poly->vertCount);
}

bool check_path_ex(int nav_mesh_idx, FindRequest &req, float horz_threshold, float max_vert_dist)
{
  return check_path(get_nav_mesh_data(nav_mesh_idx).navQuery, req, get_nav_params(nav_mesh_idx), horz_threshold, max_vert_dist,
    nullptr);
}

bool check_path_ex(int nav_mesh_idx, const Point3 &start_pos, const Point3 &end_pos, const Point3 &extents, float horz_threshold,
  float max_vert_dist, const CustomNav *custom_nav, int flags)
{
  FindRequest req = {start_pos, end_pos, flags, 0, extents, FLT_MAX, 0, dtPolyRef(), dtPolyRef()};
  return check_path(get_nav_mesh_data(nav_mesh_idx).navQuery, req, get_nav_params(nav_mesh_idx), horz_threshold, max_vert_dist,
    custom_nav);
}

bool check_path(FindRequest &req, float horz_threshold, float max_vert_dist)
{
  return check_path(navQuery, req, get_nav_params(NM_MAIN), horz_threshold, max_vert_dist, nullptr);
}

bool check_path(const Point3 &start_pos, const Point3 &end_pos, const Point3 &extents, float horz_threshold, float max_vert_dist,
  const CustomNav *custom_nav, int flags)
{
  FindRequest req = {start_pos, end_pos, flags, 0, extents, FLT_MAX, 0, dtPolyRef(), dtPolyRef()};
  return check_path(navQuery, req, get_nav_params(NM_MAIN), horz_threshold, max_vert_dist, custom_nav);
}

inline bool check_path_thresholds(dtNavMeshQuery *nav_query, const Point3 &start_pos, const Point3 &end_pos, Point3 extents,
  const NavParams &nav_params, float horz_threshold, float max_vert_dist, dtPolyRef polys[max_path_size], int num_polys,
  const CustomNav *custom_nav)
{
  Point3 targetPos;
  nav_query->closestPointOnPoly(polys[num_polys - 1], &end_pos.x, &targetPos.x, nullptr);
  Point3 finalPos;
  traceray_navmesh(nav_query, targetPos, end_pos, extents, nav_params, finalPos, custom_nav);
  const float horzDistSq = lengthSq(Point2::xz(finalPos - end_pos));
  const float vertDist = fabsf(finalPos.y - end_pos.y);
  bool horzViolatesThreshold = horz_threshold > 0.f && horzDistSq > sqr(horz_threshold);
  bool horzTooFar = horzDistSq >= lengthSq(start_pos - end_pos) * 0.001f && horzViolatesThreshold;
  return !horzTooFar && vertDist < max_vert_dist;
}

bool check_path(dtNavMeshQuery *nav_query, FindRequest &req, const NavParams &nav_params, float horz_threshold, float max_vert_dist,
  const CustomNav *custom_nav)
{
  if (nav_query)
  {
    dtPolyRef polys[max_path_size];
    // trace from last poly to end_pos
    CheckPathFilter filter(nav_params, req.maxJumpUpHeight);
    filter.setIncludeFlags(req.includeFlags);
    filter.setExcludeFlags(req.excludeFlags);
    if (find_poly_path(nav_query, req, nav_params, polys, filter, max_path_size) > FPR_FAILED)
    {
      bool thresholdsValid = check_path_thresholds(nav_query, req.start, req.end, req.extents, nav_params, horz_threshold,
        max_vert_dist, polys, req.numPolys, custom_nav);
      return thresholdsValid;
    }
  }
  return false;
}

bool check_path(dtNavMeshQuery *nav_query, const Point3 &start_pos, const Point3 &end_pos, const Point3 &extents,
  const NavParams &nav_params, float horz_threshold, float max_vert_dist, const CustomNav *custom_nav, int flags)
{
  FindRequest req = {start_pos, end_pos, flags, 0, extents, FLT_MAX, 0, dtPolyRef(), dtPolyRef()};
  return check_path(nav_query, req, nav_params, horz_threshold, max_vert_dist, custom_nav);
}

float calc_approx_path_length(FindRequest &req, float horz_threshold, float max_vert_dist)
{
  return calc_approx_path_length(navQuery, req, get_nav_params(NM_MAIN), horz_threshold, max_vert_dist);
}

float calc_approx_path_length(const Point3 &start_pos, const Point3 &end_pos, const Point3 &extents, float horz_threshold,
  float max_vert_dist, int flags)
{
  FindRequest req = {start_pos, end_pos, flags, 0, extents, FLT_MAX, 0, dtPolyRef(), dtPolyRef()};
  return calc_approx_path_length(navQuery, req, get_nav_params(NM_MAIN), horz_threshold, max_vert_dist);
}

float calc_approx_path_length(dtNavMeshQuery *nav_query, FindRequest &req, const NavParams &nav_params, float horz_threshold,
  float max_vert_dist)
{
  if (nav_query)
  {
    dtPolyRef polys[max_path_size];
    // trace from last poly to end_pos
    CheckPathFilter filter(nav_params, req.maxJumpUpHeight);
    filter.setIncludeFlags(req.includeFlags);
    filter.setExcludeFlags(req.excludeFlags);
    if (find_poly_path(nav_query, req, nav_params, polys, filter, max_path_size) > FPR_FAILED)
    {
      bool thresholdsValid = check_path_thresholds(nav_query, req.start, req.end, req.extents, nav_params, horz_threshold,
        max_vert_dist, polys, req.numPolys, nullptr);

      if (!thresholdsValid)
        return INVALID_PATH_DISTANCE;

      const dtMeshTile *tile = nullptr;
      const dtPoly *poly = nullptr;
      float length = 0.0f;
      Point3 prevPathPos = req.start;

      for (int i = 0; i < req.numPolys; ++i)
      {
        if (dtStatusFailed(nav_query->getAttachedNavMesh()->getTileAndPolyByRef(polys[i], &tile, &poly)))
          return INVALID_PATH_DISTANCE;

        Point3 polyCenter = get_poly_center(tile, poly);
        length += (polyCenter - prevPathPos).length();
        prevPathPos = polyCenter;
      }

      length += (req.end - prevPathPos).length();

      return length;
    }
  }
  return INVALID_PATH_DISTANCE;
}

static inline bool project_to_nearest_navmesh_point_impl(Point3 &pos, float horx_extents, float hory_extents, float horz_extents,
  uint16_t inc_flags, const CustomNav *custom_nav)
{
  if (!navQuery)
    return false;
  NavQueryFilter filter(custom_nav);
  filter.setIncludeFlags(inc_flags);
  const Point3 extents(horx_extents, hory_extents, horz_extents);
  Point3 resPos;
  dtPolyRef nearestPoly;
  if (dtStatusSucceed(navQuery->findNearestPoly(&pos.x, &extents.x, &filter, &nearestPoly, &resPos.x)) && nearestPoly != 0)
  {
    pos = resPos;
    return true;
  }
  return false;
}

bool project_to_nearest_navmesh_point_ref(Point3 &pos, const Point3 &extents, dtPolyRef &nearestPoly)
{
  if (!navQuery)
    return false;
  NavQueryFilter filter(nullptr);
  filter.setIncludeFlags(POLYFLAGS_WALK);
  Point3 resPos;
  if (dtStatusSucceed(navQuery->findNearestPoly(&pos.x, &extents.x, &filter, &nearestPoly, &resPos.x)) && nearestPoly != 0)
  {
    pos = resPos;
    return true;
  }
  return false;
}

bool project_to_nearest_navmesh_point(Point3 &pos, const Point3 &extents, const CustomNav *custom_nav)
{
  return project_to_nearest_navmesh_point_impl(pos, extents.x, extents.y, extents.z, POLYFLAGS_WALK, custom_nav);
}

bool project_to_nearest_navmesh_point(Point3 &pos, float horz_extents, const CustomNav *custom_nav)
{
  return project_to_nearest_navmesh_point_impl(pos, horz_extents, FLT_MAX, horz_extents, POLYFLAGS_WALK, custom_nav);
}

bool project_to_nearest_navmesh_point_no_obstacles(Point3 &pos, const Point3 &extents, const CustomNav *custom_nav)
{
  return project_to_nearest_navmesh_point_impl(pos, extents.x, extents.y, extents.z, POLYFLAG_GROUND, custom_nav);
}

bool navmesh_point_has_obstacle(const Point3 &pos, const CustomNav *custom_nav)
{
  if (!navQuery)
    return false;
  NavQueryFilter filter(custom_nav);
  filter.setIncludeFlags(POLYFLAGS_WALK);
  const Point3 extents(0.01f, FLT_MAX, 0.01f);
  Point3 resPos;
  dtPolyRef nearestPoly;
  if (dtStatusSucceed(navQuery->findNearestPoly(&pos.x, &extents.x, &filter, &nearestPoly, &resPos.x)) && nearestPoly != 0)
  {
    filter.setIncludeFlags(POLYFLAG_OBSTACLE);
    return navQuery->isValidPolyRef(nearestPoly, &filter);
  }
  return false;
}

static bool query_navmesh_projections_impl(const Point3 &pos, const Point3 &extents, Tab<Point3> &projections,
  Tab<dtPolyRef> *out_polys, int points_num, const CustomNav *custom_nav)
{
  if (!navQuery)
    return false;
  NavQueryFilter filter(custom_nav);
  filter.setIncludeFlags(POLYFLAGS_WALK);
  filter.setExcludeFlags(0);
  eastl::fixed_vector<dtPolyRef, 16, true, framemem_allocator> polys;
  polys.resize(points_num);
  int numPolys = 0;
  navQuery->queryPolygons(&pos.x, &extents.x, &filter, polys.data(), &numPolys, points_num);
  for (auto &poly : polys)
  {
    Point3 res;
    bool posOverPoly;
    if (dtStatusFailed(navQuery->closestPointOnPoly(poly, &pos.x, &res.x, &posOverPoly)))
      continue;
    projections.push_back(res);
    if (out_polys != nullptr)
      out_polys->push_back(poly);
  }
  return !projections.empty();
}

bool query_navmesh_projections(const Point3 &pos, const Point3 &extents, Tab<Point3> &projections, int points_num,
  const CustomNav *custom_nav)
{
  return query_navmesh_projections_impl(pos, extents, projections, nullptr, points_num, custom_nav);
}

bool query_navmesh_projections(const Point3 &pos, const Point3 &extents, Tab<Point3> &projections, Tab<dtPolyRef> &out_polys,
  int points_num, const CustomNav *custom_nav)
{
  return query_navmesh_projections_impl(pos, extents, projections, &out_polys, points_num, custom_nav);
}

float get_distance_to_wall(const Point3 &pos, float horz_extents, float search_rad, const CustomNav *custom_nav)
{
  Point3 hitNorm = Point3(0.f, 1.f, 0.f);
  return get_distance_to_wall(pos, horz_extents, search_rad, hitNorm, custom_nav);
}

float get_distance_to_wall(const Point3 &pos, float horz_extents, float search_rad, Point3 &hit_norm, const CustomNav *custom_nav)
{
  if (!navQuery)
    return FLT_MAX;
  NavQueryFilter filter(custom_nav);
  filter.setIncludeFlags(POLYFLAGS_WALK);
  filter.setExcludeFlags(0);
  const Point3 extents(horz_extents, FLT_MAX, horz_extents);
  Point3 resPos;
  dtPolyRef nearestPoly;
  if (dtStatusFailed(navQuery->findNearestPoly(&pos.x, &extents.x, &filter, &nearestPoly, &resPos.x)) && nearestPoly == 0)
    return FLT_MAX;
  Point3 hitPos = resPos;
  float dist = 0.f;
  navQuery->findDistanceToWall(nearestPoly, &resPos.x, search_rad, &filter, &dist, &hitPos.x, &hit_norm.x);
  return dist;
}

bool find_random_point_around_circle(const Point3 &pos, float radius, Point3 &res, const CustomNav *custom_nav)
{
  if (!navQuery)
    return false;
  dtPolyRef startRef;
  if (!find_polyref(NM_MAIN, pos, 0.1f, POLYFLAGS_WALK, 0, startRef, custom_nav))
    return false;
  NavQueryFilter filter(custom_nav);
  filter.setIncludeFlags(POLYFLAGS_WALK);
  filter.setExcludeFlags(0);
  dtPolyRef retRef;
  return dtStatusSucceed(navQuery->findRandomPointAroundCircle(startRef, &pos.x, radius, &filter, gfrnd, &retRef, &res.x));
}

bool find_random_points_around_circle(const Point3 &pos, float radius, int num_points, Tab<Point3> &points)
{
  if (!navQuery)
    return false;
  dtPolyRef startRef;
  if (!find_polyref(NM_MAIN, pos, 0.1f, POLYFLAGS_WALK, 0, startRef, nullptr))
    return false;
  NavQueryFilter filter(nullptr);
  filter.setIncludeFlags(POLYFLAGS_WALK);
  filter.setExcludeFlags(0);
  Tab<dtPolyRef> refs(framemem_ptr());
  refs.resize(num_points);
  points.resize(num_points);
  int resCount = 0;
  dtStatus status = navQuery->findRandomPointsAroundCircle(startRef, &pos.x, radius, &filter, grnd, gfrnd, &refs[0], &points[0].x,
    &resCount, num_points);
  points.resize(resCount);
  return dtStatusSucceed(status);
}

bool find_random_point_inside_circle(const Point3 &start_pos, float radius, float extents, Point3 &res)
{
  if (!navQuery)
    return false;
#if DAGOR_DBGLEVEL > 0
  if (radius > 100.f)
    logerr("find_random_point_inside_circle is optimized for a search radius lower than 100(m). Current radius is %d(m)", radius);
#endif
  dtPolyRef startRef;
  if (!find_polyref(NM_MAIN, start_pos, 0.1f, POLYFLAGS_WALK, 0, startRef, nullptr))
    return false;
  Tab<Point3> points(framemem_ptr());
  constexpr int numPoints = 16;
  if (!pathfinder::find_random_points_around_circle(start_pos, radius, numPoints, points))
    return false;

  NavQueryFilter filter(nullptr);
  filter.setIncludeFlags(POLYFLAGS_WALK);
  filter.setExcludeFlags(0);
  float t;
  Point3 norm;
  dtPolyRef polys[max_path_size];
  int pathCount = 0;
  float radiusSq = sqr(radius);

  for (int i = 0; i < (int)points.size(); ++i)
  {
    const Point3 resPos = points[grnd() % (int)points.size()];
    const Point3 dir = resPos - start_pos;
    const float distSq = lengthSq(dir);
    if (distSq < radiusSq)
    {
      res = resPos;
      return true;
    }
    const float dist = sqrt(distSq);
    const Point3 nearestPos = resPos - dir * safediv(rnd_float(dist - radius + extents, dist), dist);

    if (
      dtStatusFailed(navQuery->raycast(startRef, &start_pos.x, &nearestPos.x, &filter, &t, &norm.x, polys, &pathCount, max_path_size)))
      continue;

    if (pathCount == 0)
      continue;

    Point3 outPos = lerp(start_pos, nearestPos, min(t, 1.f));
    float h = outPos.y;
    navQuery->getPolyHeight(polys[pathCount - 1], &outPos.x, &h);
    outPos.y = h;
    if (lengthSq(outPos - start_pos) < radiusSq)
    {
      res = outPos;
      return true;
    }
  }
  return false;
}

bool is_on_same_polygon(const Point3 &p1, const Point3 &p2, const CustomNav *custom_nav)
{
  if (!navQuery)
    return false;
  dtPolyRef r1 = 0, r2 = 0;
  if (!find_polyref(NM_MAIN, p1, 0.1f, POLYFLAGS_WALK, 0, r1, custom_nav) ||
      !find_polyref(NM_MAIN, p2, 0.1f, POLYFLAGS_WALK, 0, r2, custom_nav))
    return false;
  return r1 == r2;
}

bool traceray_navmesh(const Point3 &start_pos, const Point3 &end_pos, const Point3 &extents, Point3 &out_pos,
  const CustomNav *custom_nav)
{
  return traceray_navmesh(navQuery, start_pos, end_pos, extents, get_nav_params(NM_MAIN), out_pos, custom_nav);
}

bool traceray_navmesh(const Point3 &start_pos, const Point3 &end_pos, const Point3 &extents, Point3 &out_pos, dtPolyRef &out_poly,
  const CustomNav *custom_nav)
{
  return traceray_navmesh(navQuery, start_pos, end_pos, extents, get_nav_params(NM_MAIN), out_pos, out_poly, custom_nav);
}

bool traceray_navmesh(dtNavMeshQuery *nav_query, const Point3 &start_pos, const Point3 &end_pos, const Point3 &extents,
  const NavParams &nav_params, Point3 &out_pos, const CustomNav *custom_nav)
{
  dtPolyRef poly;
  return traceray_navmesh(nav_query, start_pos, end_pos, extents, nav_params, out_pos, poly, custom_nav);
}

bool traceray_navmesh(dtNavMeshQuery *nav_query, const Point3 &start_pos, const Point3 &end_pos, const Point3 &extents,
  const NavParams &nav_params, Point3 &out_pos, dtPolyRef &out_poly, const CustomNav *custom_nav)
{
  out_pos = start_pos;
  if (!nav_query)
  {
    out_pos = end_pos;
    return true;
  }
  NavQueryFilter filter(nav_query->getAttachedNavMesh(), nav_params, custom_nav);
  filter.setIncludeFlags(POLYFLAGS_WALK);
  filter.setExcludeFlags(0);
  dtPolyRef startPoly;
  nav_query->findNearestPoly(&start_pos.x, &extents.x, &filter, &startPoly, NULL);
  float t;
  Point3 norm;
  dtPolyRef polys[max_path_size];
  int pathCount = 0;
  nav_query->raycast(startPoly, &start_pos.x, &end_pos.x, &filter, &t, &norm.x, polys, &pathCount, max_path_size);
  const float safeT = min(t, 1.f); // t value can by very high value (FLT_MAX), @see dtNavMeshQuery::raycast documentation
  out_pos = lerp(start_pos, end_pos, safeT);
  if (pathCount > 0)
  {
    float h = out_pos.y;
    nav_query->getPolyHeight(polys[pathCount - 1], &out_pos.x, &h);
    out_pos.y = h;
    out_poly = polys[pathCount - 1];
  }
  else
  {
    out_poly = startPoly;
  }
  return t <= 1.f;
}

static inline E3DCOLOR rnd_color(int &seed)
{
  uint32_t r1 = _rnd(seed);
  uint32_t r2 = _rnd(seed);
  uint32_t rgbValue = (r1 << 8u) | (r2 >> 8u);
  uint32_t forceBits = (0xC0 << (8u * (r2 % 3u))) | 0xFF000000;
  return E3DCOLOR(rgbValue | forceBits);
}

void renderDebugReset();

void renderDebug(const Frustum *p_frustum, int nav_mesh_idx)
{
  static eastl::vector_map<int, E3DCOLOR> areaToColor;

  if (nav_mesh_idx != lastNavMeshDebugIdx)
    renderDebugReset();
  lastNavMeshDebugIdx = nav_mesh_idx;

  bool calculateBoxes = p_frustum && navDebugBoxes.capacity() == 0;

  const Point3 offset(0.f, 1.f, 0.f);
  for (int i = 0; i < (int)pathForDebug.size() - 1; ++i)
    draw_cached_debug_line(pathForDebug[i] + offset, pathForDebug[i + 1] + offset, E3DCOLOR(0, 255, 0, 255));

  dtNavMesh *drawnNavMesh = get_nav_mesh_ptr(nav_mesh_idx);

  if (drawnNavMesh)
  {
    int maxTiles = drawnNavMesh->getMaxTiles();
    if (calculateBoxes)
      navDebugBoxes.assign(maxTiles, BBox3());

    for (int i = 0; i < maxTiles; ++i)
    {
      const dtMeshTile *tile = ((const dtNavMesh *)drawnNavMesh)->getTile(i);
      if (!tile->header)
        continue;
      if (calculateBoxes)
      {
        BBox3 &bbox = navDebugBoxes[i];
        for (int j = 0; j < tile->header->polyCount; ++j)
        {
          const dtPoly *poly = &tile->polys[j];
          if (poly->getType() != DT_POLYTYPE_OFFMESH_CONNECTION)
          {
            const dtPolyDetail *pd = &tile->detailMeshes[j];
            for (int k = 0; k < pd->triCount; ++k)
            {
              const unsigned char *t = &tile->detailTris[(pd->triBase + k) * 4];
              Point3 posA(t[0] < poly->vertCount ? &tile->verts[poly->verts[t[0]] * 3]
                                                 : &tile->detailVerts[(pd->vertBase + t[0] - poly->vertCount) * 3],
                Point3::CTOR_FROM_PTR);
              Point3 posB(t[1] < poly->vertCount ? &tile->verts[poly->verts[t[1]] * 3]
                                                 : &tile->detailVerts[(pd->vertBase + t[1] - poly->vertCount) * 3],
                Point3::CTOR_FROM_PTR);
              Point3 posC(t[2] < poly->vertCount ? &tile->verts[poly->verts[t[2]] * 3]
                                                 : &tile->detailVerts[(pd->vertBase + t[2] - poly->vertCount) * 3],
                Point3::CTOR_FROM_PTR);
              bbox += posA;
              bbox += posB;
              bbox += posC;
            }
          }
        }
      }

      if (p_frustum && !p_frustum->testBoxB(navDebugBoxes[i]))
        continue;

      for (int j = 0; j < tile->header->offMeshConCount; ++j)
      {
        const dtOffMeshConnection &conn = tile->offMeshCons[j];
        const Point3 a(conn.pos, Point3::CTOR_FROM_PTR);
        const Point3 b(conn.pos + 3, Point3::CTOR_FROM_PTR);
        draw_cached_debug_line(a, b, E3DCOLOR(0, 0, (tile->polys[conn.poly].getArea() == POLYAREA_OBSTACLE) ? 100 : 255, 255));
      }

      for (int j = 0; j < tile->header->polyCount; ++j)
      {
        const dtPoly *poly = &tile->polys[j];
        if (poly->getType() != DT_POLYTYPE_OFFMESH_CONNECTION)
        {
          const int polyArea = poly->getArea();
          int c = (polyArea == POLYAREA_OBSTACLE) ? 100 : 255;
          const dtPolyDetail *pd = &tile->detailMeshes[j];
          for (int k = 0; k < pd->triCount; ++k)
          {
            const unsigned char *t = &tile->detailTris[(pd->triBase + k) * 4];
            Point3 posA(t[0] < poly->vertCount ? &tile->verts[poly->verts[t[0]] * 3]
                                               : &tile->detailVerts[(pd->vertBase + t[0] - poly->vertCount) * 3],
              Point3::CTOR_FROM_PTR);
            Point3 posB(t[1] < poly->vertCount ? &tile->verts[poly->verts[t[1]] * 3]
                                               : &tile->detailVerts[(pd->vertBase + t[1] - poly->vertCount) * 3],
              Point3::CTOR_FROM_PTR);
            Point3 posC(t[2] < poly->vertCount ? &tile->verts[poly->verts[t[2]] * 3]
                                               : &tile->detailVerts[(pd->vertBase + t[2] - poly->vertCount) * 3],
              Point3::CTOR_FROM_PTR);
            draw_cached_debug_line(posA, posB, E3DCOLOR(c, 0, 0, 255));
            draw_cached_debug_line(posA, posC, E3DCOLOR(c, 0, 0, 255));
            draw_cached_debug_line(posC, posB, E3DCOLOR(c, 0, 0, 255));

            auto colorIt = areaToColor.find(polyArea);
            if (colorIt == areaToColor.end())
            {
              int seed = polyArea;
              E3DCOLOR polyColor = rnd_color(seed);
              polyColor.a = 0x7F;
              bool inserted;
              eastl::tie(colorIt, inserted) = areaToColor.insert(eastl::make_pair(polyArea, polyColor));
            }

            const Point3 ptArray[3] = {posA, posB, posC};
            draw_cached_debug_solid_triangle(ptArray, colorIt->second);
          }
        }
        Point3 midVert = get_poly_center(tile, poly);
        for (unsigned int k = poly->firstLink; k != DT_NULL_LINK; k = tile->links[k].next)
        {
          dtPolyRef neiRef = tile->links[k].ref;
          if (!neiRef)
            continue;
          const dtPoly *neiPoly = NULL;
          const dtMeshTile *neiTile = NULL;
          drawnNavMesh->getTileAndPolyByRefUnsafe(neiRef, &neiTile, &neiPoly);
          if (!neiPoly || !neiTile)
            continue;
          draw_cached_debug_line(midVert, get_poly_center(neiTile, neiPoly), E3DCOLOR_MAKE(255, 255, 0, 255));
        }
      }
    }
  }

  if (nav_mesh_idx == NM_MAIN)
    tilecache_render_debug(p_frustum);
}

void setPathForDebug(dag::ConstSpan<Point3> path) { pathForDebug = path; }

void renderDebugReset() { clear_and_shrink(navDebugBoxes); }

void init_path_corridor(dtPathCorridor &corridor) { corridor.init(max_path_size); }

inline NavQueryFilter init_request_filter(int nav_mesh_idx, const CorridorInput &inp, const CustomNav *custom_nav, FindRequest &req)
{
  req = {inp.start, inp.target, inp.includeFlags, inp.excludeFlags, inp.extents, inp.maxJumpUpHeight, 0, inp.startPoly, inp.targetPoly,
    inp.areasCost};
  NavQueryFilter filter(get_nav_mesh_ptr(nav_mesh_idx), get_nav_params(nav_mesh_idx), custom_nav, req.maxJumpUpHeight, false,
    inp.costAddition);
  filter.setIncludeFlags(req.includeFlags);
  filter.setExcludeFlags(req.excludeFlags);
  filter.setAreasCost(req.areasCost);

  return filter;
}

inline NavQueryFilter init_request_filter(const CorridorInput &inp, const CustomNav *custom_nav, FindRequest &req)
{
  return init_request_filter(NM_MAIN, inp, custom_nav, req);
}

FindPathResult set_path_corridor(dtPathCorridor &corridor, const CorridorInput &inp, const CustomNav *custom_nav)
{
  if (!navQuery)
    return FPR_FAILED;
  dtPolyRef polys[max_path_size];
  FindRequest req;
  NavQueryFilter filter = init_request_filter(inp, custom_nav, req);
  FindPathResult fpr;
  fpr = find_poly_path(navQuery, req, get_nav_params(NM_MAIN), polys, filter, max_path_size);

  if (fpr > FPR_FAILED)
  {
    corridor.reset(req.startPoly, &inp.start.x);
    corridor.setCorridor(&inp.target.x, polys, req.numPolys);
  }
  return fpr;
}

FindPathResult set_curved_path_corridor(dtPathCorridor &corridor, const CorridorInput &inp, float max_deflection_angle,
  float min_curved_pathlen_sq_threshold, const CustomNav *custom_nav)
{
  if (!navQuery)
    return FPR_FAILED;
  dtPolyRef polys[max_path_size];
  FindRequest req;
  NavQueryFilter filter = init_request_filter(inp, custom_nav, req);
  FindPathResult fpr;
  const NavParams &navParams = get_nav_params(NM_MAIN);
  if (lengthSq(req.end - req.start) > min_curved_pathlen_sq_threshold)
  {
    dtPolyRef auxPolys[max_path_size];
    const size_t tryFindPathNum = 3;
    // try tryFindPathNum times (with different random angle and offset) to find full curve before giving up...
    for (size_t i = 0; i < tryFindPathNum; ++i)
    {
      fpr = find_poly_path_curved(navQuery, req, navParams, polys, auxPolys, filter, max_path_size, max_deflection_angle);
      if (fpr == FPR_FULL)
        break;
    }
  }
  else
    fpr = find_poly_path(navQuery, req, navParams, polys, filter, max_path_size);

  if (fpr > FPR_FAILED)
  {
    corridor.reset(req.startPoly, &inp.start.x);
    corridor.setCorridor(&inp.target.x, polys, req.numPolys);
  }
  return fpr;
}

bool get_offmesh_connection_end_points(int nav_mesh_idx, dtPolyRef poly, const float *next_pos, Point3 &start, Point3 &end)
{
  NavMeshData &nmData = get_nav_mesh_data(nav_mesh_idx);
  const dtOffMeshConnection *connection = nmData.navMesh->getOffMeshConnectionByRef(poly);
  if (!connection)
    return false;
  const float aDistSq = dtVdistSqr(next_pos, &connection->pos[0]);
  const float bDistSq = dtVdistSqr(next_pos, &connection->pos[3]);
  if (aDistSq > bDistSq)
  {
    start = Point3(&connection->pos[0], Point3::CTOR_FROM_PTR);
    end = Point3(&connection->pos[3], Point3::CTOR_FROM_PTR);
  }
  else
  {
    start = Point3(&connection->pos[3], Point3::CTOR_FROM_PTR);
    end = Point3(&connection->pos[0], Point3::CTOR_FROM_PTR);
  }
  return true;
}

// This function different from corridor::findCorners in that it does not cut off-mesh connections
int find_corridor_corners(dtPathCorridor &corridor, float *corner_verts, unsigned char *corner_flags, dtPolyRef *corner_polys,
  const int max_corners)
{
  if (!corridor.getPath() || corridor.getPathCount() == 0)
    return 0;

  static const float MIN_TARGET_DIST = 0.01f;

  int ncorners = 0;
  navQuery->findStraightPath(corridor.getPos(), corridor.getTarget(), corridor.getPath(), corridor.getPathCount(), corner_verts,
    corner_flags, corner_polys, &ncorners, max_corners);

  // Prune points in the beginning of the path which are too close.
  while (ncorners)
  {
    if ((corner_flags[0] & DT_STRAIGHTPATH_OFFMESH_CONNECTION) ||
        dtVdist2DSqr(&corner_verts[0], corridor.getPos()) > dtSqr(MIN_TARGET_DIST))
      break;
    uint16_t polyFlags = 0;
    // cut-off almost passed offmesh connections
    if (ncorners > 1 && dtStatusSucceed(navMesh->getPolyFlags(corner_polys[0], &polyFlags)) &&
        (polyFlags & (POLYFLAG_JUMP | POLYFLAG_LADDER)))
    {
      Point3 start, end;
      if (get_offmesh_connection_end_points(NM_MAIN, corner_polys[0], &corner_verts[3], start, end) &&
          dtVdistSqr(corridor.getPos(), &end.x) > dtSqr(MIN_TARGET_DIST))
      {
        corner_flags[0] = corner_flags[0] | DT_STRAIGHTPATH_OFFMESH_CONNECTION;
        break;
      }
    }
    ncorners--;
    if (ncorners)
    {
      memmove(corner_flags, corner_flags + 1, sizeof(unsigned char) * ncorners);
      memmove(corner_polys, corner_polys + 1, sizeof(dtPolyRef) * ncorners);
      memmove(corner_verts, corner_verts + 3, sizeof(float) * 3 * ncorners);
    }
  }

  return ncorners;
}

FindCornersResult find_corridor_corners(dtPathCorridor &corridor, Tab<Point3> &corners, int num, const CustomNav *)
{
  if (!navQuery || corridor.getPathCount() < 1)
    return {};

  FindCornersResult res;
  res.cornerFlags.resize(num);
  res.cornerPolys.resize(num);

  corners.resize(num);
  int numCorners = find_corridor_corners(corridor, &corners[0].x, res.cornerFlags.data(), res.cornerPolys.data(), num);
  corners.resize(numCorners);

  return res;
}

bool set_poly_flags(dtPolyRef ref, unsigned short flags) { return dtStatusSucceed(navMesh->setPolyFlags(ref, flags)); }

bool get_poly_flags(dtPolyRef ref, unsigned short &result_flags) { return dtStatusSucceed(navMesh->getPolyFlags(ref, &result_flags)); }

bool get_poly_area(dtPolyRef ref, unsigned char &result_area) { return dtStatusSucceed(navMesh->getPolyArea(ref, &result_area)); }

bool set_poly_area(dtPolyRef ref, unsigned char area) { return dtStatusSucceed(navMesh->setPolyArea(ref, area)); }

bool corridor_moveOverOffmeshConnection(dtPathCorridor &corridor, dtPolyRef offmesh_con_ref, dtPolyRef &start_ref, dtPolyRef &end_ref,
  Point3 &start_pos, Point3 &end_pos)
{
  dtPolyRef refs[2];
  bool moveOver = corridor.moveOverOffmeshConnection(offmesh_con_ref, refs, &start_pos.x, &end_pos.x, navQuery);
  start_ref = refs[0];
  end_ref = refs[1];
  return moveOver;
}

int move_over_offmesh_link_in_corridor(dtPathCorridor &corridor, const Point3 &pos, const Point3 &extents,
  const FindCornersResult &ctx, Tab<Point3> &corners, int &over_link, Point3 &out_from, Point3 &out_to, const CustomNav *custom_nav)
{
  enum
  {
    OVER_JUMP_LINK = 1,
    OVER_LADDER_LINK = 2,
  };

  const float minJumpThreshold = 2.5f;
  if (ctx.cornerFlags.empty() || ctx.cornerPolys.empty()) // we must have at least two refs for offmesh connection
    return 0;
  uint8_t cornerFlag = ctx.cornerFlags[0];
  if (!(cornerFlag & DT_STRAIGHTPATH_OFFMESH_CONNECTION))
  {
    over_link = 0;
    return 0;
  }
  dtPolyRef connRef = ctx.cornerPolys[0];
  uint16_t polyFlags = 0;
  if (dtStatusFailed(navMesh->getPolyFlags(connRef, &polyFlags)))
    return 0;
  if (!(polyFlags & (POLYFLAG_JUMP | POLYFLAG_LADDER)))
    return 0;
  // Recreate part of dtPathCorridor::moveOverOffmeshConnection without the prune part
  dag::ConstSpan<dtPolyRef> pathSlice(corridor.getPath(), corridor.getPathCount());
  int idx = find_value_idx(pathSlice, connRef);
  if (idx < 0 || idx > pathSlice.size() - 2) // if not found, or last idx
    return 0;
  dtPolyRef endRef = pathSlice[idx + 1];

  if (polyFlags & POLYFLAG_LADDER)
  {
    Point3 closestNextPos;
    bool overNextPos;
    if (dtStatusFailed(navQuery->closestPointOnPoly(endRef, &pos.x, &closestNextPos.x, &overNextPos)))
      return 0;
    Point3 start, end;
    if (!get_offmesh_connection_end_points(NM_MAIN, connRef, &closestNextPos.x, start, end))
      return 0;
    const float minDist = 0.5f;
    const float offDist = 0.1f;
    if (over_link != OVER_LADDER_LINK)
    {
      Point2 dir = normalize(Point2::xz(end - start));
      Point2 pt = Point2::xz(pos - start);
      if (abs(dir.y * pt.x - dir.x * pt.y) > offDist)
        return 0;
      if (dir * pt < -minDist)
        return 0;
      over_link = OVER_LADDER_LINK;
    }
    out_from = start;
    out_to = end;
    dtPolyRef refs[2];
    corridor.moveOverOffmeshConnection(connRef, refs, &start.x, &end.x, navQuery);
    clear_and_shrink(corners);
    insert_item_at(corners, 0, end);
    return SHOULD_CLIMB_LADDER;
  }

  dtPolyRef curRef;
  bool overConnection = true;
  NavQueryFilter filter(custom_nav);
  filter.setIncludeFlags(POLYFLAGS_WALK | POLYFLAG_JUMP);
  filter.setExcludeFlags(0);
  if (dtStatusSucceed(navQuery->findNearestPoly(&pos.x, &extents.x, &filter, &curRef, nullptr)))
  {
    Point3 closestNextPos;
    bool overNextPos;
    if (dtStatusFailed(navQuery->closestPointOnPoly(endRef, &pos.x, &closestNextPos.x, &overNextPos)))
      return 0;
    Point3 start, end;
    if (!get_offmesh_connection_end_points(NM_MAIN, connRef, &closestNextPos.x, start, end))
      return 0;

    if (over_link != OVER_JUMP_LINK)
    {
      Point2 dir = normalize(Point2::xz(end - start));
      if (dir * Point2::xz(start - pos) > min(extents.x, extents.z)) // we haven't reached start yet
        return 0;
    }

    dtPolyRef refs[2];
    // moveOver only if endRef is the start of some jump link
    bool standaloneJumpLink = true;
    if (dtStatusSucceed(navMesh->getPolyFlags(endRef, &polyFlags)) && (polyFlags & POLYFLAG_JUMP))
    {
      // first moveOver is not in loop, cause idx may be greater than zero
      corridor.moveOverOffmeshConnection(connRef, refs, &start.x, &end.x, navQuery);
      standaloneJumpLink = false;
    }
    int pathCount = corridor.getPathCount();
    int prevPathCount = pathCount + 1;
    while (pathCount >= 2 && pathCount < prevPathCount) // move over sequential jump links till reaching the last jump link
    {
      // moveOver only if corridor.getPath()[1] is the start of some jump link
      if (dtStatusFailed(navMesh->getPolyFlags(corridor.getPath()[1], &polyFlags)) || !(polyFlags & POLYFLAG_JUMP))
        break;
      prevPathCount = pathCount;
      corridor.moveOverOffmeshConnection(corridor.getPath()[0], refs, &start.x, &end.x, navQuery);
      pathCount = corridor.getPathCount();
    }
    if (!standaloneJumpLink)
    {
      if (corridor.getPathCount() < 2)
        return 0;
      connRef = corridor.getPath()[0];
      endRef = corridor.getPath()[1];
    }
    over_link = OVER_JUMP_LINK;

    if (dtStatusFailed(navQuery->closestPointOnPoly(endRef, &pos.x, &closestNextPos.x, &overNextPos)))
      return 0;
    if (!get_offmesh_connection_end_points(NM_MAIN, connRef, &closestNextPos.x, start, end))
      return 0;
    static const float MIN_TARGET_DIST_SQ = 0.01f;
    if (lengthSq(end - pos) < MIN_TARGET_DIST_SQ)
      overConnection = false;
    if (start.y - end.y > minJumpThreshold)
      overConnection = false;
    clear_and_shrink(corners);
    insert_item_at(corners, 0, end);
    if (curRef == endRef) // we're over final polygon - prune
    {
      over_link = 0;
      corridor.moveOverOffmeshConnection(connRef, refs, &start.x, &end.x, navQuery);
    }
  }
  return overConnection ? SHOULD_JUMP : 0;
}

bool set_corridor_agent_position(dtPathCorridor &corridor, const Point3 &pos, dag::ConstSpan<Point2> *areas_cost,
  const CustomNav *custom_nav)
{
  if (!navQuery || corridor.getPathCount() < 1)
    return false;
  NavQueryFilter filter(custom_nav);
  filter.setIncludeFlags(POLYFLAGS_WALK | POLYFLAG_JUMP | POLYFLAG_LADDER);
  filter.setExcludeFlags(0);
  if (areas_cost)
    filter.setAreasCost(*areas_cost);
  return corridor.movePosition(&pos.x, navQuery, &filter);
}

bool set_corridor_target(dtPathCorridor &corridor, const Point3 &target, dag::ConstSpan<Point2> *areas_cost,
  const CustomNav *custom_nav)
{
  if (!navQuery || corridor.getPathCount() < 1)
    return false;
  NavQueryFilter filter(custom_nav);
  filter.setIncludeFlags(POLYFLAGS_WALK | POLYFLAG_JUMP);
  filter.setExcludeFlags(0);
  if (areas_cost)
    filter.setAreasCost(*areas_cost);
  return corridor.moveTargetPosition(&target.x, navQuery, &filter);
}

bool optimize_corridor_path(dtPathCorridor &corridor, const FindRequest &req, bool inc_jump, const CustomNav *custom_nav)
{
  if (!navQuery || corridor.getPathCount() < 1)
    return false;
  NavQueryFilter filter(custom_nav, req.maxJumpUpHeight, true);
  const int incFlags = POLYFLAGS_WALK | (inc_jump ? POLYFLAG_JUMP : 0);
  filter.setIncludeFlags(incFlags);
  filter.setExcludeFlags(POLYFLAG_OBSTACLE);
  filter.setAreasCost(req.areasCost);
  corridor.optimizePathTopology(navQuery, &filter);

  if (corridor.getPathCount() > 0)
  {
    const dtPolyRef *polys = corridor.getPath();
    Point3 start = Point3(corridor.getPos(), Point3::CTOR_FROM_PTR);
    Point3 target = Point3(corridor.getTarget(), Point3::CTOR_FROM_PTR);
    FindRequest auxReq = {
      start, target, POLYFLAGS_WALK, 0, req.extents, req.maxJumpUpHeight, 0, dtPolyRef(), dtPolyRef(), req.areasCost};
    // replan
    if (find_end_poly(auxReq, custom_nav) && auxReq.endPoly != polys[corridor.getPathCount() - 1])
    {
      CorridorInput inp = {
        start, target, auxReq.extents, incFlags, 0, auxReq.maxJumpUpHeight, req.startPoly, req.endPoly, req.areasCost};
      return set_path_corridor(corridor, inp, custom_nav);
    }
  }
  return false;
}

bool optimize_corridor_path(dtPathCorridor &corridor, const Point3 &extents, bool inc_jump, const CustomNav *custom_nav)
{
  FindRequest req;
  req.extents = extents;
  req.maxJumpUpHeight = FLT_MAX;
  return optimize_corridor_path(corridor, req, inc_jump, custom_nav);
}

bool is_corridor_valid(dtPathCorridor &corridor, const CustomNav *custom_nav)
{
  if (!tilecache_is_loaded())
    return true;
  NavQueryFilter filter(custom_nav);
  filter.setIncludeFlags(POLYFLAGS_WALK | POLYFLAG_JUMP | POLYFLAG_LADDER);
  filter.setExcludeFlags(0);
  // For the find_corridor_corners function, we should to check the whole path,
  // otherwise find_corridor_corners may return zero and bot will get stuck
  return corridor.isValid(corridor.getPathCount(), navQuery, &filter);
}

void draw_corridor_path(const dtPathCorridor &corridor)
{
  if (!navMesh)
    return;
  const dtPolyRef *polys = corridor.getPath();
  Point3 from = Point3(corridor.getPos(), Point3::CTOR_FROM_PTR);
  Point3 target = Point3(corridor.getTarget(), Point3::CTOR_FROM_PTR);
  draw_cached_debug_line(from, target, E3DCOLOR_MAKE(0, 0, 0, 255));
  for (int i = 0; i < corridor.getPathCount(); ++i)
  {
    const dtMeshTile *tile = nullptr;
    const dtPoly *poly = nullptr;
    if (dtStatusFailed(navMesh->getTileAndPolyByRef(polys[i], &tile, &poly)))
      break;
    Point3 polyC = get_poly_center(tile, poly);
    draw_cached_debug_line(from, polyC, E3DCOLOR_MAKE(255, 255, 255, 255));
    from = polyC;
  }
}

bool move_along_surface(FindRequest &req)
{
  if (!navMesh)
    return false;
  NavQueryFilter filter(nullptr);
  filter.setIncludeFlags(req.includeFlags);
  filter.setExcludeFlags(req.excludeFlags);
  filter.setAreasCost(req.areasCost);
  if (!navQuery->isValidPolyRef(req.startPoly, &filter))
  {
    if (dtStatusFailed(navQuery->findNearestPoly(&req.start.x, &req.extents.x, &filter, &req.startPoly, nullptr)))
      return false;
  }
  dtPolyRef visited[16];
  int nvisited = 0;
  Point3 res{};
  dtStatus result = navQuery->moveAlongSurface(req.startPoly, &req.start.x, &req.end.x, &filter, &res.x, visited, &nvisited, 16);
  if (dtStatusSucceed(result) && nvisited > 0)
  {
    req.endPoly = visited[nvisited - 1];
    float h = res.y;
    navQuery->getPolyHeight(req.endPoly, &res.x, &h);
    res[1] = h;
    req.end = res;
    return true;
  }
  return false;
}

bool is_valid_poly_ref(const FindRequest &req)
{
  if (!navQuery)
    return false;
  NavQueryFilter filter(nullptr);
  filter.setIncludeFlags(req.includeFlags);
  filter.setExcludeFlags(req.excludeFlags);
  filter.setAreasCost(req.areasCost);
  return navQuery->isValidPolyRef(req.startPoly, &filter);
}

bool navmesh_is_valid_poly_ref(dtPolyRef ref) { return navMesh && navMesh->isValidPolyRef(ref); }

bool get_triangle_from_poly(const dtPolyRef result_ref, const dtMeshTile &tile, const dtPoly &poly, const Point3 &pos, bool check_pos,
  NavMeshTriangle &result)
{
  if (EASTL_UNLIKELY(poly.getType() == DT_POLYTYPE_OFFMESH_CONNECTION))
  {
    const float *v0 = &tile.verts[poly.verts[0] * 3];
    const float *v1 = &tile.verts[poly.verts[1] * 3];
    result.p0 = Point3(v0, Point3::CTOR_FROM_PTR);
    result.p1 = Point3(v1, Point3::CTOR_FROM_PTR);
    result.p2 = result.p1;
    result.polyRef = result_ref;
    Point3 pt;
    float t;
    if (!check_pos)
      return true;
    const float distSq = distanceSqToSeg(pos, result.p1, result.p2, t, pt);
    constexpr float distThreshold = 0.01f;
    return distSq < sqr(distThreshold);
  }
  const int dimensionsInPoint = 3;
  const int pointsInTriangle = 3;
  const unsigned int polyId = (unsigned int)(&poly - tile.polys);
  const dtPolyDetail *pd = &tile.detailMeshes[polyId];
  float triangle[pointsInTriangle * dimensionsInPoint];
  for (int i = 0; i < pd->triCount; ++i)
  {
    const unsigned char *t = &tile.detailTris[(pd->triBase + i) * 4];
    for (int j = 0; j < pointsInTriangle; ++j)
    {
      float *trianglePoint =
        t[j] < poly.vertCount ? &tile.verts[poly.verts[t[j]] * 3] : &tile.detailVerts[(pd->vertBase + t[j] - poly.vertCount) * 3];
      memcpy(&triangle[j * dimensionsInPoint], trianglePoint, sizeof(float) * dimensionsInPoint);
    }
    if (!check_pos || dtPointInPolygon(&pos.x, &triangle[0], pointsInTriangle))
    {
      result.p0 = Point3(&triangle[0 * dimensionsInPoint], Point3::CTOR_FROM_PTR);
      result.p1 = Point3(&triangle[1 * dimensionsInPoint], Point3::CTOR_FROM_PTR);
      result.p2 = Point3(&triangle[2 * dimensionsInPoint], Point3::CTOR_FROM_PTR);
      result.polyRef = result_ref;
      return true;
    }
  }
  return false;
}

bool get_triangle_by_pos(const Point3 &pos, NavMeshTriangle &result, float horz_dist, int incl_flags, int excl_flags,
  const CustomNav *custom_nav, float max_vert_dist)
{
  return get_triangle_by_pos_ex(NM_MAIN, pos, result, horz_dist, incl_flags, excl_flags, custom_nav, max_vert_dist);
}

bool get_triangle_by_pos_ex(int nav_mesh_idx, const Point3 &pos, NavMeshTriangle &result, float horz_dist, int incl_flags,
  int excl_flags, const CustomNav *custom_nav, float max_vert_dist)
{
  dtPolyRef resultRef;
  if (find_polyref(nav_mesh_idx, pos, horz_dist, incl_flags, excl_flags, resultRef, custom_nav, max_vert_dist))
  {
    const dtMeshTile *bestTile = nullptr;
    const dtPoly *bestPoly = nullptr;
    get_nav_mesh_ptr(nav_mesh_idx)->getTileAndPolyByRefUnsafe(resultRef, &bestTile, &bestPoly);
    return get_triangle_from_poly(resultRef, *bestTile, *bestPoly, pos, true, result);
  }
  return false;
}

bool get_triangle_by_poly(const Point3 &pos, const dtPolyRef poly_id, NavMeshTriangle &result)
{
  const dtMeshTile *bestTile = nullptr;
  const dtPoly *bestPoly = nullptr;
  if (dtStatusSucceed(navMesh->getTileAndPolyByRef(poly_id, &bestTile, &bestPoly)))
    return get_triangle_from_poly(poly_id, *bestTile, *bestPoly, pos, true, result);
  return false;
}

bool get_triangle_by_poly(const dtPolyRef poly_id, NavMeshTriangle &result)
{
  const dtMeshTile *bestTile = nullptr;
  const dtPoly *bestPoly = nullptr;
  if (dtStatusSucceed(navMesh->getTileAndPolyByRef(poly_id, &bestTile, &bestPoly)))
    return get_triangle_from_poly(poly_id, *bestTile, *bestPoly, Point3(), false, result);
  return false;
}

bool find_nearest_triangle(const dtMeshTile &tile, const dtPoly &poly, const Point3 &pos, float dist_threshold,
  NavMeshTriangle &result)
{
  if (EASTL_UNLIKELY(poly.getType() == DT_POLYTYPE_OFFMESH_CONNECTION))
  {
    const float *v0 = &tile.verts[poly.verts[0] * 3];
    const float *v1 = &tile.verts[poly.verts[1] * 3];
    result.p0 = Point3(v0, Point3::CTOR_FROM_PTR);
    result.p1 = Point3(v1, Point3::CTOR_FROM_PTR);
    result.p2 = result.p1;
    Point3 pt;
    float t;
    const float distSq = distanceSqToSeg(pos, result.p1, result.p2, t, pt);
    return distSq < sqr(dist_threshold);
  }
  const int dimensionsInPoint = 3;
  const int pointsInTriangle = 3;
  const unsigned int polyId = (unsigned int)(&poly - tile.polys);
  const dtPolyDetail *pd = &tile.detailMeshes[polyId];
  float triangle[pointsInTriangle * dimensionsInPoint];
  float minDist = dist_threshold;
  for (int i = 0; i < pd->triCount; ++i)
  {
    const unsigned char *t = &tile.detailTris[(pd->triBase + i) * 4];
    for (int j = 0; j < pointsInTriangle; ++j)
    {
      float *trianglePoint =
        t[j] < poly.vertCount ? &tile.verts[poly.verts[t[j]] * 3] : &tile.detailVerts[(pd->vertBase + t[j] - poly.vertCount) * 3];
      memcpy(&triangle[j * dimensionsInPoint], trianglePoint, sizeof(float) * dimensionsInPoint);
    }
    auto p0 = Point3(&triangle[0 * dimensionsInPoint], Point3::CTOR_FROM_PTR);
    auto p1 = Point3(&triangle[1 * dimensionsInPoint], Point3::CTOR_FROM_PTR);
    auto p2 = Point3(&triangle[2 * dimensionsInPoint], Point3::CTOR_FROM_PTR);
    Point3 contact, normal;
    auto dist = distance_to_triangle(pos, p0, p2, p1, contact, normal);
    if (dist < minDist)
    {
      minDist = dist;
      result.p0 = Point3(&triangle[0 * dimensionsInPoint], Point3::CTOR_FROM_PTR);
      result.p1 = Point3(&triangle[1 * dimensionsInPoint], Point3::CTOR_FROM_PTR);
      result.p2 = Point3(&triangle[2 * dimensionsInPoint], Point3::CTOR_FROM_PTR);
    }
  }
  return minDist < dist_threshold;
}

bool find_nearest_triangle_by_pos(const Point3 &pos, const dtPolyRef poly_id, float dist_threshold, NavMeshTriangle &result)
{
  const dtMeshTile *bestTile = nullptr;
  const dtPoly *bestPoly = nullptr;
  if (dtStatusSucceed(navMesh->getTileAndPolyByRef(poly_id, &bestTile, &bestPoly)))
    return find_nearest_triangle(*bestTile, *bestPoly, pos, dist_threshold, result);
  return false;
}

__forceinline void squash_jumplink(const dtMeshTile &tile, const dtPoly &poly, dtOffMeshConnection &connection,
  float world_y_threshold, float min_world_y)
{
  float minY = eastl::min(connection.pos[1], connection.pos[4]);
  if (minY > world_y_threshold) // drop on ground links higher than 40% of bbox height
    minY = min_world_y;
  // drop link
  connection.pos[1] = minY;
  connection.pos[4] = connection.pos[1];
  // override tile vertices too
  dtVcopy(&tile.verts[poly.verts[0] * 3], &connection.pos[0]);
  dtVcopy(&tile.verts[poly.verts[1] * 3], &connection.pos[3]);
}

int squash_jumplinks(const TMatrix &tm, const BBox3 &bbox)
{
  if (!navMesh)
    return -1;
  int minx, miny, maxx, maxy;
  const TMatrix itm = inverse(tm);
  BBox3 bigAabb = bbox;
  bigAabb.inflate(0.2f);    // extra 0.2m horizontal threshold
  bigAabb.lim[0].y -= 0.3f; // extra 1m vertical threshold
  bigAabb.lim[1].y += 0.3f;
  const BBox3 waabb = tm * bbox;
  navMesh->calcTileLoc(&waabb.lim[0].x, &minx, &miny);
  navMesh->calcTileLoc(&waabb.lim[1].x, &maxx, &maxy);
  const float yThreshold = lerp(waabb.lim[0].y, waabb.lim[1].y, 0.4f);
  const float minY = waabb.lim[0].y;

  static const int MAX_NEIS = 64;
  const dtMeshTile *neis[MAX_NEIS];
  int res = 0;
  for (int y = miny; y <= maxy; ++y)
    for (int x = minx; x <= maxx; ++x)
    {
      // collect nearest tiles
      const int nneis = navMesh->getTilesAt(x, y, neis, MAX_NEIS);
      for (int i = 0; i < nneis; ++i)
      {
        const dtMeshTile *tile = neis[i];
        const uint64_t baseRef = navMesh->getPolyRefBase(tile);
        // iter all offmesh connections on tile
        for (int j = 0; j < tile->header->offMeshConCount; ++j)
        {
          dtOffMeshConnection &connection = tile->offMeshCons[j];
          const Point3 posA(&connection.pos[0], Point3::CTOR_FROM_PTR);
          const Point3 posB(&connection.pos[3], Point3::CTOR_FROM_PTR);
          // process only nearest links
          if (!(bigAabb & (itm * posA)) && !(bigAabb & (itm * posB)))
            continue;

          const dtPoly &connectionPoly = tile->polys[connection.poly];
          const dtPolyRef connectionRef = baseRef | connection.poly;
          squash_jumplink(*tile, connectionPoly, connection, yThreshold, minY);
          // iterate over all neighbours
          for (unsigned k = connectionPoly.firstLink; k != DT_NULL_LINK; k = tile->links[k].next)
          {
            const dtPolyRef neiRef = tile->links[k].ref;
            if (!neiRef || connectionRef == neiRef)
              continue;
            uint16_t endRefPolyFlags = 0;
            navMesh->getPolyFlags(neiRef, &endRefPolyFlags);
            if (!(endRefPolyFlags & POLYFLAG_JUMP))
              continue;
            // process only links connected with another links
            if (const dtOffMeshConnection *neiConnection = navMesh->getOffMeshConnectionByRef(neiRef))
            {
              const dtMeshTile *neiTile = nullptr;
              const dtPoly *neiPoly = nullptr;
              if (dtStatusSucceed(navMesh->getTileAndPolyByRef(neiRef, &neiTile, &neiPoly)))
              {
                dtOffMeshConnection &mutNeiConnection = const_cast<dtOffMeshConnection &>(*neiConnection);
                squash_jumplink(*neiTile, *neiPoly, mutNeiConnection, yThreshold, minY);
                ++res;
              }
            }
          }
        }
      }
    }
  return res;
}

bool find_polys_in_circle_by_tile(const dtMeshTile &tile, dag::Vector<dtPolyRef, framemem_allocator> &polys, const Point3 &pos,
  float radius, float height_half_offset)
{
  // iter all polys in tile
  bool res = false;
  float posY = pos.y;
  const dtPolyRef base = navMesh->getPolyRefBase(&tile);
  BSphere3 bsphere(pos, radius);
  for (int k = 0; k < tile.header->polyCount; ++k)
  {
    dtPoly &poly = tile.polys[k];
    if (EASTL_UNLIKELY(poly.getType() == DT_POLYTYPE_OFFMESH_CONNECTION))
    {
      continue;
    }
    const int dimensionsInPoint = 3;
    const int pointsInTriangle = 3;
    const unsigned int polyId = (unsigned int)(&poly - tile.polys);
    const dtPolyDetail *pd = &tile.detailMeshes[polyId];
    float triangle[pointsInTriangle * dimensionsInPoint];
    for (int i = 0; i < pd->triCount; ++i)
    {
      const uint8_t *t = &tile.detailTris[(pd->triBase + i) * 4];
      for (int j = 0; j < pointsInTriangle; ++j)
      {
        float *trianglePoint =
          t[j] < poly.vertCount ? &tile.verts[poly.verts[t[j]] * 3] : &tile.detailVerts[(pd->vertBase + t[j] - poly.vertCount) * 3];
        memcpy(&triangle[j * dimensionsInPoint], trianglePoint, sizeof(float) * dimensionsInPoint);
      }
      auto p0 = Point3(&triangle[0 * dimensionsInPoint], Point3::CTOR_FROM_PTR);
      auto p1 = Point3(&triangle[1 * dimensionsInPoint], Point3::CTOR_FROM_PTR);
      auto p2 = Point3(&triangle[2 * dimensionsInPoint], Point3::CTOR_FROM_PTR);
      const float deltaH0 = p0.y - posY;
      const float deltaH1 = p1.y - posY;
      const float deltaH2 = p2.y - posY;
      if (test_triangle_sphere_intersection(p0, p1, p2, bsphere) &&
          !(deltaH0 > height_half_offset && deltaH1 > height_half_offset && deltaH2 > height_half_offset) &&
          !(-deltaH0 > height_half_offset && -deltaH1 > height_half_offset && -deltaH2 > height_half_offset))
      {
        const dtPolyRef polyRef = base | (dtPolyRef)k;
        polys.push_back(polyRef);
        res = true;
        break;
      }
    }
  }
  return res;
}

bool find_polys_in_circle(dag::Vector<dtPolyRef, framemem_allocator> &polys, const Point3 &pos, float radius, float height_half_offset)
{
  if (!navMesh)
    return false;
  polys.clear();
  int minx, miny, maxx, maxy;
  BBox3 bbox(pos, radius * 2.f);
  bbox.inflateXZ(0.2f);  // extra 0.2m horizontal threshold
  bbox.lim[0].y = -0.1f; // flatten vertical threshold
  bbox.lim[1].y = 0.1f;
  navMesh->calcTileLoc(&bbox.lim[0].x, &minx, &miny);
  navMesh->calcTileLoc(&bbox.lim[1].x, &maxx, &maxy);

  static const int MAX_NEIS = 64;
  const dtMeshTile *neis[MAX_NEIS];
  bool res = false;
  for (int y = miny; y <= maxy; ++y)
    for (int x = minx; x <= maxx; ++x)
    {
      // collect nearest tiles
      const int nneis = navMesh->getTilesAt(x, y, neis, MAX_NEIS);
      for (int i = 0; i < nneis; ++i)
      {
        const dtMeshTile *tile = neis[i];
        res |= find_polys_in_circle_by_tile(*tile, polys, pos, radius, height_half_offset);
      }
    }
  return res;
}

void mark_polygons_lower(float world_level, uint8_t area_id)
{
  if (!navMesh)
    return;

  for (int tileId = 0, maxTiles = navMesh->getMaxTiles(); tileId < maxTiles; ++tileId)
  {
    const dtMeshTile *tile = const_cast<const dtNavMesh *>(navMesh)->getTile(tileId);
    if (!tile->header)
      continue;

    const dtPolyRef base = navMesh->getPolyRefBase(tile);

    for (dtPolyRef polyId = 0, maxPolys = tile->header->polyCount; polyId < maxPolys; ++polyId)
    {
      unsigned char polyArea = POLYAREA_UNWALKABLE;
      if (!get_poly_area(base | polyId, polyArea) || polyArea == POLYAREA_OBSTACLE)
        continue;

      const dtPoly *poly = &tile->polys[polyId];
      if (poly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
        continue;

      const Point3 polyCenter = get_poly_center(tile, poly);
      if (polyCenter.y < world_level)
        set_poly_area(base | polyId, area_id);
    }
  }
}

void mark_polygons_upper(float world_level, uint8_t area_id)
{
  if (!navMesh)
    return;

  for (int tileId = 0, maxTiles = navMesh->getMaxTiles(); tileId < maxTiles; ++tileId)
  {
    const dtMeshTile *tile = const_cast<const dtNavMesh *>(navMesh)->getTile(tileId);
    if (!tile->header)
      continue;

    const dtPolyRef base = navMesh->getPolyRefBase(tile);

    for (dtPolyRef polyId = 0, maxPolys = tile->header->polyCount; polyId < maxPolys; ++polyId)
    {
      unsigned char polyArea = POLYAREA_UNWALKABLE;
      if (!get_poly_area(base | polyId, polyArea) || polyArea == POLYAREA_OBSTACLE)
        continue;

      const dtPoly *poly = &tile->polys[polyId];
      if (poly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
        continue;

      const dtPolyDetail *pd = &tile->detailMeshes[polyId];
      const Point3 polyTop = get_poly_top(tile, poly, pd);
      if (polyTop.y > world_level)
        set_poly_area(base | polyId, area_id);
    }
  }
}
} // namespace pathfinder
