// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <pathFinder/tileCache.h>
#include <pathFinder/tileCacheUtil.h>
#include <pathFinder/pathFinder.h>
#include <scene/dag_tiledScene.h>
#include <math/dag_mathUtils.h>
#include <detourCommon.h>
#include <detourNavMesh.h>
#include <detourNavMeshQuery.h>
#include <arc/fastlz/fastlz.h>
#define ZDICT_STATIC_LINKING_ONLY 1
#include <zstd.h>
#include <dictBuilder/zdict.h>

namespace pathfinder
{
static bool generateDynamicJumpLinks = true;
static bool generateDynamicLadderLinks = true;
static float maxDynamicJumpLinkDist = 2.5f;

static bool tileCacheRemoveCbEnabled = true;
static tile_remove_cb_t tileCacheRemoveCb = nullptr;

void tilecache_disable_dynamic_jump_links() { generateDynamicJumpLinks = false; }
void tilecache_disable_dynamic_ladder_links() { generateDynamicLadderLinks = false; }
bool tilecache_is_dynamic_ladder_links_enabled() { return generateDynamicLadderLinks; }

void tilecache_disable_tile_remove_cb() { tileCacheRemoveCbEnabled = false; }
void tilecache_set_tile_remove_cb(tile_remove_cb_t tile_remove_cb) { tileCacheRemoveCb = tile_remove_cb; }

static const float fastlzMaxCompressedSizeFactor = 1.05f;

static const unsigned int staticJLBias = 1000;
static const unsigned int staticOLBias = 2000;
static const unsigned int staticEndBias = 3000;
static const unsigned int obstacleBias = 1u << 16u;
static const unsigned int overlinkBias = 8u << 16u;

static const float distToProjJumpLink = 0.25f;

static const float distToProjLadder1 = 0.25f;
static const float distToProjLadder2 = 0.30f;
static const float distToProjLadder3 = 0.35f;
static const float distToSideProjLadder1 = 0.75f;
static const float distToSideProjLadder2 = 0.85f;
static const float distToSideProjLadder3 = 0.95f;
static const float distToProjLadderMax = distToProjLadder3;
static const float distDiffYOver1 = 0.4f;
static const float distDiffYOver2 = 0.6f;
static const float distDiffYOver3 = 0.8f;
static const float distDiffYOverMax = distDiffYOver3;
static const float distDiffYMax = 0.3f;
static const float distDiffYUp = 0.7f;
static const float distDiffYBot = 1.1f;
static const float distDiffYDown = 1.1f;
static const float nudgeDist = 0.05f;
static const float ptNearDist = 0.05f;
static const float ptNearDistSq = sqr(ptNearDist);

static const float maxAgentJump = 1.5;
static const int offmeshLinkSz = 6;
static int reservedSpace = 32;
static float connectionRadius = 0.25f;
static float invStepSize = 2.f;

static inline bool is_obstacle_in_tile(const dtCompressedTileRef *a, const int n, const dtCompressedTileRef v)
{
  for (int i = 0; i < n; ++i)
    if (a[i] == v)
      return true;
  return false;
}

static inline bool is_pt_in_obstacle(const float *center, const float *half_extents, const float *rot_aux, const float *p)
{
  const float y = p[1] - center[1];
  if (y > half_extents[1] || y < -half_extents[1])
    return false;
  const float x = 2.0f * (p[0] - center[0]);
  const float z = 2.0f * (p[2] - center[2]);
  const float xrot = rot_aux[1] * x + rot_aux[0] * z;
  if (xrot > half_extents[0] || xrot < -half_extents[0])
    return false;
  const float zrot = rot_aux[1] * z - rot_aux[0] * x;
  if (zrot > half_extents[2] || zrot < -half_extents[2])
    return false;
  return true;
}

TileCacheCompressor::~TileCacheCompressor()
{
  ZSTD_freeDCtx(dctx);
  ZSTD_freeDDict(dDict);
}

void TileCacheCompressor::reset(bool isZSTD, const Tab<char> &zstdDictBuff)
{
  ZSTD_freeDCtx(dctx);
  ZSTD_freeDDict(dDict);
  dctx = nullptr;
  dDict = nullptr;

  if (!isZSTD)
    return;

  dctx = ZSTD_createDCtx();
  G_ASSERT(dctx);
  if (!zstdDictBuff.empty())
  {
    dDict = ZSTD_createDDict(zstdDictBuff.data(), zstdDictBuff.size());
    G_ASSERT(dDict);
  }
}

int TileCacheCompressor::maxCompressedSize(const int bufferSize)
{
  return (dctx != nullptr) ? (int)ZSTD_compressBound(bufferSize) : (int)(bufferSize * fastlzMaxCompressedSizeFactor);
}

dtStatus TileCacheCompressor::compress(const unsigned char *buffer, const int bufferSize, unsigned char *compressed,
  const int maxCompressedSize, int *compressedSize)
{
  if (dctx != nullptr)
  {
    size_t res = ZSTD_compress(compressed, maxCompressedSize, buffer, bufferSize, -1);
    *compressedSize = ZSTD_isError(res) ? -1 : (int)res;
  }
  else
    *compressedSize = fastlz_compress((const void *const)buffer, bufferSize, compressed);
  return *compressedSize < 0 ? DT_FAILURE : DT_SUCCESS;
}

dtStatus TileCacheCompressor::decompress(const unsigned char *compressed, const int compressedSize, unsigned char *buffer,
  const int maxBufferSize, int *bufferSize)
{
  if (dctx != nullptr)
  {
    size_t res;
    if (dDict)
      res = ZSTD_decompress_usingDDict(dctx, buffer, maxBufferSize, compressed, compressedSize, dDict);
    else
      res = ZSTD_decompressDCtx(dctx, buffer, maxBufferSize, compressed, compressedSize);

    if (!ZSTD_isError(res))
      *bufferSize = (int)res;
    else
    {
      // if failed try fastlz (for example for navmesh patch tiles)
      int sz = fastlz_decompress(compressed, compressedSize, buffer, maxBufferSize);
      *bufferSize = sz > 0 ? sz : -1;
    }
  }
  else
    *bufferSize = fastlz_decompress(compressed, compressedSize, buffer, maxBufferSize);
  return *bufferSize < 0 ? DT_FAILURE : DT_SUCCESS;
}

static unsigned int make_static_over_link_user_id(int type) { return staticOLBias + (unsigned int)(type & 3); }

static int get_static_over_link_type(unsigned int user_id)
{
  if (user_id < staticOLBias || user_id >= staticEndBias)
    return -1;
  return (int)((user_id - staticOLBias) & 3);
}

static unsigned int make_over_link_user_id(int type, int index)
{
  return overlinkBias + ((unsigned int)index << 2) | ((unsigned int)type & 3);
}

static int get_over_link_type(unsigned int user_id)
{
  if (user_id < overlinkBias)
    return -1;
  return (int)((user_id - overlinkBias) & 3);
}

static int get_over_link_index(unsigned int user_id)
{
  if (user_id < overlinkBias)
    return -1;
  return (int)((user_id - overlinkBias) >> 2);
}

static bool check_pt_near_pt(const Point3 &pt1, const float *pt2)
{
  const float dx = pt1.x - pt2[0];
  const float dy = pt1.y - pt2[1];
  const float dz = pt1.z - pt2[2];
  return dx * dx + dy * dy + dz * dz < ptNearDistSq;
}

static bool check_in_range_xz(const Point3 &pt1, const Point3 &pt2, float range)
{
  const float dx = pt1.x - pt2.x;
  const float dz = pt1.z - pt2.z;
  return dx * dx + dz * dz < sqr(range);
}

static bool project_to_nearest_navmesh_point(dtNavMeshQuery *nav_query, Point3 &pos, float horz_extents, bool no_obstacles)
{
  if (!nav_query)
    return false;
  dtQueryFilter filter;
  filter.setIncludeFlags(no_obstacles ? POLYFLAG_GROUND : POLYFLAGS_WALK);
  const Point3 extents(horz_extents, FLT_MAX, horz_extents);
  Point3 resPos;
  dtPolyRef nearestPoly;
  if (dtStatusSucceed(nav_query->findNearestPoly(&pos.x, &extents.x, &filter, &nearestPoly, &resPos.x)) && nearestPoly != 0)
  {
    pos = resPos;
    return true;
  }
  return false;
}

static bool project_ladder_point_on_navmesh(dtNavMeshQuery *nav_query, Point3 &out_pt, const Point3 &pt, const LadderInfo &ladder,
  float dist, bool top)
{
  const float maxdiffYBot = top ? distDiffYMax : distDiffYBot;
  out_pt = pt;
  if (top)
    out_pt.y += distDiffYMax;
  if (!project_to_nearest_navmesh_point(nav_query, out_pt, dist, false) || !check_in_range_xz(out_pt, pt, dist) ||
      out_pt.y < ladder.hmin - maxdiffYBot || out_pt.y > ladder.hmax + distDiffYMax)
  {
    if (!top)
      return false;
    out_pt = pt;
    out_pt.y += distDiffYOver1 + distDiffYMax;
    if (!project_to_nearest_navmesh_point(nav_query, out_pt, dist, false) || !check_in_range_xz(out_pt, pt, dist) ||
        out_pt.y < ladder.hmin - maxdiffYBot || out_pt.y > ladder.hmax + distDiffYOver1 + distDiffYMax)
    {
      out_pt = pt;
      out_pt.y += distDiffYOver2 + distDiffYMax;
      if (!project_to_nearest_navmesh_point(nav_query, out_pt, dist, false) || !check_in_range_xz(out_pt, pt, dist) ||
          out_pt.y < ladder.hmin - maxdiffYBot || out_pt.y > ladder.hmax + distDiffYOver2 + distDiffYMax)
      {
        out_pt = pt;
        out_pt.y += distDiffYOver3 + distDiffYMax;
        if (!project_to_nearest_navmesh_point(nav_query, out_pt, dist, false) || !check_in_range_xz(out_pt, pt, dist) ||
            out_pt.y < ladder.hmin - maxdiffYBot || out_pt.y > ladder.hmax + distDiffYOver3 + distDiffYMax)
          return false;
      }
    }
  }

  Point3 nudgeVec = out_pt - ladder.base;
  nudgeVec.y = 0.f;
  nudgeVec.normalize();
  const Point3 nudgedPt = out_pt + nudgeVec * nudgeDist;
  Point3 nudgedPtProj = nudgedPt;
  if (project_to_nearest_navmesh_point(nav_query, nudgedPtProj, nudgeDist, false))
    if (check_in_range_xz(nudgedPtProj, nudgedPt, 0.001f) && abs(nudgedPtProj.y - nudgedPt.y) < nudgeDist)
      out_pt = nudgedPtProj;

  // NOTE: Rotate forward only in one direction, because forw vector will be negated externally to check for another side
  Point3 forward = ladder.isSideExit ? Point3(-ladder.forw.z, 0.0f, ladder.forw.x) : ladder.forw;
  if (!top && dot(forward, out_pt - ladder.base) > 0.f)
    return false;
  if (top && dot(forward, out_pt - ladder.base) < 0.f)
    return false;
  if (abs(out_pt.y - pt.y) > distDiffYMax && !(top && out_pt.y > pt.y - distDiffYDown) && !(!top && out_pt.y < pt.y + distDiffYUp))
    return false;

  return true;
}

static bool check_side_ladder_point_on_navmesh(dtNavMeshQuery *navQuery, Point3 &out_to, mat44f_cref tm, float coef, float side_coef)
{
  LadderInfo ladder;
  tilecache_calc_ladder_info(ladder, tm, coef * distToProjLadder2, side_coef * distToSideProjLadder1, true);
  ladder.forw *= side_coef;
  if (!project_ladder_point_on_navmesh(navQuery, out_to, ladder.to, ladder, distToProjLadder2, true))
  {
    tilecache_calc_ladder_info(ladder, tm, coef * distToProjLadder2, side_coef * distToSideProjLadder2, true);
    ladder.forw *= side_coef;
    if (!project_ladder_point_on_navmesh(navQuery, out_to, ladder.to, ladder, distToProjLadder2, true))
    {
      tilecache_calc_ladder_info(ladder, tm, coef * distToProjLadder2, side_coef * distToSideProjLadder3, true);
      ladder.forw *= side_coef;
      if (!project_ladder_point_on_navmesh(navQuery, out_to, ladder.to, ladder, distToProjLadder2, true))
      {
        return false;
      }
    }
  }
  return true;
}

static bool calc_side_ladder_points_on_navmesh(dtNavMeshQuery *navQuery, Point3 &out_best_from, Point3 &out_best_to, mat44f_cref tm)
{
  bool found = false;
  float bestDist1 = 0.f;
  for (int side = 0; side < 2; ++side)
  {
    float coef = (side == 0) ? 1.f : -1.f;
    float dist1 = distToProjLadder1;
    float dist2 = distToProjLadder1;
    Point3 out_from(0.f, 0.f, 0.f);
    Point3 out_to(0.f, 0.f, 0.f);

    if (!check_side_ladder_point_on_navmesh(navQuery, out_to, tm, coef, 1.0f))
    {
      if (!check_side_ladder_point_on_navmesh(navQuery, out_to, tm, coef, -1.0f))
      {
        continue;
      }
    }

    LadderInfo ladder;
    tilecache_calc_ladder_info(ladder, tm, coef * dist1, coef * dist2);
    ladder.forw *= coef;
    if (!project_ladder_point_on_navmesh(navQuery, out_from, ladder.from, ladder, dist1, false))
    {
      dist1 = distToProjLadder2;
      tilecache_calc_ladder_info(ladder, tm, coef * dist1, coef * dist2);
      ladder.forw *= coef;
      if (!project_ladder_point_on_navmesh(navQuery, out_from, ladder.from, ladder, dist1, false))
      {
        dist1 = distToProjLadder3;
        tilecache_calc_ladder_info(ladder, tm, coef * dist1, coef * dist2);
        ladder.forw *= coef;
        if (!project_ladder_point_on_navmesh(navQuery, out_from, ladder.from, ladder, dist1, false))
          continue;
      }
    }

    if (!found || dist1 < bestDist1)
    {
      found = true;
      bestDist1 = dist1;
      out_best_from = out_from;
      out_best_to = out_to;
    }
  }
  return found;
}

static bool calc_ladder_points_on_navmesh(dtNavMeshQuery *navQuery, Point3 &out_from, Point3 &out_to, mat44f_cref tm)
{
  int found = -1;
  float bestDist1 = 0.f;
  Point3 bestFrom(0.f, 0.f, 0.f);
  Point3 bestTo(0.f, 0.f, 0.f);

  LadderInfo ladder;
  for (int side = 0; side < 2; ++side)
  {
    float coef = (side == 0) ? 1.f : -1.f;
    float dist1 = distToProjLadder1;
    float dist2 = distToProjLadder1;

    tilecache_calc_ladder_info(ladder, tm, coef * dist1, coef * dist2);

    ladder.forw *= coef;
    if (!project_ladder_point_on_navmesh(navQuery, out_to, ladder.to, ladder, dist2, true))
    {
      dist2 = distToProjLadder2;
      tilecache_calc_ladder_info(ladder, tm, coef * dist1, coef * dist2);
      ladder.forw *= coef;
      if (!project_ladder_point_on_navmesh(navQuery, out_to, ladder.to, ladder, dist2, true))
      {
        dist2 = distToProjLadder3;
        tilecache_calc_ladder_info(ladder, tm, coef * dist1, coef * dist2);
        ladder.forw *= coef;
        if (!project_ladder_point_on_navmesh(navQuery, out_to, ladder.to, ladder, dist2, true))
          continue;
      }
    }

    if (!project_ladder_point_on_navmesh(navQuery, out_from, ladder.from, ladder, dist1, false))
    {
      dist1 = distToProjLadder2;
      tilecache_calc_ladder_info(ladder, tm, coef * dist1, coef * dist2);
      ladder.forw *= coef;
      if (!project_ladder_point_on_navmesh(navQuery, out_from, ladder.from, ladder, dist1, false))
      {
        dist1 = distToProjLadder3;
        tilecache_calc_ladder_info(ladder, tm, coef * dist1, coef * dist2);
        ladder.forw *= coef;
        if (!project_ladder_point_on_navmesh(navQuery, out_from, ladder.from, ladder, dist1, false))
          continue;
      }
    }

    if (found < 0 || dist1 < bestDist1)
    {
      found = side;
      bestDist1 = dist1;
      bestFrom = out_from;
      bestTo = out_to;
    }
  }

  if (found < 0 && !calc_side_ladder_points_on_navmesh(navQuery, bestFrom, bestTo, tm))
    return false;

  out_from = bestFrom;
  out_to = bestTo;
  return true;
}

void TileCacheMeshProcess::calcQueryLaddersBBox(BBox3 &out_box, const float *bmin, const float *bmax)
{
  out_box.boxMin().set(bmin[0], bmin[1] - distDiffYOverMax - distDiffYMax, bmin[2]);
  out_box.boxMax().set(bmax[0], bmax[1] + distDiffYBot, bmax[2]);
  out_box.inflateXZ(distToProjLadderMax);
}

bool TileCacheMeshProcess::checkOverLink(const float *from, const float *to, unsigned int &user_id) const
{
  if (get_static_over_link_type(user_id) == OVERLINK_LADDER && ladders)
  {
    bool found = false;

    BBox3 box;
    box.boxMin().set(from[0], from[1], from[2]);
    box.boxMax().set(to[0], to[1], to[2]);
    bbox3f bbox = v_ldu_bbox3(box);
    ladders->boxCull<false, true>(bbox, 0, 0, [&](scene::node_index idx, mat44f_cref tm) {
      if (found)
        return;
      Point3 from2, to2;
      if (!calc_ladder_points_on_navmesh(meshQuery, from2, to2, tm))
        return;
      if (!check_pt_near_pt(from2, from) || !check_pt_near_pt(to2, to))
        return;
      user_id = make_over_link_user_id(OVERLINK_LADDER, (int)idx);
      found = true;
    });

    return found;
  }

  if (get_over_link_type(user_id) == OVERLINK_LADDER && ladders)
  {
    const int idx = get_over_link_index(user_id);
    // NB: It's perfectly safe to use isAliveNode() here, because even if ladder
    //     was be replaced by another one matching this one, it will just keep
    //     this over link active and drop link otherwise i.e. if not alive or
    //     located in mismatched coordinates. So using reinterpret cast here.
    if (!ladders->isAliveNode(idx))
      return false;
    if (ladders->getNodeIndex(idx) >= ladders->getNodesCount())
      return false;
    const mat44f tm = ladders->getNode(idx);
    Point3 from2, to2;
    if (!calc_ladder_points_on_navmesh(meshQuery, from2, to2, tm))
      return false;
    if (!check_pt_near_pt(from2, from) || !check_pt_near_pt(to2, to))
      return false;
    return true;
  }

  return false;
}

void TileCacheMeshProcess::remove(int tx, int ty, int tlayer)
{
  const dtMeshTile *tile = mesh->getTileAt(tx, ty, tlayer);
  if (!tile)
    return;

  if (tileCacheRemoveCbEnabled && tileCacheRemoveCb)
  {
    uint32_t tileId = mesh->decodePolyIdTile(mesh->getTileRef(tile));
    tileCacheRemoveCb(tileId, tx, ty, tlayer);
  }
}

void TileCacheMeshProcess::process(struct dtNavMeshCreateParams *params, unsigned char *polyAreas, unsigned short *polyFlags,
  dtCompressedTileRef ref)
{
  for (int i = 0; i < params->polyCount; ++i)
  {
    const auto area = polyAreas[i];
    unsigned short flags = POLYFLAG_GROUND;
    switch (area)
    {
      case POLYAREA_UNWALKABLE: flags = 0; break;
      case POLYAREA_OBSTACLE: flags |= POLYFLAG_OBSTACLE; break;
      case POLYAREA_BLOCKED: flags |= POLYFLAG_BLOCKED; break;
    }
    polyFlags[i] = flags;
  }

  const dtMeshTile *tile = mesh->getTileAt(params->tileX, params->tileY, params->tileLayer);
  if (!tile)
    return;

  if (tileCacheRemoveCbEnabled && tileCacheRemoveCb)
  {
    uint32_t tileId = mesh->decodePolyIdTile(mesh->getTileRef(tile));
    tileCacheRemoveCb(tileId, params->tileX, params->tileY, params->tileLayer);
  }

  Tab<int> removedObstaclesId(framemem_ptr());

  for (int i = 0; i < tc->getObstacleCount(); ++i)
  {
    const dtTileCacheObstacle *ob = tc->getObstacle(i);
    if (ob->state != DT_OBSTACLE_REMOVING)
      continue;

    if (!is_obstacle_in_tile(ob->touched, ob->ntouched, ref))
      continue;

    removedObstaclesId.push_back(obstacleBias + i);
  }

  // Copy over offmesh links from existing tile.

  int numOffMeshLinks = tile->header->offMeshConCount;

  offMeshConVerts.resize(numOffMeshLinks * offmeshLinkSz);
  offMeshConRads.resize(numOffMeshLinks);
  offMeshConDirs.resize(numOffMeshLinks);
  offMeshConAreas.resize(numOffMeshLinks);
  offMeshConFlags.resize(numOffMeshLinks);
  offMeshConId.resize(numOffMeshLinks);

  int jumps = -1, jumpLast = -1;
  int overs = -1, overLast = -1;

  for (int i = 0, iEnd = numOffMeshLinks, it = 0; i < iEnd; ++i)
  {
    const dtOffMeshConnection &conn = tile->offMeshCons[i];
    unsigned int userId = conn.userId;

    const bool isStaticJumpLink = userId >= staticJLBias && userId < staticOLBias;
    const bool isStaticOverLink = userId >= staticOLBias && userId < staticEndBias;
    const bool isDynamicJumpLink = userId >= obstacleBias && userId < overlinkBias;
    const bool isDynamicOverLink = userId >= overlinkBias;

    if (isDynamicJumpLink)
    {
      // Skip dynamic jump links for removed obstacles (which added here below last time)
      if (find_value_idx(removedObstaclesId, userId) != -1)
      {
        numOffMeshLinks--;
        continue;
      }
      if (jumps < 0)
        jumps = it;
      jumpLast = it;
    }

    const bool isJumpLink = isStaticJumpLink || isDynamicJumpLink;
    const bool isOverLink = isStaticOverLink || isDynamicOverLink;

    // When rebuilding tiles multiple times we're not able to find out original area and flags
    // of an offmesh connection, that's because this information may be lost if it becomes an
    // obstacle at least once. That's why we set area and flags to a type based on connection
    // userId (XXX_JUMP for jump links, POLYAREA_BRIDGE/POLYFLAG_LADDER for over links)
    unsigned char areas = isJumpLink ? POLYAREA_JUMP : 0;
    unsigned short flags = isJumpLink ? POLYFLAG_JUMP : 0;

    if (isOverLink)
    {
      bool check = true;
      if (isStaticOverLink && get_static_over_link_type(userId) == OVERLINK_LADDER && !generateDynamicLadderLinks)
        check = false;
      if (check)
      {
        // Skip any over links not connecting anything, convert static overlinks to dynamic (userId)
        if (!checkOverLink(&conn.pos[0], &conn.pos[3], userId))
        {
          numOffMeshLinks--;
          continue;
        }
        if (overs < 0)
          overs = it;
        overLast = it;

        switch (get_over_link_type(userId))
        {
          case OVERLINK_LADDER:
            areas = POLYAREA_BRIDGE;
            flags = POLYFLAG_LADDER;
            break;
          case OVERLINK_BRIDGE: areas = POLYAREA_BRIDGE; break;
        }
      }
    }

    memcpy(&offMeshConVerts[it * offmeshLinkSz], &conn.pos[0], sizeof(float) * offmeshLinkSz);
    offMeshConRads[it] = conn.rad;
    offMeshConDirs[it] = ((conn.flags & DT_OFFMESH_CON_BIDIR) != 0) ? DT_OFFMESH_CON_BIDIR : 0;
    offMeshConAreas[it] = areas;
    offMeshConFlags[it] = flags;
    offMeshConId[it] = userId;

    it++;
  }

  offMeshConVerts.resize(numOffMeshLinks * offmeshLinkSz);
  offMeshConRads.resize(numOffMeshLinks);
  offMeshConDirs.resize(numOffMeshLinks);
  offMeshConAreas.resize(numOffMeshLinks);
  offMeshConFlags.resize(numOffMeshLinks);
  offMeshConId.resize(numOffMeshLinks);

  offMeshConVerts.reserve(numOffMeshLinks * offmeshLinkSz + reservedSpace * 3);
  offMeshConRads.reserve(numOffMeshLinks + reservedSpace);
  offMeshConDirs.reserve(numOffMeshLinks + reservedSpace);
  offMeshConAreas.reserve(numOffMeshLinks + reservedSpace);
  offMeshConFlags.reserve(numOffMeshLinks + reservedSpace);
  offMeshConId.reserve(numOffMeshLinks + reservedSpace);

  const int maxObstacles = (staticLinks || !generateDynamicJumpLinks) ? 0 : tc->getObstacleCount();
  for (int i = 0; i < maxObstacles; ++i)
  {
    const dtTileCacheObstacle *ob = tc->getObstacle(i);
    if (ob->state == DT_OBSTACLE_EMPTY || ob->state == DT_OBSTACLE_REMOVING || ob->areaId != POLYAREA_OBSTACLE)
      continue;

    if (!is_obstacle_in_tile(ob->touched, ob->ntouched, ref))
      continue;

    if (jumps >= 0)
    {
      dag::ConstSpan<uint32_t> span(&offMeshConId[0] + jumps, jumpLast - jumps + 1);
      if (find_value_idx(span, obstacleBias + i) != -1)
        continue;
    }

    const float lenX = ob->orientedBox.halfExtents[0] * 2.;
    const float lenZ = ob->orientedBox.halfExtents[2] * 2.;

    const bool tooLongX = lenX > maxDynamicJumpLinkDist;
    const bool tooLongZ = lenZ > maxDynamicJumpLinkDist;
    if (tooLongX && tooLongZ)
      continue;

    const int numLinksX = tooLongZ ? 0 : static_cast<int>(lenX * invStepSize);
    const int numLinksZ = tooLongX ? 0 : static_cast<int>(lenZ * invStepSize);

    const float stepX = numLinksX != 0 ? lenX / numLinksX : 0.;
    const float stepZ = numLinksZ != 0 ? lenZ / numLinksZ : 0.;

    const float arrCoeff[] = {2.f, -2.};

    const float orientedZA = (ob->orientedBox.halfExtents[2] + 0.5) * arrCoeff[0];
    const float orientedZB = (ob->orientedBox.halfExtents[2] + 0.5) * arrCoeff[1];

    for (int x = 1, xEnd = numLinksX - 1; x <= xEnd; ++x)
    {
      const float orientedX = (ob->orientedBox.halfExtents[0] - x * stepX) * 2.f;

      const float rotatedXA = ob->orientedBox.rotAux[1] * orientedX + ob->orientedBox.rotAux[0] * orientedZA;
      const float rotatedZA = -ob->orientedBox.rotAux[1] * orientedZA + ob->orientedBox.rotAux[0] * orientedX;

      const float rotatedXB = ob->orientedBox.rotAux[1] * orientedX + ob->orientedBox.rotAux[0] * orientedZB;
      const float rotatedZB = -ob->orientedBox.rotAux[1] * orientedZB + ob->orientedBox.rotAux[0] * orientedX;

      Point3 ptA(ob->orientedBox.center[0] + rotatedXA, ob->orientedBox.center[1], ob->orientedBox.center[2] + rotatedZA);

      bool isCanProjectPointA = pathfinder::project_to_nearest_navmesh_point(ptA, distToProjJumpLink);

      if (!isCanProjectPointA || ptA.y < ob->orientedBox.center[1] - ob->orientedBox.halfExtents[1] ||
          ptA.y > ob->orientedBox.center[1] + ob->orientedBox.halfExtents[1])
        continue;

      if (ob->orientedBox.center[1] + ob->orientedBox.halfExtents[1] - ptA.y > maxAgentJump)
        continue;

      Point3 ptB(ob->orientedBox.center[0] + rotatedXB, ob->orientedBox.center[1], ob->orientedBox.center[2] + rotatedZB);

      bool isCanProjectPointB = pathfinder::project_to_nearest_navmesh_point(ptB, distToProjJumpLink);

      if (!isCanProjectPointB || ptB.y < ob->orientedBox.center[1] - ob->orientedBox.halfExtents[1] ||
          ptB.y > ob->orientedBox.center[1] + ob->orientedBox.halfExtents[1])
        continue;

      if (ob->orientedBox.center[1] + ob->orientedBox.halfExtents[1] - ptB.y > maxAgentJump)
        continue;

      Point3 outPos;
      if (pathfinder::traceray_navmesh(ptA, ptB, Point3(distToProjJumpLink, FLT_MAX, distToProjJumpLink), outPos))
        continue;

      offMeshConVerts.push_back(ptA.x);
      offMeshConVerts.push_back(ptA.y);
      offMeshConVerts.push_back(ptA.z);

      offMeshConVerts.push_back(ptB.x);
      offMeshConVerts.push_back(ptB.y);
      offMeshConVerts.push_back(ptB.z);

      offMeshConRads.push_back(connectionRadius);
      offMeshConDirs.push_back(1);

      offMeshConAreas.push_back(POLYAREA_JUMP);
      offMeshConFlags.push_back(POLYFLAG_JUMP);

      offMeshConId.push_back(obstacleBias + i);

      numOffMeshLinks++;
    }

    const float orientedXA = (ob->orientedBox.halfExtents[0] + 0.5) * arrCoeff[0];
    const float orientedXB = (ob->orientedBox.halfExtents[0] + 0.5) * arrCoeff[1];

    for (int z = 1, z_e = numLinksZ - 1; z <= z_e; ++z)
    {
      const float orientedZ = (ob->orientedBox.halfExtents[2] - z * stepZ) * 2.f;

      const float rotatedXA = ob->orientedBox.rotAux[1] * orientedXA + ob->orientedBox.rotAux[0] * orientedZ;
      const float rotatedZA = -ob->orientedBox.rotAux[1] * orientedZ + ob->orientedBox.rotAux[0] * orientedXA;

      const float rotatedXB = ob->orientedBox.rotAux[1] * orientedXB + ob->orientedBox.rotAux[0] * orientedZ;
      const float rotatedZB = -ob->orientedBox.rotAux[1] * orientedZ + ob->orientedBox.rotAux[0] * orientedXB;

      Point3 ptA(ob->orientedBox.center[0] + rotatedXA, ob->orientedBox.center[1], ob->orientedBox.center[2] + rotatedZA);

      bool isCanProjectPointA = pathfinder::project_to_nearest_navmesh_point(ptA, distToProjJumpLink);

      if (!isCanProjectPointA || ptA.y < ob->orientedBox.center[1] - ob->orientedBox.halfExtents[1] ||
          ptA.y > ob->orientedBox.center[1] + ob->orientedBox.halfExtents[1])
        continue;

      if (ob->orientedBox.center[1] + ob->orientedBox.halfExtents[1] - ptA.y > maxAgentJump)
        continue;

      Point3 ptB(ob->orientedBox.center[0] + rotatedXB, ob->orientedBox.center[1], ob->orientedBox.center[2] + rotatedZB);

      bool isCanProjectPointB = pathfinder::project_to_nearest_navmesh_point(ptB, distToProjJumpLink);

      if (!isCanProjectPointB || ptB.y < ob->orientedBox.center[1] - ob->orientedBox.halfExtents[1] ||
          ptB.y > ob->orientedBox.center[1] + ob->orientedBox.halfExtents[1])
        continue;

      if (ob->orientedBox.center[1] + ob->orientedBox.halfExtents[1] - ptB.y > maxAgentJump)
        continue;

      Point3 outPos;
      if (pathfinder::traceray_navmesh(ptA, ptB, Point3(distToProjJumpLink, FLT_MAX, distToProjJumpLink), outPos))
        continue;

      offMeshConVerts.push_back(ptA.x);
      offMeshConVerts.push_back(ptA.y);
      offMeshConVerts.push_back(ptA.z);

      offMeshConVerts.push_back(ptB.x);
      offMeshConVerts.push_back(ptB.y);
      offMeshConVerts.push_back(ptB.z);

      offMeshConRads.push_back(connectionRadius);
      offMeshConDirs.push_back(1);

      offMeshConAreas.push_back(POLYAREA_JUMP);
      offMeshConFlags.push_back(POLYFLAG_JUMP);

      offMeshConId.push_back(obstacleBias + i);

      numOffMeshLinks++;
    }
  }

  if (generateDynamicLadderLinks && ladders)
  {
    BBox3 box;
    calcQueryLaddersBBox(box, params->bmin, params->bmax);
    bbox3f bbox = v_ldu_bbox3(box);
    ladders->boxCull<false, true>(bbox, 0, 0, [&](scene::node_index idx, mat44f_cref tm) {
      const unsigned int userId =
        staticLinks ? make_static_over_link_user_id(OVERLINK_LADDER) : make_over_link_user_id(OVERLINK_LADDER, (int)idx);

      if (overs >= 0)
      {
        dag::ConstSpan<uint32_t> span(&offMeshConId[0] + overs, overLast - overs + 1);
        if (find_value_idx(span, userId) != -1)
          return;
      }

      Point3 from, to;
      if (!calc_ladder_points_on_navmesh(meshQuery, from, to, tm))
        return;

      // Don't add if ladder starts in another tile
      if (from.x < params->bmin[0] || from.x > params->bmax[0])
        return;
      if (from.z < params->bmin[2] || from.z > params->bmax[2])
        return;

      offMeshConVerts.push_back(from.x);
      offMeshConVerts.push_back(from.y);
      offMeshConVerts.push_back(from.z);

      offMeshConVerts.push_back(to.x);
      offMeshConVerts.push_back(to.y);
      offMeshConVerts.push_back(to.z);

      offMeshConRads.push_back(connectionRadius);
      offMeshConDirs.push_back(1);

      offMeshConAreas.push_back(POLYAREA_BRIDGE);
      offMeshConFlags.push_back(POLYFLAG_LADDER);

      offMeshConId.push_back(userId);

      numOffMeshLinks++;
    });
  }

  params->offMeshConVerts = offMeshConVerts.data();
  params->offMeshConRad = offMeshConRads.data();
  params->offMeshConDir = offMeshConDirs.data();
  params->offMeshConAreas = offMeshConAreas.data();
  params->offMeshConFlags = offMeshConFlags.data();
  params->offMeshConUserID = offMeshConId.data();
  params->offMeshConCount = numOffMeshLinks;
}

BBox3 tilecache_calc_tile_bounds(const dtTileCacheParams *params, const dtTileCacheLayerHeader *header)
{
  BBox3 aabb;
  const float cs = params->cs;
  aabb.lim[0].x = header->bmin[0] + header->minx * cs;
  aabb.lim[0].y = header->bmin[1];
  aabb.lim[0].z = header->bmin[2] + header->miny * cs;
  aabb.lim[1].x = header->bmin[0] + (header->maxx + 1) * cs;
  aabb.lim[1].y = header->bmax[1];
  aabb.lim[1].z = header->bmin[2] + (header->maxy + 1) * cs;
  return aabb;
}
} // namespace pathfinder
