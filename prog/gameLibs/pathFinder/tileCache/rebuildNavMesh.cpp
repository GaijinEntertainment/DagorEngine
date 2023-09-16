#include <pathFinder/pathFinder.h>
#include <detourCommon.h>
#include <math/dag_mathUtils.h>
#include <detourNavMesh.h>

#include <regExp/regExp.h>

#include <osApiWrappers/dag_direct.h>
#include <rendInst/rendinstHashMap.h>

#include <pathFinder/tileCache.h>
#include <pathFinder/tileCacheRI.h>
#include <pathFinder/tileCacheUtil.h>

#include <pathFinder/tileRICommon.h>
#include <recastTools/recastNavMeshTile.h>
#include <gamePhys/collision/collisionLib.h>

#include <osApiWrappers/dag_files.h>
#include <ioSys/dag_dataBlock.h>


namespace pathfinder
{
extern dtNavMesh *navMesh;
extern dtTileCache *tileCache;

extern RiexHashMap<RiObstacle> riHandle2obstacle;

void renderDebugReset();

static Tab<uint32_t> removedNavMeshTiles;
static ska::flat_hash_set<uint32_t> removedObstacles;
static ska::flat_hash_set<uint32_t> addedObstacles;

Tab<dtTileRef> tilesToSave;
Tab<dtCompressedTileRef> tileCToSave;

struct RebuildTilesHasher
{
  size_t operator()(const eastl::pair<int, int> &key) const
  {
    return (size_t)(((uint32_t)key.second * 16777619) ^ (uint32_t)key.first);
  }
};

struct RebuildNavMeshSetup
{
  float agentMaxSlope = 60.0f;
  int vertsPerPoly = 3;
  float regionMinSize = 9.0f;
  float regionMergeSize = 100.0f;
  int minBorderSize = 3;
  float detailSampleDist = 3.0f;
  float detailSampleMaxError = 2.0f;
  float edgeMaxLen = 128.0f;
  float waterLevel = 0.0f;
};
RebuildNavMeshSetup rebuildParams;
enum ERebuildStep
{
  RS_UNINIT,
  RS_WAIT_ADD_TILES,
  RS_REBUILDING_TILES,
  RS_GENERATING_JUMPLINKS,
  RS_GENERATING_COVERS,
  RS_GENERATING_OVERLINKS_LADDERS,
  RS_FINISHED,
};
ERebuildStep rebuildStep = RS_UNINIT;
typedef ska::flat_hash_map<eastl::pair<int, int>, eastl::pair<float, float>, RebuildTilesHasher> RebuildTiles;
static RebuildTiles rebuildedTiles;
int rebuildedTilesTotalSz = 0;
static RebuildTiles generateTiles;

struct MarkData
{
  Point3 c;
  Point3 e;
  float y;

  rendinst::riex_handle_t h;
  uint32_t r;
};

// This version differs from the one used by the editor - a different format for iterating renderinsts in a loaded game
// and a different format for marking obstacles:
static class NavmeshLayers
{
  bool isLoaded;

  void buildFilter(Tab<eastl::unique_ptr<RegExp>> &filter, const DataBlock *filterBlk)
  {
    for (int i = 0; i < filterBlk->blockCount(); i++)
    {
      SimpleString filterVal(filterBlk->getBlock(i)->getBlockName());
      eastl::unique_ptr<RegExp> re(new RegExp);
      if (re->compile(filterVal.str()))
        filter.push_back(eastl::move(re));
      else
        logerr("Wrong regExp \"%s\"", filterVal.str());
    }
  }

  template <typename Lamda>
  void applyRegExp(const DataBlock *filterBlk, Lamda lambda)
  {
    if (!filterBlk)
      return;

    Tab<eastl::unique_ptr<RegExp>> filter(tmpmem);
    buildFilter(filter, filterBlk);

    for (int i = 0, size = rendinst::getRiGenExtraResCount(); i < size; ++i)
      for (const auto &re : filter)
        if (re->test(rendinst::getRIGenExtraName(i)))
          lambda((int)i);
  }

public:
  Bitarray pools;
  ska::flat_hash_set<int> transparentPools;
  ska::flat_hash_map<int, uint32_t> obstaclePools;
  ska::flat_hash_map<int, uint32_t> materialPools;
  ska::flat_hash_map<int, float> navMeshOffsetPools;

  NavmeshLayers() : isLoaded(false) {}

