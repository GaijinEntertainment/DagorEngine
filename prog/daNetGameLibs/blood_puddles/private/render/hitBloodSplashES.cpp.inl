// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "blood_puddles/public/render/bloodPuddles.h"

template <typename Callable>
inline void dark_zones_ecs_query(ecs::EntityManager &manager, Callable c);

ECS_NO_ORDER
ECS_TAG(server)
ECS_ON_EVENT(eastl::integral_constant<ecs::event_type_t, ecs_str_hash("EventOnMovingZoneStarted")>)
static inline void blood_puddles_dark_zone_shredder_es_event_handler(const ecs::Event &, ecs::EntityManager &manager)
{
  if (!get_blood_puddles_mgr())
    return;
  eastl::vector<eastl::pair<Point3, float>, framemem_allocator> zones;
  dark_zones_ecs_query(manager, [&zones](const TMatrix &transform, float sphere_zone__radius, float dark_zone_shredder__start) {
    zones.emplace_back(transform.getcol(3), sqr(sphere_zone__radius + dark_zone_shredder__start));
  });

  if (zones.size())
    get_blood_puddles_mgr()->erasePuddles([&zones](const PuddleInfo &p) {
      return !eastl::any_of(zones.begin(), zones.end(), [&p](auto zone) { return lengthSq(zone.first - p.pos) < zone.second; });
    });
}
