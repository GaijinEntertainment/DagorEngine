// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include "blood_puddles/public/render/bloodPuddles.h"

template <typename Callable>
inline void dark_zones_ecs_query(Callable c);

ECS_NO_ORDER
ECS_TAG(server)
ECS_ON_EVENT(eastl::integral_constant<ecs::event_type_t, str_hash_fnv1("EventOnMovingZoneStarted")>)
static inline void blood_puddles_dark_zone_shredder_es_event_handler(const ecs::Event &)
{
  if (!get_blood_puddles_mgr())
    return;
  eastl::vector<eastl::pair<Point3, float>, framemem_allocator> zones;
  dark_zones_ecs_query([&zones](const TMatrix &transform, float sphere_zone__radius, float dark_zone_shredder__start) {
    zones.emplace_back(transform.getcol(3), sqr(sphere_zone__radius + dark_zone_shredder__start));
  });

  if (zones.size())
    get_blood_puddles_mgr()->erasePuddles([&zones](const PuddleInfo &p) {
      return !eastl::any_of(zones.begin(), zones.end(), [&p](auto zone) { return lengthSq(zone.first - p.pos) < zone.second; });
    });
}
