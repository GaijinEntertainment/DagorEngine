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
CONSOLE_BOOL_VAL("render", distant_fog, false);
CONSOLE_BOOL_VAL("render", hq_volfog, false);
CONSOLE_BOOL_VAL("volfog", disable_volfog_shadows, false);


ECS_BROADCAST_EVENT_TYPE(OnExplicitVolfogSettingsChange)
ECS_REGISTER_EVENT(OnExplicitVolfogSettingsChange)


ECS_ON_EVENT(on_appear)
ECS_TRACK(render_settings__HQVolfog, render_settings__volumeFogQuality)
static void volfog_convar_init_es(
  const ecs::Event &, bool render_settings__HQVolfog, const ecs::string &render_settings__volumeFogQuality)
{
  distant_fog = render_settings__volumeFogQuality == "far";
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

  if (distant_fog.pullValueChange())
    render_settings__volumeFogQuality = distant_fog ? "far" : "close";

  if (disable_volfog_shadows.pullValueChange())
    g_entity_mgr->broadcastEvent(OnExplicitVolfogSettingsChange());
}


static bool has_distant_fog(const ecs::string &render_settings__volumeFogQuality)
{
  return render_settings__volumeFogQuality == "far";
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
    VolumeLight::DistantFogQuality dfQuality = has_distant_fog(render_settings__volumeFogQuality)
                                                 ? VolumeLight::DistantFogQuality::EnableDistantFog
                                                 : VolumeLight::DistantFogQuality::DisableDistantFog;

    // DF needs checkerboard depth, which upscale tex provides
    auto checkUpscaleTex = []() -> bool {
      auto wr = static_cast<WorldRenderer *>(get_world_renderer());
      return wr && wr->hasFeature(FeatureRenderFlags::UPSCALE_SAMPLING_TEX);
    };

    if (dfQuality == VolumeLight::DistantFogQuality::EnableDistantFog && !checkUpscaleTex())
    {
      dfQuality = VolumeLight::DistantFogQuality::DisableDistantFog;
#if DAGOR_DBGLEVEL > 0 // it's a harmless logerr, we shouldn't expose it to players
      logwarn("distant fog feature was requested without having upscale tex");
      // TODO: for some reason, BM can think it has DF, it needs to be investigated
#endif
    }

    volumeLight->onSettingsChange(volfogQuality, shadowCasting, dfQuality);
  }
}
