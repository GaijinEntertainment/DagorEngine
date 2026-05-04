// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/lights/light.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <ecs/lights/lightHandle.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/component.h>
#include <daECS/core/componentsMap.h>
#include <daECS/core/entityComponent.h>
#include <math/dag_color.h>
#include <render/omniLight.h>
#include <render/spotLight.h>
#include <ecs/anim/anim.h>
#include "render/omniLightsManager.h"
#include <render/clusteredLights.h>
#include <render/lightShadowParams.h>
#include <render/light_consts.hlsli>
#include <daEditorE/editorCommon/inGameEditor.h>
#include <debug/dag_debug3d.h>

#include "ecs/render/updateStageRender.h"
#include "util/dag_convar.h"


const TMatrix &get_cam_itm();

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


enum class EditorLightDrawMode
{
  None,
  All,
  Selected,

  FirstValue = None,
  LastValue = Selected,
};

static ConVarT<bool, false> editor_draw_light_restriction_box("light.editor_draw_restriction_box", false,
  "turn on resriction boxes for lights");

static ConVarT<int, true> editor_light_draw_mode("light.editor_draw", (int)EditorLightDrawMode::None,
  (int)EditorLightDrawMode::FirstValue, (int)EditorLightDrawMode::LastValue, "0 = none, 1 = all lights, 2 = selected lights");


ECS_REGISTER_EVENT(CmdUpdateHighPriorityLights)
ECS_REGISTER_EVENT(CmdRecreateSpotLights)
ECS_REGISTER_EVENT(CmdRecreateOmniLights)
ECS_REGISTER_EVENT(CmdRecreateAllLights)

void LightHandleDeleter::operator()(int h) const { lights_impl::destroy_light(h); }

bool OmniLightEntity::ensureLightIdCreated(bool should_create)
{
  if (!should_create)
  {
    lightId.reset();
    return false;
  }

  if (lightId)
    return true;

  lightId = lights_impl::add_omni_light(OmniLight::create_empty(), OmniLightMaskType::OMNI_LIGHT_MASK_DEFAULT);

  return lightId.valid();
}

ECS_REGISTER_RELOCATABLE_TYPE(OmniLightEntity, nullptr);
ECS_AUTO_REGISTER_COMPONENT(OmniLightEntity, "omni_light", nullptr, 0);

bool SpotLightEntity::ensureLightIdCreated(bool should_create)
{
  if (!should_create)
  {
    lightId.reset();
    return false;
  }

  if (lightId)
    return true;

  lightId = lights_impl::add_spot_light(SpotLight::create_empty(), SpotLightMaskType::SPOT_LIGHT_MASK_DEFAULT);

  return lightId.valid();
}

ECS_REGISTER_RELOCATABLE_TYPE(SpotLightEntity, nullptr);
ECS_AUTO_REGISTER_COMPONENT(SpotLightEntity, "spot_light", nullptr, 0);


template <typename LightType>
static bool is_light_visible(const LightType &light)
{
  if (!light.lightId)
    return false;

  return lights_impl::is_light_visible(light.lightId.get());
}

template <typename Callable>
static void set_omni_lights_ecs_query(ecs::EntityManager &manager, Callable c);

void set_up_omni_lights()
{
  if (!lights_impl::is_lights_render_available())
    return;

  set_omni_lights_ecs_query(*g_entity_mgr,
    [&](const OmniLightEntity &omni_light,
      TMatrix &light__box ECS_REQUIRE(eastl::true_type light__use_box) ECS_REQUIRE(eastl::true_type light__automatic_box)) {
      if (!omni_light.lightId)
        return;

      OmniLight light = lights_impl::get_omni_light(omni_light.lightId.get());

      if (lights_impl::get_box_around(Point3{light.pos_radius.x, light.pos_radius.y, light.pos_radius.z}, light__box)) //-V1051
      {
        light.setBox(light__box);
        lights_impl::set_omni_light(omni_light.lightId.get(), light, false);
      }
    });
}

enum class LightUseBox
{
  Off = 0,
  On = 1
};
enum class LightUseClip
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

static void validate_max_light_radius(bool &light__force_max_light_radius, float light__max_radius)
{
  if (light__force_max_light_radius && light__max_radius <= 0)
  {
    light__force_max_light_radius = false;
    logerr("Light has no max radius set, but force_max_light_radius is set to true.\n"
           "Force max light flag reseted to false");
  }
}

