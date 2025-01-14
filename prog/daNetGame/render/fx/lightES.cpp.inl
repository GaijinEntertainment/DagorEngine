// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <daEditorE/editorCommon/inGameEditor.h>
#include <debug/dag_debug3d.h>
#include <ecs/core/attributeEx.h>
#include <math/dag_color.h>
#include <render/omniLight.h>
#include <render/spotLight.h>
#include <ecs/anim/anim.h>
#include <ecs/render/updateStageRender.h>
#include "render/omniLightsManager.h"
#include "render/skies.h"
#include "lightHandle.h"
#include "render/renderEvent.h"
#include <shaders/light_consts.hlsli>
#include <render/lightShadowParams.h>
#include <render/renderSettings.h>
#include <util/dag_convar.h>
#include <camera/sceneCam.h>

#define INSIDE_RENDERER 1
#include "../world/private_worldRenderer.h"

enum class EditorLightDrawMode
{
  None,
  All,
  Selected,

  FirstValue = None,
  LastValue = Selected,
};

static ConVarT<int, true> editor_light_draw_mode("light.editor_draw",
  (int)EditorLightDrawMode::None,
  (int)EditorLightDrawMode::FirstValue,
  (int)EditorLightDrawMode::LastValue,
  "0 = none, 1 = all lights, 2 = selected lights");

template <class T>
inline void omni_lights_ecs_query(T b);

static __forceinline WorldRenderer *get_renderer() { return ((WorldRenderer *)get_world_renderer()); }

ECS_BROADCAST_EVENT_TYPE(RecreateSpotLights)
ECS_REGISTER_EVENT(RecreateSpotLights)

ECS_BROADCAST_EVENT_TYPE(RecreateOmniLights)
ECS_REGISTER_EVENT(RecreateOmniLights)

void LightHandleDeleter::operator()(int h) const
{
  if (WorldRenderer *wr = get_renderer())
    wr->destroyLight(h);
}

struct OmniLightEntity
{
  LightHandle lightId;

  bool ensureLightIdCreated(bool should_create)
  {
    if (!should_create)
    {
      lightId.reset();
      return false;
    }

    if (lightId)
      return true;

    if (WorldRenderer *wr = get_renderer())
      lightId = wr->addOmniLight(OmniLight::create_empty(), OmniLightsManager::GI_LIGHT_MASK);

    return lightId;
  }
};

ECS_DECLARE_RELOCATABLE_TYPE(OmniLightEntity);
ECS_REGISTER_RELOCATABLE_TYPE(OmniLightEntity, nullptr);
ECS_AUTO_REGISTER_COMPONENT(OmniLightEntity, "omni_light", nullptr, 0);

struct SpotLightEntity
{
  LightHandle lightId;

  bool ensureLightIdCreated(bool should_create)
  {
    if (!should_create)
    {
      lightId.reset();
      return false;
    }

    if (lightId)
      return true;

    if (WorldRenderer *wr = get_renderer())
      lightId = wr->addSpotLight(SpotLight::create_empty(), SpotLightsManager::GI_LIGHT_MASK);

    return lightId;
  }
};

ECS_DECLARE_RELOCATABLE_TYPE(SpotLightEntity);
ECS_REGISTER_RELOCATABLE_TYPE(SpotLightEntity, nullptr);
ECS_AUTO_REGISTER_COMPONENT(SpotLightEntity, "spot_light", nullptr, 0);

template <typename LightType>
static bool is_light_visible(const LightType &light)
{
  if (light.lightId < 0)
    return false;

  if (get_renderer())
    return get_renderer()->isLightVisible(light.lightId);

  return false;
}

template <typename Callable>
static void set_omni_lights_ecs_query(Callable c);

void set_up_omni_lights()
{
  if (!get_renderer())
    return;

  set_omni_lights_ecs_query([&](const OmniLightEntity &omni_light, TMatrix &light__box ECS_REQUIRE(eastl::true_type light__use_box)
                                                                     ECS_REQUIRE(eastl::true_type light__automatic_box)) {
    if (!omni_light.lightId)
      return;

    OmniLight light = get_renderer()->getOmniLight(omni_light.lightId);

    if (get_renderer()->getBoxAround(Point3{light.pos_radius.x, light.pos_radius.y, light.pos_radius.z}, light__box)) //-V1051
    {
      light.setBox(light__box);
      get_renderer()->setLight(omni_light.lightId, light, false);
    }
  });

  g_entity_mgr->broadcastEvent(UpdateEffectRestrictionBoxes());
}

