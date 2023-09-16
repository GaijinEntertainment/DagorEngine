//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daScript/daScript.h>
#include <daECS/core/entityId.h>
#include <dasModules/aotEcs.h>
#include <ecs/scripts/dasEcsEntity.h>
#include <ecs/game/zones/zoneQuery.h>
#include <math/dag_vecMathCompatibility.h>
#include <dasModules/dasModulesCommon.h>
#include <math/dag_math2d.h>

namespace bind_dascript
{

inline vec4f get_active_capzones_on_pos(Point3 pos, const char *tag_to_have,
  const das::TBlock<void, const das::TTemporary<const das::TArray<ecs::EntityId>>> &block, das::Context *context, das::LineInfoArg *at)
{
  Tab<ecs::EntityId> zones_in(framemem_ptr());
  game::get_active_capzones_on_pos(pos, tag_to_have, zones_in);

  das::Array arr;
  arr.data = (char *)zones_in.data();
  arr.size = zones_in.size();
  arr.capacity = zones_in.size();
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
  return v_zero();
}

inline bool is_point_in_polygon_zone(const Point3 &pos, const das::TArray<das::float2> &battleAreaPoints,
  bool poly_zone__inverted = false)
{
  return poly_zone__inverted ^ is_point_in_poly(Point2::xz(pos), (const Point2 *)battleAreaPoints.data, int(battleAreaPoints.size));
}
} // namespace bind_dascript
