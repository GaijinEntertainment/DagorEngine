// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <pathFinder/pathFinder.h>
#include <DetourCommon.h>
#include <math/dag_mathUtils.h>
#include <math/dag_check_nan.h>
#include <DetourNavMesh.h>

#include <regExp/regExp.h>

#include <osApiWrappers/dag_direct.h>
#include <rendInst/riexHashMap.h>
#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstAccess.h>

#include <pathFinder/tileCache.h>
#include <pathFinder/tileCacheRI.h>
#include <pathFinder/tileCacheUtil.h>

#include <pathFinder/tileRICommon.h>
#include <recastTools/recastNavMeshTile.h>
#include <recastTools/recastBuildEdges.h>
#include <recastTools/recastBuildJumpLinks.h>
#include <recastTools/recastObstacleFlags.h>
#include <gamePhys/collision/collisionLib.h>

#include <osApiWrappers/dag_files.h>
#include <ioSys/dag_dataBlock.h>

#include <util/dag_string.h>

#include <scene/dag_tiledScene.h>

namespace pathfinder
{
extern dtTileCache *tileCache;

extern RiexHashMap<RiObstacle> riHandle2obstacle;

void renderDebugReset();

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
  bool tiled = false;
  float cellSize = 0.0f;
  float cellHeight = 0.0f;
  float agentHeight = 0.0f;
  float agentMaxClimb = 0.0f;
  float agentRadius = 0.0f;
  float agentClimbAfterGluingMeshes = 0.0f;
  float edgeMaxError = 0.0f;
  int tileSize = 0;
  float tileWidth = 0.0f;
  float tileHeight = 0.0f;
  Point3 origin = ZERO<Point3>();
  float traceStep = 0.5f;
  float crossingWaterDepth = 0.0f;
  NavmeshExportType navmeshExportType = NavmeshExportType::GEOMETRY;
  bool useDetailGeometry = true;
  TileCacheDetailSettings detailSettings;

  float agentMaxSlope = 60.0f;
  int vertsPerPoly = 3;
  float regionMinSize = 9.0f;
  float regionMergeSize = 100.0f;
  int minBorderSize = 3;
  float detailSampleDist = 3.0f;
  float detailSampleMaxError = 2.0f;
  float edgeMaxLen = 128.0f;
  float waterLevel = 0.0f;
  int covExtraCells = 32;

  recastbuild::JumpLinksParams jlkParams{
    true, 0, 1.0f, 5.0f, 0.3f, 80.0f, 10.0f, 0.1f, 1.8f, 2.5f, 1.f, 1.5f, 1.f, 0.5f, 30.0f, 15.f, 5.0f, 0.2f, 0.0f, false, false};
  recastbuild::MergeEdgeParams mergeParams{true, 0.4f, 0.3f, Point2(2.0f, 1.5f), 0.5f, 0.2f, 1.f};
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
bool rebuildShouldReloadOriginalNavmesh = true;
typedef ska::flat_hash_map<eastl::pair<int, int>, eastl::pair<float, float>, RebuildTilesHasher> RebuildTiles;
static RebuildTiles rebuildedTiles;
int rebuildedTilesTotalSz = 0;
static RebuildTiles generateTiles;

struct MarkData
{
  Point3 c;
  Point3 e;
  float y;
  bool b;

  rendinst::riex_handle_t h;
  uint32_t r;
};

struct TiledObstacleData
{
  BBox3 box;
  float y;
  uint32_t flags;
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

  template <typename Lambda>
  void applyRegExp(const DataBlock *filterBlk, Lambda lambda)
  {
    if (!filterBlk)
      return;

    Tab<eastl::unique_ptr<RegExp>> filter(tmpmem);
    buildFilter(filter, filterBlk);

    rendinst::iterateRIExtraMap([&](int i, const char *name) {
      for (const auto &re : filter)
        if (re->test(name))
          lambda(rendinst::RendinstVertexDataCbBase::make_pool_id(i, true));
    });
  }

public:
  Bitarray poolsToIgnore;
  ska::flat_hash_set<int> transparentPools;
  ska::flat_hash_map<int, uint32_t> obstaclePools;
  ska::flat_hash_map<int, uint32_t> materialPools;
  ska::flat_hash_map<uint32_t, uint32_t> obstacleFlags;
  rendinst::obstacle_settings_t obstaclesSettings;

  NavmeshLayers() : isLoaded(false) {}