enum class LightUseBox
{
  Off = 0,
  On = 1
};
enum class LightDynamicLight
{
  Off = 0,
  On = 1
};
enum class LightContactShadows
{
  Off = 0,
  On = 1
};
enum class LightHasShadows
{
  Off = 0,
  On = 1
};
enum class LightHasDynamicCasters
{
  Off = 0,
  On = 1
};
enum class LightApproximateStatic
{
  Off = 0,
  On = 1
};
enum class LightRenderGpuObjects
{
  Off = 0,
  On = 1
};
enum class LightAffectVolumes
{
  Off = 0,
  On = 1
};
enum class LightLensFlares
{
  Off = 0,
  On = 1
};

static __forceinline void omni_light_set(OmniLightEntity &omni_light,
  const TMatrix &transform,
  LightUseBox light__use_box,
  bool &light__automatic_box,
  bool &light__force_max_light_radius,
  TMatrix &light__box,
  const Point3 &light__offset,
  const Point3 &light__direction,
  const ecs::string &light__texture_name,
  const E3DCOLOR light__color,
  float light__brightness,
  LightHasShadows omni_light__shadows,
  LightHasDynamicCasters omni_light__dynamic_obj_shadows,
  int omni_light__shadow_quality,
  int omni_light__priority,
  int omni_light__shadow_shrink,
  float light__radius_scale,
  float light__max_radius,
  LightDynamicLight light__dynamic_light, // TODO: use a shared one with spot lights
  LightContactShadows light__contact_shadows,
  LightApproximateStatic light__approximate_static_shadows,
  LightAffectVolumes light__affect_volumes,
  LightRenderGpuObjects light__render_gpu_objects,
  LightLensFlares light__enable_lens_flares)
{
  if (!get_renderer())
    return;
  if (!omni_light.lightId)
    return;

  ShadowsQuality shadowQuality = get_renderer()->getSettingsShadowsQuality();
  LightShadowParams shadowParams;
  shadowParams.isEnabled = bool(omni_light__shadows);
  shadowParams.approximateStatic = bool(light__approximate_static_shadows);
  shadowParams.isDynamic = bool(light__dynamic_light);
  shadowParams.supportsDynamicObjects =
    shadowQuality >= ShadowsQuality::SHADOWS_MEDIUM || bool(light__dynamic_light) ? bool(omni_light__dynamic_obj_shadows) : false;
  shadowParams.supportsGpuObjects = shadowQuality >= ShadowsQuality::SHADOWS_HIGH ? bool(light__render_gpu_objects) : false;
  shadowParams.quality = omni_light__shadow_quality;
  shadowParams.shadowShrink = omni_light__shadow_shrink;
  shadowParams.priority = omni_light__priority;
  bool contact_shadows = shadowQuality >= ShadowsQuality::SHADOWS_HIGH ? bool(light__contact_shadows) : false;

  get_renderer()->updateLightShadow(omni_light.lightId, shadowParams);
  Color3 color = color3(light__color) * light__brightness;

  const float dropOffLuminance = 0.05; // 5% of the original luminance
  const float baseLuminance = average(color);
  // This is the distance where most of the luminance (95%) drops off due to inverse square law.
  // Use the light__radius_scale to adjust the drop-off radius
  float autoDropOffRadius = sqrtf(max(0.0f, baseLuminance / dropOffLuminance)) * light__radius_scale;
  if (light__max_radius > 0)
    autoDropOffRadius = min(light__max_radius, autoDropOffRadius);

  if (light__force_max_light_radius && light__max_radius <= 0)
  {
    light__force_max_light_radius = false;
    logerr("Light has no max radius set, but force_max_light_radius is set to true.\n"
           "Force max light flag reseted to false");
  }

  float radius = light__force_max_light_radius ? light__max_radius : autoDropOffRadius;

  const Point3 position = transform * light__offset;
  const Point3 direction = normalize(transform % light__direction);
  if (light__use_box == LightUseBox::Off)
  {
    const float defaultBoxSize = radius * 3.0f;
    light__box.setcol(0, Point3{defaultBoxSize, 0, 0});
    light__box.setcol(1, Point3{0, defaultBoxSize, 0});
    light__box.setcol(2, Point3{0, 0, defaultBoxSize});
    light__box.setcol(3, position);
  }
  else
  {
    if (lengthSq(light__box.getcol(0)) == 0.f)
      light__automatic_box = true;
    if (light__automatic_box)
      get_renderer()->getBoxAround(position, light__box);
  }
  IesTextureCollection *photometryTextures = IesTextureCollection::getSingleton();
#if DAGOR_DBGLEVEL > 0
  float dist = length(position - light__box.getcol(3));
  float boundingSphere =
    sqrt(max(lengthSq(light__box.getcol(0)), max(lengthSq(light__box.getcol(1)), lengthSq(light__box.getcol(2)))));
  if (dist > boundingSphere + radius)
  {
    LOGWARN_ONCE(
      "omni light at (%f, %f, %f) source does not intersect its area of influence (box around: (%f, %f, %f), box boundingSphere: %f)",
      position.x, position.y, position.z, light__box.getcol(3).x, light__box.getcol(3).y, light__box.getcol(3).z, boundingSphere);
  }
#endif

  const int tex_idx =
    photometryTextures && light__texture_name != "" ? photometryTextures->getTextureIdx(light__texture_name.c_str()) : -1;
#if DAGOR_DBGLEVEL > 0
  if (!photometryTextures)
  {
    logerr("Photometry textures are not initialized");
  }
  else if (tex_idx == -1 && light__texture_name != "")
  {
    logerr("Texture not found: %s", light__texture_name.c_str());
  }
#endif
  IesTextureCollection::PhotometryData data;
  if (photometryTextures && tex_idx != -1)
    data = photometryTextures->getTextureData(tex_idx);
  OmniLight omniLight =
    OmniLight(position, direction, color, radius, bool(contact_shadows) ? 1.0f : 0.f, tex_idx, data.zoom, data.rotated, light__box);
  OmniLightsManager::mask_type_t mask = ~OmniLightsManager::mask_type_t(0);
  if (light__dynamic_light == LightDynamicLight::On || light__affect_volumes == LightAffectVolumes::Off)
    mask &= ~OmniLightsManager::GI_LIGHT_MASK;
  if (light__enable_lens_flares == LightLensFlares::Off)
    mask &= ~OmniLightsManager::MASK_LENS_FLARE;
  get_renderer()->setLightWithMask(omni_light.lightId, eastl::move(omniLight), mask, true);
}