  void load()
  {
    if (isLoaded)
      return;

    const DataBlock *settingsBlk = ::dgs_get_settings();
    const char *navmeshLayersBlkFn = settingsBlk->getStr("navmeshLayers", "config/navmesh_layers.blk");
    const char *rendinstDmgBlkFn = settingsBlk->getStr("rendinstDmg", "config/rendinst_dmg.blk");
    const char *navObstacleBlkFn = settingsBlk->getStr("navmeshObstacles", "config/navmesh_obstacles.blk");

    DataBlock navmblk;
    if (!dd_file_exists(navmeshLayersBlkFn))
    {
      logerr("%s not found", navmeshLayersBlkFn);
      return;
    }
    navmblk.load(navmeshLayersBlkFn);

    pools.resize(rendinst::getRiGenExtraResCount());
    pools.reset();

    obstaclePools.clear();

    applyRegExp(navmblk.getBlock(navmblk.findBlock("filter")), [&](int pool) { pools.set(pool); });
    applyRegExp(navmblk.getBlock(navmblk.findBlock("filter_exclude")), [&](int pool) { pools.reset(pool); });
    applyRegExp(navmblk.getBlock(navmblk.findBlock("filter_include")), [&](int pool) { pools.set(pool); });

    for (int blkIt = 0; blkIt < navmblk.blockCount(); blkIt++)
    {
      const DataBlock *blk = navmblk.getBlock(blkIt);
      if (blk->getBool("ignoreCollision", false))
      {
        for (int i = 0; i < blk->blockCount(); i++)
        {
          int pool = rendinst::getRIGenExtraResIdx(blk->getBlock(i)->getBlockName());
          if (pool > -1)
            pools.set(pool);
        }
      }
      else
      {
        for (int i = 0; i < blk->blockCount(); i++)
        {
          int pool = rendinst::getRIGenExtraResIdx(blk->getBlock(i)->getBlockName());
          if (pool > -1)
            pools.reset(pool);
        }
      }

      if (blk->getBool("ignoreTracing", false))
      {
        for (int i = 0; i < blk->blockCount(); i++)
        {
          int pool = rendinst::getRIGenExtraResIdx(blk->getBlock(i)->getBlockName());
          if (pool > -1 && transparentPools.count(pool) == 0)
            transparentPools.emplace(pool);
        }
      }
    }

    isLoaded = true;

    DataBlock dmgblk;
    if (!dd_file_exists(rendinstDmgBlkFn))
    {
      logerr("%s not found", rendinstDmgBlkFn);
      return;
    }

    dmgblk.load(rendinstDmgBlkFn);

    const DataBlock *riExtraBlk = dmgblk.getBlockByName("riExtra");
    if (!riExtraBlk)
    {
      logwarn("%s not found or riExtra block is missing inside it, tilecached navmesh will be built without obstacles",
        rendinstDmgBlkFn);
      return;
    }

    ska::flat_hash_set<uint32_t> obstacleResHashes;
    ska::flat_hash_set<uint32_t> materialResHashes;

    for (int blkIt = 0; blkIt < riExtraBlk->blockCount(); blkIt++)
    {
      const DataBlock *blk = riExtraBlk->getBlock(blkIt);
      int pool = rendinst::getRIGenExtraResIdx(blk->getBlockName());
      if (pool <= -1)
        continue;
      // All riExtras that have hp or destructionImpulse are considered as temporary obstacles,
      // additionally you can specify isObstacle=false to not make it an obstacle. One use case for this
      // are RIs that have very large bounding boxes and you don't want the bots to avoid them. Another use case
      // are RIs that looks like they're 80% or so destroyed and a bot can walk through even when RI isn't totally destroyed.
      if ((blk->getReal("hp", 0.0f) > 0.0f || blk->getReal("destructionImpulse", 0.0f) > 0.0f) && blk->getBool("isObstacle", true))
        if (obstaclePools.count(pool) == 0)
        {
          uint32_t hash = str_hash_fnv1(blk->getBlockName());
          if (!obstacleResHashes.emplace(hash).second)
            logerr("Obstacle resource hash collision for %s, expect missing/extra obstacles in level!", blk->getBlockName());
          obstaclePools.emplace(pool, hash);
        }
      const char *material = blk->getStr("material", "");
      if (strcmp(material, "barbwire") == 0)
        if (materialPools.count(pool) == 0)
        {
          uint32_t hash = str_hash_fnv1(blk->getBlockName());
          if (!materialResHashes.emplace(hash).second)
            logerr("Material resource hash collision for %s, expect missing/extra materials in level!", blk->getBlockName());
          materialPools.emplace(pool, hash);
        }
    }


    DataBlock navObstaclesBlk;
    if (!dd_file_exists(navObstacleBlkFn))
    {
      logerr("%s not found", navObstacleBlkFn);
      return;
    }

    if (dblk::load(navObstaclesBlk, navObstacleBlkFn, dblk::ReadFlag::ROBUST))
    {
      for (int blkIt = 0; blkIt < navObstaclesBlk.blockCount(); blkIt++)
      {
        const DataBlock *blk = navObstaclesBlk.getBlock(blkIt);
        int pool = rendinst::getRIGenExtraResIdx(blk->getBlockName());
        if (pool <= -1)
          continue;

        const float navMeshBoxOffset = blk->getReal("navMeshBoxOffset", 0.0f);
        if (navMeshBoxOffset > 0.0f)
        {
          if (navMeshOffsetPools.count(pool) == 0)
            navMeshOffsetPools.emplace(pool, navMeshBoxOffset);
        }
      }
    }
  }
} navmeshLayers;

struct RendinstVertexDataCbGame : public rendinst::RendinstVertexDataCbBase
{
  Tab<IPoint2> &transparent;
  Tab<MarkData> &obstacles;

  RendinstVertexDataCbGame(Tab<Point3> &verts, Tab<int> &inds, Tab<IPoint2> &transparent, Bitarray &pools,
    ska::flat_hash_map<int, uint32_t> &obstaclePools, ska::flat_hash_map<int, uint32_t> &materialPools,
    ska::flat_hash_map<int, float> &navMeshOffsetPools, Tab<MarkData> &obstacles) :
    RendinstVertexDataCbBase(verts, inds, pools, obstaclePools, materialPools, navMeshOffsetPools),
    transparent(transparent),
    obstacles(obstacles)
  {}
  ~RendinstVertexDataCbGame() { clear_all_ptr_items(riCache); }

