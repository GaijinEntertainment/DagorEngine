// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>
#include <daECS/core/coreEvents.h>
#include "main/gameObjects.h"
#include "main/level.h"

#define INSIDE_RENDERER 1
#include "render/world/private_worldRenderer.h"

static void gi_objects_es_event_handler(const EventGameObjectsCreated &, GameObjects &game_objects)
{
  WorldRenderer *renderer = (WorldRenderer *)get_world_renderer();
  if (!renderer)
    return;
  // game_objects.moveScene("indoor_walls", [&](GOScenePtr &&s) {renderer->initGIWalls(eastl::move(s));});
  game_objects.moveScene("wall_hole", [&](GOScenePtr &&s) { renderer->initGIWindows(eastl::move(s)); });
}

ECS_REQUIRE(GameObjects &game_objects)
ECS_ON_EVENT(on_disappear)
static void gi_objects_es_event_handler(const ecs::Event &)
{
  WorldRenderer *renderer = (WorldRenderer *)get_world_renderer();
  if (!renderer)
    return;
  // renderer->initGIWalls(eastl::unique_ptr<class scene::TiledScene>());
  renderer->initGIWindows(eastl::unique_ptr<class scene::TiledScene>());
}

ECS_TAG(render)
ECS_REQUIRE(ecs::Tag gi_ignore_static_shadows)
ECS_ON_EVENT(on_appear)
static void gi_ignore_static_shadows_on_appear_es(const ecs::Event)
{
  if (auto wr = (WorldRenderer *)get_world_renderer())
    wr->setIgnoreStaticShadowsForGI(true);
}
ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(ecs::Tag gi_ignore_static_shadows)
static void gi_ignore_static_shadows_on_disappear_es(const ecs::Event)
{
  if (auto wr = (WorldRenderer *)get_world_renderer())
    wr->setIgnoreStaticShadowsForGI(false);
}