static __forceinline void omni_light_set(OmniLightEntity &omni_light, const TMatrix &transform, LightUseBox light__use_box,
  LightUseClip light__use_clip, bool &light__automatic_box, bool &light__force_max_light_radius, TMatrix &light__box,
  const Point3 &light__offset, const Point3 &light__direction, const ecs::string &light__texture_name, const E3DCOLOR light__color,
  float light__brightness, LightHasShadows omni_light__shadows, LightHasDynamicCasters omni_light__dynamic_obj_shadows,
  int omni_light__shadow_quality, int omni_light__priority, int omni_light__shadow_shrink, float light__radius_scale,
  float light__max_radius, Point2 omni_light__shadow_near_far_planes,
  LightDynamicLight light__dynamic_light, // TODO: use a shared one with spot lights
  LightContactShadows light__contact_shadows, LightApproximateStatic light__approximate_static_shadows,
  LightAffectVolumes light__affect_volumes, LightRenderGpuObjects light__render_gpu_objects, LightLensFlares light__enable_lens_flares,
  bool light__shadow_two_sided, bool light__force_affect_volfog)
{
  if (!lights_impl::is_lights_render_available())
    return;
  if (!omni_light.lightId)
    return;

  LightShadowParams shadowParams;
  shadowParams.isEnabled = bool(omni_light__shadows);
  shadowParams.approximateStatic = bool(light__approximate_static_shadows);
  shadowParams.isDynamic = bool(light__dynamic_light);
  shadowParams.supportsDynamicObjects = lights_impl::get_feature_dynamic_objects() ? bool(omni_light__dynamic_obj_shadows) : false;
  shadowParams.supportsGpuObjects = lights_impl::get_feature_gpu_objects() ? bool(light__render_gpu_objects) : false;
  shadowParams.quality = omni_light__shadow_quality;
  shadowParams.isTwoSided = light__shadow_two_sided;
  shadowParams.shadowShrink = omni_light__shadow_shrink;
  shadowParams.priority = omni_light__priority;
  bool contact_shadows = lights_impl::get_feature_contact_shadows() ? bool(light__contact_shadows) : false;

  lights_impl::update_light_shadow(omni_light.lightId.get(), shadowParams);
  Color3 color = color3(light__color) * light__brightness;

  const float dropOffLuminance = 0.05; // 5% of the original luminance
  const float baseLuminance = average(color);
  // This is the distance where most of the luminance (95%) drops off due to inverse square law.
  // Use the light__radius_scale to adjust the drop-off radius
  float autoDropOffRadius = sqrtf(max(0.0f, baseLuminance / dropOffLuminance)) * light__radius_scale;
  if (light__max_radius > 0)
    autoDropOffRadius = min(light__max_radius, autoDropOffRadius);

  validate_max_light_radius(light__force_max_light_radius, light__max_radius);

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

    if (light__use_clip != LightUseClip::Off)
      lights_impl::get_clip_plane_around(position, light__box);
  }
  else
  {
    if (lengthSq(light__box.getcol(0)) == 0.f)
      light__automatic_box = true;
    if (light__automatic_box)
      lights_impl::get_box_around(position, light__box);
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
  color = bool(omni_light__shadows) ? color : -color;
  OmniLight omniLight = OmniLight(position, direction, color, radius, bool(contact_shadows) ? 1.0f : 0.f, tex_idx, data.zoom,
    data.rotated, light__box, omni_light__shadow_near_far_planes);
  OmniLightMaskType mask = OmniLightMaskType::OMNI_LIGHT_MASK_NONE;
  if (light__dynamic_light == LightDynamicLight::Off && light__affect_volumes == LightAffectVolumes::On)
  {
    mask |= OmniLightMaskType::OMNI_LIGHT_MASK_VOLFOG; //-V1016
    mask |= OmniLightMaskType::OMNI_LIGHT_MASK_GI;     //-V1016
  }
  if (light__force_affect_volfog)
    mask |= OmniLightMaskType::OMNI_LIGHT_MASK_VOLFOG; //-V1016
  if (light__enable_lens_flares == LightLensFlares::On)
    mask |= OmniLightMaskType::OMNI_LIGHT_MASK_LENS_FLARE; //-V1016
  lights_impl::set_omni_light_with_mask(omni_light.lightId.get(), eastl::move(omniLight), mask, true);
}

