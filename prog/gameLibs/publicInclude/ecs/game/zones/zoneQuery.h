//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/component.h>
#include <daECS/core/componentsMap.h>
#include <generic/dag_tab.h>
#include <math/dag_Point3.h>
#include <math/dag_bounds3.h>
#include <EASTL/fixed_function.h>

namespace game
{
extern const BBox3 IDENTITY_BBOX3;
void get_active_capzones_on_pos(const Point3 &pos, const char *tag_to_have_not_null, Tab<ecs::EntityId> &zones_in);
bool is_point_in_capzone(const Point3 &pos, ecs::EntityId zone_eid, float zone_scale = 1.f);
bool is_entity_in_capzone(const ecs::EntityId eid, const ecs::EntityId zone_eid);
bool is_point_in_poly_battle_area(const Point3 &pos, ecs::EntityId zone_eid);
bool is_inside_truncated_sphere_zone(const Point3 &pos, const TMatrix &zone_tm, float zone_radius, float *zone_truncate_below,
  float scale = 1.f);
}; // namespace game
