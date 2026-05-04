// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/renderEvent.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/ecsQuery.h>
#include <daECS/core/component.h>
#include <daECS/core/componentsMap.h>
#include <daECS/core/entityComponent.h>
#include <ecs/render/updateStageRender.h>

#include "tiledMapContext.h"

ECS_TAG(render)
ECS_REQUIRE(ecs::Tag &tiled_map)
static void hud_tiled_map_es(const RenderEventUI &evt) { tiled_map_on_render_ui(evt); }


ECS_TAG(render)
ECS_BEFORE(hud_tiled_map_fog_of_war_es)
static void hud_tiled_map_fog_of_war_update_data_es(
  const UpdateStageInfoBeforeRender &evt, const ecs::IntList &fog_of_war__data, const int fog_of_war__dataGen)
{
  tiled_map_fog_of_war_update_data(evt, fog_of_war__data, fog_of_war__dataGen);
}

// updates the fog of war texture even if there is no map entity in the scene
ECS_TAG(render)
ECS_NO_ORDER
static void hud_tiled_map_fog_of_war_es(const UpdateStageInfoBeforeRender &evt) { tiled_map_fog_of_war_before_render(evt); }

template <typename Cb>
static inline void tiled_map_fog_of_war_get_data_ecs_query(ecs::EntityManager &manager, Cb cb);
eastl::vector<uint32_t> tiled_map_fog_of_war_get_data()
{
  eastl::vector<uint32_t> data;
  tiled_map_fog_of_war_get_data_ecs_query(*g_entity_mgr, [&](const ecs::IntList &fog_of_war__data) {
    data.resize(fog_of_war__data.size());
    memcpy(data.data(), fog_of_war__data.data(), fog_of_war__data.size() * sizeof(uint32_t));
  });
  return data;
}


template <typename Cb>
static inline void tiled_map_fog_of_war_set_data_ecs_query(ecs::EntityManager &manager, Cb cb);
void tiled_map_fog_of_war_set_data(const eastl::vector<uint32_t> &data)
{
  tiled_map_fog_of_war_set_data_ecs_query(*g_entity_mgr, [&](ecs::IntList &fog_of_war__data) {
    fog_of_war__data.resize(data.size());
    memcpy(fog_of_war__data.data(), data.data(), data.size() * sizeof(uint32_t));
  });
}

ECS_TAG(render)
void tiled_map_fog_of_war_after_reset_es(const AfterDeviceReset &) { tiled_map_fog_of_war_after_reset(); }
