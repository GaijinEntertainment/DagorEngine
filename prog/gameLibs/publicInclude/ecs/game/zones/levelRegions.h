//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <levelSplines/splineRegions.h>
#include <generic/dag_tab.h>
#include <daECS/core/entityComponent.h>

typedef Tab<splineregions::SplineRegion> LevelRegions;
ECS_DECLARE_RELOCATABLE_TYPE(LevelRegions);

void create_level_splines(dag::ConstSpan<levelsplines::Spline> splines);
void create_level_regions(dag::ConstSpan<levelsplines::Spline> splines, const DataBlock &regions_blk);
const splineregions::SplineRegion *get_region_by_pos(const Point2 &pos);
const char *get_region_name_by_pos(const Point2 &pos);
void clean_up_level_entities();