ECS_TAG(render)
ECS_ON_EVENT(OnRenderSettingsReady)
ECS_TRACK(render_settings__shadowsQuality)
ECS_REQUIRE(const ecs::string &render_settings__shadowsQuality)
ECS_AFTER(init_shadows_es)
static void shadows_quality_changed_es(const ecs::Event &, ecs::EntityManager &manager)
{
  manager.broadcastEventImmediate(RecreateOmniLights());
  manager.broadcastEventImmediate(RecreateSpotLights());
}

static inline TMatrix light_tm(ecs::EntityId eid,
  const ecs::EntityManager &manager,
  const TMatrix *transform,
  const AnimV20::AnimcharBaseComponent *animchar,
  const TMatrix *light_mod_tm)
{
  if (transform)
    return *transform;

  if (!animchar || !light_mod_tm)
  {
    LOGERR_ONCE("Invalid light eid=%d, template='%s'\n"
                "Should have either component transform or (animchar and lightModTm)",
      static_cast<ecs::entity_id_t>(eid), manager.getEntityTemplateName(eid));
    return TMatrix::IDENT;
  }

  TMatrix tm = TMatrix::IDENT;
  animchar->getTm(tm);
  tm *= *light_mod_tm;

  return tm;
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear, ChangeRenderFeatures, BeforeLoadLevel, RecreateOmniLights)
ECS_TRACK(*)
static __forceinline void update_omni_light_es(const ecs::Event &evt,
  ecs::EntityId eid,
  const ecs::EntityManager &manager,
  OmniLightEntity &omni_light,
  bool light__use_box,
  bool &light__automatic_box,
  bool &light__force_max_light_radius,
  TMatrix &light__box,
  const Point3 &light__offset,
  const Point3 &light__direction,
  const ecs::string &light__texture_name,
  const E3DCOLOR light__color,
  float light__brightness,
  bool omni_light__shadows,
  bool omni_light__dynamic_obj_shadows,
  int omni_light__shadow_quality = 0,
  int omni_light__priority = 0,
  int omni_light__shadow_shrink = 0,
  float light__radius_scale = 1,
  float light__max_radius = -1,
  bool light__dynamic_light = false,
  bool light__contact_shadows = true,
  bool light__affect_volumes = true,
  bool light__render_gpu_objects = false,
  bool light__is_paused = false,
  bool light__enable_lens_flares = false,
  bool light__approximate_static = false,
  const TMatrix *transform = nullptr,
  const AnimV20::AnimcharBaseComponent *animchar = nullptr,
  const TMatrix *lightModTm = nullptr)
{
  if (auto evtRenderFeatures = evt.cast<ChangeRenderFeatures>())
  {
    if (!evtRenderFeatures->isFeatureChanged(FeatureRenderFlags::CLUSTERED_LIGHTS) &&
        !evtRenderFeatures->isFeatureChanged(FeatureRenderFlags::DYNAMIC_LIGHTS_SHADOWS))
      return;
  }

  bool shouldCreate = !light__is_paused;
  if (!omni_light.ensureLightIdCreated(shouldCreate))
    return;

  const TMatrix tm = light_tm(eid, manager, transform, animchar, lightModTm);

  omni_light_set(omni_light, tm, LightUseBox(light__use_box), light__automatic_box, light__force_max_light_radius, light__box,
    light__offset, light__direction, light__texture_name, light__color, light__brightness, LightHasShadows(omni_light__shadows),
    LightHasDynamicCasters(omni_light__dynamic_obj_shadows), omni_light__shadow_quality, omni_light__priority,
    omni_light__shadow_shrink, light__radius_scale, light__max_radius, LightDynamicLight(light__dynamic_light),
    LightContactShadows(light__contact_shadows), LightApproximateStatic(light__approximate_static),
    LightAffectVolumes(light__affect_volumes), LightRenderGpuObjects(light__render_gpu_objects),
    LightLensFlares(light__enable_lens_flares));
}

