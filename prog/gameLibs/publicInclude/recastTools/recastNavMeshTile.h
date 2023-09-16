//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <recast.h>

#include <detourCommon.h>
#include <detourNavMesh.h>
#include <detourTileCache.h>
#include <detourTileCacheBuilder.h>
#include <detourNavMeshBuilder.h>

#include <EASTL/unique_ptr.h>
#include <math/integer/dag_IPoint2.h>

#define MAX_OFFMESH_CONNECTIONS 512

namespace recastnavmesh
{
struct OffMeshConnectionsStorage
{
  float m_offMeshConVerts[MAX_OFFMESH_CONNECTIONS * 3 * 2] = {0};
  float m_offMeshConRads[MAX_OFFMESH_CONNECTIONS] = {0};
  unsigned char m_offMeshConDirs[MAX_OFFMESH_CONNECTIONS] = {0};
  unsigned char m_offMeshConAreas[MAX_OFFMESH_CONNECTIONS] = {0};
  unsigned short m_offMeshConFlags[MAX_OFFMESH_CONNECTIONS] = {0};
  unsigned int m_offMeshConId[MAX_OFFMESH_CONNECTIONS] = {0};
  int m_offMeshConCount = 0;
};

struct BuildTileData
{
  int layer = 0;
  unsigned char *navMeshData = nullptr;
  int navMeshDataSz = 0;
  unsigned char *tileCacheData = nullptr;
  int tileCacheDataSz = 0;
};

struct RecastTileLayerContext
{
  explicit RecastTileLayerContext(struct dtTileCacheAlloc *a) : layer(nullptr), lcset(nullptr), lmesh(nullptr), alloc(a) {}
  ~RecastTileLayerContext()
  {
    dtFreeTileCacheLayer(alloc, layer);
    layer = nullptr;
    dtFreeTileCacheContourSet(alloc, lcset);
    lcset = nullptr;
    dtFreeTileCachePolyMesh(alloc, lmesh);
    lmesh = nullptr;
  }

  dtTileCacheLayer *layer;
  dtTileCacheContourSet *lcset;
  dtTileCachePolyMesh *lmesh;
  dtTileCacheAlloc *alloc;
};

struct RecastTileContext
{
  rcHeightfield *solid = NULL;
  rcCompactHeightfield *chf = NULL;
  // tiled specific
  rcContourSet *cset = NULL;
  rcPolyMesh *pmesh = NULL;
  rcPolyMeshDetail *dmesh = NULL;
  // tilecached specific
  rcHeightfieldLayerSet *lset = NULL;

