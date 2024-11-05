// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <daScript/ast/ast_typedecl.h>
#include <ecs/core/entityManager.h>
#include <ecs/scripts/dasEcsEntity.h>
#include <dasModules/aotEcs.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/dasManagedTab.h>
#include <dasModules/aotDagorMath.h>
#include <phys/collisionTraces.h>

DAS_BASE_BIND_ENUM(SortIntersections, SortIntersections, YES, NO);
DAS_BIND_ENUM_CAST(SortIntersections);

MAKE_TYPE_FACTORY(IntersectedEntityData, IntersectedEntityData);
MAKE_TYPE_FACTORY(IntersectedEntity, IntersectedEntity);

DAS_BIND_VECTOR(IntersectedEntities, IntersectedEntities, IntersectedEntity, " ::IntersectedEntities")

namespace bind_dascript
{
inline bool das_trace_to_collision_nodes(Point3 from,
  ecs::EntityId target,
  SortIntersections do_sort,
  float ray_tolerance,
  const das::TBlock<void, const das::TTemporary<const das::TArray<IntersectedEntity>>> &block,
  das::Context *context,
  das::LineInfoArg *at)
{
  IntersectedEntities entities;
  const bool res = ::trace_to_collision_nodes(from, target, entities, do_sort, ray_tolerance);

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
inline bool das_trace_to_capsule_approximation(Point3 from,
  ecs::EntityId target,
  SortIntersections do_sort,
  float ray_tolerance,
  const das::TBlock<void, const das::TTemporary<const das::TArray<IntersectedEntity>>> &block,
  das::Context *context,
  das::LineInfoArg *at)
{
  IntersectedEntities entities;
  const bool res = ::trace_to_capsule_approximation(from, target, entities, do_sort, ray_tolerance);

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
} // namespace bind_dascript
