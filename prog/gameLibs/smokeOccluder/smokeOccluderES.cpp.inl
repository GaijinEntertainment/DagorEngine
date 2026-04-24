// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <smokeOccluder/smokeOccluder.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <math/dag_mathUtils.h>
#include <math/dag_Point3.h>
#include <daECS/core/ecsQuery.h>

template <typename Callable>
static inline ecs::QueryCbResult smoke_occluders_ecs_query(ecs::EntityManager &manager, Callable c);

bool rayhit_smoke_occluders(const Point3 &from, const Point3 &to)
{
  return smoke_occluders_ecs_query(*g_entity_mgr, [&](const Point4 &smoke_occluder__sphere) {
    float sphereRadiusSq = sqr(smoke_occluder__sphere.w);
    if (test_segment_sphere_intersection(from, to, Point3::xyz(smoke_occluder__sphere), sphereRadiusSq))
      return ecs::QueryCbResult::Stop;
    return ecs::QueryCbResult::Continue;
  }) == ecs::QueryCbResult::Stop;
}