ECS_TAG(render)
ECS_REQUIRE_NOT(const TMatrix &transform)
ECS_REQUIRE_NOT(const bool &light__use_box)
ECS_NO_ORDER
static __forceinline void omni_light_es(const ecs::UpdateStageInfoAct &,
  OmniLightEntity &omni_light,
  AnimV20::AnimcharBaseComponent &animchar,
  const TMatrix &lightModTm,
  const Point3 &light__offset,
  const Point3 &light__direction,
  const ecs::string &light__texture_name,
  bool &light__force_max_light_radius,
  const E3DCOLOR light__color,
  float light__brightness,
  bool omni_light__shadows,
  bool omni_light__dynamic_obj_shadows,
  int omni_light__shadow_quality = 0,
  int omni_light__priority = 0,
  int omni_light__shadow_shrink = 0,
  float light__radius_scale = 1,
  float light__max_radius = -1,
  bool light__dynamic_light = false,
  bool light__contact_shadows = true,
  bool animchar_render__enabled = true,
  bool light__affect_volumes = true,
  bool light__render_gpu_objects = false,
  bool light__enable_lens_flares = false,
  bool light__approximate_static = false)
{
  TMatrix transform = TMatrix::IDENT;
  animchar.getTm(transform);
  transform = transform * lightModTm;
  TMatrix emptyBoxDummy = TMatrix::ZERO;
  bool isAutomaticBoxDummy = false;
  omni_light_set(omni_light, transform, LightUseBox::Off, isAutomaticBoxDummy, light__force_max_light_radius, emptyBoxDummy,
    light__offset, light__direction, light__texture_name, light__color, animchar_render__enabled ? light__brightness : 0.f,
    LightHasShadows(omni_light__shadows), LightHasDynamicCasters(omni_light__dynamic_obj_shadows), omni_light__shadow_quality,
    omni_light__priority, omni_light__shadow_shrink, light__radius_scale, light__max_radius, LightDynamicLight(light__dynamic_light),
    LightContactShadows(light__contact_shadows), LightApproximateStatic(light__approximate_static),
    LightAffectVolumes(light__affect_volumes), LightRenderGpuObjects(light__render_gpu_objects),
    LightLensFlares(light__enable_lens_flares));
}

