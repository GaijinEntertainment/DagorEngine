// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <util/dag_compilerDefs.h>
#include <heightMapLand/dag_hmlTraceRay.h>

namespace dacoll
{

// height_at(Point2, float &) answers false where the field has no surface: off the map, or inside a hole the landmesh cut.
template <typename HeightAt>
inline bool terrain_point_inside(const Point3 &p, HeightAt height_at)
{
  float ht = 0.f;
  return height_at(Point2::xz(p), ht) && p.y < ht;
}

// The ground half of a segment, into sink(t, facing, pos_xz). A start under the ground opens with a synthetic enter at a
// negative t, since it crossed no surface to get there and no crossing can report it. A crossing inside a hole is dropped:
// a hole's wall is no surface, so a segment that rises out of one reports that leave alone. True: the start is underground.
template <class HM, typename HeightAt, typename HoleAt, typename Sink>
inline bool trace_terrain_span(const HM &hmap, const Point3 &from, const Point3 &dir, float len, HeightAt height_at, HoleAt hole_at,
  Sink sink)
{
  const bool inside = terrain_point_inside(from, height_at);
  if (inside)
    sink(-1.f, /*enter*/ 1, Point2::xz(from));
  trace_all_crossings_heightmap(hmap, from, dir, len, [&](float at, int8_t facing) DAGOR_FORCE_INLINE_LAMBDA {
    const Point2 pos = Point2::xz(from + dir * at);
    if (!hole_at(pos))
      sink(at, facing, pos);
  });
  return inside;
}

} // namespace dacoll