static inline TMatrix light_tm(ecs::EntityId eid, const ecs::EntityManager &manager, const TMatrix *transform,
  const AnimV20::AnimcharBaseComponent *animchar, const TMatrix *light_mod_tm)
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
ECS_ON_EVENT(on_appear, CmdRecreateOmniLights, CmdRecreateAllLights)
ECS_TRACK(*)
static __forceinline void update_omni_light_es(const ecs::Event &, ecs::EntityId eid, const ecs::EntityManager &manager,
  OmniLightEntity &omni_light, bool light__use_box, bool &light__automatic_box, bool &light__force_max_light_radius,
  TMatrix &light__box, const Point3 &light__offset, const Point3 &light__direction, const ecs::string &light__texture_name,
  const E3DCOLOR light__color, float light__brightness, bool omni_light__shadows, bool omni_light__dynamic_obj_shadows,
  bool light__use_clip = false, bool light__nightly = false, int omni_light__shadow_quality = 0, int omni_light__priority = 0,
  int omni_light__shadow_shrink = 0, float light__radius_scale = 1, float light__max_radius = -1,
  Point2 omni_light__shadow_near_far_planes = Point2(0, 0), bool light__dynamic_light = false, bool light__contact_shadows = true,
  bool light__affect_volumes = true, bool light__render_gpu_objects = false, bool light__is_paused = false,
  bool light_switch__on = true, bool light__enable_lens_flares = false, bool light__approximate_static = false,
  bool light__shadow_two_sided = false, bool light__force_affect_volfog = false, const TMatrix *transform = nullptr,
  const AnimV20::AnimcharBaseComponent *animchar = nullptr, const TMatrix *lightModTm = nullptr)
{
  if (!lights_impl::is_lights_render_available())
    return;

  bool shouldCreate = light_switch__on && !light__is_paused && (!light__nightly || lights_impl::is_night_time());
  if (!omni_light.ensureLightIdCreated(shouldCreate))
    return;

  const TMatrix tm = light_tm(eid, manager, transform, animchar, lightModTm);

  omni_light_set(omni_light, tm, LightUseBox(light__use_box), LightUseClip(light__use_clip), light__automatic_box,
    light__force_max_light_radius, light__box, light__offset, light__direction, light__texture_name, light__color, light__brightness,
    LightHasShadows(omni_light__shadows), LightHasDynamicCasters(omni_light__dynamic_obj_shadows), omni_light__shadow_quality,
    omni_light__priority, omni_light__shadow_shrink, light__radius_scale, light__max_radius, omni_light__shadow_near_far_planes,
    LightDynamicLight(light__dynamic_light), LightContactShadows(light__contact_shadows),
    LightApproximateStatic(light__approximate_static), LightAffectVolumes(light__affect_volumes),
    LightRenderGpuObjects(light__render_gpu_objects), LightLensFlares(light__enable_lens_flares), light__shadow_two_sided,
    light__force_affect_volfog);
}

ECS_TAG(render)
ECS_ON_EVENT(CmdUpdateHighPriorityLights)
ECS_REQUIRE(ecs::Tag light__high_priority_update)
ECS_TRACK(*)
static __forceinline void update_high_priority_omni_light_es(const ecs::Event &, ecs::EntityId eid, const ecs::EntityManager &manager,
  OmniLightEntity &omni_light, bool light__use_box, bool light__use_clip, bool &light__automatic_box,
  bool &light__force_max_light_radius, TMatrix &light__box, const Point3 &light__offset, const Point3 &light__direction,
  const ecs::string &light__texture_name, const E3DCOLOR light__color, float light__brightness, bool omni_light__shadows,
  bool omni_light__dynamic_obj_shadows, int omni_light__shadow_quality = 0, int omni_light__priority = 0,
  int omni_light__shadow_shrink = 0, float light__radius_scale = 1, float light__max_radius = -1,
  Point2 omni_light__shadow_near_far_planes = Point2(0, 0), bool light__dynamic_light = false, bool light__contact_shadows = true,
  bool light__affect_volumes = true, bool light__render_gpu_objects = false, bool light__enable_lens_flares = false,
  bool light__approximate_static = false, bool light__shadow_two_sided = false, bool light__force_affect_volfog = false,
  const TMatrix *transform = nullptr, const AnimV20::AnimcharBaseComponent *animchar = nullptr, const TMatrix *lightModTm = nullptr)
{
  const TMatrix tm = light_tm(eid, manager, transform, animchar, lightModTm);

  omni_light_set(omni_light, tm, LightUseBox(light__use_box), LightUseClip(light__use_clip), light__automatic_box,
    light__force_max_light_radius, light__box, light__offset, light__direction, light__texture_name, light__color, light__brightness,
    LightHasShadows(omni_light__shadows), LightHasDynamicCasters(omni_light__dynamic_obj_shadows), omni_light__shadow_quality,
    omni_light__priority, omni_light__shadow_shrink, light__radius_scale, light__max_radius, omni_light__shadow_near_far_planes,
    LightDynamicLight(light__dynamic_light), LightContactShadows(light__contact_shadows),
    LightApproximateStatic(light__approximate_static), LightAffectVolumes(light__affect_volumes),
    LightRenderGpuObjects(light__render_gpu_objects), LightLensFlares(light__enable_lens_flares), light__shadow_two_sided,
    light__force_affect_volfog);
}

