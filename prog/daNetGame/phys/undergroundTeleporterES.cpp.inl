// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>
#include <gamePhys/collision/collisionLib.h>
#include "phys/physEvents.h"
#include "phys/undergroundTeleporter.h"
#include <daECS/core/coreEvents.h>

template <typename Callable>
inline void lanmesh_holes_search_ecs_query(Callable c);

bool is_allow_underground(const Point3 &pos)
{
  BBox3 identity_bbox(Point3::ZERO, 1.f);

  bool res = false;
  lanmesh_holes_search_ecs_query([&](ECS_REQUIRE(ecs::Tag underground_zone) const TMatrix &transform) {
    res = (identity_bbox & (inverse(transform) * pos));
    return res ? ecs::QueryCbResult::Stop : ecs::QueryCbResult::Continue;
  });
  return res;
}
