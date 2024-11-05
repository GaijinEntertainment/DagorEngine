// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <daScript/ast/ast_typedecl.h>
#include <ecs/core/entityManager.h>
#include <ecs/scripts/dasEcsEntity.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/dasManagedTab.h>
#include <dasModules/aotDagorMath.h>
#include <dasModules/aotEcs.h>
#include <ecs/game/zones/levelRegions.h>
#include "main/levelRoads.h"
#include "main/level.h"

typedef Tab<Point3> SplineRegionBorder;
DAS_BIND_VECTOR(SplineRegionBorder, SplineRegionBorder, Point3, "SplineRegionBorder");
MAKE_TYPE_FACTORY(SplineRegion, splineregions::SplineRegion);
DAS_BIND_VECTOR(LevelRegions, LevelRegions, splineregions::SplineRegion, " ::LevelRegions");

extern Point3 calculate_server_sun_dir();

namespace bind_dascript
{
inline bool das_spline_region_checkPoint(const splineregions::SplineRegion &spline_region, Point2 pt)
{
  return spline_region.checkPoint(pt);
}

inline void das_get_points_on_road_splines(const Point3 &start_path_pos,
  const Point3 &end_path_pos,
  int points_count,
  float points_distance,
  float roads_search_rad,
  const das::TBlock<void, const das::TTemporary<const das::TArray<TMatrix>>> &block,
  das::Context *context,
  das::LineInfoArg *at)
{
  Tab<TMatrix> path = get_points_on_road_route(start_path_pos, end_path_pos, points_count, points_distance, roads_search_rad);
  das::Array arr;
  arr.data = (char *)path.data();
  arr.size = uint32_t(path.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

inline void das_add_region_to_list(LevelRegions &level_regions, const char *name, const ecs::Point3List &points_list)
{
  for (auto it = level_regions.begin(); it != level_regions.end(); ++it)
    if (strcmp(it->getNameStr(), name) == 0)
    {
      logerr("[LevelRegions] Region with name <%s> already exist in level_regions.", name);
      return;
    }

  splineregions::SplineRegion newRegion;
  levelsplines::Spline spline;
  spline.name = name;
  spline.pathPoints.reserve(points_list.size());
  for (uint32_t i = 0; i < points_list.size(); ++i)
    spline.pathPoints.push_back({points_list[i], points_list[i], points_list[i]});
  construct_region_from_spline(newRegion, spline, 0);
  newRegion.isVisible = false;
  level_regions.push_back(newRegion);
}

inline void das_remove_region_from_list(LevelRegions &level_regions, const char *name)
{
  for (auto it = level_regions.begin(); it != level_regions.end(); ++it)
    if (strcmp(it->getNameStr(), name) == 0)
    {
      level_regions.erase(it);
      return;
    }
}
} // namespace bind_dascript
