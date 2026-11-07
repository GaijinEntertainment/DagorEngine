// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/coreEvents.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <ecs/render/updateStageRender.h>
#include <render/cameraIndoor/cameraIndoor_api.h>

#include "main/gameObjects.h"

ECS_TAG(render)
ECS_NO_ORDER
static void camera_indoor_es(const UpdateStageInfoBeforeRender &stg,
  ecs::EntityManager &manager,
  Point3 &camera_indoor__pos,
  float &camera_indoor__target,
  float &camera_indoor__value)
{
  const GameObjects *gameObjects =
    manager.getNullable<GameObjects>(manager.getSingletonEntity(ECS_HASH("game_objects")), ECS_HASH("game_objects"));
  if (gameObjects && gameObjects->indoors)
    update_camera_indoor(*gameObjects->indoors, stg.camPos, stg.realDt, camera_indoor__pos, camera_indoor__target,
      camera_indoor__value);
}

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(float camera_indoor__value)
static void camera_indoor_level_unload_es(const ecs::Event &) { reset_camera_indoor(); }
