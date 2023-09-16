//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotEcs.h>
#include <pathFinder/pathFinder.h>
#include <pathFinder/customNav.h>
#include <math/dag_vecMathCompatibility.h>
#include "memory/dag_framemem.h"
#include <dasModules/aotDagorMath.h>
#include <detourPathCorridor.h>
#include <dasModules/aotEcs.h>
#include <dasModules/dasManagedTab.h>
#include <pathFinder/pathFinder.h>
#include <dag/dag_vector.h>


DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(pathfinder::FindPathResult, FindPathResult);
DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(pathfinder::PolyFlag, PolyFlag);
DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(dtStraightPathFlags, StraightPathFlags);

using CornerFlags = eastl::vector<unsigned char, framemem_allocator>;
using CornerPolys = eastl::vector<uint64_t, framemem_allocator>;

DAS_BIND_VECTOR(CornerFlags, CornerFlags, unsigned char, "CornerFlags");
DAS_BIND_VECTOR(CornerPolys, CornerPolys, uint64_t, "CornerPolys");

MAKE_TYPE_FACTORY(CustomNav, pathfinder::CustomNav);
MAKE_TYPE_FACTORY(FindRequest, pathfinder::FindRequest);
MAKE_TYPE_FACTORY(CorridorInput, pathfinder::CorridorInput);
MAKE_TYPE_FACTORY(dtPathCorridor, dtPathCorridor);
MAKE_TYPE_FACTORY(NavMeshTriangle, pathfinder::NavMeshTriangle);
MAKE_TYPE_FACTORY(FindCornersResult, pathfinder::FindCornersResult);

