// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <levelSplines/splineRoads.h>
#include <generic/dag_tab.h>
#include <daECS/core/entityComponent.h>
#include <math/dag_bezier.h>

ECS_DECLARE_RELOCATABLE_TYPE(splineroads::SplineRoads);

void create_level_roads(splineroads::SplineRoads &roads);
Tab<TMatrix> get_points_on_road_route(
  const Point3 &start_route_pos, const Point3 &end_route_pos, int points_count, float points_dist, float roads_search_rad = 150.f);