  void clearIntermediate(Tab<BuildTileData> *tileData)
  {
    if (tileData)
    {
      for (BuildTileData &td : *tileData)
      {
        dtFree(td.navMeshData);
        td.navMeshData = nullptr;
        dtFree(td.tileCacheData);
        td.tileCacheData = nullptr;
      }
      clear_and_shrink(*tileData);
    }
    rcFreeHeightField(solid);
    solid = NULL;
    rcFreeCompactHeightfield(chf);
    chf = NULL;
    rcFreeContourSet(cset);
    cset = NULL;
    rcFreePolyMesh(pmesh);
    pmesh = NULL;
    rcFreePolyMeshDetail(dmesh);
    dmesh = NULL;
    rcFreeHeightfieldLayerSet(lset);
    lset = NULL;
  }
};

template <class T, class TFunc>
bool finalize_navmesh_tilecached_tile(rcContext &ctx, const rcConfig &cfg, dtTileCacheAlloc *tcAllocator,
  dtTileCacheCompressor *tcComp, OffMeshConnectionsStorage *conn_storage, RecastTileContext &tile_ctx, int tx, int ty,
  float walkableClimb, float walkableHeight, float walkableRadius, const T &obstacles, Tab<BuildTileData> &tile_data,
  TFunc markObstaclesCb)
{
  tile_ctx.lset = rcAllocHeightfieldLayerSet();
  if (!tile_ctx.lset)
  {
    tile_ctx.clearIntermediate(&tile_data);
    return false;
  }
  if (!rcBuildHeightfieldLayers(&ctx, *tile_ctx.chf, cfg.borderSize, cfg.walkableHeight, *tile_ctx.lset))
  {
    tile_ctx.clearIntermediate(&tile_data);
    return false;
  }

  const int walkableClimbVx = (int)(walkableClimb / cfg.ch);

  const int maxLayersPerTile = 32;
  int nlayers = rcMin(tile_ctx.lset->nlayers, maxLayersPerTile);

  for (int i = 0; i < nlayers; ++i)
  {
    BuildTileData &td = tile_data.push_back() = BuildTileData();
    td.layer = i;
    const rcHeightfieldLayer *layer = &tile_ctx.lset->layers[i];

    // Store header
    dtTileCacheLayerHeader header;
    header.magic = DT_TILECACHE_MAGIC;
    header.version = DT_TILECACHE_VERSION;

    // Tile layer location in the navmesh.
    header.tx = tx;
    header.ty = ty;
    header.tlayer = i;
    dtVcopy(header.bmin, layer->bmin);
    dtVcopy(header.bmax, layer->bmax);

    // Tile info.
    G_ASSERT(layer->width <= 255);
    G_ASSERT(layer->height <= 255);
    G_ASSERT(layer->minx <= 255);
    G_ASSERT(layer->maxx <= 255);
    G_ASSERT(layer->miny <= 255);
    G_ASSERT(layer->maxy <= 255);
    G_ASSERT(layer->hmin <= 65535);
    G_ASSERT(layer->hmax <= 65535);
    header.width = (unsigned char)layer->width;
    header.height = (unsigned char)layer->height;
    header.minx = (unsigned char)layer->minx;
    header.maxx = (unsigned char)layer->maxx;
    header.miny = (unsigned char)layer->miny;
    header.maxy = (unsigned char)layer->maxy;
    header.hmin = (unsigned short)layer->hmin;
    header.hmax = (unsigned short)layer->hmax;

    dtStatus status =
      dtBuildTileCacheLayer(tcComp, &header, layer->heights, layer->areas, layer->cons, &td.tileCacheData, &td.tileCacheDataSz);
    if (dtStatusFailed(status))
    {
      ctx.log(RC_LOG_ERROR, "buildNavigation: Could not build tile cache layer.");
      tile_ctx.clearIntermediate(&tile_data);
      return false;
    }

    RecastTileLayerContext lctx(tcAllocator);

    // Decompress tile layer data.
    status = dtDecompressTileCacheLayer(tcAllocator, tcComp, td.tileCacheData, td.tileCacheDataSz, &lctx.layer);
    if (dtStatusFailed(status))
    {
      ctx.log(RC_LOG_ERROR, "buildNavigation: Could not decompress tile cache data.");
      tile_ctx.clearIntermediate(&tile_data);
      return false;
    }

    markObstaclesCb(obstacles, cfg, *lctx.layer, header);

    // Build navmesh
    status = dtBuildTileCacheRegions(tcAllocator, *lctx.layer, walkableClimbVx);
    if (dtStatusFailed(status))
    {
      ctx.log(RC_LOG_ERROR, "buildNavigation: Could not build tile cache regions.");
      tile_ctx.clearIntermediate(&tile_data);
      return false;
    }

    lctx.lcset = dtAllocTileCacheContourSet(tcAllocator);
    if (!lctx.lcset)
    {
      ctx.log(RC_LOG_ERROR, "buildNavigation: Could not allocate tile cache contour set.");
      tile_ctx.clearIntermediate(&tile_data);
      return false;
    }

    status = dtBuildTileCacheContours(tcAllocator, *lctx.layer, walkableClimbVx, cfg.maxSimplificationError, *lctx.lcset);
    if (dtStatusFailed(status))
    {
      ctx.log(RC_LOG_ERROR, "buildNavigation: Could not build tile cache contours.");
      tile_ctx.clearIntermediate(&tile_data);
      return false;
    }

    lctx.lmesh = dtAllocTileCachePolyMesh(tcAllocator);
    if (!lctx.lmesh)
    {
      ctx.log(RC_LOG_ERROR, "buildNavigation: Could not allocate tile poly mesh.");
      tile_ctx.clearIntermediate(&tile_data);
      return false;
    }

    status = dtBuildTileCachePolyMesh(tcAllocator, *lctx.lcset, *lctx.lmesh);
    if (dtStatusFailed(status))
    {
      ctx.log(RC_LOG_ERROR, "buildNavigation: Could not build tile poly mesh.");
      tile_ctx.clearIntermediate(&tile_data);
      return false;
    }

    // Early out if the mesh tile is empty.
    if (!lctx.lmesh->npolys)
    {
      // Free tile cache data, empty 'td' means empty tile.
      dtFree(td.tileCacheData);
      td.tileCacheData = nullptr;
      td.tileCacheDataSz = 0;
      continue;
    }

    if (cfg.maxVertsPerPoly > DT_VERTS_PER_POLYGON)
    {
      tile_ctx.clearIntermediate(&tile_data);
      return true;
    }

    for (int j = 0; j < lctx.lmesh->npolys; ++j)
      lctx.lmesh->flags[j] = 1;

    dtNavMeshCreateParams params;
    memset(&params, 0, sizeof(params));
    params.verts = lctx.lmesh->verts;
    params.vertCount = lctx.lmesh->nverts;
    params.polys = lctx.lmesh->polys;
    params.polyAreas = lctx.lmesh->areas;
    params.polyFlags = lctx.lmesh->flags;
    params.polyCount = lctx.lmesh->npolys;
    params.nvp = lctx.lmesh->nvp;
    if (conn_storage)
    {
      // we're feeding offmesh links built for all layers to each separate layer
      // but that's ok, dtCreateNavMeshData will take care of that, it'll strip
      // all offmesh links that don't intersect with this layer.
      params.offMeshConVerts = conn_storage->m_offMeshConVerts;
      params.offMeshConRad = conn_storage->m_offMeshConRads;
      params.offMeshConDir = conn_storage->m_offMeshConDirs;
      params.offMeshConAreas = conn_storage->m_offMeshConAreas;
      params.offMeshConFlags = conn_storage->m_offMeshConFlags;
      params.offMeshConUserID = conn_storage->m_offMeshConId;
      params.offMeshConCount = conn_storage->m_offMeshConCount;
    }
    params.walkableHeight = walkableHeight;
    params.walkableRadius = walkableRadius;
    params.walkableClimb = walkableClimb;
    params.tileX = tx;
    params.tileY = ty;
    params.tileLayer = i;
    rcVcopy(params.bmin, header.bmin);
    rcVcopy(params.bmax, header.bmax);
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
    td.navMeshData = navData;
    td.navMeshDataSz = navDataSize;
  }

  tile_ctx.clearIntermediate(nullptr);

  return true;
}

/// Normalizes the vector.
///  @param[in,out]	v	The vector to normalize. [(x, y, z)]
inline void rcVnormalizeSafe(float *v)
{
  float l = rcSqrt(rcSqr(v[0]) + rcSqr(v[1]) + rcSqr(v[2]));

  if (l <= 1e-3)
    return;

  float d = 1.0f / l;
  v[0] *= d;
  v[1] *= d;
  v[2] *= d;
}

inline void calcTriNormalSafe(const float *v0, const float *v1, const float *v2, float *norm)
{
  float e0[3], e1[3];
  rcVsub(e0, v1, v0);
  rcVsub(e1, v2, v0);
  rcVcross(norm, e0, e1);
  rcVnormalizeSafe(norm);
}

/// @par
///
/// Only sets the area id's for the walkable triangles.  Does not alter the
/// area id's for unwalkable triangles.
///
/// See the #rcConfig documentation for more information on the configuration parameters.
///
/// @see rcHeightfield, rcClearUnwalkableTriangles, rcRasterizeTriangles
inline void rcMarkWalkableTrianglesSafe(const float walkableSlopeAngle, const float *verts, const int *tris, int nt,
  unsigned char *areas)
{
  const float walkableThr = cosf(walkableSlopeAngle / 180.0f * M_PI);

  float norm[3];

  for (int i = 0; i < nt; ++i)
  {
    const int *tri = &tris[i * 3];
    calcTriNormalSafe(&verts[tri[0] * 3], &verts[tri[1] * 3], &verts[tri[2] * 3], norm);
    // Check if the face is walkable.
    if (norm[1] > walkableThr)
      areas[i] = RC_WALKABLE_AREA;
  }
}

template <class T, class U, class V>
bool prepare_tile_context(rcContext &ctx, rcConfig &cfg, RecastTileContext &tile_ctx, const T &vertices, const U &indices,
  const V &transparent, bool build_cset)
{
  //
  // Step 1. Rasterize input polygon soup.
  //

  // Allocate voxel heightfield where we rasterize our input data to.
  tile_ctx.solid = rcAllocHeightfield();

  if (!tile_ctx.solid)
  {
    ctx.log(RC_LOG_ERROR, "buildNavigation: Out of memory 'tile_ctx.solid'.");
    tile_ctx.clearIntermediate(nullptr);
    return false;
  }
  if (!rcCreateHeightfield(&ctx, *tile_ctx.solid, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch))
  {
    ctx.log(RC_LOG_ERROR, "buildNavigation: Could not create tile_ctx.solid heightfield.");
    tile_ctx.clearIntermediate(nullptr);
    return false;
  }

  // Allocate array that can hold triangle area types.
  // If you have multiple meshes you need to process, allocate
  // and array which can hold the max number of triangles you need to process.
  eastl::unique_ptr<unsigned char[]> triareas(new unsigned char[indices.size() / 3]);

  // Find triangles which are walkable based on their slope and rasterize them.
  // If your input data is multiple meshes, you can transform them here, calculate
  // the are type for each of the meshes and rasterize them.
  memset(triareas.get(), 0, indices.size() / 3 * sizeof(unsigned char));
  rcMarkWalkableTrianglesSafe(cfg.walkableSlopeAngle, (float *)vertices.data(), indices.data(), indices.size() / 3, triareas.get());

  if (transparent.empty())
  {
    rcRasterizeTriangles(&ctx, (const float *)vertices.data(), vertices.size(), indices.data(), triareas.get(), indices.size() / 3,
      *tile_ctx.solid, cfg.walkableClimb);
  }
  else
  {
    const float *v = (const float *)vertices.data();
    const int vn = vertices.size();

    int indAt = 0;
    const int indNum = (int)indices.size();
    const int trNum = (int)transparent.size();
    for (int i = 0; i < trNum; ++i)
    {
      IPoint2 tr = transparent[i];
      const int indCnt = tr.x - indAt;
      G_ASSERT((indAt % 3) == 0);
      G_ASSERT((indCnt % 3) == 0);
      if (indCnt > 0)
      {
        rcRasterizeTriangles(&ctx, v, vn, indices.data() + indAt, triareas.get() + indAt / 3, indCnt / 3, *tile_ctx.solid,
          cfg.walkableClimb);
      }
      indAt = tr.x + tr.y;
    }
    const int indCnt = indNum - indAt;
    if (indCnt > 0)
    {
      G_ASSERT((indAt % 3) == 0);
      G_ASSERT((indCnt % 3) == 0);
      rcRasterizeTriangles(&ctx, v, vn, indices.data() + indAt, triareas.get() + indAt / 3, indCnt / 3, *tile_ctx.solid,
        cfg.walkableClimb);
    }
  }

  triareas.reset();

  //
  // Step 2. Filter walkables surfaces.
  //

  // Once all geoemtry is rasterized, we do initial pass of filtering to
  // remove unwanted overhangs caused by the conservative rasterization
  // as well as filter spans where the character cannot possibly stand.
  rcFilterLowHangingWalkableObstacles(&ctx, cfg.walkableClimb, *tile_ctx.solid);
  rcFilterLedgeSpans(&ctx, cfg.walkableHeight, cfg.walkableClimb, *tile_ctx.solid);
  rcFilterWalkableLowHeightSpans(&ctx, cfg.walkableHeight, *tile_ctx.solid);

  //
  // Step 3. Partition walkable surface to simple regions.
  //

  // Compact the heightfield so that it is faster to handle from now on.
  // This will result more cache coherent data as well as the neighbours
  // between walkable cells will be calculated.
  tile_ctx.chf = rcAllocCompactHeightfield();
  if (!tile_ctx.chf)
  {
    ctx.log(RC_LOG_ERROR, "buildNavigation: Out of memory 'tile_ctx.chf'.");
    tile_ctx.clearIntermediate(nullptr);
    return false;
  }
  if (!rcBuildCompactHeightfield(&ctx, cfg.walkableHeight, cfg.walkableClimb, *tile_ctx.solid, *tile_ctx.chf))
  {
    ctx.log(RC_LOG_ERROR, "buildNavigation: Could not build compact data.");
    tile_ctx.clearIntermediate(nullptr);
    return false;
  }

  // Erode the walkable area by agent radius.
  if (!rcErodeWalkableArea(&ctx, cfg.walkableRadius, *tile_ctx.chf))
  {
    ctx.log(RC_LOG_ERROR, "buildNavigation: Could not erode.");
    tile_ctx.clearIntermediate(nullptr);
    return false;
  }

  switch (1)
  {
    case 1:
      // Prepare for region partitioning, by calculating distance field along the walkable surface.
      if (!rcBuildDistanceField(&ctx, *tile_ctx.chf))
      {
        ctx.log(RC_LOG_ERROR, "buildNavigation: Could not build distance field.");
        tile_ctx.clearIntermediate(nullptr);
        return false;
      }
      // Partition the walkable surface into simple regions without holes.
      if (!rcBuildRegions(&ctx, *tile_ctx.chf, cfg.borderSize, cfg.minRegionArea, cfg.mergeRegionArea))
      {
        ctx.log(RC_LOG_ERROR, "buildNavigation: Could not build watershed regions.");
        tile_ctx.clearIntermediate(nullptr);
        return false;
      }
      break;
    case 2:
      // Partition the walkable surface into simple regions without holes.
      // Monotone partitioning does not need distancefield.
      if (!rcBuildRegionsMonotone(&ctx, *tile_ctx.chf, cfg.borderSize, cfg.minRegionArea, cfg.mergeRegionArea))
      {
        ctx.log(RC_LOG_ERROR, "buildNavigation: Could not build monotone regions.");
        tile_ctx.clearIntermediate(nullptr);
        return false;
      }
      break;
    default: G_ASSERTF(0, "SAMPLE_PARTITION_LAYERS unsupported");
  }

  if (build_cset)
  {
    //
    // Step 5. Trace and simplify region contours.
    //

    // Create contours.
    tile_ctx.cset = rcAllocContourSet();
    if (!tile_ctx.cset)
    {
      ctx.log(RC_LOG_ERROR, "buildNavigation: Out of memory 'tile_ctx.cset'.");
      tile_ctx.clearIntermediate(nullptr);
      return false;
    }
    if (!rcBuildContours(&ctx, *tile_ctx.chf, cfg.maxSimplificationError, cfg.maxEdgeLen, *tile_ctx.cset))
    {
      ctx.log(RC_LOG_ERROR, "buildNavigation: Could not create contours.");
      tile_ctx.clearIntermediate(nullptr);
      return false;
    }
  }

  return true;
}
} // namespace recastnavmesh
