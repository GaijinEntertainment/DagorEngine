// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/sharedComponent.h>
#include <daECS/core/entityManager.h>
#include <rendInst/rendInstGen.h>

template <typename Callable>
static void respawn_camera_target_point_ecs_query(ecs::EntityId eid, Callable c);

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void respawn_cam_created_es(const ecs::Event &, ecs::EntityId respawnCameraTargerPoint)
{
  respawn_camera_target_point_ecs_query(respawnCameraTargerPoint,
    [](float respawnCameraLODMultiplier) { rendinst::setAdditionalRiExtraLodDistMul(respawnCameraLODMultiplier); });
}

ECS_TAG(render)
ECS_REQUIRE(ecs::EntityId respawnCameraTargerPoint)
ECS_ON_EVENT(on_disappear)
static void respawn_cam_destroyed_es(const ecs::Event &) { rendinst::setAdditionalRiExtraLodDistMul(1); }