namespace bind_dascript
{
inline void find_path_common(Tab<Point3> &path, const das::TBlock<void, const das::TTemporary<const das::TArray<Point3>>> &block,
  das::Context *context, das::LineInfoArg *at)
{
  das::Array arr;
  arr.data = (char *)path.data();
  arr.size = uint32_t(path.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}
inline pathfinder::FindPathResult find_path_req(pathfinder::FindRequest &req, float step_size, float slop,
  const das::TBlock<void, const das::TTemporary<const das::TArray<Point3>>> &block, das::Context *context, das::LineInfoArg *at)
{
  Tab<Point3> path(framemem_ptr());
  const pathfinder::FindPathResult res = pathfinder::findPath(path, req, step_size, slop, nullptr);

  find_path_common(path, block, context, at);
  return res;
}
inline pathfinder::FindPathResult find_path(const Point3 &start_pos, const Point3 &end_pos, const Point3 extents, float step_size,
  float slop, const pathfinder::CustomNav *custom_nav,
  const das::TBlock<void, const das::TTemporary<const das::TArray<Point3>>> &block, das::Context *context, das::LineInfoArg *at)
{
  Tab<Point3> path(framemem_ptr());
  const pathfinder::FindPathResult res = pathfinder::findPath(start_pos, end_pos, path, extents, step_size, slop, custom_nav);

  find_path_common(path, block, context, at);
  return res;
}
inline bool check_path_req(pathfinder::FindRequest &req, float horz_threshold, float max_vert_dist)
{
  return pathfinder::check_path(req, horz_threshold, max_vert_dist);
}
inline bool traceray_navmesh(const Point3 &start_pos, const Point3 &end_pos, const Point3 &extents, das::float3 &out_pos)
{
  return pathfinder::traceray_navmesh(start_pos, end_pos, extents, reinterpret_cast<Point3 &>(out_pos));
}
inline bool traceray_navmesh(const Point3 &start_pos, const Point3 &end_pos, const Point3 &extents, das::float3 &out_pos,
  pathfinder::CustomNav *custom_nav)
{
  return pathfinder::traceray_navmesh(start_pos, end_pos, extents, reinterpret_cast<Point3 &>(out_pos), custom_nav);
}
inline bool project_to_nearest_navmesh_point(das::float3 &pos, float horz_extents)
{
  return pathfinder::project_to_nearest_navmesh_point(reinterpret_cast<Point3 &>(pos), horz_extents);
}
inline bool project_to_nearest_navmesh_point_3d(das::float3 &pos, Point3 extents)
{
  return pathfinder::project_to_nearest_navmesh_point(reinterpret_cast<Point3 &>(pos), extents);
}
inline bool project_to_nearest_navmesh_point_no_obstacles_3d(das::float3 &pos, Point3 extents)
{
  return pathfinder::project_to_nearest_navmesh_point_no_obstacles(reinterpret_cast<Point3 &>(pos), extents);
}
inline bool project_to_nearest_navmesh_point(das::float3 &pos, float horz_extents, pathfinder::CustomNav *custom_nav)
{
  return pathfinder::project_to_nearest_navmesh_point(reinterpret_cast<Point3 &>(pos), horz_extents, custom_nav);
}
inline bool project_to_nearest_navmesh_point_ref(das::float3 &pos, Point3 extents, dtPolyRef &nearestPoly)
{
  return pathfinder::project_to_nearest_navmesh_point_ref(reinterpret_cast<Point3 &>(pos), extents, nearestPoly);
}
inline int tilecache_obstacle_add(const TMatrix &tm, const BBox3 &oobb) { return pathfinder::tilecache_obstacle_add(tm, oobb); }
inline void tilecache_obstacle_move(int handle, const TMatrix &tm, const BBox3 &oobb)
{
  pathfinder::tilecache_obstacle_move(handle, tm, oobb);
}
inline bool tilecache_obstacle_remove(int handle) { return pathfinder::tilecache_obstacle_remove(handle); }
inline bool tilecache_ri_obstacle_add(rendinst::riex_handle_t ri_handle, const BBox3 &oobb_inflate)
{
  return pathfinder::tilecache_ri_obstacle_add(ri_handle, oobb_inflate);
}
inline bool tilecache_ri_obstacle_remove(rendinst::riex_handle_t ri_handle)
{
  return pathfinder::tilecache_ri_obstacle_remove(ri_handle);
}
inline void walk_removed_tile_cache_tiles(const das::TBlock<void, const das::TTemporary<const das::TArray<uint32_t>>> &block,
  das::Context *context, das::LineInfoArg *at)
{
  const Tab<uint32_t> &removedTiles = pathfinder::get_removed_tile_cache_tiles();
  das::Array arr;
  arr.data = (char *)removedTiles.data();
  arr.size = uint32_t(removedTiles.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}
inline void walk_removed_rebuild_tile_cache_tiles(const das::TBlock<void, const das::TTemporary<const das::TArray<uint32_t>>> &block,
  das::Context *context, das::LineInfoArg *at)
{
  const Tab<uint32_t> &removedTiles = pathfinder::get_removed_rebuild_tile_cache_tiles();
  das::Array arr;
  arr.data = (char *)removedTiles.data();
  arr.size = uint32_t(removedTiles.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}
inline bool query_navmesh_projections(const Point3 &pos, das::float3 extents, int points_num,
  const das::TBlock<void, const das::TTemporary<const das::TArray<das::float3>>> &block, das::Context *context, das::LineInfoArg *at)
{
  Tab<Point3> points(framemem_ptr());
  const bool res = pathfinder::query_navmesh_projections(pos, reinterpret_cast<Point3 &>(extents), points, points_num);
  das::Array arr;
  arr.data = (char *)points.data();
  arr.size = uint32_t(points.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
  return res;
}

// Note: pos itself must be close to navmesh no matter how large the radius is. Or else it fails.
inline bool find_random_points_around_circle(const Point3 &pos, float radius, int num_points,
  const das::TBlock<void, const das::TTemporary<const das::TArray<das::float3>>> &block, das::Context *context, das::LineInfoArg *at)
{
  Tab<Point3> points(framemem_ptr());
  const bool res = pathfinder::find_random_points_around_circle(pos, radius, num_points, points);
  das::Array arr;
  arr.data = (char *)points.data();
  arr.size = uint32_t(points.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
  return res;
}

inline void das_find_corridor_corners(dtPathCorridor &corridor, int num,
  const das::TBlock<void, const das::TTemporary<const das::TArray<das::float3>>> &block, das::Context *context, das::LineInfoArg *at)
{
  Tab<Point3> corners(framemem_ptr());
  pathfinder::find_corridor_corners(corridor, corners, num);
  das::Array arr;
  arr.data = (char *)corners.data();
  arr.size = uint32_t(corners.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}
inline void das_find_corridor_corners_result(dtPathCorridor &corridor, int num,
  const das::TBlock<void, const das::TTemporary<const das::TArray<das::float3>>, das::TTemporary<pathfinder::FindCornersResult>>
    &block,
  das::Context *context, das::LineInfoArg *at)
{
  Tab<Point3> corners(framemem_ptr());
  pathfinder::FindCornersResult res = pathfinder::find_corridor_corners(corridor, corners, num);
  das::Array arr;
  arr.data = (char *)corners.data();
  arr.size = uint32_t(corners.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f args[] = {das::cast<das::Array *>::from(&arr), das::cast<pathfinder::FindCornersResult &>::from(res)};
  context->invoke(block, args, nullptr, at);
}

inline Point3 das_get_corridor_pos(const dtPathCorridor &corridor) { return Point3(corridor.getPos(), Point3::CTOR_FROM_PTR); }
inline Point3 das_get_corridor_target(const dtPathCorridor &corridor) { return Point3(corridor.getTarget(), Point3::CTOR_FROM_PTR); }
inline void corridor_get_path(const dtPathCorridor &corridor,
  const das::TBlock<void, const das::TTemporary<const das::TArray<dtPolyRef>>> &block, das::Context *context, das::LineInfoArg *at)
{
  const dtPolyRef *path = corridor.getPath();
  const int pathSize = corridor.getPathCount();
  das::Array arr;
  arr.data = (char *)path;
  arr.size = uint32_t(pathSize);
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}
inline int das_move_over_offmesh_link_in_corridor(dtPathCorridor &corridor, const Point3 &pos, const Point3 &extents,
  const pathfinder::FindCornersResult &ctx, int &over_link, Point3 &out_from, Point3 &out_to,
  const das::TBlock<void, const das::TTemporary<const das::TArray<das::float3>>> &block, das::Context *context, das::LineInfoArg *at)
{
  Tab<Point3> corners(framemem_ptr());
  const int result = pathfinder::move_over_offmesh_link_in_corridor(corridor, pos, extents, ctx, corners, over_link, out_from, out_to);
  das::Array arr;
  arr.data = (char *)corners.data();
  arr.size = uint32_t(corners.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
  return result;
}
inline bool find_polys_in_circle(const Point3 &pos, float radius, float height_half_offset,
  const das::TBlock<void, const das::TTemporary<const das::TArray<uint64_t>>> &block, das::Context *context, das::LineInfoArg *at)
{
  dag::Vector<dtPolyRef, framemem_allocator> polyRefs;
  const bool res = pathfinder::find_polys_in_circle(polyRefs, pos, radius, height_half_offset);
  das::Array arr;
  arr.data = (char *)polyRefs.data();
  arr.size = uint32_t(polyRefs.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
  return res;
}

inline bool set_corridor_agent_position(dtPathCorridor &corridor, const Point3 &pos, const ecs::List<Point2> *areas_cost)
{
  auto areasCostView = areas_cost ? make_span_const(*areas_cost) : dag::ConstSpan<Point2>();
  return pathfinder::set_corridor_agent_position(corridor, pos, &areasCostView, nullptr);
}

inline bool set_corridor_target(dtPathCorridor &corridor, const Point3 &target, const ecs::List<Point2> *areas_cost)
{
  auto areasCostView = areas_cost ? make_span_const(*areas_cost) : dag::ConstSpan<Point2>();
  return pathfinder::set_corridor_target(corridor, target, &areasCostView, nullptr);
}

inline void using_find_request(const Point3 &start, const Point3 &end, Point3 extents,
  const das::TBlock<void, das::TTemporary<pathfinder::FindRequest>> &block, das::Context *context, das::LineInfoArg *at)
{
  pathfinder::FindRequest res;
  res.start = start;
  res.end = end;
  res.includeFlags = POLYFLAGS_WALK;
  res.excludeFlags = 0;
  res.extents = extents;
  res.maxJumpUpHeight = FLT_MAX;
  res.numPolys = 0;
  res.startPoly = 0;
  res.endPoly = 0;

  vec4f arg = das::cast<pathfinder::FindRequest &>::from(res);
  context->invoke(block, &arg, nullptr, at);
}

inline void using_corridor_input(const Point3 &start, const Point3 &end, Point3 extents,
  const das::TBlock<void, das::TTemporary<pathfinder::CorridorInput>> &block, das::Context *context, das::LineInfoArg *at)
{
  pathfinder::CorridorInput res;
  res.start = start;
  res.target = end;
  res.extents = extents;
  res.includeFlags = POLYFLAGS_WALK;
  res.excludeFlags = 0;
  res.maxJumpUpHeight = FLT_MAX;
  res.startPoly = 0;
  res.targetPoly = 0;

  vec4f arg = das::cast<pathfinder::CorridorInput &>::from(res);
  context->invoke(block, &arg, nullptr, at);
}

inline void corridor_input_add_area_cost(pathfinder::CorridorInput &inp, uint8_t area, float cost)
{
  inp.areasCost.emplace_back(float(area), cost);
}

inline void find_request_add_area_cost(pathfinder::FindRequest &req, uint8_t area, float cost)
{
  req.areasCost.emplace_back(float(area), cost);
}

inline void for_each_linked_poly(const dtPolyRef polyRef, const das::TBlock<void, dtPolyRef> &block, das::Context *context,
  das::LineInfoArg *at)
{
  const dtMeshTile *tile = 0;
  const dtPoly *poly = 0;
  if (dtStatusSucceed(pathfinder::getNavMeshPtr()->getTileAndPolyByRef(polyRef, &tile, &poly)))
  {
    for (unsigned int k = poly->firstLink; k != DT_NULL_LINK; k = tile->links[k].next)
    {
      const dtLink *link = &tile->links[k];
      vec4f arg = das::cast<dtPolyRef>::from(link->ref);
      context->invoke(block, &arg, nullptr, at);
    }
  }
}


inline void for_each_nmesh_poly(const das::TBlock<void, dtPolyRef> &block, das::Context *context, das::LineInfoArg *at)
{
  if (!pathfinder::getNavMeshPtr())
    return;

  for (int tileId = 0, maxTiles = pathfinder::getNavMeshPtr()->getMaxTiles(); tileId < maxTiles; ++tileId)
  {
    const dtMeshTile *tile = const_cast<const dtNavMesh *>(pathfinder::getNavMeshPtr())->getTile(tileId);
    if (!tile->header)
      continue;

    const dtPolyRef base = pathfinder::getNavMeshPtr()->getPolyRefBase(tile);

    for (dtPolyRef polyId = 0, maxPolys = tile->header->polyCount; polyId < maxPolys; ++polyId)
    {
      vec4f arg = das::cast<dtPolyRef>::from(base | polyId);
      context->invoke(block, &arg, nullptr, at);
    }
  }
}

} // namespace bind_dascript