static __forceinline void spot_light_set(SpotLightEntity &spot_light,
  const TMatrix &transform,
  const Point3 &light__offset,
  const ecs::string &light__texture_name,
  const E3DCOLOR light__color,
  float light__brightness,
  LightDynamicLight spot_light__dynamic_light,
  LightHasShadows spot_light__shadows,
  LightHasDynamicCasters spot_light__dynamic_obj_shadows,
  bool &light__force_max_light_radius,
  int spot_light__shadow_quality,
  int spot_light__priority,
  int spot_light__shadow_shrink,
  float spot_light__cone_angle,
  float spot_light__inner_attenuation,
  float light__radius_scale,
  float light__max_radius,
  LightAffectVolumes light__affect_volumes,
  LightContactShadows spot_light__contact_shadows,
  LightApproximateStatic light__approximate_static_shadows,
  LightRenderGpuObjects light__render_gpu_objects,
  LightLensFlares light__enable_lens_flares)
{
  if (!get_renderer())
    return;
  if (!spot_light.lightId)
    return;

  ShadowsQuality shadowQuality = get_renderer()->getSettingsShadowsQuality();
  LightShadowParams shadowParams;
  shadowParams.isEnabled = bool(spot_light__shadows);
  shadowParams.approximateStatic = bool(light__approximate_static_shadows);
  shadowParams.isDynamic = bool(spot_light__dynamic_light);
  shadowParams.supportsDynamicObjects =
    shadowQuality >= ShadowsQuality::SHADOWS_MEDIUM || bool(spot_light__dynamic_light) ? bool(spot_light__dynamic_obj_shadows) : false;
  shadowParams.supportsGpuObjects = shadowQuality >= ShadowsQuality::SHADOWS_HIGH ? bool(light__render_gpu_objects) : false;
  shadowParams.quality = spot_light__shadow_quality;
  shadowParams.shadowShrink = spot_light__shadow_shrink;
  shadowParams.priority = spot_light__priority;
  bool contact_shadows = shadowQuality >= ShadowsQuality::SHADOWS_HIGH ? bool(spot_light__contact_shadows) : false;

  get_renderer()->updateLightShadow(spot_light.lightId, shadowParams);
  Color3 color = color3(light__color) * light__brightness;

  float auto_radius = light__radius_scale * rgbsum(color) * (4.47f / 3); // 4.47 = 1/sqrt(0.05)/ (3 for rgbsum), i.e. 95% of brightness
                                                                         // has already lost due to inverse squares law. If light is in
                                                                         // very dark place, it can be increased by scale
  if (light__max_radius > 0)
    auto_radius = min(light__max_radius, auto_radius);

  if (light__force_max_light_radius && light__max_radius <= 0)
  {
    light__force_max_light_radius = false;
    logerr("Light has no max radius set, but force_max_light_radius is set to true.\n"
           "Force max light flag reseted to false");
  }

  float radius = light__force_max_light_radius ? light__max_radius : auto_radius;

  float halfTan, col_length;
  halfTan = col_length = length(transform.getcol(2));

  if (spot_light__cone_angle >= 0)
  {
    float angle = spot_light__cone_angle * PI / 180.0;
    halfTan = tanf(angle / 2);
  }
  float attCos = cosf(spot_light__inner_attenuation * atanf(halfTan));

  IesTextureCollection *photometryTextures = IesTextureCollection::getSingleton();
  const int tex_idx =
    photometryTextures && light__texture_name != "" ? photometryTextures->getTextureIdx(light__texture_name.c_str()) : -1;
#if DAGOR_DBGLEVEL > 0
  if (!photometryTextures)
  {
    logerr("Photometry textures are not initialized");
  }
  else if (tex_idx == -1 && light__texture_name != "")
  {
    logerr("Texture not found: %s", light__texture_name.c_str());
  }
#endif
  IesTextureCollection::PhotometryData data;
  if (photometryTextures && tex_idx != -1)
    data = photometryTextures->getTextureData(tex_idx);
  SpotLight spotLight =
    SpotLight(transform * light__offset, color, light__brightness > 0 ? radius : -1, attCos, transform.getcol(2) / col_length, halfTan,
      bool(contact_shadows) && bool(spot_light__shadows), bool(spot_light__shadows), tex_idx, data.zoom, data.rotated);
  SpotLightsManager::mask_type_t mask = ~SpotLightsManager::mask_type_t(0);
  if (spot_light__dynamic_light == LightDynamicLight::On || light__affect_volumes == LightAffectVolumes::Off)
    mask &= ~SpotLightsManager::GI_LIGHT_MASK;
  if (light__enable_lens_flares == LightLensFlares::Off)
    mask &= ~SpotLightsManager::MASK_LENS_FLARE;
  get_renderer()->setLight(spot_light.lightId, eastl::move(spotLight), mask, true);
}

