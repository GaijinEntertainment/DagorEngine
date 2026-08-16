// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER 1

#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>

#include <render/renderEvent.h>
#include <render/world/dafgCameraRegistrator.h>
#include <render/world/private_worldRenderer.h>


void recreate_camera_registrator_nodes(const ecs::string &dafg_camera_registrator__name)
{
  if (auto *wr = get_world_renderer())
    static_cast<WorldRenderer *>(wr)->reCreateCameraViewNodes(dafg_camera_registrator__name.c_str());
}

void destroy_camera_registrator(const ecs::EntityId eid)
{
  const ecs::string *registratorName = ECS_GET_COMPONENT(ecs::string, eid, dafg_camera_registrator__name);
  auto *wr = get_world_renderer();

  if (registratorName && wr)
  {
    static_cast<WorldRenderer *>(wr)->unregisterCameraViewNodes(registratorName->c_str(), eid);
    g_entity_mgr->destroyEntity(eid);
  }
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear, OnWorldRendererCreated)
static void dafg_camera_registrator_appear_es(const ecs::Event &, ecs::EntityId eid, const ecs::string &dafg_camera_registrator__name)
{
  if (auto *wr = get_world_renderer())
    static_cast<WorldRenderer *>(wr)->registerCameraViewNodes(dafg_camera_registrator__name.c_str(), eid);
}

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
static void dafg_camera_registrator_disappear_es(
  const ecs::Event &, ecs::EntityId eid, const ecs::string &dafg_camera_registrator__name)
{
  if (auto *wr = get_world_renderer())
    static_cast<WorldRenderer *>(wr)->unregisterCameraViewNodes(dafg_camera_registrator__name.c_str(), eid);
}
