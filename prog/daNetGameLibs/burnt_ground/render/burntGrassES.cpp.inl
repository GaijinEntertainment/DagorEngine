// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/componentTypes.h>
#include <render/daFrameGraph/nodeHandle.h>
#include <render/daFrameGraph/ecs/frameGraphNode.h>
#include <ecs/render/updateStageRender.h>
#include <render/renderEvent.h>
#include <EASTL/vector_set.h>
#include <burnt_ground/render/burntGroundEvents.h>
#include <drv/3d/dag_resetDevice.h>


#include "burntGrassRenderer.h"
#include "burntGrassNodes.h"

ECS_REGISTER_BOXED_TYPE(BurntGrassRenderer, nullptr);

template <typename Callable>
static void get_burnt_grass_renderer_ecs_query(ecs::EntityManager &manager, Callable c);

void set_up_burnt_grass_renderer(BurntGrassRenderer &burnt_grass_renderer, dafg::NodeHandle &burnt_grass_renderer__prepare_node)
{
  burnt_grass_renderer__prepare_node = create_burnt_grass_prepare_node(&burnt_grass_renderer);
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void burnt_grass_renderer_on_appear_es(
  const ecs::Event &, BurntGrassRenderer &burnt_grass_renderer, dafg::NodeHandle &burnt_grass_renderer__prepare_node)
{
  set_up_burnt_grass_renderer(burnt_grass_renderer, burnt_grass_renderer__prepare_node);
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear, OnLevelLoaded)
static void burnt_grass_renderer_set_up_biomes_es(
  const ecs::Event &, BurntGrassRenderer &burnt_grass_renderer, const ecs::StringList &burnt_grass_renderer__biomeNames)
{
  burnt_grass_renderer.setAllowedBiomeGroups(burnt_grass_renderer__biomeNames);
}

ECS_TAG(render)
ECS_ON_EVENT(FireOnGroundEvent)
static void burnt_grass__on_fire_appear_event_es(const FireOnGroundEvent &evt, BurntGrassRenderer &burnt_grass_renderer)
{
  if (evt.adjustedRadius <= 0)
    return;
  burnt_grass_renderer.onBurnGrass(evt);
}

static void burnt_grass_after_device_reset(bool /*full_reset*/)
{
  get_burnt_grass_renderer_ecs_query(*g_entity_mgr,
    [](BurntGrassRenderer &burnt_grass_renderer) { burnt_grass_renderer.afterDeviceReset(); });
}

REGISTER_D3D_AFTER_RESET_FUNC(burnt_grass_after_device_reset);