ECS_TAG(render)
ECS_NO_ORDER
static __forceinline void omni_light_upd_visibility_es(
  const UpdateStageInfoBeforeRender &, const OmniLightEntity &omni_light, bool &light__visible)
{
  light__visible = is_light_visible(omni_light);
}


ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static inline void pause_lights_set_sq_es(const ecs::Event &,
  float light__render_range_limit,
  float light__render_range_gap,
  float &light__render_range_limit_start_sq,
  float &light__render_range_limit_stop_sq)
{
  light__render_range_limit = max(light__render_range_limit, 0.0f);

  light__render_range_limit_start_sq = sqr(light__render_range_limit * (1 - light__render_range_gap));
  light__render_range_limit_stop_sq = sqr(light__render_range_limit * (1 + light__render_range_gap));
}

template <typename Callable>
static inline void pause_lights_ecs_query(Callable c);

ECS_TAG(render)
ECS_AFTER(after_camera_sync)
static inline void pause_lights_es(const ecs::UpdateStageInfoAct &, ecs::EntityManager &manager)
{
  const TMatrix &camItm = get_cam_itm();
  const Point3 &cameraPos = camItm.getcol(3);

  pause_lights_ecs_query([&cameraPos, &manager](ecs::EntityId eid, float light__render_range_limit_start_sq,
                           float light__render_range_limit_stop_sq, bool &light__is_paused, const TMatrix *transform = nullptr,
                           const AnimV20::AnimcharBaseComponent *animchar = nullptr, const TMatrix *lightModTm = nullptr) {
    if (light__render_range_limit_start_sq <= 0)
    {
      light__is_paused = false;
      return;
    }

    const TMatrix tm = light_tm(eid, manager, transform, animchar, lightModTm);
    const Point3 &effectPos = tm.getcol(3);
    float distanceSq = lengthSq(effectPos - cameraPos);

    // NOTE: to prevent blinking near render range
    // camera                             ]
    // ...                                ]
    // light__render_range_limit_start_sq ] paused = false
    // ...                                  paused = same as previous frame
    // light__render_range_limit_stop_sq  ] paused = true
    // ...                                ]
    if (distanceSq < light__render_range_limit_start_sq)
      light__is_paused = false;
    else if (distanceSq >= light__render_range_limit_stop_sq)
      light__is_paused = true;
  });
}


