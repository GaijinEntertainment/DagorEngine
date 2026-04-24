// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/componentTypes.h>
#include <daECS/net/time.h>

ECS_TAG(server)
ECS_ON_EVENT(on_appear)
static void burnt_ground__on_fire_appear_event_es(
  const ecs::Event &, ecs::EntityManager &manager, const TMatrix &transform, float burnt_ground__burn_radius)
{
  if (burnt_ground__burn_radius <= 0)
    return;

  ecs::ComponentsInitializer attrs;
  ECS_INIT(attrs, transform, transform);
  ECS_INIT(attrs, burnt_ground__radius, burnt_ground__burn_radius);
  ECS_INIT(attrs, burnt_ground__placement_time, get_sync_time());
  manager.createEntityAsync("burnt_ground", eastl::move(attrs));
}