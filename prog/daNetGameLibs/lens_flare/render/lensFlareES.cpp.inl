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
static void lens_glow_enabled_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable c);

ECS_TAG(render)
ECS_ON_EVENT(on_appear, OnRenderSettingsUpdated, ChangeRenderFeatures)
static void lens_flare_renderer_on_reinit_es(const ecs::Event &evt,
  ecs::EntityManager &manager,
  LensFlareRenderer &lens_flare_renderer,
  dafg::NodeHandle &lens_flare_renderer__per_camera_res_node,
  dafg::NodeHandle &lens_flare_renderer__render_node,
  dafg::NodeHandle &lens_flare_renderer__prepare_lights_node,
  dafg::NodeHandle &lens_flare_renderer__camcam_lens_prepare_lights_node,
  dafg::NodeHandle &lens_flare_renderer__camcam_lens_render_node)
{
  if (evt.cast<ecs::EventEntityCreated>())
    lens_flare_renderer.init();

  const ecs::EntityId settingEid = get_render_settings(evt);
  bool renderOptionalFlareProviders = true;

  lens_glow_enabled_ecs_query(manager, settingEid,
    [&renderOptionalFlareProviders](bool render_settings__lensFlares, bool render_settings__bare_minimum) {
      renderOptionalFlareProviders = render_settings__lensFlares && !render_settings__bare_minimum;
    });

  lens_flare_renderer.toggleOptionalProvidersProcessing(renderOptionalFlareProviders);

  const LensFlareQualityParameters quality = {0.5f};
  lens_flare_renderer__per_camera_res_node = create_lens_flare_per_camera_res_node(quality);
  lens_flare_renderer__prepare_lights_node = create_lens_flare_prepare_lights_node(&lens_flare_renderer, 0);
  lens_flare_renderer__render_node = create_lens_flare_render_node(&lens_flare_renderer, quality, 0);

  if (renderer_has_feature(CAMERA_IN_CAMERA))
  {
    lens_flare_renderer__camcam_lens_prepare_lights_node = create_lens_flare_prepare_lights_node(&lens_flare_renderer, 1);
    lens_flare_renderer__camcam_lens_render_node = create_lens_flare_render_node(&lens_flare_renderer, quality, 1);
  }
  else
  {
    lens_flare_renderer__camcam_lens_prepare_lights_node = {};
    lens_flare_renderer__camcam_lens_render_node = {};
  }
}

template <typename Callable>
static void init_lens_flare_nodes_ecs_query(ecs::EntityManager &manager, Callable);

static inline void use_lens_flare_fg_resources(dafg::Registry registry)
{
  (registry.root() / "lens_flare").readTexture("lens_flares").atStage(dafg::Stage::PS).bindToShaderVar("lens_flare_tex").optional();
  (registry.root() / "lens_flare").readBlob<int>("has_lens_flares").optional().bindToShaderVar("lens_flares_enabled");
}

ECS_TAG(render)
ECS_REQUIRE_NOT(ecs::Tag renderInDistortion)
ECS_REQUIRE(const LensFlareRenderer &lens_flare_renderer)
static void register_lens_flare_for_postfx_es(const RegisterPostfxResources &evt) { use_lens_flare_fg_resources(evt.get<0>()); }

ECS_TAG(render)
ECS_REQUIRE(ecs::Tag renderInDistortion)
ECS_REQUIRE(const LensFlareRenderer &lens_flare_renderer)
static void register_lens_flare_for_distortion_es(const RegisterDistortionModifiersResources &evt)
{
  use_lens_flare_fg_resources(evt.get<0>());
}
