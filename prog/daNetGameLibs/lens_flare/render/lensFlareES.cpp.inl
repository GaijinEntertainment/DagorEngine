// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER 1

#include <render/lensFlare/render/lensFlareRenderer.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/componentTypes.h>
#include <render/daFrameGraph/nodeHandle.h>
#include <render/daFrameGraph/ecs/frameGraphNode.h>
#include <ecs/render/updateStageRender.h>
#include <render/renderEvent.h>
#include <EASTL/vector_set.h>
#include <generic/dag_enumerate.h>
#include <render/renderSettings.h>

#include "lensFlareNodes.h"

template <typename Callable>
static void lens_glow_enabled_ecs_query(ecs::EntityId eid, Callable c);
static bool query_lens_flare_enabled()
{
  bool enabled = false;
  ecs::EntityId settingEid = g_entity_mgr->getSingletonEntity(ECS_HASH("render_settings"));
  lens_glow_enabled_ecs_query(settingEid, [&enabled](bool render_settings__lensFlares, bool render_settings__bare_minimum) {
    enabled = render_settings__lensFlares && !render_settings__bare_minimum;
  });
  return enabled;
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void lens_flare_renderer_on_appear_es(const ecs::Event &,
  LensFlareRenderer &lens_flare_renderer,
  dafg::NodeHandle &lens_flare_renderer__render_node,
  dafg::NodeHandle &lens_flare_renderer__prepare_lights_node)
{
  lens_flare_renderer.init();
  bool enabled = query_lens_flare_enabled();
  set_up_lens_flare_renderer(enabled, lens_flare_renderer, lens_flare_renderer__render_node, create_lens_flare_render_node,
    lens_flare_renderer__prepare_lights_node, create_lens_flare_prepare_lights_node);
}

template <typename Callable>
static void init_lens_flare_nodes_ecs_query(Callable);

ECS_TAG(render)
ECS_ON_EVENT(OnRenderSettingsReady)
ECS_TRACK(render_settings__lensFlares, render_settings__bare_minimum)
static void lens_flare_renderer_on_settings_changed_es(
  const ecs::Event &, bool render_settings__lensFlares, bool render_settings__bare_minimum)
{
  bool enabled = render_settings__lensFlares && !render_settings__bare_minimum;
  init_lens_flare_nodes_ecs_query([enabled](LensFlareRenderer &lens_flare_renderer, dafg::NodeHandle &lens_flare_renderer__render_node,
                                    dafg::NodeHandle &lens_flare_renderer__prepare_lights_node) {
    set_up_lens_flare_renderer(enabled, lens_flare_renderer, lens_flare_renderer__render_node, create_lens_flare_render_node,
      lens_flare_renderer__prepare_lights_node, create_lens_flare_prepare_lights_node);
  });
}

ECS_TAG(render)
ECS_REQUIRE(const LensFlareRenderer &lens_flare_renderer)
static void register_lens_flare_for_postfx_es(const RegisterPostfxResources &evt)
{
  (evt.get<0>().root() / "lens_flare")
    .readTexture("lens_flares")
    .atStage(dafg::Stage::PS)
    .bindToShaderVar("lens_flare_tex")
    .optional();
  (evt.get<0>().root() / "lens_flare").readBlob<int>("has_lens_flares").optional().bindToShaderVar("lens_flares_enabled");
}
