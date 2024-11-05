// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ecs/game/generic/grid.h>
#include <generic/dag_span.h>
#include <phys/collisionTraces.h>

enum GridHideFlag : int
{
  EGH_HIDDEN_POS = (1 << 0),
  EGH_IN_VEHICLE = (1 << 1),
  EGH_GAME_EFFECT = (1 << 2),
};

struct CollisionObject;

typedef eastl::fixed_function<sizeof(intptr_t) * 4, float(ecs::EntityId /*eid*/)> get_capsule_radius_cb_t;
using GameObjectsCollisionsCb = eastl::fixed_function<6 * sizeof(void *), void(CollisionObject obj)>;

bool grid_trace_main_entities(const Point3 &from, const Point3 &dir, float &t, Point3 &out_vel, int ignore_id, int ray_mat_id);
void grid_process_main_entities_collision_objects_on_ray(
  const Point3 &from, const Point3 &to, float radius, GameObjectsCollisionsCb coll_cb);
bool trace_entities_in_grid(uint32_t grid_hash,
  const Point3 &from,
  const Point3 &dir,
  float &t,
  ecs::EntityId ignore_eid,
  IntersectedEntities &intersected_entities,
  SortIntersections do_sort);
bool trace_entities_in_grid(uint32_t grid_hash, const Point3 &from, const Point3 &dir, float &t, ecs::EntityId ignore_eid);
bool trace_entities_in_grid_by_capsule(uint32_t grid_hash,
  const Point3 &from,
  const Point3 &dir,
  float &t,
  float radius,
  ecs::EntityId ignore_eid,
  IntersectedEntities &intersected_entities,
  SortIntersections do_sort);
bool trace_entities_in_grid_by_capsule(uint32_t grid_hash,
  const Point3 &from,
  const Point3 &dir,
  float &t,
  float max_radius,
  ecs::EntityId ignore_eid,
  IntersectedEntities &intersected_entities,
  SortIntersections do_sort,
  const get_capsule_radius_cb_t &get_radius);
bool trace_entities_in_grid_by_capsule(
  uint32_t grid_hash, const Point3 &from, const Point3 &dir, float &t, float radius, ecs::EntityId ignore_eid);
bool rayhit_entities_in_grid(uint32_t grid_hash, const Point3 &from, const Point3 &dir, float t, ecs::EntityId ignore_eid);
bool query_entities_intersections_in_grid(uint32_t grid_hash,
  dag::ConstSpan<plane3f> convex,
  const TMatrix &tm,
  float rad,
  bool rayhit,
  IntersectedEntities &entities,
  SortIntersections do_sort);
