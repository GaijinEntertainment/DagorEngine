// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/lights/light.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/component.h>
#include <daECS/core/componentsMap.h>
#include <daECS/core/entityComponent.h>

#include "render/renderEvent.h"
#include "render/renderSettings.h"
#include "render/light_consts.hlsli"
#include "render/skies.h"

#define INSIDE_RENDERER 1
#include "../world/private_worldRenderer.h"
#include "game/gameEvents.h"


namespace lights_impl
{

bool is_lights_render_available();
bool get_feature_dynamic_objects();
bool get_feature_gpu_objects();
bool get_feature_contact_shadows();
bool is_night_time();

void destroy_light(uint32_t h);
uint32_t add_omni_light(OmniLight light, OmniLightMaskType mask);
uint32_t add_spot_light(SpotLight light, SpotLightMaskType mask);
OmniLight get_omni_light(uint32_t h);
SpotLight get_spot_light(uint32_t h);

bool is_light_visible(uint32_t h);
bool get_box_around(const Point3 &position, TMatrix &box);
bool get_clip_plane_around(const Point3 &position, TMatrix &box);
void get_spot_light_shadow_view_proj(uint32_t id, mat44f &view_itm, mat44f &proj);

void set_omni_light(uint32_t h, const OmniLight &light, bool invalidate_shadow);
void set_omni_light_with_mask(uint32_t id, const OmniLight &light, OmniLightMaskType mask, bool invalidate_shadow);
void set_spot_light(uint32_t id, const SpotLight &light, SpotLightMaskType mask, bool invalidate_shadow);

void update_light_shadow(uint32_t id, const LightShadowParams &params);

} // namespace lights_impl

static __forceinline WorldRenderer *get_renderer() { return ((WorldRenderer *)get_world_renderer()); }
static __forceinline WorldRenderer &get_renderer_ref()
{
  auto *wr = get_renderer();
  G_ASSERT(wr);
  return *wr;
}

namespace lights_impl
{

bool is_lights_render_available() { return get_renderer(); }
bool get_feature_dynamic_objects() { return get_renderer_ref().getSettingsShadowsQuality() >= ShadowsQuality::SHADOWS_MEDIUM; }
bool get_feature_gpu_objects() { return get_renderer_ref().getSettingsShadowsQuality() >= ShadowsQuality::SHADOWS_HIGH; }
bool get_feature_contact_shadows() { return get_renderer_ref().getSettingsShadowsQuality() >= ShadowsQuality::SHADOWS_HIGH; }

bool is_night_time()
{
  DaSkies *skies = get_daskies();
  return skies ? skies->getSunDir().y < NIGHT_SUN_COS : true;
}

void destroy_light(uint32_t h)
{
  if (auto *wr = get_renderer())
    wr->destroyLight(h);
}
uint32_t add_omni_light(OmniLight light, OmniLightMaskType mask) { return get_renderer_ref().addOmniLight(light, mask); }
uint32_t add_spot_light(SpotLight light, SpotLightMaskType mask) { return get_renderer_ref().addSpotLight(light, mask); }
OmniLight get_omni_light(uint32_t h) { return get_renderer_ref().getOmniLight(h); }
SpotLight get_spot_light(uint32_t h) { return get_renderer_ref().getSpotLight(h); }

bool is_light_visible(uint32_t h)
{
  if (auto *wr = get_renderer())
    return wr->isLightVisible(h);
  return false;
}

bool get_box_around(const Point3 &position, TMatrix &box)
{
  if (auto *wr = get_renderer())
    return wr->getBoxAround(position, box);
  return false;
}

bool get_clip_plane_around(const Point3 &, TMatrix &) { return false; }
void get_spot_light_shadow_view_proj(uint32_t id, mat44f &view_itm, mat44f &proj)
{
  return get_renderer_ref().getSpotLightShadowViewProj(id, view_itm, proj);
}

void set_omni_light(uint32_t id, const OmniLight &light, bool invalidate_shadow)
{
  get_renderer_ref().setLight(id, light, invalidate_shadow);
}
void set_omni_light_with_mask(uint32_t id, const OmniLight &light, OmniLightMaskType mask, bool invalidate_shadow)
{
  get_renderer_ref().setLightWithMask(id, light, mask, invalidate_shadow);
}
void set_spot_light(uint32_t id, const SpotLight &light, SpotLightMaskType mask, bool invalidate_shadow)
{
  get_renderer_ref().setLight(id, light, mask, invalidate_shadow);
}

void update_light_shadow(uint32_t id, const LightShadowParams &params) { get_renderer_ref().updateLightShadow(id, params); }

} // namespace lights_impl


ECS_TAG(render)
ECS_ON_EVENT(OnRenderSettingsReady)
ECS_TRACK(render_settings__shadowsQuality)
ECS_REQUIRE(const ecs::string &render_settings__shadowsQuality)
ECS_AFTER(init_shadows_es)
static void shadows_quality_changed_es(const ecs::Event &, ecs::EntityManager &manager)
{
  manager.broadcastEventImmediate(CmdRecreateOmniLights());
  manager.broadcastEventImmediate(CmdRecreateSpotLights());
}

ECS_ON_EVENT(ChangeRenderFeatures, BeforeLoadLevel)
static void render_features_changed_es(const ecs::Event &evt, ecs::EntityManager &manager)
{
  if (auto evtRenderFeatures = evt.cast<ChangeRenderFeatures>())
  {
    if (!evtRenderFeatures->isFeatureChanged(FeatureRenderFlags::CLUSTERED_LIGHTS) &&
        !evtRenderFeatures->isFeatureChanged(FeatureRenderFlags::DYNAMIC_LIGHTS_SHADOWS))
      return;
  }

  manager.broadcastEventImmediate(CmdRecreateSpotLights());
  manager.broadcastEventImmediate(CmdRecreateOmniLights());
}

static void update_high_priority_lights_es(const EventOnGameUpdateAfterGameLogic &, ecs::EntityManager &manager)
{
  manager.broadcastEventImmediate(CmdUpdateHighPriorityLights());
}

void set_nightly_spot_lights() { g_entity_mgr->broadcastEventImmediate(CmdRecreateSpotLights()); }
