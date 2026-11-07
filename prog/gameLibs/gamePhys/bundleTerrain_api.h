// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_bounds3.h>
#include <util/dag_compilerDefs.h>
#include <heightMapLand/dag_hmlTraceRay.h>
#include <float.h>

namespace dacoll
{

// The ground is solid from the surface down to depth (above zero) metres under it; FLT_MAX is solid to any depth.
// height_at(Point2, float &) answers false where the field has no surface: off the map, or inside a hole the landmesh cut.
template <typename HeightAt>
inline bool terrain_point_inside(const Point3 &p, HeightAt height_at, float depth = FLT_MAX)
{
  float ht = 0.f;
  return height_at(Point2::xz(p), ht) && p.y < ht && p.y > ht - depth;
}

// The ground half of a segment, into sink(t, facing, pos_xz). A start inside the ground opens with a synthetic enter at a negative t,
// since it crossed no surface to get there and no crossing can report it. A crossing inside a hole is dropped:
// a hole's wall is no surface, so a segment that rises out of one reports that leave alone. True: the start is inside.
template <class HM, typename HeightAt, typename HoleAt, typename Sink>
inline bool trace_terrain_span(const HM &hmap, const Point3 &from, const Point3 &dir, float len, HeightAt height_at, HoleAt hole_at,
  Sink sink, float depth = FLT_MAX)
{
  const bool inside = terrain_point_inside(from, height_at, depth);
  if (inside)
    sink(-1.f, /*enter*/ 1, Point2::xz(from));
  const int crossings = trace_all_crossings_heightmap(hmap, from, dir, len, [&](float at, int8_t facing) DAGOR_FORCE_INLINE_LAMBDA {
    const Point2 pos = Point2::xz(from + dir * at);
    if (!hole_at(pos))
      sink(at, facing, pos);
  });
  if (depth >= FLT_MAX)
    return inside;
  float ht = 0.f;
  const Point3 to = from + dir * len;
  // with no crossing, an end on the map tells the side
  if (!crossings && !inside &&
      (hmap.getHeight(Point2::xz(from), ht, nullptr) ? from.y >= ht : hmap.getHeight(Point2::xz(to), ht, nullptr) && to.y >= ht))
    return inside;
  // the raised walk crosses the bottom, facing flipped
  trace_all_crossings_heightmap(hmap, from + Point3(0.f, depth, 0.f), dir, len,
    [&](float at, int8_t facing) DAGOR_FORCE_INLINE_LAMBDA {
      const Point2 pos = Point2::xz(from + dir * at);
      if (!hole_at(pos))
        sink(at, int8_t(-facing), pos);
    });
  return inside;
}

// True: the bottom stands over all of box, by the field's own min and max there; out_bottom_top is its highest.
template <class HM>
inline bool terrain_bottom_over(const HM &hmap, const BBox3 &box, float depth, float &out_bottom_top)
{
  float hmin = 0.f, hmax = 0.f;
  // off the map there is no layer
  if (depth >= FLT_MAX || !hmap.getHeight(Point2::xz(box[0]), hmin, nullptr) || !hmap.getHeight(Point2::xz(box[1]), hmin, nullptr) ||
      !hmap.getHeightmapHeightMinMaxInChunk(Point2::xz(box[0]), max(box.width().x, box.width().z), hmin, hmax))
    return false;
  out_bottom_top = hmax - depth;
  return hmin - depth >= box[1].y;
}

} // namespace dacoll
