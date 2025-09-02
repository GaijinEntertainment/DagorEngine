// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/componentTypes.h>

#include <render/world/wrDispatcher.h>
#include <render/world/shadowsManager.h>
#include <render/renderEvent.h>
#include <render/renderSettings.h>
#include <render/resolution.h>
#include "ssss.h"
#include <ecs/rendInst/riExtra.h>
#include <math/dag_math3d.h>
#include <render/world/bvh.h>


ECS_ON_EVENT(OnRenderSettingsReady, ChangeRenderFeatures)
ECS_TRACK(
  render_settings__ssssQuality, render_settings__waterQuality, render_settings__forwardRendering, render_settings__combinedShadows)
ECS_AFTER(ssss_settings_tracking_es)
ECS_REQUIRE(
  const ecs::string &render_settings__waterQuality, bool render_settings__forwardRendering, bool render_settings__combinedShadows)
static void shadows_settings_tracking_es(const ecs::Event &evt, const ecs::string &render_settings__ssssQuality)
{
  if (!WRDispatcher::isReadyToUse())
    return;

  if (auto *changedFeatures = evt.cast<ChangeRenderFeatures>())
  {
    if (!changedFeatures->isFeatureChanged(CAMERA_IN_CAMERA))
      return;
  }

  bool needSsss = isSsssTransmittanceEnabled(render_settings__ssssQuality);
  WRDispatcher::getShadowsManager().initCombineShadowsNode();
  WRDispatcher::getShadowsManager().setNeedSsss(needSsss);
}

template <typename Callable>
static void use_rgba_fmt_ecs_query(Callable c);
bool combined_shadows_use_additional_textures()
{
  bool useAdditionalTextures = false;
  use_rgba_fmt_ecs_query([&useAdditionalTextures](bool combined_shadows__use_additional_textures) {
    useAdditionalTextures = useAdditionalTextures || combined_shadows__use_additional_textures;
  });
  return useAdditionalTextures;
}

template <typename Callable>
static void bind_additional_textures_ecs_query(Callable c);
void combined_shadows_bind_additional_textures(dafg::Registry &registry)
{
  bind_additional_textures_ecs_query(
    [&registry](bool combined_shadows__use_additional_textures, const ecs::StringList &combined_shadows__additional_textures,
      const ecs::StringList &combined_shadows__additional_samplers) {
      if (!combined_shadows__use_additional_textures)
        return;

      for (const ecs::string texName : combined_shadows__additional_textures)
        registry.readTexture(texName.c_str()).atStage(dafg::Stage::PS).bindToShaderVar(texName.c_str());
      for (const ecs::string samplerName : combined_shadows__additional_samplers)
        registry.read(samplerName.c_str()).blob<d3d::SamplerHandle>().bindToShaderVar(samplerName.c_str());
    });
}

ECS_ON_EVENT(on_appear, EventRendinstsLoaded)
ECS_AFTER(rendinst_move_es)
ECS_AFTER(rendinst_with_handle_move_es)
static void update_world_bbox_es(
  const ecs::Event &, const TMatrix &transform, const Point3 &ri_extra__bboxMin, const Point3 ri_extra__bboxMax)
{
  BBox3 worldSpaceBBox = transform * BBox3(ri_extra__bboxMin, ri_extra__bboxMax);
  WRDispatcher::updateWorldBBox(worldSpaceBBox);
  WRDispatcher::getShadowsManager().markWorldBBoxDirty();
}

ECS_TAG(render)
ECS_ON_EVENT(OnRenderSettingsReady)
ECS_TRACK(render_settings__shadowsQuality, render_settings__enableRTSM)
ECS_REQUIRE(const ecs::string &render_settings__shadowsQuality, const ecs::string &render_settings__enableRTSM)
ECS_AFTER(bvh_render_settings_changed_es)
static void init_shadows_es(const ecs::Event &)
{
  auto &shadowsManager = WRDispatcher::getShadowsManager();
  shadowsManager.initShadows();
  // Static shadows is re-created, so world size has to be passed to it (it's only passed on level load otherwise)
  shadowsManager.staticShadowsSetWorldSize();
}

ECS_TAG(render)
ECS_ON_EVENT(OnRenderSettingsReady)
ECS_TRACK(render_settings__enableRTSM)
ECS_REQUIRE(const ecs::string &render_settings__enableRTSM)
ECS_AFTER(bvh_render_settings_changed_es)
static void init_vsm_es(const ecs::Event &)
{
  auto &shadowsManager = WRDispatcher::getShadowsManager();
  if (is_rtsm_enabled())
    shadowsManager.initVSM();
  else
    shadowsManager.closeVSM();
}