// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <daScript/ast/ast_typedecl.h>
#include <ecs/core/entityManager.h>
#include <ecs/scripts/dasEcsEntity.h>
#include <ecs/game/generic/grid.h>
#include <dasModules/aotEcs.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/dasManagedTab.h>
#include <dasModules/aotDagorMath.h>
#include <dasModules/collisionTraces.h>
#include "phys/gridCollision.h"

DAS_BIND_ENUM_CAST(GridHideFlag);

namespace bind_dascript
{
inline bool das_trace_entities_in_grid(uint32_t grid_name_hash,
  Point3 from,
  Point3 dir,
  float &t,
  ecs::EntityId ignore_eid,
  SortIntersections do_sort,
  const das::TBlock<void, const das::TTemporary<const das::TArray<IntersectedEntity>>> &block,
  das::Context *context,
  das::LineInfoArg *at)
{
  IntersectedEntities entities;
  const bool res = ::trace_entities_in_grid(grid_name_hash, from, dir, t, ignore_eid, entities, do_sort);

  das::Array arr;
  arr.data = (char *)entities.data();
  arr.size = uint32_t(entities.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
  return res;
}

inline bool das_query_entities_intersections_in_grid(uint32_t grid_name_hash,
  const das::TArray<das::float4> &convex,
  const TMatrix &tm,
  float rad,
  bool rayhit,
  SortIntersections do_sort,
  const das::TBlock<void, const das::TTemporary<const das::TArray<IntersectedEntity>>> &block,
  das::Context *context,
  das::LineInfoArg *at)
{
  dag::ConstSpan<plane3f> planes =
    convex.size > 0 ? dag::ConstSpan<plane3f>((plane3f *)convex.data, convex.size) : dag::ConstSpan<plane3f>();

  IntersectedEntities entities;
  const bool res = ::query_entities_intersections_in_grid(grid_name_hash, planes, tm, rad, rayhit, entities, do_sort);

  das::Array arr;
  arr.data = (char *)entities.data();
  arr.size = uint32_t(entities.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
  return res;
}

inline bool das_trace_entities_in_grid_by_capsule(uint32_t grid_hash,
  const Point3 &from,
  const Point3 &dir,
  float &t,
  float max_radius,
  ecs::EntityId ignore_eid,
  IntersectedEntities &intersected_entities,
  SortIntersections do_sort,
  const das::TBlock<float, ecs::EntityId> &block,
  das::Context *context,
  das::LineInfoArg *at)
{
  return trace_entities_in_grid_by_capsule(grid_hash, from, dir, t, max_radius, ignore_eid, intersected_entities, do_sort,
    [&](ecs::EntityId eid) {
      vec4f arg = das::cast<ecs::EntityId>::from(eid);
      return das::cast<float>::to(context->invoke(block, &arg, nullptr, at));
    });
}
} // namespace bind_dascript
