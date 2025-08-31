// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/entityManager.h>

#include <render/world/wrDispatcher.h>
#include <render/renderSettings.h>
#include <render/volumetricLights/volumetricLights.h>
#include <nodeBasedShaderManager/nodeBasedShaderManager.h>

#include <ecs/render/updateStageRender.h>
#include <util/dag_convar.h>

#define INSIDE_RENDERER 1
#include <render/world/private_worldRenderer.h>

// these are just for quick testing
CONSOLE_INT_VAL("render", distant_fog_quality, 0, 0, 2);
CONSOLE_BOOL_VAL("render", hq_volfog, false);
CONSOLE_BOOL_VAL("volfog", disable_volfog_shadows, false);


ECS_BROADCAST_EVENT_TYPE(OnExplicitVolfogSettingsChange)
ECS_REGISTER_EVENT(OnExplicitVolfogSettingsChange)


static VolumeLight::DistantFogQuality get_distant_fog_quality(const ecs::string &render_settings__volumeFogQuality)
{
  if (render_settings__volumeFogQuality == "far")
    return VolumeLight::DistantFogQuality::High;
  else if (render_settings__volumeFogQuality == "medium")
    return VolumeLight::DistantFogQuality::Medium;
  else
    return VolumeLight::DistantFogQuality::Disabled;
}
static eastl::string get_distant_fog_quality_str(VolumeLight::DistantFogQuality quality)
{
  switch (quality)
  {
    case VolumeLight::DistantFogQuality::High: return "far";
    case VolumeLight::DistantFogQuality::Medium: return "medium";
    default: return "close";
  }
}

ECS_ON_EVENT(on_appear)
ECS_TRACK(render_settings__HQVolfog, render_settings__volumeFogQuality)
static void volfog_convar_init_es(
  const ecs::Event &, bool render_settings__HQVolfog, const ecs::string &render_settings__volumeFogQuality)
{
  distant_fog_quality = static_cast<int>(get_distant_fog_quality(render_settings__volumeFogQuality));
  hq_volfog = render_settings__HQVolfog;
}

ECS_NO_ORDER
ECS_TAG(render, dev)
ECS_AFTER(animchar_before_render_es) // require for execute animchar_before_render_es as early as possible
static void volfog_convar_change_es(
  const UpdateStageInfoBeforeRender &, bool &render_settings__HQVolfog, ecs::string &render_settings__volumeFogQuality)
{
  if (hq_volfog.pullValueChange())
    render_settings__HQVolfog = hq_volfog;

  if (distant_fog_quality.pullValueChange())
    render_settings__volumeFogQuality =
      get_distant_fog_quality_str(static_cast<VolumeLight::DistantFogQuality>(distant_fog_quality.get()));

  if (disable_volfog_shadows.pullValueChange())
    g_entity_mgr->broadcastEvent(OnExplicitVolfogSettingsChange());
}


static bool has_shadow_casting(const ecs::string &render_settings__shadowsQuality)
{
  return !disable_volfog_shadows && (render_settings__shadowsQuality == "medium" || render_settings__shadowsQuality == "high" ||
                                      render_settings__shadowsQuality == "ultra");
}

ECS_ON_EVENT(OnRenderSettingsReady, OnExplicitVolfogSettingsChange)
ECS_TRACK(render_settings__HQVolfog, render_settings__volumeFogQuality, render_settings__shadowsQuality)
static void volfog_change_settings_es(const ecs::Event &,
  bool render_settings__HQVolfog,
  const ecs::string &render_settings__volumeFogQuality,
  const ecs::string &render_settings__shadowsQuality)
{
  VolumeLight *volumeLight = WRDispatcher::getVolumeLight();
  if (volumeLight)
  {
    NodeBasedShaderManager::clearAllCachedResources();

    VolumeLight::VolfogQuality volfogQuality =
      render_settings__HQVolfog ? VolumeLight::VolfogQuality::HighQuality : VolumeLight::VolfogQuality::Default;
    VolumeLight::VolfogShadowCasting shadowCasting = has_shadow_casting(render_settings__shadowsQuality)
                                                       ? VolumeLight::VolfogShadowCasting::Yes
                                                       : VolumeLight::VolfogShadowCasting::No;
    VolumeLight::DistantFogQuality dfQuality = get_distant_fog_quality(render_settings__volumeFogQuality);

    // DF needs checkerboard depth, which upscale tex provides
    auto checkUpscaleTex = []() -> bool {
      auto wr = static_cast<WorldRenderer *>(get_world_renderer());
      return wr && wr->hasFeature(FeatureRenderFlags::UPSCALE_SAMPLING_TEX);
    };

    if (dfQuality != VolumeLight::DistantFogQuality::Disabled && !checkUpscaleTex())
    {
      dfQuality = VolumeLight::DistantFogQuality::Disabled;
#if DAGOR_DBGLEVEL > 0 // it's a harmless logerr, we shouldn't expose it to players
      logwarn("distant fog feature was requested without having upscale tex");
      // TODO: for some reason, BM can think it has DF, it needs to be investigated
#endif
    }

    volumeLight->onSettingsChange(volfogQuality, shadowCasting, dfQuality);
  }
}