  virtual void processCollision(const rendinst::RendInstCollisionCB::CollisionInfo &coll_info) override
  {
    if (!coll_info.collRes)
      return;

    RiData *data = NULL;
    for (int i = 0; i < riCache.size(); ++i)
    {
      if (*riCache[i] == coll_info)
      {
        data = riCache[i];
        break;
      }
    }

    if (!data)
    {
      // found new one, add it.
      RiData *rdata = new RiData;
      riCache.push_back(rdata);
      rdata->build(coll_info);
      data = rdata;
    }

    mat44f instTm;
    v_mat44_make_from_43cu_unsafe(instTm, coll_info.tm.array);
    const int indBase = indices.size();
    int idxBase = vertices.size();

    auto materialIt = navmeshLayers.materialPools.find(coll_info.desc.pool);
    auto obstacleIt = navmeshLayers.obstaclePools.find(coll_info.desc.pool);
    if (materialIt == navmeshLayers.materialPools.end() && obstacleIt == navmeshLayers.obstaclePools.end())
    {
      Point3_vec4 tmpVert;
      for (int i = 0; i < data->vertices.size(); ++i)
      {
        v_st(&tmpVert.x, v_mat44_mul_vec3p(instTm, data->vertices[i]));
        vertices.push_back(tmpVert);
      }
      for (int i = 0; i < data->indices.size(); ++i)
        indices.push_back(data->indices[i] + idxBase);

      if (navmeshLayers.transparentPools.find(coll_info.desc.pool) != navmeshLayers.transparentPools.end())
        transparent.push_back({indBase, (int)data->indices.size()});
    }

    if (obstacleIt != navmeshLayers.obstaclePools.end())
    {
      TMatrix tm = rendinst::getRIGenMatrix(coll_info.desc);
      BBox3 oobb = rendinst::getRIGenBBox(coll_info.desc);

      Point3 c, ext;
      float angY;
      // We need to get unpadded obstacle aabb size, so we pass zero padding.
      tilecache_calc_obstacle_pos(tm, oobb, tileCache->getParams()->cs, ZERO<Point2>(), c, ext, angY);
      if (!tilecache_ri_obstacle_too_low(ext.y * 2.0f, tileCache->getParams()->walkableClimb))
      {
        MarkData markData;
        tilecache_calc_obstacle_pos(tm, oobb, tileCache->getParams()->cs,
          Point2(tileCache->getParams()->walkableRadius, tileCache->getParams()->walkableHeight), markData.c, markData.e, markData.y);

        markData.h = coll_info.desc.getRiExtraHandle();
        markData.r = obstacleIt->second;
        obstacles.push_back(markData);
      }
    }
  }
};

static bool finalize_navmesh_tilecached_tile(rcContext &ctx, const rcConfig &cfg, recastnavmesh::RecastTileContext &tile_ctx, int tx,
  int ty, const Tab<MarkData> &obstacles, Tab<recastnavmesh::BuildTileData> &tile_data)
{
  auto fn = [](const Tab<MarkData> &obstacles, const rcConfig &cfg, dtTileCacheLayer &layer, const dtTileCacheLayerHeader &header) {
    for (const auto &obs : obstacles)
    {
      float coshalf = cosf(+0.5f * obs.y);
      float sinhalf = sinf(-0.5f * obs.y);
      float rotAux[2] = {coshalf * sinhalf, coshalf * coshalf - 0.5f};

      dtMarkBoxArea(layer, header.bmin, cfg.cs, cfg.ch, &obs.c.x, &obs.e.x, &rotAux[0], 0);
    }
  };

  return finalize_navmesh_tilecached_tile(ctx, cfg, tileCache->getAlloc(), tileCache->getCompressor(), nullptr, tile_ctx, tx, ty,
    tileCache->getParams()->walkableClimb, tileCache->getParams()->walkableHeight, tileCache->getParams()->walkableRadius, obstacles,
    tile_data, fn);
}

static void init_tile_config(rcConfig &cfg, const Tab<Point3> &vertices)
{
  memset(&cfg, 0, sizeof(cfg));

  cfg.cs = tileCache->getParams()->cs;
  cfg.ch = tileCache->getParams()->ch;

  cfg.walkableSlopeAngle = rebuildParams.agentMaxSlope;
  cfg.walkableHeight = (int)ceilf(tileCache->getParams()->walkableHeight / cfg.ch);
  cfg.walkableClimb = (int)ceilf(tileCache->getParams()->walkableClimb / cfg.ch);
  cfg.walkableRadius = (int)ceilf(tileCache->getParams()->walkableRadius / cfg.cs);
  cfg.maxEdgeLen = (int)(rebuildParams.edgeMaxLen / cfg.cs);

  cfg.maxSimplificationError = tileCache->getParams()->maxSimplificationError;
  cfg.minRegionArea = int(rebuildParams.regionMinSize * sqr(safeinv(cfg.cs)));
  cfg.mergeRegionArea = int(rebuildParams.regionMergeSize * sqr(safeinv(cfg.cs)));
  cfg.maxVertsPerPoly = rebuildParams.vertsPerPoly;
  cfg.tileSize = tileCache->getParams()->width;
  cfg.borderSize = cfg.walkableRadius + rebuildParams.minBorderSize;
  cfg.width = tileCache->getParams()->width + cfg.borderSize * 2;
  cfg.height = tileCache->getParams()->height + cfg.borderSize * 2;
  cfg.detailSampleDist = rebuildParams.detailSampleDist < 0.9f ? 0 : cfg.cs * rebuildParams.detailSampleDist;
  cfg.detailSampleMaxError = cfg.ch * rebuildParams.detailSampleMaxError;

  // REF: recastNavMesh.cpp(buildAndWriteNavMesh): float min_h = 0, max_h = water_lev;
  float minY = 0.0f;
  float maxY = rebuildParams.waterLevel;

  for (auto vx : vertices)
  {
    if (vx.y < minY)
      minY = vx.y;

    if (vx.y > maxY)
      maxY = vx.y;
  }

  // REF: recastNavMesh.cpp(buildAndWriteNavMesh): landBBox[0].set_xVy(nav_area[0], max(min_h, water_lev) - 1);
  //                                               landBBox[1].set_xVy(nav_area[1], max_h + 1);
  minY = max(minY, rebuildParams.waterLevel) - 1.0f;
  maxY += 1.0f;

  cfg.bmin[1] = minY;
  cfg.bmax[1] = maxY;
}

static bool prepare_tile_context(rcContext &ctx, rcConfig &cfg, recastnavmesh::RecastTileContext &tile_ctx, int tx, int ty,
  const Tab<Point3> &vertices, const Tab<int> &indices, const Tab<IPoint2> &transparent)
{
  const float tileBoxExt = cfg.borderSize * cfg.cs;
  const float tileSize = cfg.cs * cfg.tileSize;

  cfg.bmin[0] = tileCache->getParams()->orig[0] + tx * tileSize - tileBoxExt;
  cfg.bmin[2] = tileCache->getParams()->orig[2] + ty * tileSize - tileBoxExt;

  cfg.bmax[0] = tileCache->getParams()->orig[0] + (tx + 1) * tileSize + tileBoxExt;
  cfg.bmax[2] = tileCache->getParams()->orig[2] + (ty + 1) * tileSize + tileBoxExt;

  return prepare_tile_context(ctx, cfg, tile_ctx, vertices, indices, transparent, false);
}

void collect_rendinst(const BBox3 &box, Tab<Point3> &vertices, Tab<int> &indices, Tab<IPoint2> &transparent, Tab<MarkData> &obstacles)
{
  navmeshLayers.load();

  RendinstVertexDataCbGame cb(vertices, indices, transparent, navmeshLayers.pools, navmeshLayers.obstaclePools,
    navmeshLayers.materialPools, navmeshLayers.navMeshOffsetPools, obstacles);
  rendinst::testObjToRIGenIntersection(box, TMatrix::IDENT, cb, rendinst::GatherRiTypeFlag::RiGenAndExtra);
  cb.procAllCollision();
}

void collect_height_map_geometry(const BBox3 &box, Tab<Point3> &vertices, Tab<int> &indices)
{
  const float traceStep = 0.5;

  const int w = box.width().x / traceStep;
  const int h = box.width().z / traceStep;

  vertices.resize((w * h));
  indices.resize((w - 1) * (h - 1) * 6);

  float startXfloat = box.boxMin().x;
  float startZfloat = box.boxMin().z;

  float *verticesPtr = &vertices[0].x;

  for (int x = 0; x < w; ++x)
  {
    for (int z = 0; z < h; ++z, verticesPtr += 3)
    {
      verticesPtr[0] = startXfloat + traceStep * x;
      verticesPtr[2] = startZfloat + traceStep * z;

      verticesPtr[1] = dacoll::traceht_hmap(Point2(verticesPtr[0], verticesPtr[2]));
    }
  }

  int *indicesPtr = &indices[0];

  for (int x = 0; x < w - 1; ++x)
  {
    for (int z = 0; z < h - 1; ++z)
    {
      *(indicesPtr++) = z + x * w + w;
      *(indicesPtr++) = z + x * w;
      *(indicesPtr++) = z + x * w + 1;

      *(indicesPtr++) = z + x * w + w;
      *(indicesPtr++) = z + x * w + 1;
      *(indicesPtr++) = z + x * w + w + 1;
    }
  }
}

void rebuildNavMesh_init()
{
  rebuildParams = RebuildNavMeshSetup();

  navmeshLayers = NavmeshLayers();

  clear_and_shrink(tilesToSave);
  clear_and_shrink(tileCToSave);

  addedObstacles.clear();
  removedObstacles.clear();

  rebuildNavMesh_close();

  rebuildStep = RS_WAIT_ADD_TILES;

  rebuildedTiles.clear();
  rebuildedTiles.reserve(1000);
  rebuildedTilesTotalSz = 0;

  generateTiles.clear();
}

void rebuildNavMesh_setup(const char *name, float value)
{
  if (!name)
    return;
  else if (strcmp(name, "agentMaxSlope") == 0)
    rebuildParams.agentMaxSlope = value;
  else if (strcmp(name, "vertsPerPoly") == 0)
    rebuildParams.vertsPerPoly = (int)floorf(value + 0.5f);
  else if (strcmp(name, "regionMinSize") == 0)
    rebuildParams.regionMinSize = value;
  else if (strcmp(name, "regionMergeSize") == 0)
    rebuildParams.regionMergeSize = value;
  else if (strcmp(name, "minBorderSize") == 0)
    rebuildParams.minBorderSize = (int)floorf(value + 0.5f);
  else if (strcmp(name, "detailSampleDist") == 0)
    rebuildParams.detailSampleDist = value;
  else if (strcmp(name, "detailSampleMaxError") == 0)
    rebuildParams.detailSampleMaxError = value;
  else if (strcmp(name, "edgeMaxLen") == 0)
    rebuildParams.edgeMaxLen = value;
  else if (strcmp(name, "waterLevel") == 0)
    rebuildParams.waterLevel = value;
  else
    logdbg("Unknown rebuildNavMesh_setup param: %s, value: %f", name, value);
}

void rebuildNavMesh_addBBox(const BBox3 &bbox)
{
  if (rebuildStep != RS_WAIT_ADD_TILES)
    return;

  if (!tilecache_is_inside(bbox))
    return;

  const float tw = tileCache->getParams()->width * tileCache->getParams()->cs;
  const float th = tileCache->getParams()->height * tileCache->getParams()->cs;

  const int tx0 = (int)dtMathFloorf((bbox.boxMin().x - tileCache->getParams()->orig[0]) / tw);
  const int tx1 = (int)dtMathFloorf((bbox.boxMax().x - tileCache->getParams()->orig[0]) / tw);
  const int ty0 = (int)dtMathFloorf((bbox.boxMin().z - tileCache->getParams()->orig[2]) / th);
  const int ty1 = (int)dtMathFloorf((bbox.boxMax().z - tileCache->getParams()->orig[2]) / th);

  for (int ty = ty0; ty <= ty1; ++ty)
  {
    for (int tx = tx0; tx <= tx1; ++tx)
    {
      float bmin = bbox.lim[0].y;
      float bmax = bbox.lim[1].y;

      auto it = rebuildedTiles.find(eastl::pair<int, int>(tx, ty));
      if (it != rebuildedTiles.end())
      {
        bmin = min(bmin, it->second.first);
        bmax = max(bmax, it->second.second);
      }

      rebuildedTiles[eastl::pair<int, int>(tx, ty)] = eastl::pair<float, float>(bmin, bmax);
    }
  }
}

void rebuildNavMesh_update_reloadNavMesh();
void rebuildNavMesh_update_removeTiles();
bool rebuildNavMesh_update_buildTiles(int n);

bool rebuildNavMesh_update(bool interactive)
{
  const int maxTiles = interactive ? 1 : rebuildedTiles.size();

  bool result = false;
  switch (rebuildStep)
  {
    default:
    case RS_UNINIT: result = false; break;

    case RS_WAIT_ADD_TILES:
      rebuildNavMesh_update_reloadNavMesh();
      rebuildNavMesh_update_removeTiles();
      generateTiles = rebuildedTiles;
      rebuildStep = RS_REBUILDING_TILES;
      [[fallthrough]];
    case RS_REBUILDING_TILES:
      result = rebuildNavMesh_update_buildTiles(maxTiles);
      if (rebuildedTiles.empty())
        rebuildStep = RS_FINISHED;
      break;

    case RS_FINISHED:
      // bool upToDate = false;
      // if (DT_SUCCESS != tileCache->update(0, navMesh, &upToDate) || !upToDate)
      //   return false;
      result = true;
      break;
  }
  return result;
}

void rebuildNavMesh_update_reloadNavMesh()
{
  const int maxTiles = 8;
  const int extraTiles = rebuildedTiles.size() * maxTiles;
  reload_nav_mesh(extraTiles);
  navMesh->reconstructFreeList();
}

void rebuildNavMesh_update_removeTiles()
{
  navMesh->reconstructFreeList();

  for (auto it = rebuildedTiles.begin(); it != rebuildedTiles.end(); ++it)
  {
    int tx = it->first.first;
    int ty = it->first.second;

    // remove tiles
    {
      const int maxTiles = 32;
      dtCompressedTileRef tiles[maxTiles];
      const int ntiles = tileCache->getTilesAt(tx, ty, tiles, maxTiles);

      for (int i = 0; i < ntiles; ++i)
      {
        const dtCompressedTile *tile = tileCache->getTileByRef(tiles[i]);

        dtTileRef tileRef = navMesh->getTileRefAt(tile->header->tx, tile->header->ty, tile->header->tlayer);

        removedNavMeshTiles.push_back(navMesh->decodePolyIdTile(tileRef));

        dtStatus status1 = navMesh->removeTile(tileRef, 0, 0);
        dtStatus status2 = tileCache->removeTile(tiles[i], NULL, NULL);

        if (!dtStatusSucceed(status1) || !dtStatusSucceed(status2))
        {
          logerr("Rebuild NavMesh: failed to remove tile at (%d,%d) layer %d", tx, ty, tile->header->tlayer);
        }
      }
    }
  }
  navMesh->reconstructFreeList();
}

bool rebuildNavMesh_update_buildTiles(int n)
{
  Tab<IPoint2> transparent;
  Tab<MarkData> obstacles;

  for (int i = 0; i < n && !rebuildedTiles.empty(); ++i)
  {
    const float tileSize = tileCache->getParams()->cs * tileCache->getParams()->width;

    int tx = rebuildedTiles.begin()->first.first;
    int ty = rebuildedTiles.begin()->first.second;

    BBox3 bbox;

    bbox.lim[0] = Point3(tileCache->getParams()->orig[0] + tx * tileSize, rebuildedTiles.begin()->second.first,
      tileCache->getParams()->orig[2] + ty * tileSize);

    bbox.lim[1] = Point3(tileCache->getParams()->orig[0] + (tx + 1) * tileSize, rebuildedTiles.begin()->second.second,
      tileCache->getParams()->orig[2] + (ty + 1) * tileSize);

    Tab<Point3> vertices;
    Tab<int> indices;

    BBox3 extGeomBox(bbox);
    extGeomBox.inflate(tileCache->getParams()->width * tileCache->getParams()->cs);

    collect_height_map_geometry(extGeomBox, vertices, indices);
    collect_rendinst(extGeomBox, vertices, indices, transparent, obstacles);
    const Tab<IPoint2> noTransparent;

    // build tiles
    {
      rcContext ctx;
      rcConfig cfg;

      init_tile_config(cfg, vertices);

      recastnavmesh::RecastTileContext tile_ctx;
      Tab<recastnavmesh::BuildTileData> tile_data;

      if (!prepare_tile_context(ctx, cfg, tile_ctx, tx, ty, vertices, indices, noTransparent))
      {
        logerr("Rebuild NavMesh: failed to prepare tile context at (%d,%d)", tx, ty);
        continue;
      }

      // TODO LATER Use transparent array to build heightmap for covers tracing without transparent geometry
      // TODO LATER when covers generation added here.

      if (!finalize_navmesh_tilecached_tile(ctx, cfg, tile_ctx, tx, ty, obstacles, tile_data))
      {
        logerr("Rebuild NavMesh: failed to generate navmesh tiles at (%d,%d)", tx, ty);
        continue;
      }

      for (int i = 0; i < tile_data.size(); ++i)
      {
        if (tile_data[i].tileCacheDataSz == 0 || tile_data[i].navMeshDataSz == 0)
          continue;

        rebuildedTilesTotalSz += tile_data[i].tileCacheDataSz;
        rebuildedTilesTotalSz += tile_data[i].navMeshDataSz;

        dtCompressedTileRef res = 0;
        dtTileRef nav = 0;

        {
          dtStatus status =
            tileCache->addTile(tile_data[i].tileCacheData, tile_data[i].tileCacheDataSz, DT_COMPRESSEDTILE_FREE_DATA, &res);

          if (dtStatusSucceed(status) && res != 0)
            tileCToSave.push_back(res);
          else
          {
            logerr("Rebuild NavMesh: failed to add tilecache tile at (%d,%d)", tx, ty);
          }
        }

        {
          dtStatus status = navMesh->addTile(tile_data[i].navMeshData, tile_data[i].navMeshDataSz, DT_TILE_FREE_DATA, 0, &nav);

          if (dtStatusSucceed(status) && nav != 0)
            tilesToSave.push_back(nav);
          else
          {
            logerr("Rebuild NavMesh: failed to add navmesh tile at (%d,%d)", tx, ty);
          }
        }

        tile_ctx.clearIntermediate(nullptr);
      }
    }

    rebuildedTiles.erase(rebuildedTiles.begin());
  }

  Tab<obstacle_handle_t> removedHandles;

  for (const auto &obstacle : obstacles)
  {
    auto it = riHandle2obstacle.find(obstacle.h);
    if (it != riHandle2obstacle.end())
    {
      removedHandles.push_back(it->second.obstacle_handle);
      removedObstacles.insert(obstacle.r);
    }
  }

  for (const auto &obstacle : obstacles)
  {
    riHandle2obstacle[obstacle.h].obstacle_handle = tilecache_obstacle_add(obstacle.c, obstacle.e, obstacle.y, true, false);
    addedObstacles.insert(obstacle.r);
  }

  for (const auto &obstacle : removedHandles)
    tilecache_obstacle_remove(obstacle, false);

  // logdbg("rebuild_tiles: total size %d bytes, %d tiles left", rebuildedTilesTotalSz, rebuildedTiles.size());
  return true;
}

int rebuildNavMesh_getProgress()
{
  if (rebuildStep == RS_UNINIT)
    return 0;
  if (rebuildStep == RS_FINISHED)
    return 100;
  int value = 100;
  if (!generateTiles.empty())
    value = 100 - (100 * rebuildedTiles.size()) / generateTiles.size();
  return (value < 1) ? 1 : (value > 99) ? 99 : value;
}

int rebuildNavMesh_getTotalTiles()
{
  if (rebuildStep == RS_UNINIT)
    return 0;
  return generateTiles.size();
}

bool rebuildNavMesh_saveToFile(const char *file_name)
{
  eastl::unique_ptr<void, decltype(&df_close)> h(df_open(file_name, DF_WRITE | DF_CREATE), &df_close);

  if (!h)
    return false;

  uint32_t totalSize = tilesToSave.size() * sizeof(dtTileRef) + tilesToSave.size() * sizeof(uint32_t) +
                       tileCToSave.size() * sizeof(uint32_t) + removedObstacles.size() * sizeof(uint32_t) +
                       addedObstacles.size() * sizeof(uint32_t) + 4 * sizeof(uint32_t);

  // Step 1: calc total size
  for (auto tileToSave : tileCToSave)
  {
    const dtCompressedTile *tile = tileCache->getTileByRef(tileToSave);

    totalSize += tile ? tile->dataSize : 0;
  }

  for (auto tileToSave : tilesToSave)
  {
    const dtMeshTile *tile = navMesh->getTileByRef(tileToSave);

    totalSize += tile ? tile->dataSize : 0;
  }

  if (df_write(h.get(), &totalSize, sizeof(uint32_t)) != sizeof(uint32_t))
    return false;

  // Step 2: save removed obstacles
  {
    uint32_t size = removedObstacles.size();
    if (df_write(h.get(), &size, sizeof(uint32_t)) != sizeof(uint32_t))
      return false;

    for (auto obstacle : removedObstacles)
    {
      if (df_write(h.get(), &obstacle, sizeof(uint32_t)) != sizeof(uint32_t))
        return false;
    }
  }

  // Step 3: save added obstacles
  {
    uint32_t size = addedObstacles.size();
    if (df_write(h.get(), &size, sizeof(uint32_t)) != sizeof(uint32_t))
      return false;

    for (auto obstacle : addedObstacles)
    {
      if (df_write(h.get(), &obstacle, sizeof(uint32_t)) != sizeof(uint32_t))
        return false;
    }
  }

  // Step 4: save tile cached tiles
  {
    uint32_t size = tileCToSave.size();
    if (df_write(h.get(), &size, sizeof(uint32_t)) != sizeof(uint32_t))
      return false;
  }

  for (auto tileToSave : tileCToSave)
  {
    const dtCompressedTile *tile = tileCache->getTileByRef(tileToSave);
    if (!tile)
    {
      uint32_t sz = 0;
      if (df_write(h.get(), &sz, sizeof(uint32_t)) != sizeof(uint32_t))
        return false;
      continue;
    }

    uint32_t sz = tile->dataSize;
    if (df_write(h.get(), &sz, sizeof(uint32_t)) != sizeof(uint32_t))
      return false;

    if (df_write(h.get(), tile->data, tile->dataSize) != tile->dataSize)
      return false;
  }

  // Step 5: save nav mesh tiles
  {
    uint32_t size = tilesToSave.size();
    if (df_write(h.get(), &size, sizeof(uint32_t)) != sizeof(uint32_t))
      return false;
  }

  for (auto tileToSave : tilesToSave)
  {
    if (df_write(h.get(), &tileToSave, sizeof(dtTileRef)) != sizeof(dtTileRef))
      return false;

    const dtMeshTile *tile = navMesh->getTileByRef(tileToSave);
    if (!tile)
    {
      uint32_t sz = 0;
      if (df_write(h.get(), &sz, sizeof(uint32_t)) != sizeof(uint32_t))
        return false;
      continue;
    }

    uint32_t sz = tile->dataSize;
    if (df_write(h.get(), &sz, sizeof(uint32_t)) != sizeof(uint32_t))
      return false;

    if (df_write(h.get(), tile->data, tile->dataSize) != tile->dataSize)
      return false;
  }

  return true;
}

void rebuildNavMesh_close()
{
  rebuildStep = RS_UNINIT;

  rebuildedTiles.clear();
  rebuildedTiles.shrink_to_fit();
  rebuildedTilesTotalSz = 0;

  generateTiles.clear();
  generateTiles.shrink_to_fit();

  renderDebugReset();
}

uint32_t patchedNavMesh_getFileSizeAndNumTiles(const char *file_name, int &num_tiles)
{
  eastl::unique_ptr<void, decltype(&df_close)> h(df_open(file_name, DF_READ | DF_IGNORE_MISSING), &df_close);

  if (!h)
    return 0u;

  int fileSize = df_length(h.get());
  if (fileSize < sizeof(uint32_t))
    return 0u;

  uint32_t totalSize = 0u;
  if (df_read(h.get(), &totalSize, sizeof(uint32_t)) != sizeof(uint32_t))
    return 0u;

  if (totalSize != fileSize - sizeof(uint32_t))
    return 0u;

  num_tiles = 0;
  const uint32_t failSize = totalSize;

  // skip removed obstacles
  {
    uint32_t count = 0u;
    if (df_read(h.get(), &count, sizeof(uint32_t)) != sizeof(uint32_t))
      return failSize;
    if (df_seek_rel(h.get(), count * sizeof(uint32_t)) != 0)
      return failSize;
  }

  // skip added obstacles
  {
    uint32_t count = 0u;
    if (df_read(h.get(), &count, sizeof(uint32_t)) != sizeof(uint32_t))
      return failSize;
    if (df_seek_rel(h.get(), count * sizeof(uint32_t)) != 0)
      return failSize;
  }

  // read tilecache tiles
  {
    uint32_t count = 0u;
    if (df_read(h.get(), &count, sizeof(uint32_t)) != sizeof(uint32_t))
      return failSize;

    num_tiles = count;
    if (num_tiles < 0)
      num_tiles = 0;

    for (uint32_t i = 0u; i < count; ++i)
    {
      uint32_t dataSize = 0u;
      if (df_read(h.get(), &dataSize, sizeof(uint32_t)) != sizeof(uint32_t))
        return failSize;
      if (df_seek_rel(h.get(), dataSize) != 0)
        return failSize;
    }
  }

  // read navmesh tiles
  {
    uint32_t count = 0u;
    if (df_read(h.get(), &count, sizeof(uint32_t)) != sizeof(uint32_t))
      return failSize;

    if ((uint32_t)num_tiles < count)
      num_tiles = count;
    if (num_tiles < 0)
      num_tiles = 0;

    // and ignore the rest
  }

  return totalSize;
}

bool patchedNavMesh_loadFromFile(const char *fileName, dtTileCache *tlCache, uint8_t *storageData,
  ska::flat_hash_set<uint32_t> &obstacles)
{
  eastl::unique_ptr<void, decltype(&df_close)> h(df_open(fileName, DF_READ | DF_IGNORE_MISSING), &df_close);

  if (!h)
    return false;

  uint32_t totalSize = 0u;

  // Step 1: load total size data to the buffer

  if (df_read(h.get(), &totalSize, sizeof(uint32_t)) != sizeof(uint32_t))
    return false;

  if (df_read(h.get(), storageData, totalSize) != totalSize)
    return false;

  uint8_t *navData = storageData;
  int64_t sizeLeft = totalSize;

  // Step 2: load removed obstacles
  {
    if (sizeLeft < 4)
      return false;
    uint32_t count = *(uint32_t *)navData;
    navData += 4;
    sizeLeft -= 4;

    int64_t sz = (int64_t)count * 4;
    if (sizeLeft < sz)
      return false;
    for (uint32_t i = 0u; i < count; ++i)
    {
      uint32_t obstacle = *(const uint32_t *)navData;
      navData += 4;
      sizeLeft -= 4;

      obstacles.erase(obstacle);
    }
  }

  // Step 3: load added obstacles
  {
    if (sizeLeft < 4)
      return false;
    uint32_t count = *(uint32_t *)navData;
    navData += 4;
    sizeLeft -= 4;

    int64_t sz = (int64_t)count * 4;
    if (sizeLeft < sz)
      return false;
    for (uint32_t i = 0u; i < count; ++i)
    {
      uint32_t obstacle = *(const uint32_t *)navData;
      navData += 4;
      sizeLeft -= 4;

      obstacles.insert(obstacle);
    }
  }

  // Step 4: load tile cached tiles
  {
    if (sizeLeft < 4)
      return false;
    uint32_t count = *(uint32_t *)navData;
    navData += 4;
    sizeLeft -= 4;

    uint8_t *tmp = navData;
    int64_t tmpsz = sizeLeft;

    for (uint32_t i = 0u; i < count; ++i)
    {
      if (sizeLeft < 4)
        return false;
      uint32_t dataSize = *(uint32_t *)navData;
      navData += 4;
      sizeLeft -= 4;

      if (dataSize == 0 || sizeLeft < dataSize)
        return false;
      unsigned char *data = (unsigned char *)navData;
      navData += dataSize;
      sizeLeft -= dataSize;

      const dtTileCacheLayerHeader *header = (const dtTileCacheLayerHeader *)data;

      // force remove tiles for all layers at (tx,ty)
      const int tx = header->tx;
      const int ty = header->ty;
      {
        const int maxTiles = 32;
        dtCompressedTileRef tiles[maxTiles];
        const int ntiles = tlCache->getTilesAt(tx, ty, tiles, maxTiles);

        for (int j = 0; j < ntiles; ++j)
        {
          const dtCompressedTile *tile = tlCache->getTileByRef(tiles[j]);
          if (!tile)
            continue;

          dtTileRef tileRef = navMesh->getTileRefAt(tile->header->tx, tile->header->ty, tile->header->tlayer);

          navMesh->removeTile(tileRef, 0, 0);
          tlCache->removeTile(tiles[j], NULL, NULL);
        }
      }
    }

    navData = tmp;
    sizeLeft = tmpsz;

    for (uint32_t i = 0u; i < count; ++i)
    {
      if (sizeLeft < 4)
        return false;
      uint32_t dataSize = *(uint32_t *)navData;
      navData += 4;
      sizeLeft -= 4;

      if (dataSize == 0 || sizeLeft < dataSize)
        return false;
      unsigned char *data = (unsigned char *)navData;
      navData += dataSize;
      sizeLeft -= dataSize;

      {
        tlCache->addTile(data, dataSize, 0, 0);
      }
    }
  }

  // Step 5: load nav mesh tiles
  {
    if (sizeLeft < 4)
      return false;
    uint32_t count = *(uint32_t *)navData;
    navData += 4;
    sizeLeft -= 4;

    for (uint32_t i = 0u; i < count; ++i)
    {
      if (sizeLeft < sizeof(dtTileRef))
        return false;
      const dtTileRef tileToSave = *(const dtTileRef *)navData;
      navData += sizeof(dtTileRef);
      sizeLeft -= sizeof(dtTileRef);

      if (sizeLeft < 4)
        return false;
      uint32_t dataSize = *(uint32_t *)navData;
      navData += 4;
      sizeLeft -= 4;

      if (dataSize == 0 || sizeLeft < dataSize)
        return false;
      unsigned char *data = (unsigned char *)navData;
      navData += dataSize;
      sizeLeft -= dataSize;

      navMesh->addTile(data, dataSize, 0, tileToSave, 0);
    }
  }

  if (sizeLeft > 0)
    return false;

  return true;
}

const Tab<uint32_t> &get_removed_rebuild_tile_cache_tiles() { return removedNavMeshTiles; }

void clear_removed_rebuild_tile_cache_tiles() { clear_and_shrink(removedNavMeshTiles); }
} // namespace pathfinder