ECS_TAG(render)
ECS_ON_EVENT(on_appear, ChangeRenderFeatures, BeforeLoadLevel, RecreateSpotLights)
ECS_TRACK(*)
static __forceinline void update_spot_light_es(const ecs::Event &evt,
  ecs::EntityId eid,
  const ecs::EntityManager &manager,
  SpotLightEntity &spot_light,
  const Point3 &light__offset,
  const ecs::string &light__texture_name,
  const E3DCOLOR light__color,
  float light__brightness,
  bool spot_light__dynamic_light,
  bool spot_light__shadows,
  bool spot_light__dynamic_obj_shadows,
  bool &light__force_max_light_radius,
  bool light__nightly = false,
  int spot_light__shadow_quality = 0,
  int spot_light__priority = 0,
  int spot_light__shadow_shrink = 0,
  float spot_light__cone_angle = -1.0,
  float spot_light__inner_attenuation = 0.95,
  float light__radius_scale = 1,
  float light__max_radius = -1,
  bool light__affect_volumes = true,
  bool spot_light__contact_shadows = false,
  bool light__render_gpu_objects = false,
  bool light__is_paused = false,
  const TMatrix *transform = nullptr,
  const AnimV20::AnimcharBaseComponent *animchar = nullptr,
  const TMatrix *lightModTm = nullptr,
  bool light__enable_lens_flares = false,
  bool light__approximate_static = false)
{
  if (auto evtRenderFeatures = evt.cast<ChangeRenderFeatures>())
  {
    if (!evtRenderFeatures->isFeatureChanged(FeatureRenderFlags::CLUSTERED_LIGHTS) &&
        !evtRenderFeatures->isFeatureChanged(FeatureRenderFlags::DYNAMIC_LIGHTS_SHADOWS))
      return;
  }

  bool shouldCreate = !light__is_paused && (light__nightly ? get_daskies()->getSunDir().y < NIGHT_SUN_COS : true);
  if (!spot_light.ensureLightIdCreated(shouldCreate))
    return;

  const TMatrix tm = light_tm(eid, manager, transform, animchar, lightModTm);

  spot_light_set(spot_light, tm, light__offset, light__texture_name, light__color, light__brightness,
    LightDynamicLight(spot_light__dynamic_light), LightHasShadows(spot_light__shadows),
    LightHasDynamicCasters(spot_light__dynamic_obj_shadows), light__force_max_light_radius, spot_light__shadow_quality,
    spot_light__priority, spot_light__shadow_shrink, spot_light__cone_angle, spot_light__inner_attenuation, light__radius_scale,
    light__max_radius, LightAffectVolumes(light__affect_volumes), LightContactShadows(spot_light__contact_shadows),
    LightApproximateStatic(light__approximate_static), LightRenderGpuObjects(light__render_gpu_objects),
    LightLensFlares(light__enable_lens_flares));
}

ECS_TAG(render)
ECS_REQUIRE_NOT(const TMatrix &transform)
ECS_NO_ORDER
static __forceinline void spot_light_es(const ecs::UpdateStageInfoAct &,
  SpotLightEntity &spot_light,
  AnimV20::AnimcharBaseComponent &animchar,
  const TMatrix &lightModTm,
  const Point3 &light__offset,
  const ecs::string &light__texture_name,
  const E3DCOLOR light__color,
  float light__brightness,
  bool spot_light__dynamic_light,
  bool spot_light__shadows,
  bool spot_light__dynamic_obj_shadows,
  bool &light__force_max_light_radius,
  int spot_light__shadow_quality = 0,
  int spot_light__priority = 0,
  int spot_light__shadow_shrink = 0,
  float spot_light__inner_attenuation = 0.95,
  float spot_light__cone_angle = -1.0,
  float light__radius_scale = 1,
  float light__max_radius = -1,
  bool animchar_render__enabled = true,
  bool light__affect_volumes = true,
  bool spot_light__contact_shadows = false,
  bool light__render_gpu_objects = false,
  bool light__enable_lens_flares = false,
  bool light__approximate_static = false)
{
  TMatrix transform = TMatrix::IDENT;
  animchar.getTm(transform);
  transform = transform * lightModTm;

  spot_light_set(spot_light, transform, light__offset, light__texture_name, light__color,
    animchar_render__enabled ? light__brightness : 0.f, LightDynamicLight(spot_light__dynamic_light),
    LightHasShadows(spot_light__shadows), LightHasDynamicCasters(spot_light__dynamic_obj_shadows), light__force_max_light_radius,
    spot_light__shadow_quality, spot_light__priority, spot_light__shadow_shrink, spot_light__cone_angle, spot_light__inner_attenuation,
    light__radius_scale, light__max_radius, LightAffectVolumes(light__affect_volumes),
    LightContactShadows(spot_light__contact_shadows), LightApproximateStatic(light__approximate_static),
    LightRenderGpuObjects(light__render_gpu_objects), LightLensFlares(light__enable_lens_flares));
}

template <typename Callable>
static void spot_light_set_all_ecs_query(Callable c);

void set_nightly_spot_lights() { g_entity_mgr->broadcastEventImmediate(RecreateSpotLights()); }

