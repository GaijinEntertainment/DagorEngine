// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/coreEvents.h>
#include <daECS/core/component.h>
#include <daECS/core/componentsMap.h>
#include <daECS/core/entityComponent.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "main/gameObjects.h"
#include "main/level.h"

#define INSIDE_RENDERER 1
#include "render/world/private_worldRenderer.h"

static void restriction_box_objects_es_event_handler(const EventGameObjectsCreated &, GameObjects &game_objects)
{
  WorldRenderer *renderer = (WorldRenderer *)get_world_renderer();
  if (renderer)
    game_objects.moveScene("omni_light_restriction_box", [&](GOScenePtr &&s) { renderer->initRestrictionBoxes(eastl::move(s)); });
}

ECS_REQUIRE(GameObjects &game_objects)
ECS_ON_EVENT(on_disappear)
static void restriction_box_objects_es_event_handler(const ecs::Event &)
{
  WorldRenderer *renderer = (WorldRenderer *)get_world_renderer();
  if (!renderer)
    return;
  renderer->initRestrictionBoxes(eastl::unique_ptr<class scene::TiledScene>());
}