ECS_TAG(render)
ECS_REQUIRE_NOT(const TMatrix &transform)
ECS_REQUIRE_NOT(const bool &light__use_box)
ECS_REQUIRE_NOT(const bool &light__use_clip)
ECS_NO_ORDER
static __forceinline void omni_light_es(const ecs::UpdateStageInfoAct &, OmniLightEntity &omni_light,
  AnimV20::AnimcharBaseComponent &animchar, const TMatrix &lightModTm, const Point3 &light__offset, const Point3 &light__direction,
  const ecs::string &light__texture_name, bool &light__force_max_light_radius, const E3DCOLOR light__color, float light__brightness,
  bool omni_light__shadows, bool omni_light__dynamic_obj_shadows, int omni_light__shadow_quality = 0, int omni_light__priority = 0,
  int omni_light__shadow_shrink = 0, float light__radius_scale = 1, float light__max_radius = -1,
  Point2 omni_light__shadow_near_far_planes = Point2(0, 0), bool light__dynamic_light = false, bool light__contact_shadows = true,
  bool animchar_render__enabled = true, bool light__affect_volumes = true, bool light__render_gpu_objects = false,
  bool light__enable_lens_flares = false, bool light__approximate_static = false, bool light__shadow_two_sided = false,
  bool light__force_affect_volfog = false)
{
  TMatrix transform = TMatrix::IDENT;
  animchar.getTm(transform);
  transform = transform * lightModTm;
  TMatrix emptyBoxDummy = TMatrix::ZERO;
  bool isAutomaticBoxDummy = false;
  omni_light_set(omni_light, transform, LightUseBox::Off, LightUseClip::On, isAutomaticBoxDummy, light__force_max_light_radius,
    emptyBoxDummy, light__offset, light__direction, light__texture_name, light__color,
    animchar_render__enabled ? light__brightness : 0.f, LightHasShadows(omni_light__shadows),
    LightHasDynamicCasters(omni_light__dynamic_obj_shadows), omni_light__shadow_quality, omni_light__priority,
    omni_light__shadow_shrink, light__radius_scale, light__max_radius, omni_light__shadow_near_far_planes,
    LightDynamicLight(light__dynamic_light), LightContactShadows(light__contact_shadows),
    LightApproximateStatic(light__approximate_static), LightAffectVolumes(light__affect_volumes),
    LightRenderGpuObjects(light__render_gpu_objects), LightLensFlares(light__enable_lens_flares), light__shadow_two_sided,
    light__force_affect_volfog);
}

