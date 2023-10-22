#include <pathFinder/pathFinder.h>
#include <pathFinder/customNav.h>
#include <math/dag_mathUtils.h>
#include <util/dag_hash.h>
#include <memory/dag_framemem.h>
#include <detourNavMesh.h>
#include <detourCommon.h>
#include <EASTL/sort.h>

namespace pathfinder
{
static uint32_t lastAreaId = 0;

// Variation of 'does_line_intersect_box' that is optimized to be as fast as possible with
// minimum branching and math, uses a lot of precalculated stuff.
// See also: http://psgraphics.blogspot.com/2016/02/new-simple-ray-box-test-from-andrew.html
static inline bool testLineAABB(const BBox3 &aabb, const Point3 &pos, const Point3 &normDir, const Point3 &dir, float len, Point3 &res)
{
  float tmin = -eastl::numeric_limits<float>::max();
  float tmax = eastl::numeric_limits<float>::max();
  for (int i = 0; i < 3; ++i)
  {
    float invD = fastinv(dir[i]); // inf/nan in case of 0.0f is handled correctly as well by the code below.
    float t0 = (aabb.lim[0][i] - pos[i]) * invD;
    float t1 = (aabb.lim[1][i] - pos[i]) * invD;
    if (invD < 0.0f)
      eastl::swap(t0, t1);
    tmin = t0 > tmin ? t0 : tmin;
    tmax = t1 < tmax ? t1 : tmax;
    if (tmax <= tmin)
      return false;
  }
  if (tmin >= len)
    return false;
  else if (tmin <= 0.0f)
  {
    // If starting point is inside aabb we return final point, not the starting point.
    // might be counterintuitive, but we need this.
    res = pos + dir;
  }
  else
    res = pos + normDir * tmin;
  return true;
}

static inline bool testCircleAABB(const BBox3 &aabb, const Point3 &c, float r2)
{
  float dmin = 0;
  if (c.x < aabb.lim[0].x)
    dmin += SQR(c.x - aabb.lim[0].x);
  else if (c.x > aabb.lim[1].x)
    dmin += SQR(c.x - aabb.lim[1].x);
  if (c.z < aabb.lim[0].z)
    dmin += SQR(c.z - aabb.lim[0].z);
  else if (c.z > aabb.lim[1].z)
    dmin += SQR(c.z - aabb.lim[1].z);
  return dmin <= r2;
}

static inline bool lineChangesQuadrant(const Point3 &origin, const Point3 &p1, const Point3 &p2)
{
  Point2 c = Point2::xz(origin);
  Point2 d1 = Point2::xz(p1) - c;
  Point2 d2 = Point2::xz(p2) - c;
  return (sign(d1.x) != sign(d2.x)) || (sign(d1.y) != sign(d2.y));
}

uint32_t CustomNav::allocAreaId()
{
  uint32_t res = ++lastAreaId;
  if (res == 0)
    res = ++lastAreaId;
  return res;
}

void CustomNav::areaUpdateBox(uint32_t area_id, const TMatrix &tm, const BBox3 &oobb, float w1, float w2, bool optimize,
  float posThreshold, float angCosThreshold)
{
  Area area;
  area.id = area_id;
  area.oobb = oobb;
  area.tm = tm;
  area.invTm = inverse(tm);
  area.weight[0] = w1;
  area.weight[1] = w2;
  area.optimize = optimize;
  areaUpdate(area, posThreshold, angCosThreshold);
}

void CustomNav::areaUpdateCylinder(uint32_t area_id, const BBox3 &aabb, float w1, float w2, bool optimize, float posThreshold,
  float angCosThreshold)
{
  Area area;
  area.id = area_id;
  area.isCylinder = true;
  area.oobb = aabb;
  area.tm = TMatrix::IDENT;
  area.weight[0] = w1;
  area.weight[1] = w2;
  area.optimize = optimize;
  areaUpdate(area, posThreshold, angCosThreshold);
}

void CustomNav::areaUpdate(const Area &area, float posThreshold, float angCosThreshold)
{
  dtNavMesh *navMesh = getNavMeshPtr();
  if (!navMesh || (area.id == 0))
    return;
  auto it = areas.find(area.id);
  if (it != areas.end())
  {
    if ((it->second.isCylinder == area.isCylinder) && (it->second.weight[0] == area.weight[0]) &&
        (it->second.weight[1] == area.weight[1]) && (it->second.optimize == area.optimize))
    {
      float cDistSq = area.isCylinder ? lengthSq(area.oobb.center() - it->second.oobb.center())
                                      : lengthSq(area.tm * area.oobb.center() - it->second.tm * it->second.oobb.center());
      Point3 szDiff = abs(area.oobb.width() - it->second.oobb.width());
      float maxSzDiff = max(max(szDiff.x, szDiff.y), szDiff.z);
      if ((cDistSq <= posThreshold * posThreshold) && (maxSzDiff <= posThreshold))
      {
        if (area.isCylinder)
          return;
        else
        {
          // For boxes, also check orientation.
          Point3 prevForwardDir = it->second.tm % Point3(1.0f, 0.0f, 0.0f);
          Point3 forwardDir = area.tm % Point3(1.0f, 0.0f, 0.0f);
          if (dot(prevForwardDir, forwardDir) >= angCosThreshold)
            return;
        }
      }
    }
    Tab<uint32_t> affectedTiles(framemem_ptr());
    areaRemoveFromTiles(it->second, affectedTiles);
    uint32_t generation = it->second.generation;
    it->second = area;
    it->second.generation = generation + 1;
    areaAddToTiles(it->second);
    for (uint32_t tile_id : affectedTiles)
    {
      auto jt = tiles.find(tile_id);
      if ((jt != tiles.end()) && jt->second.cylinderAreas.empty() && jt->second.boxAreas.empty())
        tiles.erase(jt);
    }
  }
  else
  {
    it = areas.insert(eastl::make_pair(area.id, area)).first;
    areaAddToTiles(it->second);
  }
}

template <typename T>
void CustomNav::walkAreaTiles(const Area &area, T cb)
{
  dtNavMesh *navMesh = getNavMeshPtr();
  if (!navMesh)
    return;
  BBox3 aabb = area.getAABB();
  int minx, miny, maxx, maxy;
  navMesh->calcTileLoc(&aabb.lim[0].x, &minx, &miny);
  navMesh->calcTileLoc(&aabb.lim[1].x, &maxx, &maxy);
  static const int MAX_NEIS = 32;
  const dtMeshTile *neis[MAX_NEIS];
  for (int y = miny; y <= maxy; ++y)
    for (int x = minx; x <= maxx; ++x)
    {
      const int nneis = navMesh->getTilesAt(x, y, neis, MAX_NEIS);
      for (int j = 0; j < nneis; ++j)
      {
        BBox3 tileAABB(Point3(&neis[j]->header->bmin[0], Point3::CTOR_FROM_PTR),
          Point3(&neis[j]->header->bmax[0], Point3::CTOR_FROM_PTR));
        if (!(aabb & tileAABB))
          continue;
        if (area.isCylinder)
        {
          Point3 sz = area.oobb.width();
          if (testCircleAABB(tileAABB, area.oobb.center(), sz.x * sz.z))
            cb(navMesh->decodePolyIdTile(navMesh->getTileRef(neis[j])));
        }
        else if (check_bbox_intersection(area.oobb, area.tm, tileAABB, TMatrix::IDENT))
          cb(navMesh->decodePolyIdTile(navMesh->getTileRef(neis[j])));
      }
    }
}

void CustomNav::areaAddToTiles(Area &area)
{
  walkAreaTiles(area, [this, &area](uint32_t tile_id) {
    Tile &tile = tiles[tile_id];
    if (area.isCylinder)
    {
      for (const Area &a : tile.cylinderAreas)
      {
        if (EASTL_UNLIKELY(a.id == area.id))
        {
          logerr("a.id == area.id  %d == %d for cylinder area in %s", a.id, area.id, __FUNCTION__);
          debug("tile count %d and gen %d", area.tileCount, area.generation);
          BBox3 bbox = area.getAABB();
          debug("customNav area bbox  %f %f %f %f %f %f", bbox.boxMin().x, bbox.boxMin().y, bbox.boxMin().z, bbox.boxMax().x,
            bbox.boxMax().y, bbox.boxMax().z);
          return;
        }
      }
      tile.cylinderAreas.push_back(area);
      ++area.tileCount;
    }
    else
    {
      for (const Area &a : tile.boxAreas)
      {
        if (EASTL_UNLIKELY(a.id == area.id))
        {
          logerr("a.id == area.id  %d == %d for box area in %s", a.id, area.id, __FUNCTION__);
          debug("tile count %d and gen %d", area.tileCount, area.generation);
          BBox3 bbox = area.getAABB();
          debug("customNav area bbox  %f %f %f %f %f %f", bbox.boxMin().x, bbox.boxMin().y, bbox.boxMin().z, bbox.boxMax().x,
            bbox.boxMax().y, bbox.boxMax().z);
          return;
        }
      }
      tile.boxAreas.push_back(area);
      ++area.tileCount;
    }
  });
}

void CustomNav::removeTile(uint32_t tile_id) { tiles.erase(tile_id); }

void CustomNav::areaRemoveFromTiles(Area &area, Tab<uint32_t> &affected_tiles)
{
  walkAreaTiles(area, [this, &area, &affected_tiles](uint32_t tile_id) {
    auto it = tiles.find(tile_id);
    if (it == tiles.end())
      return;
    if (area.isCylinder)
    {
      for (auto jt = it->second.cylinderAreas.begin(); jt != it->second.cylinderAreas.end(); ++jt)
        if (jt->id == area.id)
        {
          it->second.cylinderAreas.erase(jt);
          --area.tileCount;
          if (it->second.cylinderAreas.empty() && it->second.boxAreas.empty())
            affected_tiles.push_back(tile_id);
          break;
        }
    }
    else
    {
      for (auto jt = it->second.boxAreas.begin(); jt != it->second.boxAreas.end(); ++jt)
        if (jt->id == area.id)
        {
          it->second.boxAreas.erase(jt);
          --area.tileCount;
          if (it->second.cylinderAreas.empty() && it->second.boxAreas.empty())
            affected_tiles.push_back(tile_id);
          break;
        }
    }
  });
}

void CustomNav::areaRemove(uint32_t area_id)
{
  auto it = areas.find(area_id);
  if (it != areas.end())
  {
    Tab<uint32_t> affectedTiles(framemem_ptr());
    areaRemoveFromTiles(it->second, affectedTiles);
    for (uint32_t tile_id : affectedTiles)
      tiles.erase(tile_id);
    areas.erase(it);
  }
}

float CustomNav::getWeight(uint32_t tile_id, const Point3 &p1, const Point3 &p2, const Point3 &norm_dir, float len, float inv_len,
  bool &optimize) const
{
  auto it = tiles.find(tile_id);
  if (it == tiles.end())
  {
    // Fast path #1, if the tile does not intersect with any areas we return immediately.
    return 0.0f;
  }
  float w = 0.0f;
  for (const Area &area : it->second.boxAreas)
  {
    Point3 p1Inv = area.invTm * p1;
    Point3 p2Inv = area.invTm * p2;
    if (area.oobb & p1Inv)
    {
      // Fast path #2, if starting point is inside the box then there's no point
      // testing for intersection, we simply take weight at the final point.
      float t = saturate((p2Inv.z - area.oobb.lim[0].z) * fastinv(area.oobb.lim[1].z - area.oobb.lim[0].z));
      w += lerp(area.weight[0], area.weight[1], t);
      if (optimize && !area.optimize)
      {
        // We don't want to optimize through this area, but only when
        // the path crosses quadrants, located at oobb center. This prevents
        // the optimizer from penetrating our area, but still respects the weights.
        optimize = !lineChangesQuadrant(area.oobb.center(), p1Inv, p2Inv);
      }
      continue;
    }
    Point3 dir = p2Inv - p1Inv;
    Point3 res;
    // Slow path that is taken only on box boundaries, but it's still pretty fast.
    if (testLineAABB(area.oobb, p1Inv, dir * inv_len, dir, len, res))
    {
      float t = saturate((min(res.z, p2Inv.z) - area.oobb.lim[0].z) * fastinv(area.oobb.lim[1].z - area.oobb.lim[0].z));
      w += lerp(area.weight[0], area.weight[1], t);
      if (optimize && !area.optimize)
        optimize = !lineChangesQuadrant(area.oobb.center(), p1Inv, p2Inv);
    }
  }
  for (const Area &area : it->second.cylinderAreas)
  {
    // For cylinders there's only one path that's fast, since
    // the math is quite simple.
    Point3 c = area.oobb.center();
    float t = (c - p1) * norm_dir;
    Point3 fPt;
    if (t < 0.0f)
      fPt = p1;
    else if (t >= len)
      fPt = p2;
    else
      fPt = p1 + norm_dir * t;
    float l2 = lengthSq(Point2::xz(c) - Point2::xz(fPt));
    Point3 sz = area.oobb.width();
    float r2 = sz.x * sz.z;
    if ((l2 <= r2) && (fPt.y >= area.oobb.lim[0].y) && (fPt.y <= area.oobb.lim[1].y))
    {
      t = l2 * fastinv(r2);
      w += lerp(area.weight[0], area.weight[1], t);
      optimize &= area.optimize;
    }
  }
  return w;
}

void CustomNav::getHashes(dag::ConstSpan<uint64_t> poly_refs, PolyHashes &hashes) const
{
  hashes.clear();
  dtNavMesh *navMesh = getNavMeshPtr();
  if (!navMesh)
    return;
  hashes.clear();
  for (uint64_t polyRef : poly_refs)
    hashes[polyRef] = getHash(polyRef);
}

bool CustomNav::checkHashes(dag::ConstSpan<uint64_t> poly_refs, const PolyHashes &hashes) const
{
  dtNavMesh *navMesh = getNavMeshPtr();
  if (!navMesh)
    return true;
  for (uint64_t polyRef : poly_refs)
  {
    auto it = hashes.find(polyRef);
    if (it == hashes.end())
      continue;
    if (getHash(polyRef) != it->second)
      return false;
  }
  return true;
}

uint32_t CustomNav::getHash(uint64_t poly_ref) const
{
  dtNavMesh *navMesh = getNavMeshPtr();

  uint32_t tileId = navMesh->decodePolyIdTile(poly_ref);
  auto it = tiles.find(tileId);
  if (it == tiles.end())
    return 0;

  const dtMeshTile *tile = nullptr;
  const dtPoly *poly = nullptr;
  if (dtStatusFailed(navMesh->getTileAndPolyByRef(poly_ref, &tile, &poly)))
    return 0;

  tmpHashBuff.clear();

  BBox3 polyAABB;
  Point3 verts[DT_VERTS_PER_POLYGON];
  for (int k = 0; k < poly->vertCount; ++k)
  {
    dtVcopy(&verts[k].x, &tile->verts[poly->verts[k] * 3]);
    polyAABB += verts[k];
  }
  // Inflate a bit, we need aabb to intersect with things.
  polyAABB.lim[0].y -= 0.1f;
  polyAABB.lim[1].y += 0.1f;

  for (const Area &area : it->second.boxAreas)
  {
    if (!check_bbox_intersection(area.oobb, area.tm, polyAABB, TMatrix::IDENT))
      continue;
    if (area.oobb & (area.invTm * verts[0]))
    {
      // Fast path, at least one vertex is within the area.
      tmpHashBuff.push_back(eastl::make_pair(area.id, area.generation));
    }
    else
    {
      const Point3 areaPts[4] = {area.tm * area.oobb.lim[0],
        area.tm * Point3(area.oobb.lim[1].x, area.oobb.lim[0].y, area.oobb.lim[0].z),
        area.tm * Point3(area.oobb.lim[1].x, area.oobb.lim[0].y, area.oobb.lim[1].z),
        area.tm * Point3(area.oobb.lim[0].x, area.oobb.lim[0].y, area.oobb.lim[1].z)};
      if (dtOverlapPolyPoly2D(&areaPts[0].x, 4, &verts[0].x, poly->vertCount))
        tmpHashBuff.push_back(eastl::make_pair(area.id, area.generation));
    }
  }
  for (const Area &area : it->second.cylinderAreas)
  {
    if (!(area.oobb & polyAABB))
      continue;
    float edged[DT_VERTS_PER_POLYGON];
    float edget[DT_VERTS_PER_POLYGON];
    Point3 c = area.oobb.center();
    Point3 sz = area.oobb.width();
    float r2 = sz.x * sz.z;
    // Fast path, at least one vertex is within the radius.
    bool ok = lengthSq(Point2::xz(c) - Point2::xz(verts[0])) <= r2;
    if (!ok)
      ok = dtDistancePtPolyEdgesSqr(&c.x, &verts[0].x, poly->vertCount, edged, edget);
    if (!ok)
    {
      float dminSq = edged[0];
      for (int j = 1; j < poly->vertCount; ++j)
        dminSq = min(dminSq, edged[j]);
      ok = dminSq <= r2;
    }
    if (ok)
      tmpHashBuff.push_back(eastl::make_pair(area.id, area.generation));
  }

  if (tmpHashBuff.empty())
    return 0;

  // Sort by area_id, needed in order to get hash match even if area position was
  // changed.
  eastl::sort(tmpHashBuff.begin(), tmpHashBuff.end(), [](auto a, auto b) { return a.first < b.first; });
  G_STATIC_ASSERT(sizeof(tmpHashBuff[0]) == 8);
  return mem_hash_fnv1((const char *)&tmpHashBuff[0], tmpHashBuff.size() * sizeof(tmpHashBuff[0]));
}
} // namespace pathfinder
