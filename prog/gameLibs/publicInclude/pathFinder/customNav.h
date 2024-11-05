//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>
#include <math/dag_math3d.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <EASTL/array.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/vector_map.h>

namespace pathfinder
{
class CustomNav
{
public:
  using PolyHashes = eastl::vector_map<uint64_t, uint32_t>;

  // Area ids are allocated globally, but one can use the same id with multiple different agents.
  // This is very useful since one can allocate a single id and setup areas for mulitple agents and
  // remove areas for multiple agents, i.e. area id describes an area that can be seen differently by different agents.
  // 0 is invalid area id and does not create / remove any areas.
  static uint32_t allocAreaId();

  // areaUpdateXXX / areaRemove - are all (almost) O(1) complexity and they're really fast, they also handle
  // small differences in areas and do not update areas when not needed. You can call these as often
  // as (almost) realtime.

  // Setup area as oriented box, w1 is weight at -z of the box, w2 - at +z.
  // optimize == true means that this area allows path optimization through it, this is not
  // always desired since path optimization can take shortcuts through thin areas with high cost, if you really want
  // to avoid this - pass false.
  void areaUpdateBox(uint32_t area_id, const TMatrix &tm, const BBox3 &oobb, float w1, float w2, bool optimize = true,
    float posThreshold = 0.25f, float angCosThreshold = 0.98f);

  // Setup area as y-oriented cylinder, w1 is weight at the center of cylinder, w2 - on the sides.
  void areaUpdateCylinder(uint32_t area_id, const BBox3 &aabb, float w1, float w2, bool optimize = true, float posThreshold = 0.25f,
    float angCosThreshold = 0.98f);

  // Remove area.
  void areaRemove(uint32_t area_id);

  void drawDebug() const;

  // Used internally by the navmesh to get weight from a p1 -> p2 path. Optimized to be fast.
  float getWeight(uint32_t tile_id, const Point3 &p1, const Point3 &p2, const Point3 &norm_dir, float len, float inv_len,
    bool &optimize) const;

  void getHashes(dag::ConstSpan<uint64_t> poly_refs, PolyHashes &hashes) const;

  // Check if path, described by polygons, has changed since it was originally created, typically
  // used by the navigation code.
  bool checkHashes(dag::ConstSpan<uint64_t> poly_refs, const PolyHashes &hashes) const;

  void removeTile(uint32_t tile_id);

private:
  struct Area
  {
    inline BBox3 getAABB() const { return isCylinder ? oobb : tm * oobb; }

    uint32_t id = 0;
    bool isCylinder = false;
    BBox3 oobb;
    TMatrix tm;    // for box areas only.
    TMatrix invTm; // for box areas only.
    eastl::array<float, 2> weight;
    bool optimize = true;
    uint32_t generation = 0;
    int tileCount = 0;
  };

  struct Tile
  {
    eastl::fixed_vector<Area, 16, true> boxAreas;
    eastl::fixed_vector<Area, 16, true> cylinderAreas;
  };

  template <typename T>
  void walkAreaTiles(const Area &area, T cb);

  void areaUpdate(const Area &area, float posThreshold, float angCosThreshold);

  void areaAddToTiles(Area &area);

  void areaRemoveFromTiles(Area &area, Tab<uint32_t> &affected_tiles);

  uint32_t getHash(uint64_t poly_ref) const;

  ska::flat_hash_map<uint32_t, Area> areas;
  ska::flat_hash_map<uint32_t, Tile> tiles;
  mutable Tab<eastl::pair<uint32_t, uint32_t>> tmpHashBuff;
};
}; // namespace pathfinder