static __forceinline void spot_light_set(SpotLightEntity &spot_light, const TMatrix &transform, const Point3 &light__offset,
  const ecs::string &light__texture_name, const E3DCOLOR light__color, float light__brightness,
  LightDynamicLight spot_light__dynamic_light, LightHasShadows spot_light__shadows,
  LightHasDynamicCasters spot_light__dynamic_obj_shadows, bool &light__force_max_light_radius, int spot_light__shadow_quality,
  int spot_light__priority, int spot_light__shadow_shrink, float spot_light__cone_angle, float spot_light__illuminating_disc_radius,
  float spot_light__inner_attenuation, float spot_light__shadow_cone_angle, float spot_light__shadow_frustum_offset,
  Point2 spot_light__shadow_near_far_planes, float light__radius_scale, float light__max_radius,
  LightAffectVolumes light__affect_volumes, LightContactShadows spot_light__contact_shadows,
  LightApproximateStatic light__approximate_static_shadows, LightRenderGpuObjects light__render_gpu_objects,
  LightLensFlares light__enable_lens_flares, bool light__shadow_two_sided, bool light__force_affect_volfog)
{
  if (!lights_impl::is_lights_render_available())
    return;
  if (!spot_light.lightId)
    return;

  LightShadowParams shadowParams;
  shadowParams.isEnabled = bool(spot_light__shadows);
  shadowParams.approximateStatic = bool(light__approximate_static_shadows);
  shadowParams.isDynamic = bool(spot_light__dynamic_light);
  shadowParams.supportsDynamicObjects =
    lights_impl::get_feature_dynamic_objects() || bool(spot_light__dynamic_light) ? bool(spot_light__dynamic_obj_shadows) : false;
  shadowParams.supportsGpuObjects = lights_impl::get_feature_gpu_objects() ? bool(light__render_gpu_objects) : false;
  shadowParams.quality = spot_light__shadow_quality;
  shadowParams.isTwoSided = light__shadow_two_sided;
  shadowParams.shadowShrink = spot_light__shadow_shrink;
  shadowParams.priority = spot_light__priority;
  bool contact_shadows = lights_impl::get_feature_contact_shadows() ? bool(spot_light__contact_shadows) : false;

  lights_impl::update_light_shadow(spot_light.lightId.get(), shadowParams);
  Color3 color = color3(light__color) * light__brightness;

  float auto_radius = light__radius_scale * rgbsum(color) * (4.47f / 3); // 4.47 = 1/sqrt(0.05)/ (3 for rgbsum), i.e. 95% of brightness
                                                                         // has already lost due to inverse squares law. If light is in
                                                                         // very dark place, it can be increased by scale
  if (light__max_radius > 0)
    auto_radius = min(light__max_radius, auto_radius);

  validate_max_light_radius(light__force_max_light_radius, light__max_radius);

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
  const float illuminatingDiscOffset = safediv(spot_light__illuminating_disc_radius, halfTan);
  constexpr float EPS = 1e-6;
  const Point3 worldUpDir(0.f, 1.f, 0.f);
  const Point3 direction = col_length > EPS ? transform.getcol(2) / col_length : worldUpDir;
  const Point3 coneApexPosistion = transform * light__offset - direction * illuminatingDiscOffset;
  SpotLight spotLight = SpotLight(coneApexPosistion, color, light__brightness > 0 ? radius : -1, attCos, direction,
    transform.getcol(1), halfTan, bool(contact_shadows) && bool(spot_light__shadows), bool(spot_light__shadows), tex_idx, data.zoom,
    data.rotated, illuminatingDiscOffset);
  spotLight.shadowNearFarClippingPlanes = spot_light__shadow_near_far_planes;
  spotLight.shadowTanHalfAngle = spot_light__shadow_cone_angle > 0.f ? tanf(spot_light__shadow_cone_angle * PI / 180.f / 2.f) : -1.f;
  spotLight.shadowFrustumOffset = spot_light__shadow_frustum_offset;
  SpotLightMaskType mask = SpotLightMaskType::SPOT_LIGHT_MASK_NONE;
  if (spot_light__dynamic_light == LightDynamicLight::Off && light__affect_volumes == LightAffectVolumes::On)
  {
    mask |= SpotLightMaskType::SPOT_LIGHT_MASK_GI;     //-V1016
    mask |= SpotLightMaskType::SPOT_LIGHT_MASK_VOLFOG; //-V1016
  }
  if (light__force_affect_volfog)
    mask |= SpotLightMaskType::SPOT_LIGHT_MASK_VOLFOG; //-V1016
  if (light__enable_lens_flares == LightLensFlares::On)
    mask |= SpotLightMaskType::SPOT_LIGHT_MASK_LENS_FLARE; //-V1016
  lights_impl::set_spot_light(spot_light.lightId.get(), eastl::move(spotLight), mask, true);
}

ECS_TAG(render)
ECS_NO_ORDER
static __forceinline void omni_light_upd_visibility_es(const UpdateStageInfoBeforeRender &, const OmniLightEntity &omni_light,
  bool &light__visible)
{
  light__visible = is_light_visible(omni_light);
}


ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static inline void pause_lights_set_sq_es(const ecs::Event &, float light__render_range_limit, float light__render_range_gap,
  float &light__render_range_limit_start_sq, float &light__render_range_limit_stop_sq)
{
  light__render_range_limit = max(light__render_range_limit, 0.0f);

  light__render_range_limit_start_sq = sqr(light__render_range_limit * (1 - light__render_range_gap));
  light__render_range_limit_stop_sq = sqr(light__render_range_limit * (1 + light__render_range_gap));
}

template <typename Callable>
static inline void pause_lights_ecs_query(ecs::EntityManager &manager, Callable c);