  void load(const char *kind = nullptr)
  {
    if (isLoaded)
      return;

    const DataBlock *settingsBlk = ::dgs_get_settings();
    String navmeshLayersBlkFn(settingsBlk->getStr("navmeshLayers", "config/navmesh_layers.blk"));
    String rendinstDmgBlkFn(settingsBlk->getStr("rendinstDmg", "config/rendinst_dmg.blk"));
    String navObstacleBlkFn(settingsBlk->getStr("navmeshObstacles", "config/navmesh_obstacles.blk"));

    navmeshLayersBlkFn = make_file_path_for_nav_mesh_kind(navmeshLayersBlkFn, kind);
    rendinstDmgBlkFn = make_file_path_for_nav_mesh_kind(rendinstDmgBlkFn, kind);
    navObstacleBlkFn = make_file_path_for_nav_mesh_kind(navObstacleBlkFn, kind);

    DataBlock navmblk;
    if (!dd_file_exists(navmeshLayersBlkFn))
    {
      logerr("%s not found", navmeshLayersBlkFn);
      return;
    }
    navmblk.load(navmeshLayersBlkFn);

    poolsToIgnore.resize(rendinst::getRiGenExtraResCount());
    poolsToIgnore.reset();

    transparentPools.clear();

    obstaclePools.clear();

    applyRegExp(navmblk.getBlock(navmblk.findBlock("filter")), [&](int pool) { poolsToIgnore.set(pool); });
    applyRegExp(navmblk.getBlock(navmblk.findBlock("filter_exclude")), [&](int pool) { poolsToIgnore.reset(pool); });
    applyRegExp(navmblk.getBlock(navmblk.findBlock("filter_include")), [&](int pool) { poolsToIgnore.set(pool); });

    const int blkSkipNameId1 = navmblk.getNameId("filter");
    const int blkSkipNameId2 = navmblk.getNameId("filter_exclude");
    const int blkSkipNameId3 = navmblk.getNameId("filter_include");

    for (int blkIt = 0; blkIt < navmblk.blockCount(); blkIt++)
    {
      const DataBlock *blk = navmblk.getBlock(blkIt);
      const int blkNameId = blk->getBlockNameId();
      if (blkNameId == blkSkipNameId1 || blkNameId == blkSkipNameId2 || blkNameId == blkSkipNameId3)
        continue;

      if (blk->getBool("ignoreCollision", false))
      {
        for (int i = 0; i < blk->blockCount(); i++)
        {
          int pool = rendinst::getRIGenExtraResIdx(blk->getBlock(i)->getBlockName());
          if (pool > -1)
            poolsToIgnore.set(rendinst::RendinstVertexDataCbBase::make_pool_id(pool, true));
        }
      }

      if (blk->getBool("returnCollision", false))
      {
        for (int i = 0; i < blk->blockCount(); i++)
        {
          int pool = rendinst::getRIGenExtraResIdx(blk->getBlock(i)->getBlockName());
          if (pool > -1)
            poolsToIgnore.reset(rendinst::RendinstVertexDataCbBase::make_pool_id(pool, true));
        }
      }

      if (blk->getBool("ignoreTracing", false))
      {
        for (int i = 0; i < blk->blockCount(); i++)
        {
          int pool = rendinst::getRIGenExtraResIdx(blk->getBlock(i)->getBlockName());
          int pool_id = pool < 0 ? -1 : rendinst::RendinstVertexDataCbBase::make_pool_id(pool, true);
          if (pool_id >= 0 && transparentPools.count(pool_id) == 0)
            transparentPools.emplace(pool_id);
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

    const DataBlock *obstacleSettingsBlk = dmgblk.getBlockByName("obstacleSettings");
    bool isObstacleByDefault = obstacleSettingsBlk ? obstacleSettingsBlk->getBool("destructibleRiIsObstacleByDefault", true) : true;
    bool nonDestructiblesCanBeObstacles =
      obstacleSettingsBlk ? obstacleSettingsBlk->getBool("nonDestructiblesCanBeObstacles", false) : false;

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
      // The obstacleSettings block (read above) can invert defaults: destructibleRiIsObstacleByDefault flips the isObstacle default,
      // nonDestructiblesCanBeObstacles drops the hp/destructionImpulse requirement.
      int pool_id = rendinst::RendinstVertexDataCbBase::make_pool_id(pool, true);
      bool canBeObstacle =
        nonDestructiblesCanBeObstacles || blk->getReal("hp", 0.0f) > 0.0f || blk->getReal("destructionImpulse", 0.0f) > 0.0f;
      if (canBeObstacle && blk->getBool("isObstacle", isObstacleByDefault))
        if (obstaclePools.count(pool_id) == 0)
        {
          uint32_t hash = str_hash_fnv1(blk->getBlockName());
          if (!obstacleResHashes.emplace(hash).second)
            logerr("Obstacle resource hash collision for %s, expect missing/extra obstacles in level!", blk->getBlockName());
          obstaclePools.emplace(pool_id, hash);
          uint32_t flags = ObstacleFlags::NONE;
          if (blk->getBool("crossWithJumpLinks", false))
            flags |= ObstacleFlags::CROSS_WITH_JL;
          if (blk->getBool("disableJumplinksAround", false))
            flags |= ObstacleFlags::DISABLE_JL_AROUND;
          if (flags != ObstacleFlags::NONE)
            obstacleFlags.emplace(hash, flags);
        }
      const char *material = blk->getStr("material", "");
      if (strcmp(material, "barbwire") == 0)
        if (materialPools.count(pool_id) == 0)
        {
          uint32_t hash = str_hash_fnv1(blk->getBlockName());
          if (!materialResHashes.emplace(hash).second)
            logerr("Material resource hash collision for %s, expect missing/extra materials in level!", blk->getBlockName());
          materialPools.emplace(pool_id, hash);
        }
    }


    if (!dd_file_exists(navObstacleBlkFn))
    {
      logerr("%s not found", navObstacleBlkFn);
      return;
    }
    if (!rendinst::load_obstacle_settings(navObstacleBlkFn, obstaclesSettings))
    {
      logerr("%s load failed", navObstacleBlkFn);
      return;
    }
  }
} navmeshLayers;

struct RendinstVertexDataCbGame : public rendinst::RendinstVertexDataCbBase
{
  Tab<IPoint2> &transparent;
  Tab<MarkData> *obstacles;
  Tab<TiledObstacleData> *tiledObstacles;

  RendinstVertexDataCbGame(Tab<Point3> &verts, Tab<int> &inds, Tab<IPoint2> &transparent, Bitarray &poolsToIgnore,
    ska::flat_hash_map<int, uint32_t> &obstaclePools, ska::flat_hash_map<int, uint32_t> &materialPools,
    rendinst::obstacle_settings_t &obstaclesSettings, Tab<MarkData> *obstacles, Tab<TiledObstacleData> *tiled_obstacles) :
    RendinstVertexDataCbBase(verts, inds, poolsToIgnore, obstaclePools, materialPools, obstaclesSettings),
    transparent(transparent),
    obstacles(obstacles),
    tiledObstacles(tiled_obstacles)
  {}
  ~RendinstVertexDataCbGame() { clear_all_ptr_items(riCache); }

  virtual void processCollision(const rendinst::CollisionInfo &coll_info) override
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
    int pool_id = rendinst::RendinstVertexDataCbBase::make_pool_id(coll_info.desc);

    auto materialIt = navmeshLayers.materialPools.find(pool_id);
    auto obstacleIt = navmeshLayers.obstaclePools.find(pool_id);
    if (materialIt == navmeshLayers.materialPools.end() && (obstacleIt == navmeshLayers.obstaclePools.end() || tiledObstacles))
    {
      Point3_vec4 tmpVert;
      for (int i = 0; i < data->vertices.size(); ++i)
      {
        v_st(&tmpVert.x, v_mat44_mul_vec3p(instTm, data->vertices[i]));
        vertices.push_back(tmpVert);
      }
      for (int i = 0; i < data->indices.size(); ++i)
        indices.push_back(data->indices[i] + idxBase);

      if (navmeshLayers.transparentPools.find(pool_id) != navmeshLayers.transparentPools.end())
        transparent.push_back({indBase, (int)data->indices.size()});
    }

    if (obstacleIt != navmeshLayers.obstaclePools.end())
    {
      TMatrix tm = rendinst::getRIGenMatrix(coll_info.desc);
      BBox3 oobb = rendinst::get_ri_extra_obstacle_bbox(coll_info.desc);
      if (tiledObstacles)
      {
        Point3 c, ext;
        float angY;
        Point2 obstaclePadding = ZERO<Point2>();
        if (const auto *setup = navmeshLayers.obstaclesSettings.findDesc(coll_info.desc); setup && setup->overridePadding)
          obstaclePadding.x = setup->overridePaddingValue;
        tilecache_calc_obstacle_pos(tm, oobb, 0.0f, obstaclePadding, c, ext, angY);
        if (tilecache_ri_obstacle_too_low(ext.y * 2.0f, rebuildParams.agentMaxClimb))
          return;
        uint32_t flags = ObstacleFlags::NONE;
        if (auto flagsIt = navmeshLayers.obstacleFlags.find(obstacleIt->second); flagsIt != navmeshLayers.obstacleFlags.end())
          flags = flagsIt->second;
        tiledObstacles->push_back({BBox3(c - ext, c + ext), angY, flags});
        return;
      }

      Point3 c, ext;
      float angY;
      // We need to get unpadded obstacle aabb size, so we pass zero padding.
      tilecache_calc_obstacle_pos(tm, oobb, rebuildParams.cellSize, ZERO<Point2>(), c, ext, angY);
      if (!tilecache_ri_obstacle_too_low(ext.y * 2.0f, rebuildParams.agentMaxClimb))
      {
        MarkData markData;
        tilecache_calc_obstacle_pos(tm, oobb, rebuildParams.cellSize, Point2(rebuildParams.agentRadius, rebuildParams.agentHeight),
          markData.c, markData.e, markData.y);

        markData.h = coll_info.desc.getRiExtraHandle();
        markData.r = obstacleIt->second;
        markData.b = tilecache_is_blocking(markData.h);
        obstacles->push_back(markData);
      }
    }
  }
};

static bool finalize_navmesh_tilecached_tile(rcContext &ctx, const rcConfig &cfg,
  recastnavmesh::OffMeshConnectionsStorage *conn_storage, recastnavmesh::RecastTileContext &tile_ctx, int tx, int ty,
  const Tab<MarkData> &obstacles, Tab<recastnavmesh::BuildTileData> &tile_data)
{
  auto fn = [](const Tab<MarkData> &obstacles, const rcConfig &cfg, dtTileCacheLayer &layer, const dtTileCacheLayerHeader &header) {
    for (const auto &obs : obstacles)
    {
      float coshalf = cosf(+0.5f * obs.y);
      float sinhalf = sinf(-0.5f * obs.y);
      float rotAux[2] = {coshalf * sinhalf, coshalf * coshalf - 0.5f};

      const int areaId = obs.b ? pathfinder::POLYAREA_BLOCKED : pathfinder::POLYAREA_OBSTACLE;
      dtMarkBoxArea(layer, header.bmin, cfg.cs, cfg.ch, &obs.c.x, &obs.e.x, &rotAux[0], areaId);
    }
  };

  return finalize_navmesh_tilecached_tile(ctx, cfg, tileCache->getAlloc(), tileCache->getCompressor(), conn_storage, tile_ctx, tx, ty,
    rebuildParams.agentMaxClimb, rebuildParams.agentHeight, rebuildParams.agentRadius,
    rebuildParams.detailSettings.includeDetailedData != 0, obstacles, tile_data, fn);
}

static void init_tile_config(rcConfig &cfg, const Tab<Point3> &vertices, int override_tile_size = 0)
{
  memset(&cfg, 0, sizeof(cfg));

  cfg.cs = rebuildParams.cellSize;
  cfg.ch = rebuildParams.cellHeight;

  cfg.walkableSlopeAngle = rebuildParams.agentMaxSlope;
  cfg.walkableHeight = (int)ceilf(rebuildParams.agentHeight / cfg.ch);
  cfg.walkableClimb = (int)ceilf(rebuildParams.agentMaxClimb / cfg.ch);
  cfg.walkableRadius = (int)ceilf(rebuildParams.agentRadius / cfg.cs);
  cfg.maxEdgeLen = (int)(rebuildParams.edgeMaxLen / cfg.cs);

  cfg.maxSimplificationError = rebuildParams.edgeMaxError;
  cfg.minRegionArea = int(rebuildParams.regionMinSize * sqr(safeinv(cfg.cs)));
  cfg.mergeRegionArea = int(rebuildParams.regionMergeSize * sqr(safeinv(cfg.cs)));
  cfg.maxVertsPerPoly = rebuildParams.vertsPerPoly;
  cfg.tileSize = override_tile_size > 0 ? override_tile_size : rebuildParams.tileSize;
  cfg.borderSize = cfg.walkableRadius + rebuildParams.minBorderSize;
  cfg.width = cfg.tileSize + cfg.borderSize * 2;
  cfg.height = cfg.tileSize + cfg.borderSize * 2;
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

  if (rebuildParams.tiled && rebuildParams.navmeshExportType == NavmeshExportType::WATER)
  {
    // REF: recastNavMesh.cpp(buildAndWriteNavMesh): landBBox[0].set_xVy(nav_area[0], min_h - 1);
    //                                               landBBox[1].set_xVy(nav_area[1], min(max_h, water_lev) + 1);
    minY -= 1.0f;
    maxY = min(maxY, rebuildParams.waterLevel) + 1.0f;
  }
  else
  {
    // REF: recastNavMesh.cpp(buildAndWriteNavMesh): landBBox[0].set_xVy(nav_area[0], max(min_h, water_lev) - 1);
    //                                               landBBox[1].set_xVy(nav_area[1], max_h + 1);
    minY = max(minY, rebuildParams.waterLevel) - 1.0f;
    maxY += 1.0f;
  }

  cfg.bmin[1] = minY;
  cfg.bmax[1] = maxY;
}

static bool prepare_tile_context(rcContext &ctx, rcConfig &cfg, recastnavmesh::RecastTileContext &tile_ctx, int tx, int ty,
  const Tab<Point3> &vertices, const Tab<int> &indices, const Tab<IPoint2> &transparent, int extra_cells = 0)
{
  float extTileSize = extra_cells * cfg.cs;

  const float tileBoxExt = cfg.borderSize * cfg.cs + extTileSize;
  const float tileSize = rebuildParams.tileWidth;

  cfg.bmin[0] = rebuildParams.origin[0] + tx * tileSize - tileBoxExt;
  cfg.bmin[1] = cfg.bmin[1] - tileBoxExt;
  cfg.bmin[2] = rebuildParams.origin[2] + ty * tileSize - tileBoxExt;

  cfg.bmax[0] = rebuildParams.origin[0] + (tx + 1) * tileSize + tileBoxExt;
  cfg.bmax[1] = cfg.bmax[1] + tileBoxExt;
  cfg.bmax[2] = rebuildParams.origin[2] + (ty + 1) * tileSize + tileBoxExt;

  return prepare_tile_context(ctx, cfg, tile_ctx, vertices, indices, transparent, rebuildParams.tiled || extra_cells > 0);
}

void collect_rendinst(const BBox3 &box, Tab<Point3> &vertices, Tab<int> &indices, Tab<IPoint2> &transparent, Tab<MarkData> &obstacles,
  Tab<TiledObstacleData> *tiled_obstacles, const char *nav_mesh_kind)
{
  navmeshLayers.load(nav_mesh_kind);

  RendinstVertexDataCbGame cb(vertices, indices, transparent, navmeshLayers.poolsToIgnore, navmeshLayers.obstaclePools,
    navmeshLayers.materialPools, navmeshLayers.obstaclesSettings, tiled_obstacles ? nullptr : &obstacles, tiled_obstacles);
  rendinst::testObjToRendInstIntersection(box, cb, rendinst::GatherRiTypeFlag::RiGenAndExtra);
  cb.procAllCollision();
}

void collect_height_map_geometry(const BBox3 &box, Tab<Point3> &vertices, Tab<int> &indices)
{
  const float traceStep = rebuildParams.traceStep;
  const bool waterNavmesh = rebuildParams.tiled && (rebuildParams.navmeshExportType == NavmeshExportType::WATER ||
                                                     rebuildParams.navmeshExportType == NavmeshExportType::WATER_AND_GEOMETRY);

  float startXfloat = box.boxMin().x;
  float startZfloat = box.boxMin().z;
  int w = box.width().x / traceStep;
  int h = box.width().z / traceStep;
  if (rebuildParams.tiled)
  {
    startXfloat = rebuildParams.origin[0] + floorf((box.boxMin().x - rebuildParams.origin[0]) / traceStep) * traceStep;
    startZfloat = rebuildParams.origin[2] + floorf((box.boxMin().z - rebuildParams.origin[2]) / traceStep) * traceStep;
    w = (int)ceilf((box.boxMax().x - startXfloat) / traceStep) + 1;
    h = (int)ceilf((box.boxMax().z - startZfloat) / traceStep) + 1;
  }

  vertices.resize((w * h));
  indices.resize((w - 1) * (h - 1) * 6);

  float *verticesPtr = &vertices[0].x;

  for (int x = 0; x < w; ++x)
  {
    for (int z = 0; z < h; ++z, verticesPtr += 3)
    {
      verticesPtr[0] = startXfloat + traceStep * x;
      verticesPtr[2] = startZfloat + traceStep * z;

      verticesPtr[1] = dacoll::traceht_hmap(Point2(verticesPtr[0], verticesPtr[2]));
      if (waterNavmesh)
        verticesPtr[1] = max(verticesPtr[1], rebuildParams.waterLevel);
    }
  }

  int *indicesPtr = &indices[0];

  for (int x = 0; x < w - 1; ++x)
  {
    for (int z = 0; z < h - 1; ++z)
    {
      const int vertexBase = z + x * (rebuildParams.tiled ? h : w);
      if (rebuildParams.tiled)
      {
        // Match the global grid and triangulation used by the editor navmesh build.
        const int triangleIndices[6] = {
          vertexBase, vertexBase + h + 1, vertexBase + h, vertexBase, vertexBase + 1, vertexBase + h + 1};
        const float waterThreshold = rebuildParams.waterLevel - rebuildParams.crossingWaterDepth;
        for (int triangle = 0; triangle < 2; ++triangle)
        {
          const int *triangleIndex = triangleIndices + triangle * 3;
          const bool includeTriangle = waterNavmesh ? vertices[triangleIndex[0]].y <= rebuildParams.waterLevel &&
                                                        vertices[triangleIndex[1]].y <= rebuildParams.waterLevel &&
                                                        vertices[triangleIndex[2]].y <= rebuildParams.waterLevel
                                                    : vertices[triangleIndex[0]].y > waterThreshold &&
                                                        vertices[triangleIndex[1]].y > waterThreshold &&
                                                        vertices[triangleIndex[2]].y > waterThreshold;
          if (includeTriangle)
            for (int vertex = 0; vertex < 3; ++vertex)
              *(indicesPtr++) = triangleIndex[vertex];
        }
      }
      else
      {
        // This could probably match the editor as well with no problem,
        // but for now it is left unchanged to avoid testing
        *(indicesPtr++) = vertexBase + w;
        *(indicesPtr++) = vertexBase;
        *(indicesPtr++) = vertexBase + 1;

        *(indicesPtr++) = vertexBase + w;
        *(indicesPtr++) = vertexBase + 1;
        *(indicesPtr++) = vertexBase + w + 1;
      }
    }
  }
  indices.resize(indicesPtr - indices.data());
}

const scene::TiledScene *tilecache_get_ladders();

bool build_tile_ladder_links(const dtMeshTile *tile)
{
  bool shouldRebuild = false;
  const scene::TiledScene *ladders = tilecache_get_ladders();
  if (ladders)
  {
    BBox3 box;
    pathfinder::TileCacheMeshProcess::calcQueryLaddersBBox(box, tile->header->bmin, tile->header->bmax);
    bbox3f bbox = v_ldu_bbox3(box);
    ladders->boxCull<false, true>(bbox, 0, 0, [&](scene::node_index, mat44f_cref) { shouldRebuild = true; });
  }
  if (!shouldRebuild)
    return false;

  const int tileX = tile->header->x;
  const int tileY = tile->header->y;
  const int tileLayer = tile->header->layer;
  const dtCompressedTile *ctile = tileCache->getTileAt(tileX, tileY, tileLayer);
  if (!ctile)
    return false;

  const dtCompressedTileRef ctileRef = tileCache->getTileRef(ctile);
  dtStatus status = tileCache->buildNavMeshTile(ctileRef, getNavMeshPtr());
  if (dtStatusFailed(status))
  {
    logerr("Rebuild NavMesh: failed to rebuild nav mesh ladder links at (%d,%d)", tileX, tileY);
    return false;
  }
  return true;
}
static bool resolve_rebuild_settings(NavMeshType nav_mesh_type, RebuildNavMeshSetup &settings)
{
  settings.tiled = nav_mesh_type == NMT_TILED;
  if (!settings.tiled && nav_mesh_type != NMT_TILECACHED)
  {
    logerr("Rebuild NavMesh: only tiled and tilecached navmeshes support runtime rebuilding");
    return false;
  }

  if (!settings.tiled)
  {
    const dtTileCacheParams &src = *tileCache->getParams();
    settings.cellSize = src.cs;
    settings.cellHeight = src.ch;
    settings.agentHeight = src.walkableHeight;
    settings.agentMaxClimb = src.walkableClimb;
    settings.agentRadius = src.walkableRadius;
    settings.edgeMaxError = src.maxSimplificationError;
    settings.tileSize = src.width;
    settings.tileWidth = src.width * src.cs;
    settings.tileHeight = src.height * src.cs;
    settings.origin = Point3(src.orig[0], src.orig[1], src.orig[2]);
    settings.detailSettings = tilecache_get_detailed_data_settings();
    return true;
  }

  const TiledNavMeshBuildSettings src = get_tiled_navmesh_build_settings();
  const dtNavMeshParams &navParams = *getNavMeshPtr()->getParams();
  settings.cellSize = src.cellSize;
  settings.cellHeight = src.cellHeight;
  settings.agentHeight = src.agentHeight;
  settings.agentMaxClimb = src.agentMaxClimb;
  settings.agentRadius = src.agentRadius;
  settings.agentClimbAfterGluingMeshes = src.agentClimbAfterGluingMeshes;
  settings.edgeMaxError = src.edgeMaxError;
  settings.tileSize = src.tileSize;
  settings.tileWidth = navParams.tileWidth;
  settings.tileHeight = navParams.tileHeight;
  settings.origin = Point3(navParams.orig[0], navParams.orig[1], navParams.orig[2]);
  settings.traceStep = src.traceStep;
  settings.waterLevel = src.waterLevel;
  settings.crossingWaterDepth = src.crossingWaterDepth;
  settings.navmeshExportType = src.navmeshExportType;
  settings.useDetailGeometry =
    src.navmeshExportType == NavmeshExportType::GEOMETRY || src.navmeshExportType == NavmeshExportType::WATER_AND_GEOMETRY;

  const float rebuildTileSize = settings.tileSize * settings.cellSize;
  const float tileSizeTolerance =
    1e-4f * max(1.0f, max(fabsf(rebuildTileSize), max(fabsf(settings.tileWidth), fabsf(settings.tileHeight))));
  if (settings.tileSize <= 0 || settings.cellSize <= 0.0f || !check_finite(rebuildTileSize) ||
      fabsf(rebuildTileSize - settings.tileWidth) > tileSizeTolerance ||
      fabsf(rebuildTileSize - settings.tileHeight) > tileSizeTolerance)
  {
    logerr("Rebuild NavMesh: tiled build settings cover %d * %g = %g units, but navmesh tiles are %g x %g; runtime "
           "rebuilding is disabled",
      settings.tileSize, settings.cellSize, rebuildTileSize, settings.tileWidth, settings.tileHeight);
    return false;
  }

  settings.agentMaxSlope = src.agentMaxSlope;
  settings.vertsPerPoly = src.vertsPerPoly;
  settings.regionMinSize = src.regionMinSize;
  settings.regionMergeSize = src.regionMergeSize;
  settings.detailSampleDist = src.detailSampleDist;
  settings.detailSampleMaxError = src.detailSampleMaxError;
  settings.edgeMaxLen = src.edgeMaxLen;
  settings.covExtraCells = src.jumpLinkExtraCells;
  settings.jlkParams = {src.jumpLinksEnabled != 0, src.jumpLinksTypeGen, src.jumpLinksJumpoffMinHeight, src.jumpLinksJumpoffMaxHeight,
    src.jumpLinksJumpoffMinLinkLength, src.jumpLinksEdgeMappingAngleDeg, src.jumpLinksEdgeMergeAngleDeg, src.jumpLinksEdgeMergeDist,
    src.jumpLinksEdgeMergeDistV1, src.jumpLinksHeight, src.jumpLinksLength, src.jumpLinksWidth, src.jumpLinksAgentHeight,
    src.jumpLinksAgentMinSpace, src.jumpLinksDeltaHeightThreshold, src.jumpLinksMaxObstructionAngleRad, src.jumpLinksMergeAngleCos,
    src.jumpLinksMergeDistCos, src.agentRadius, src.complexJumpThreshold, src.crossObstaclesWithJumplinks != 0,
    src.enableCustomJumplinks != 0};
  settings.mergeParams = {src.simplificationEdgeEnabled != 0, src.simplificationMaxExtrudeErrorSq, src.simplificationExtrudeLimitSq,
    Point2(src.simplificationWalkPrecisionX, src.simplificationWalkPrecisionY), src.simplificationSafeCutLimitSq,
    src.simplificationUnsafeCutLimitSq, src.simplificationUnsafeMaxCutSpace};
  return true;
}

static void rebuildNavMesh_initImpl(bool reload_original_navmesh)
{
  rebuildParams = RebuildNavMeshSetup();

  const NavMeshType navMeshType = get_nav_mesh_type();
  const bool supportedNavMeshType = resolve_rebuild_settings(navMeshType, rebuildParams);

  navmeshLayers = NavmeshLayers();

  clear_and_shrink(tilesToSave);
  clear_and_shrink(tileCToSave);

  addedObstacles.clear();
  removedObstacles.clear();

  rebuildNavMesh_close();
  rebuildShouldReloadOriginalNavmesh = reload_original_navmesh && navMeshType == NMT_TILECACHED;

  rebuildStep = supportedNavMeshType ? RS_WAIT_ADD_TILES : RS_UNINIT;

  rebuildedTiles.clear();
  rebuildedTiles.reserve(1000);
  rebuildedTilesTotalSz = 0;

  generateTiles.clear();
}

void rebuildNavMesh_init() { rebuildNavMesh_initImpl(true); }

void rebuildNavMesh_initFromCurrent() { rebuildNavMesh_initImpl(false); }

void rebuildNavMesh_setup(const char *name, const Point2 &value)
{
  if (!name)
    return;
  else if (strcmp(name, "edgWalkPrecision") == 0)
    rebuildParams.mergeParams.walkPrecision = value;
  else
    logdbg("Unknown rebuildNavMesh_setup param: %s, value: %@", name, value);
}

void rebuildNavMesh_setup(const char *name, float value)
{
  if (!name)
    return;
  // navmesh setup
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
  // jumplinks setup
  else if (strcmp(name, "jlkEnabled") == 0)
    rebuildParams.jlkParams.enabled = value != 0.0f;
  else if (strcmp(name, "jlkCovExtraCells") == 0)
    rebuildParams.covExtraCells = (int)floorf(value + 0.5f);
  else if (strcmp(name, "jlkJumpHeight") == 0)
    rebuildParams.jlkParams.jumpHeight = value * 2.f;
  else if (strcmp(name, "jlkJumpLength") == 0)
    rebuildParams.jlkParams.jumpLength = value;
  else if (strcmp(name, "jlkWidth") == 0)
    rebuildParams.jlkParams.width = value;
  else if (strcmp(name, "jlkAgentHeight") == 0)
    rebuildParams.jlkParams.agentHeight = value;
  else if (strcmp(name, "jlkAgentMinSpace") == 0)
    rebuildParams.jlkParams.agentMinSpace = value;
  else if (strcmp(name, "jlkDeltaHeightThreshold") == 0)
    rebuildParams.jlkParams.deltaHeightThreshold = value;
  else if (strcmp(name, "jlkComplexJumpTheshold") == 0)
    rebuildParams.jlkParams.complexJumpTheshold = value;
  else if (strcmp(name, "jlkLinkDegAngle") == 0)
    rebuildParams.jlkParams.linkDegAngle = cosf(DegToRad(value));
  else if (strcmp(name, "jlkLinkDegDist") == 0)
    rebuildParams.jlkParams.linkDegDist = cosf(DegToRad(value));
  else if (strcmp(name, "jlkAgentRadius") == 0)
    rebuildParams.jlkParams.agentRadius = value;
  else if (strcmp(name, "jlkCrossObstaclesWithJumplinks") == 0)
    rebuildParams.jlkParams.crossObstaclesWithJumplinks = value != 0.0f;
  else if (strcmp(name, "jlkEnableCustomJumplinks") == 0)
    rebuildParams.jlkParams.enableCustomJumplinks = value != 0.0f;
  // edges mergeParams setup
  else if (strcmp(name, "edgMergeEdgesEnabled") == 0)
    rebuildParams.mergeParams.enabled = value != 0.0f;
  else if (strcmp(name, "edgMaxExtrudeErrorSq") == 0)
    rebuildParams.mergeParams.maxExtrudeErrorSq = sqr(value);
  else if (strcmp(name, "edgExtrudeLimitSq") == 0)
    rebuildParams.mergeParams.extrudeLimitSq = sqr(value);
  else if (strcmp(name, "edgSafeCutLimitSq") == 0)
    rebuildParams.mergeParams.safeCutLimitSq = sqr(value);
  else if (strcmp(name, "edgUnsafeCutLimitSq") == 0)
    rebuildParams.mergeParams.unsafeCutLimitSq = sqr(value);
  else if (strcmp(name, "edgUnsafeMaxCutSpace") == 0)
    rebuildParams.mergeParams.unsafeMaxCutSpace = value;
  else
    logdbg("Unknown rebuildNavMesh_setup param: %s, value: %f", name, value);
}

void rebuildNavMesh_addBBox(const BBox3 &bbox)
{
  if (rebuildStep != RS_WAIT_ADD_TILES)
    return;

  if (!rebuildParams.tiled && !tilecache_is_inside(bbox))
    return;

  const float tw = rebuildParams.tileWidth;
  const float th = rebuildParams.tileHeight;
  const Point3 &origin = rebuildParams.origin;

  const int tx0 = (int)dtMathFloorf((bbox.boxMin().x - origin[0]) / tw);
  const int tx1 = (int)dtMathFloorf((bbox.boxMax().x - origin[0]) / tw);
  const int ty0 = (int)dtMathFloorf((bbox.boxMin().z - origin[2]) / th);
  const int ty1 = (int)dtMathFloorf((bbox.boxMax().z - origin[2]) / th);

  for (int ty = ty0; ty <= ty1; ++ty)
  {
    for (int tx = tx0; tx <= tx1; ++tx)
    {
      if (rebuildParams.tiled && !getNavMeshPtr()->getTileAt(tx, ty, 0))
        continue;
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

static void rebuildNavMesh_update_reloadNavMesh();
static bool rebuildNavMesh_update_checkTileArrays();
static void rebuildNavMesh_update_removeTiles();
static bool rebuildNavMesh_update_buildTiles(int n);
static bool rebuildNavMesh_update_buildLadders();

// worst case tiles (nav mesh layers) one rebuilt tile position can produce
static constexpr int max_tiles_per_rebuilt_pos = 8;

bool rebuildNavMesh_update(bool interactive)
{
  const int maxTiles = interactive ? 1 : rebuildedTiles.size();

  bool result = false;
  switch (rebuildStep)
  {
    default:
    case RS_UNINIT: result = false; break;

    case RS_WAIT_ADD_TILES:
      if (!rebuildParams.tiled)
      {
        if (rebuildShouldReloadOriginalNavmesh)
          rebuildNavMesh_update_reloadNavMesh();
        else if (!rebuildNavMesh_update_checkTileArrays())
        {
          // without the capacity the rebuild would only destroy the tiles it removes
          // first; keep the existing navmesh and finish with nothing rebuilt
          logerr("Rebuild NavMesh: no tile capacity for the requested area, rebuild skipped");
          rebuildedTiles.clear();
          rebuildStep = RS_FINISHED;
          result = true;
          break;
        }
        rebuildNavMesh_update_removeTiles();
      }
      generateTiles = rebuildedTiles;
      rebuildStep = RS_REBUILDING_TILES;
      [[fallthrough]];
    case RS_REBUILDING_TILES:
      result = rebuildNavMesh_update_buildTiles(maxTiles);
      if (rebuildedTiles.empty())
        rebuildStep = rebuildParams.tiled ? RS_FINISHED : RS_GENERATING_OVERLINKS_LADDERS;
      break;
    case RS_GENERATING_OVERLINKS_LADDERS:
      result = rebuildNavMesh_update_buildLadders();
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
  const int extraTiles = rebuildedTiles.size() * max_tiles_per_rebuilt_pos;
  reload_nav_mesh(extraTiles);
  getNavMeshPtr()->reconstructFreeList();
}

// Rebuilding from the current navmesh cannot reload with extra headroom like the
// reload path does without losing runtime state, and the load-time array sizing
// cannot predict rebuilds over previously empty areas; check the capacity up front
// instead. Positions that already have tiles free their slots before the rebuild
// adds new ones, so only the missing layers count. Returns false when the rebuild
// worst case does not fit, so the caller can keep the existing tiles instead of
// destroying what it cannot rebuild.
bool rebuildNavMesh_update_checkTileArrays()
{
  const dtNavMesh *navMesh = getNavMeshPtr();
  if (!navMesh || !tileCache || rebuildedTiles.empty())
    return true;
  int neededNavTiles = 0, neededTcTiles = 0;
  for (const auto &it : rebuildedTiles)
  {
    const int tx = it.first.first;
    const int ty = it.first.second;
    dtCompressedTileRef tcRefs[max_tiles_per_rebuilt_pos];
    neededTcTiles += max(max_tiles_per_rebuilt_pos - tileCache->getTilesAt(tx, ty, tcRefs, max_tiles_per_rebuilt_pos), 0);
    const dtMeshTile *navTiles[max_tiles_per_rebuilt_pos];
    neededNavTiles += max(max_tiles_per_rebuilt_pos - navMesh->getTilesAt(tx, ty, navTiles, max_tiles_per_rebuilt_pos), 0);
  }
  int freeNavTiles = 0, freeTcTiles = 0;
  for (int i = 0, n = navMesh->getMaxTiles(); i < n; ++i)
    if (!navMesh->getTile(i)->header)
      ++freeNavTiles;
  for (int i = 0, n = tileCache->getTileCount(); i < n; ++i)
    if (!tileCache->getTile(i)->header)
      ++freeTcTiles;
  bool fits = true;
  if (neededNavTiles > freeNavTiles)
  {
    logerr("Rebuild NavMesh: %d nav tiles needed but only %d slots free; raise the load sizing headroom or make the tile "
           "arrays growable (growMaxTiles follow-up change)",
      neededNavTiles, freeNavTiles);
    fits = false;
  }
  if (neededTcTiles > freeTcTiles)
  {
    logerr("Rebuild NavMesh: %d tile cache tiles needed but only %d slots free; raise the load sizing headroom or make the "
           "tile arrays growable (growMaxTiles follow-up change)",
      neededTcTiles, freeTcTiles);
    fits = false;
  }
  return fits;
}

void rebuildNavMesh_update_removeTiles()
{
  dtNavMesh *navMesh = getNavMeshPtr();
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
        if (!tile || !tile->header)
          continue;

        const int tlayer = tile->header->tlayer;
        dtTileRef tileRef = navMesh->getTileRefAt(tile->header->tx, tile->header->ty, tlayer);

        dtStatus status1 = navMesh->removeTile(tileRef, 0, 0);
        dtStatus status2 = tileCache->removeTile(tiles[i], NULL, NULL);

        if (!dtStatusSucceed(status1) || !dtStatusSucceed(status2))
        {
          logerr("Rebuild NavMesh: failed to remove tile at (%d,%d) layer %d", tx, ty, tlayer);
        }
      }
    }
  }
  navMesh->reconstructFreeList();
}

static void preserve_manual_jumplinks(recastnavmesh::OffMeshConnectionsStorage &conn_storage, int tx, int ty)
{
  const dtMeshTile *tile = getNavMeshPtr()->getTileAt(tx, ty, 0);
  if (!tile || !tile->header)
    return;
  for (int i = 0; i < tile->header->offMeshConCount; ++i)
  {
    const dtOffMeshConnection &conn = tile->offMeshCons[i];
    if (!(conn.userId & recastnavmesh::MANUAL_JUMPLINK_USER_ID_BIT) || conn.poly >= tile->header->polyCount)
      continue;
    const dtPoly &poly = tile->polys[conn.poly];
    const unsigned char bidir = (conn.flags & DT_OFFMESH_CON_BIDIR) != 0;
    recastnavmesh::add_off_mesh_connection(conn_storage, conn.pos, conn.pos + 3, conn.rad, bidir, poly.flags, poly.getArea(),
      conn.userId);
  }
}

static bool finalize_navmesh_tiled_tile(rcContext &ctx, const rcConfig &cfg, recastnavmesh::OffMeshConnectionsStorage *conn_storage,
  recastnavmesh::RecastTileContext &tile_ctx, int tx, int ty, const BBox3 &box, const Tab<TiledObstacleData> &obstacles,
  Tab<recastnavmesh::BuildTileData> &tile_data)
{
  if (!recastnavmesh::build_navmesh_tiled_tile_mesh(ctx, cfg, tile_ctx, tile_data))
    return false;
  if (conn_storage)
  {
    preserve_manual_jumplinks(*conn_storage, tx, ty);
    Tab<recastbuild::JumpLinkObstacle> crossObstacles;
    Tab<recastbuild::JumpLinkObstacle> disableObstacles;
    for (const TiledObstacleData &obstacle : obstacles)
    {
      if (rebuildParams.jlkParams.crossObstaclesWithJumplinks && (obstacle.flags & ObstacleFlags::CROSS_WITH_JL))
      {
        const auto isSameObstacle = [&](const recastbuild::JumpLinkObstacle &other) {
          return (other.box.center() - obstacle.box.center()).lengthSq() < 0.25f;
        };
        if (eastl::find_if(crossObstacles.begin(), crossObstacles.end(), isSameObstacle) == crossObstacles.end())
          crossObstacles.push_back({obstacle.box, obstacle.y});
      }
      if (obstacle.flags & ObstacleFlags::DISABLE_JL_AROUND)
        disableObstacles.push_back({obstacle.box, obstacle.y});
    }
    recastbuild::cross_obstacles_with_jumplinks(*conn_storage, *tile_ctx.dmesh, box, rebuildParams.jlkParams, rebuildParams.cellHeight,
      crossObstacles);
    recastbuild::disable_jumplinks_around_obstacle(*conn_storage, disableObstacles);
  }
  return recastnavmesh::finalize_navmesh_tiled_tile(ctx, cfg, conn_storage, tile_ctx, tx, ty, rebuildParams.agentHeight,
    rebuildParams.agentRadius, rebuildParams.jlkParams.jumpHeight, rebuildParams.agentClimbAfterGluingMeshes, tile_data);
}

static bool replace_tiled_tile(int tx, int ty, recastnavmesh::BuildTileData *replacement)
{
  dtNavMesh *navMesh = getNavMeshPtr();
  const dtMeshTile *oldTile = navMesh->getTileAt(tx, ty, 0);
  if (!oldTile)
    return false;

  const dtTileRef oldRef = navMesh->getTileRef(oldTile);
  unsigned char *oldData = oldTile->data;
  const int oldDataSize = oldTile->dataSize;
  int oldFlags = 0;
  if (oldTile->flags & DT_TILE_FREE_DATA)
  {
    oldData = (unsigned char *)dtAlloc(oldDataSize, DT_ALLOC_PERM);
    if (!oldData)
      return false;
    memcpy(oldData, oldTile->data, oldDataSize);
    oldFlags = DT_TILE_FREE_DATA;
  }

  if (dtStatusFailed(navMesh->removeTile(oldRef, nullptr, nullptr)))
  {
    if (oldFlags & DT_TILE_FREE_DATA)
      dtFree(oldData);
    return false;
  }

  if (!replacement || !replacement->navMeshData)
  {
    if (oldFlags & DT_TILE_FREE_DATA)
      dtFree(oldData);
    return true;
  }

  const dtStatus status = navMesh->addTile(replacement->navMeshData, replacement->navMeshDataSz, DT_TILE_FREE_DATA, 0, nullptr);
  if (dtStatusSucceed(status))
  {
    replacement->navMeshData = nullptr;
    if (oldFlags & DT_TILE_FREE_DATA)
      dtFree(oldData);
    return true;
  }

  if (dtStatusFailed(navMesh->addTile(oldData, oldDataSize, oldFlags, oldRef, nullptr)))
  {
    logerr("Rebuild NavMesh: failed to restore tiled navmesh tile at (%d,%d)", tx, ty);
    if (oldFlags & DT_TILE_FREE_DATA)
      dtFree(oldData);
  }
  logerr("Rebuild NavMesh: failed to replace tiled navmesh tile at (%d,%d)", tx, ty);
  return false;
}

bool rebuildNavMesh_update_buildTiles(int n)
{
  Tab<MarkData> obstacles;
  bool success = true;

  for (int i = 0; i < n && !rebuildedTiles.empty(); ++i)
  {
    const bool tiled = rebuildParams.tiled;
    const float tileSize = rebuildParams.tileWidth;
    const Point3 &origin = rebuildParams.origin;

    const auto tile = *rebuildedTiles.begin();
    rebuildedTiles.erase(rebuildedTiles.begin());
    const int tx = tile.first.first;
    const int ty = tile.first.second;

    BBox3 bbox;

    bbox.lim[0] = Point3(origin[0] + tx * tileSize, tile.second.first, origin[2] + ty * tileSize);

    bbox.lim[1] = Point3(origin[0] + (tx + 1) * tileSize, tile.second.second, origin[2] + (ty + 1) * tileSize);

    Tab<Point3> vertices;
    Tab<int> indices;
    Tab<IPoint2> transparent;
    Tab<TiledObstacleData> tiledObstacles;

    BBox3 extGeomBox(bbox);
    extGeomBox.inflate(tileSize);

    collect_height_map_geometry(extGeomBox, vertices, indices);
    if (rebuildParams.useDetailGeometry)
    {
      if (tiled)
        dacoll::append_static_collision_mesh(extGeomBox, vertices, indices);
      collect_rendinst(extGeomBox, vertices, indices, transparent, obstacles, tiled ? &tiledObstacles : nullptr,
        get_nav_mesh_kind(pathfinder::NM_MAIN));
    }
    const Tab<IPoint2> noTransparent;

    // build tiles
    {
      rcContext ctx;
      rcConfig cfg;

      recastnavmesh::RecastTileContext tile_ctx;
      Tab<recastnavmesh::BuildTileData> tile_data;

      recastnavmesh::OffMeshConnectionsStorage connStorage;
      if (rebuildParams.jlkParams.enabled)
      {
        const int baseTileSize = rebuildParams.tileSize;
        const int extraCells = min((baseTileSize - 1) / 2, rebuildParams.covExtraCells);
        const int extTileSize = baseTileSize + extraCells * 2;
        init_tile_config(cfg, vertices, extTileSize);

        if (!prepare_tile_context(ctx, cfg, tile_ctx, tx, ty, vertices, indices, noTransparent, extraCells))
        {
          logerr("Rebuild NavMesh: failed to prepare ext tile context at (%d,%d)", tx, ty);
          success = false;
          continue;
        }

        Tab<recastbuild::Edge> edges;
        recastbuild::build_edges(edges, tile_ctx.cset, tile_ctx.chf, rebuildParams.mergeParams, nullptr);
        recastbuild::build_jumplinks_connstorage(connStorage, nullptr, edges, rebuildParams.jlkParams, tile_ctx.chf, tile_ctx.solid,
          bbox);
        tile_ctx.clearIntermediate(nullptr);
      }

      init_tile_config(cfg, vertices);
      if (rebuildParams.detailSettings.includeDetailedData != 0)
      {
        cfg.detailSampleDist =
          rebuildParams.detailSettings.detailSampleDist < 0.9f ? 0 : cfg.cs * rebuildParams.detailSettings.detailSampleDist;
        cfg.detailSampleMaxError = cfg.ch * rebuildParams.detailSettings.detailSampleMaxError;
      }

      if (!prepare_tile_context(ctx, cfg, tile_ctx, tx, ty, vertices, indices, noTransparent))
      {
        logerr("Rebuild NavMesh: failed to prepare tile context at (%d,%d)", tx, ty);
        success = false;
        continue;
      }

      // TODO LATER Use transparent array to build heightmap for covers tracing without transparent geometry
      // TODO LATER when covers generation added here.

      const bool finalized = tiled
                               ? finalize_navmesh_tiled_tile(ctx, cfg, rebuildParams.jlkParams.enabled ? &connStorage : nullptr,
                                   tile_ctx, tx, ty, bbox, tiledObstacles, tile_data)
                               : finalize_navmesh_tilecached_tile(ctx, cfg, rebuildParams.jlkParams.enabled ? &connStorage : nullptr,
                                   tile_ctx, tx, ty, obstacles, tile_data);
      if (!finalized)
      {
        logerr("Rebuild NavMesh: failed to generate navmesh tiles at (%d,%d)", tx, ty);
        success = false;
        continue;
      }

      if (tiled)
      {
        recastnavmesh::BuildTileData *replacement = tile_data.empty() ? nullptr : &tile_data[0];
        if (!replace_tiled_tile(tx, ty, replacement))
        {
          logerr("Rebuild NavMesh: failed to install tiled navmesh tile at (%d,%d)", tx, ty);
          success = false;
        }
        tile_ctx.clearIntermediate(&tile_data);
      }
      else
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
              success = false;
            }
          }

          {
            dtStatus status =
              getNavMeshPtr()->addTile(tile_data[i].navMeshData, tile_data[i].navMeshDataSz, DT_TILE_FREE_DATA, 0, &nav);

            if (dtStatusSucceed(status) && nav != 0)
              tilesToSave.push_back(nav);
            else
            {
              logerr("Rebuild NavMesh: failed to add navmesh tile at (%d,%d)", tx, ty);
              success = false;
            }
          }

          tile_ctx.clearIntermediate(nullptr);
        }
    }
  }

  if (rebuildParams.tiled)
    return success;

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
    riHandle2obstacle[obstacle.h].obstacle_handle =
      tilecache_obstacle_add(obstacle.c, obstacle.e, obstacle.y, obstacle.b, true, false);
    addedObstacles.insert(obstacle.r);
  }

  for (const auto &obstacle : removedHandles)
    tilecache_obstacle_remove(obstacle, false);

  // logdbg("rebuild_tiles: total size %d bytes, %d tiles left", rebuildedTilesTotalSz, rebuildedTiles.size());
  return success;
}

bool rebuildNavMesh_update_buildLadders()
{
  for (auto &tileToSave : tilesToSave)
  {
    const dtMeshTile *tile = getNavMeshPtr()->getTileByRef(tileToSave);
    if (!tile)
      continue;

    if (build_tile_ladder_links(tile))
    {
      dtTileRef tileRef = getNavMeshPtr()->getTileRefAt(tile->header->x, tile->header->y, tile->header->layer);
      if (tileRef)
        tileToSave = tileRef;
    }
  }
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
  dtNavMesh *navMesh = getNavMeshPtr();
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

} // namespace pathfinder