ECS_TAG(render)
ECS_NO_ORDER
static __forceinline void spot_light_upd_visibility_es(
  const UpdateStageInfoBeforeRender &, const SpotLightEntity &spot_light, bool &light__visible)
{
  light__visible = is_light_visible(spot_light);
}

template <typename Callable>
static void editor_draw_omni_lights_ecs_query(Callable c);

ECS_NO_ORDER
ECS_TAG(render, dev)
static __forceinline void editor_draw_omni_light_es(const UpdateStageInfoRenderDebug &stage_info)
{
  if (!get_da_editor4().isActive())
    return;

  const EditorLightDrawMode drawMode = (EditorLightDrawMode)editor_light_draw_mode.get();
  if (drawMode == EditorLightDrawMode::None)
    return;

  const WorldRenderer *renderer = get_renderer();
  if (!renderer)
    return;

  const Point3 up = stage_info.viewItm.getcol(1);
  const Point3 look = stage_info.viewItm.getcol(0);

  begin_draw_cached_debug_lines();

  editor_draw_omni_lights_ecs_query([drawMode, renderer, up, look](ecs::EntityId eid, const OmniLightEntity &omni_light) {
    const ecs::Tag *daeditor__selected = ECS_GET_NULLABLE(ecs::Tag, eid, daeditor__selected);
    const bool selected = daeditor__selected != nullptr;
    if (!selected && drawMode == EditorLightDrawMode::Selected)
      return;

    const OmniLightsManager::Light &light = renderer->getOmniLight(omni_light.lightId);
    const float lightRadius = light.posRelToOrigin_cullRadius.w > 0.0f ? light.posRelToOrigin_cullRadius.w : light.pos_radius.w;
    if (lightRadius <= 0.0f)
      return;

    draw_cached_debug_circle(Point3(light.pos_radius.x, light.pos_radius.y, light.pos_radius.z), up, look, lightRadius,
      E3DCOLOR(0, 255, 255));
  });

  end_draw_cached_debug_lines();
}

template <typename Callable>
static void editor_draw_spot_lights_ecs_query(Callable c);

ECS_NO_ORDER
ECS_TAG(render, dev)
static __forceinline void editor_draw_spot_light_es(const UpdateStageInfoRenderDebug &)
{
  if (!get_da_editor4().isActive())
    return;

  const EditorLightDrawMode drawMode = (EditorLightDrawMode)editor_light_draw_mode.get();
  if (drawMode == EditorLightDrawMode::None)
    return;

  const WorldRenderer *renderer = get_renderer();
  if (!renderer)
    return;

  begin_draw_cached_debug_lines();

  editor_draw_spot_lights_ecs_query([drawMode, renderer](ecs::EntityId eid, const SpotLightEntity &spot_light) {
    const ecs::Tag *daeditor__selected = ECS_GET_NULLABLE(ecs::Tag, eid, daeditor__selected);
    const bool selected = daeditor__selected != nullptr;
    if (!selected && drawMode == EditorLightDrawMode::Selected)
      return;

    const SpotLightsManager::Light &light = renderer->getSpotLight(spot_light.lightId);
    const float lightRadius = light.culling_radius > 0.0f ? light.culling_radius : light.pos_radius.w;
    if (lightRadius <= 0.0f)
      return;

    const Point3 pos(light.pos_radius.x, light.pos_radius.y, light.pos_radius.z);
    const Point3 dir(light.dir_tanHalfAngle.x, light.dir_tanHalfAngle.y, light.dir_tanHalfAngle.z);
    const float outerConeHalfAngle = atanf(light.dir_tanHalfAngle.w);
    draw_cached_debug_cone(pos, pos + dir * lightRadius, outerConeHalfAngle, E3DCOLOR(255, 0, 255));

    const float innerConeHalfAngle = acosf(light.color_atten.a);
    if (innerConeHalfAngle > 0.0f && innerConeHalfAngle < DegToRad(45.0f))
    {
      const Point3 normal = normalize(cross(Point3(0.0f, 1.0f, 0.0f), dir));
      const float innerConeRadius = lightRadius * tanf(innerConeHalfAngle);
      draw_cached_debug_circle(pos + dir * lightRadius, normal, normalize(cross(dir, normal)), innerConeRadius, E3DCOLOR(64, 0, 192));
    }
  });

  end_draw_cached_debug_lines();
}