ECS_TAG(render)
ECS_AFTER(after_camera_sync)
static inline void pause_lights_es(const ecs::UpdateStageInfoAct &, ecs::EntityManager &manager)
{
  const TMatrix &camItm = get_cam_itm();
  const Point3 &cameraPos = camItm.getcol(3);

  pause_lights_ecs_query(manager,
    [&cameraPos, &manager](ecs::EntityId eid, float light__render_range_limit_start_sq, float light__render_range_limit_stop_sq,
      const bool &light__is_paused, const TMatrix *transform = nullptr, const AnimV20::AnimcharBaseComponent *animchar = nullptr,
      const TMatrix *lightModTm = nullptr) {
      if (light__render_range_limit_start_sq <= 0)
      {
        if (light__is_paused != false)
          ECS_SET_MGR(manager, eid, light__is_paused, false);
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
      {
        if (light__is_paused != false)
          ECS_SET_MGR(manager, eid, light__is_paused, false);
      }
      else if (distanceSq >= light__render_range_limit_stop_sq)
      {
        if (light__is_paused != false)
          ECS_SET_MGR(manager, eid, light__is_paused, true);
      }
    });
}


ECS_TAG(render)
ECS_ON_EVENT(on_appear, CmdRecreateSpotLights, CmdRecreateAllLights)
ECS_TRACK(*)
static void update_spot_light_es(const ecs::Event &, ecs::EntityId eid, const ecs::EntityManager &manager, SpotLightEntity &spot_light,
  const Point3 &light__offset, const ecs::string &light__texture_name, const E3DCOLOR light__color, float light__brightness,
  bool spot_light__dynamic_light, bool spot_light__shadows, bool spot_light__dynamic_obj_shadows, bool &light__force_max_light_radius,
  bool light__nightly = false, int spot_light__shadow_quality = 0, int spot_light__priority = 0, int spot_light__shadow_shrink = 0,
  float spot_light__cone_angle = -1.0, float spot_light__illuminating_disc_radius = 0.0, float spot_light__inner_attenuation = 0.95,
  float spot_light__shadow_cone_angle = -1.f, float spot_light__shadow_frustum_offset = 0.f,
  Point2 spot_light__shadow_near_far_planes = Point2::ZERO, float light__radius_scale = 1, float light__max_radius = -1,
  bool light__affect_volumes = true, bool spot_light__contact_shadows = false, bool light__render_gpu_objects = false,
  bool light__is_paused = false, bool light_switch__on = true, const TMatrix *transform = nullptr,
  const AnimV20::AnimcharBaseComponent *animchar = nullptr, const TMatrix *lightModTm = nullptr,
  bool light__enable_lens_flares = false, bool light__approximate_static = false, bool light__shadow_two_sided = false,
  bool light__force_affect_volfog = false)
{
  if (!lights_impl::is_lights_render_available())
    return;

  bool shouldCreate = light_switch__on && !light__is_paused && (!light__nightly || lights_impl::is_night_time());
  if (!spot_light.ensureLightIdCreated(shouldCreate))
    return;

  const TMatrix tm = light_tm(eid, manager, transform, animchar, lightModTm);

  spot_light_set(spot_light, tm, light__offset, light__texture_name, light__color, light__brightness,
    LightDynamicLight(spot_light__dynamic_light), LightHasShadows(spot_light__shadows),
    LightHasDynamicCasters(spot_light__dynamic_obj_shadows), light__force_max_light_radius, spot_light__shadow_quality,
    spot_light__priority, spot_light__shadow_shrink, spot_light__cone_angle, spot_light__illuminating_disc_radius,
    spot_light__inner_attenuation, spot_light__shadow_cone_angle, spot_light__shadow_frustum_offset,
    spot_light__shadow_near_far_planes, light__radius_scale, light__max_radius, LightAffectVolumes(light__affect_volumes),
    LightContactShadows(spot_light__contact_shadows), LightApproximateStatic(light__approximate_static),
    LightRenderGpuObjects(light__render_gpu_objects), LightLensFlares(light__enable_lens_flares), light__shadow_two_sided,
    light__force_affect_volfog);
}

ECS_TAG(render)
ECS_ON_EVENT(CmdUpdateHighPriorityLights)
ECS_REQUIRE(ecs::Tag light__high_priority_update)
ECS_TRACK(*)
static __forceinline void update_high_priority_spot_light_es(const ecs::Event &, ecs::EntityId eid, const ecs::EntityManager &manager,
  SpotLightEntity &spot_light, const Point3 &light__offset, const ecs::string &light__texture_name, const E3DCOLOR light__color,
  float light__brightness, bool spot_light__dynamic_light, bool spot_light__shadows, bool spot_light__dynamic_obj_shadows,
  bool &light__force_max_light_radius, int spot_light__shadow_quality = 0, int spot_light__priority = 0,
  int spot_light__shadow_shrink = 0, float spot_light__cone_angle = -1.0, float spot_light__illuminating_disc_radius = 0.0,
  float spot_light__inner_attenuation = 0.95, float spot_light__shadow_frustum_offset = 0.f,
  float spot_light__shadow_cone_angle = -1.f, Point2 spot_light__shadow_near_far_planes = Point2::ZERO, float light__radius_scale = 1,
  float light__max_radius = -1, bool light__affect_volumes = true, bool spot_light__contact_shadows = false,
  bool light__render_gpu_objects = false, const TMatrix *transform = nullptr, const AnimV20::AnimcharBaseComponent *animchar = nullptr,
  const TMatrix *lightModTm = nullptr, bool light__enable_lens_flares = false, bool light__approximate_static = false,
  bool light__shadow_two_sided = false, bool light__force_affect_volfog = false)
{
  const TMatrix tm = light_tm(eid, manager, transform, animchar, lightModTm);

  spot_light_set(spot_light, tm, light__offset, light__texture_name, light__color, light__brightness,
    LightDynamicLight(spot_light__dynamic_light), LightHasShadows(spot_light__shadows),
    LightHasDynamicCasters(spot_light__dynamic_obj_shadows), light__force_max_light_radius, spot_light__shadow_quality,
    spot_light__priority, spot_light__shadow_shrink, spot_light__cone_angle, spot_light__illuminating_disc_radius,
    spot_light__inner_attenuation, spot_light__shadow_cone_angle, spot_light__shadow_frustum_offset,
    spot_light__shadow_near_far_planes, light__radius_scale, light__max_radius, LightAffectVolumes(light__affect_volumes),
    LightContactShadows(spot_light__contact_shadows), LightApproximateStatic(light__approximate_static),
    LightRenderGpuObjects(light__render_gpu_objects), LightLensFlares(light__enable_lens_flares), light__shadow_two_sided,
    light__force_affect_volfog);
}

ECS_TAG(render)
ECS_REQUIRE_NOT(const TMatrix &transform)
ECS_NO_ORDER
static __forceinline void spot_light_es(const ecs::UpdateStageInfoAct &, SpotLightEntity &spot_light,
  AnimV20::AnimcharBaseComponent &animchar, const TMatrix &lightModTm, const Point3 &light__offset,
  const ecs::string &light__texture_name, const E3DCOLOR light__color, float light__brightness, bool spot_light__dynamic_light,
  bool spot_light__shadows, bool spot_light__dynamic_obj_shadows, bool &light__force_max_light_radius,
  int spot_light__shadow_quality = 0, int spot_light__priority = 0, int spot_light__shadow_shrink = 0,
  float spot_light__inner_attenuation = 0.95, float spot_light__cone_angle = -1.0, float spot_light__illuminating_disc_radius = 0.0,
  float spot_light__shadow_frustum_offset = 0.f, float spot_light__shadow_cone_angle = -1.f,
  Point2 spot_light__shadow_near_far_planes = Point2::ZERO, float light__radius_scale = 1, float light__max_radius = -1,
  bool animchar_render__enabled = true, bool light__affect_volumes = true, bool spot_light__contact_shadows = false,
  bool light__render_gpu_objects = false, bool light__enable_lens_flares = false, bool light__approximate_static = false,
  bool light__shadow_two_sided = false, bool light__force_affect_volfog = false)
{
  TMatrix transform = TMatrix::IDENT;
  animchar.getTm(transform);
  transform = transform * lightModTm;

  spot_light_set(spot_light, transform, light__offset, light__texture_name, light__color,
    animchar_render__enabled ? light__brightness : 0.f, LightDynamicLight(spot_light__dynamic_light),
    LightHasShadows(spot_light__shadows), LightHasDynamicCasters(spot_light__dynamic_obj_shadows), light__force_max_light_radius,
    spot_light__shadow_quality, spot_light__priority, spot_light__shadow_shrink, spot_light__cone_angle, spot_light__inner_attenuation,
    spot_light__illuminating_disc_radius, spot_light__shadow_cone_angle, spot_light__shadow_frustum_offset,
    spot_light__shadow_near_far_planes, light__radius_scale, light__max_radius, LightAffectVolumes(light__affect_volumes),
    LightContactShadows(spot_light__contact_shadows), LightApproximateStatic(light__approximate_static),
    LightRenderGpuObjects(light__render_gpu_objects), LightLensFlares(light__enable_lens_flares), light__shadow_two_sided,
    light__force_affect_volfog);
}

ECS_TAG(render)
ECS_NO_ORDER
static __forceinline void spot_light_upd_visibility_es(const UpdateStageInfoBeforeRender &, const SpotLightEntity &spot_light,
  bool &light__visible)
{
  light__visible = is_light_visible(spot_light);
}

template <typename Callable>
static void editor_draw_omni_lights_ecs_query(ecs::EntityManager &manager, Callable c);

ECS_NO_ORDER
ECS_TAG(render, dev)
static __forceinline void editor_draw_omni_light_es(const UpdateStageInfoRenderDebug &stage_info, ecs::EntityManager &manager)
{
  if (!get_da_editor4().isActive())
    return;
  if (!lights_impl::is_lights_render_available())
    return;

  const EditorLightDrawMode drawMode = (EditorLightDrawMode)editor_light_draw_mode.get();
  if (drawMode == EditorLightDrawMode::None)
    return;

  const bool drawRestrictionBoxes = editor_draw_light_restriction_box.get();

  const Point3 up = stage_info.viewItm.getcol(1);
  const Point3 look = stage_info.viewItm.getcol(0);

  begin_draw_cached_debug_lines(drawRestrictionBoxes ? false : true); // restriction boxes are hidden inside walls, so we turn off
                                                                      // ztest

  editor_draw_omni_lights_ecs_query(manager,
    [drawMode, drawRestrictionBoxes, up, look](ecs::EntityId eid, const OmniLightEntity &omni_light) {
      const ecs::Tag *daeditor__selected = ECS_GET_NULLABLE(ecs::Tag, eid, daeditor__selected);
      const bool selected = daeditor__selected != nullptr;
      if (!selected && drawMode == EditorLightDrawMode::Selected)
        return;

      if (!omni_light.lightId)
        return;

      const OmniLightsManager::Light light = lights_impl::get_omni_light(omni_light.lightId.get());
      const float lightRadius = light.posRelToOrigin_cullRadius.w > 0.0f ? light.posRelToOrigin_cullRadius.w : light.pos_radius.w;
      if (lightRadius <= 0.0f)
        return;

      if (drawRestrictionBoxes && lengthSq(Point3(light.boxR0.w, light.boxR1.w, light.boxR2.w)) > 0)
      {
        Point3 boxPos = Point3(light.boxR0.w, light.boxR1.w, light.boxR2.w);

        // transpose
        Point3 boxX = Point3(light.boxR0.x, light.boxR1.x, light.boxR2.x);
        Point3 boxY = Point3(light.boxR0.y, light.boxR1.y, light.boxR2.y);
        Point3 boxZ = Point3(light.boxR0.z, light.boxR1.z, light.boxR2.z);

        // restore true size
        boxX /= lengthSq(boxX);
        boxY /= lengthSq(boxY);
        boxZ /= lengthSq(boxZ);

        Point3 corner1 = boxPos - 0.5 * (boxX + boxY + boxZ);

        draw_cached_debug_box(corner1, boxX, boxY, boxZ, E3DCOLOR(255, 0, 0));
      }

      draw_cached_debug_circle(Point3(light.pos_radius.x, light.pos_radius.y, light.pos_radius.z), up, look, lightRadius,
        E3DCOLOR(0, 255, 255));
    });

  end_draw_cached_debug_lines();
}

template <typename Callable>
static void editor_draw_spot_lights_ecs_query(ecs::EntityManager &manager, Callable c);

ECS_NO_ORDER
ECS_TAG(render, dev)
static __forceinline void editor_draw_spot_light_es(const UpdateStageInfoRenderDebug &, ecs::EntityManager &manager)
{
  if (!get_da_editor4().isActive())
    return;
  if (!lights_impl::is_lights_render_available())
    return;

  const EditorLightDrawMode drawMode = (EditorLightDrawMode)editor_light_draw_mode.get();
  if (drawMode == EditorLightDrawMode::None)
    return;

  begin_draw_cached_debug_lines();

  editor_draw_spot_lights_ecs_query(manager, [&](ecs::EntityId eid, const SpotLightEntity &spot_light) {
    const ecs::Tag *daeditor__selected = ECS_GET_NULLABLE(ecs::Tag, eid, daeditor__selected);
    const bool selected = daeditor__selected != nullptr;
    if (!selected && drawMode == EditorLightDrawMode::Selected)
      return;

    if (!spot_light.lightId)
      return;

    const SpotLightsManager::Light light = lights_impl::get_spot_light(spot_light.lightId.get());
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

template <typename Callable>
static void editor_draw_spot_lights_shadow_ecs_query(ecs::EntityManager &manager, Callable c);

ECS_NO_ORDER
ECS_TAG(render, dev)
static __forceinline void editor_draw_spot_light_shadow_es(const UpdateStageInfoRenderDebug &, ecs::EntityManager &manager)
{
  if (!get_da_editor4().isActive())
    return;

  begin_draw_cached_debug_lines(false, false);
  editor_draw_spot_lights_shadow_ecs_query(manager, [&](ECS_REQUIRE(ecs::Tag daeditor__selected) const SpotLightEntity &spot_light) {
    if (!spot_light.lightId)
      return;
    mat44f viewItm, view, proj, viewproj;
    lights_impl::get_spot_light_shadow_view_proj(spot_light.lightId.get(), viewItm, proj);
    v_mat44_orthonormal_inverse43_to44(view, viewItm);
    v_mat44_mul(viewproj, proj, view);
    draw_cached_debug_proj_matrix(reinterpret_cast<const TMatrix4_vec4 &>(viewproj), E3DCOLOR(255, 0, 255), E3DCOLOR(255, 0, 255));
  });
  end_draw_cached_debug_lines();
}
