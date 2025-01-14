// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER 1

#include "lensFlareRenderer.h"

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/componentTypes.h>
#include <render/daBfg/nodeHandle.h>
#include <render/daBfg/ecs/frameGraphNode.h>
#include <ecs/render/updateStageRender.h>
#include <render/renderEvent.h>
#include <EASTL/vector_set.h>
#include <generic/dag_enumerate.h>
#include <render/renderSettings.h>

#include "lensFlareNodes.h"

ECS_REGISTER_BOXED_TYPE(LensFlareRenderer, nullptr);

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void lens_flare_renderer_on_appear_es(const ecs::Event &, LensFlareRenderer &lens_flare_renderer)
{
  lens_flare_renderer.init();
}

template <typename Callable>
static void init_lens_flare_nodes_ecs_query(Callable);

ECS_TAG(render)
ECS_ON_EVENT(OnRenderSettingsReady)
ECS_TRACK(render_settings__lensFlares)
static void lens_flare_renderer_on_settings_changed_es(const ecs::Event &, bool render_settings__lensFlares)
{
  init_lens_flare_nodes_ecs_query(
    [render_settings__lensFlares](LensFlareRenderer &lens_flare_renderer, dabfg::NodeHandle &lens_flare_renderer__render_node,
      dabfg::NodeHandle &lens_flare_renderer__prepare_lights_node) {
      if (render_settings__lensFlares)
      {
        LensFlareQualityParameters quality = {0.5f};
        lens_flare_renderer__render_node = create_lens_flare_render_node(&lens_flare_renderer, quality);
        lens_flare_renderer__prepare_lights_node = create_lens_flare_prepare_lights_node(&lens_flare_renderer);
      }
      else
      {
        lens_flare_renderer__render_node = {};
        lens_flare_renderer__prepare_lights_node = {};
      }
    });
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_TRACK(sun_flare_provider__flare_config)
ECS_REQUIRE(const ecs::string &sun_flare_provider__flare_config)
static void sun_flare_provider_invalidate_es(
  const ecs::Event &, int &sun_flare_provider__cached_id, int &sun_flare_provider__cached_flare_config_id)
{
  // Invalidate flare config id cache if the config name changes
  LensFlareRenderer::CachedFlareId invalidId = {};
  sun_flare_provider__cached_id = invalidId.cacheId;
  sun_flare_provider__cached_flare_config_id = invalidId.flareConfigId;
}

ECS_ON_EVENT(on_appear)
ECS_TRACK(dynamic_light_lens_flare__flare_config)
ECS_REQUIRE(const ecs::string &dynamic_light_lens_flare__flare_config)
static void point_light_flare_provider_invalidate_es(
  const ecs::Event &, int &dynamic_light_lens_flare__cached_id, int &dynamic_light_lens_flare__cached_flare_config_id)
{
  // Invalidate flare config id cache if the config name changes
  LensFlareRenderer::CachedFlareId invalidId = {};
  dynamic_light_lens_flare__cached_id = invalidId.cacheId;
  dynamic_light_lens_flare__cached_flare_config_id = invalidId.flareConfigId;
}

template <typename Callable>
static void prepare_sun_flares_ecs_query(Callable);
template <typename Callable>
static void prepare_point_light_flares_ecs_query(Callable);

void LensFlareRenderer::collectAndPrepareECSFlares(
  const Point3 &camera_dir, const TMatrix4 &view_proj, const Point3 &sun_dir, const Point3 &sun_color)
{
  if (lensFlares.empty())
    return; // if there are no lens flare configs, no need to look for entities that use them

  prepare_sun_flares_ecs_query([&, this](bool enabled, const ecs::string &sun_flare_provider__flare_config,
                                 int &sun_flare_provider__cached_id, int &sun_flare_provider__cached_flare_config_id) {
    if (!enabled)
      return;

    if (dot(camera_dir, Point3::xyz(sun_dir)) <= 0)
      return;
    Point4 projectedLightPos = Point4::xyz0(sun_dir) * view_proj;
    if (fabsf(projectedLightPos.w) < 0.0000001f)
      return;
    projectedLightPos /= projectedLightPos.w;
    if (max(fabsf(projectedLightPos.x), fabsf(projectedLightPos.y)) >= 1.f)
      return;

    CachedFlareId cachedFlareId = {sun_flare_provider__cached_id, sun_flare_provider__cached_flare_config_id};
    if (!isCachedFlareIdValid(cachedFlareId))
    {
      cachedFlareId = cacheFlareId(sun_flare_provider__flare_config.c_str());
      sun_flare_provider__cached_id = cachedFlareId.cacheId;
      sun_flare_provider__cached_flare_config_id = cachedFlareId.flareConfigId;
    }
    if (cachedFlareId.isValid())
      prepareManualFlare(cachedFlareId, Point4::xyz0(sun_dir), sun_color, true);
  });
  prepare_point_light_flares_ecs_query(
    [&, this](const ecs::string &dynamic_light_lens_flare__flare_config, int &dynamic_light_lens_flare__cached_id,
      int &dynamic_light_lens_flare__cached_flare_config_id) {
      CachedFlareId cachedFlareId = {dynamic_light_lens_flare__cached_id, dynamic_light_lens_flare__cached_flare_config_id};
      if (!isCachedFlareIdValid(cachedFlareId))
      {
        cachedFlareId = cacheFlareId(dynamic_light_lens_flare__flare_config.c_str());
        dynamic_light_lens_flare__cached_id = cachedFlareId.cacheId;
        dynamic_light_lens_flare__cached_flare_config_id = cachedFlareId.flareConfigId;
      }
      if (cachedFlareId.isValid())
        prepareDynamicLightFlares(cachedFlareId);
    });
}

template <typename Callable>
static void gather_flare_configs_ecs_query(Callable);

static const eastl::vector_set<eastl::string> ACCEPTED_FLARE_COMPONENT_PROPS = {"flare_component__enabled",
  "flare_component__gradient__falloff", "flare_component__gradient__gradient", "flare_component__gradient__inverted",
  "flare_component__radial_distortion__enabled", "flare_component__radial_distortion__relative_to_center",
  "flare_component__radial_distortion__distortion_curve_pow", "flare_component__radial_distortion__radial_edge_size",
  "flare_component__scale", "flare_component__offset", "flare_component__axis_offset", "flare_component__auto_rotation",
  "flare_component__intensity", "flare_component__roundness", "flare_component__side_count", "flare_component__tint",
  "flare_component__use_light_color", "flare_component__texture", "flare_component__rotation_offset",
  "flare_component__pre_rotation_offset"};

void LensFlareRenderer::updateConfigsFromECS()
{
  eastl::vector<LensFlareConfig> configs;
  gather_flare_configs_ecs_query(
    [&configs](const ecs::string &lens_flare_config__name, const float &lens_flare_config__smooth_screen_fadeout_distance,
      const Point2 &lens_flare_config__scale, const float &lens_flare_config__intensity, bool lens_flare_config__use_occlusion,
      const float &lens_flare_config__exposure_reduction, const ecs::Array &lens_flare_config__elements) {
      LensFlareConfig config;
      config.name = lens_flare_config__name;
      config.useOcclusion = lens_flare_config__use_occlusion;
      config.intensity = lens_flare_config__intensity;
      config.smoothScreenFadeoutDistance = lens_flare_config__smooth_screen_fadeout_distance;
      config.exposureReduction = lens_flare_config__exposure_reduction;
      config.scale = lens_flare_config__scale;
      config.elements.reserve(lens_flare_config__elements.size());
      for (const auto &[elementInd, element] : enumerate(lens_flare_config__elements))
      {
        ecs::Object configObj = element.get<ecs::Object>();
        int numComponents = configObj.size();
        int componentsHandled = 0;

#define SET_ELEMENT_CONFIG_PROP(name, T, prop)                                                                        \
  if (auto value = configObj.getNullable<T>(ECS_HASH(name)))                                                          \
  {                                                                                                                   \
    componentsHandled++;                                                                                              \
    prop = *value;                                                                                                    \
  }                                                                                                                   \
  else if (configObj.hasMember(ECS_HASH(name)))                                                                       \
  {                                                                                                                   \
    logerr("Wrong type for lens flare component (flare config / component index / component property): %s / %d / %s", \
      lens_flare_config__name, elementInd, name);                                                                     \
  }

        bool componentEnabled = true;
        SET_ELEMENT_CONFIG_PROP("flare_component__enabled", bool, componentEnabled)
        if (!componentEnabled)
          continue;

        LensFlareConfig::Element elementConfig;
        SET_ELEMENT_CONFIG_PROP("flare_component__gradient__falloff", float, elementConfig.gradient.falloff)
        SET_ELEMENT_CONFIG_PROP("flare_component__gradient__gradient", float, elementConfig.gradient.gradient)
        SET_ELEMENT_CONFIG_PROP("flare_component__gradient__inverted", bool, elementConfig.gradient.inverted)
        SET_ELEMENT_CONFIG_PROP("flare_component__radial_distortion__enabled", bool, elementConfig.radialDistortion.enabled)
        SET_ELEMENT_CONFIG_PROP("flare_component__radial_distortion__relative_to_center", bool,
          elementConfig.radialDistortion.relativeToCenter)
        SET_ELEMENT_CONFIG_PROP("flare_component__radial_distortion__distortion_curve_pow", float,
          elementConfig.radialDistortion.distortionCurvePow)
        SET_ELEMENT_CONFIG_PROP("flare_component__radial_distortion__radial_edge_size", Point2,
          elementConfig.radialDistortion.radialEdgeSize)
        SET_ELEMENT_CONFIG_PROP("flare_component__scale", Point2, elementConfig.scale)
        SET_ELEMENT_CONFIG_PROP("flare_component__offset", Point2, elementConfig.offset)
        SET_ELEMENT_CONFIG_PROP("flare_component__axis_offset", float, elementConfig.axisOffset)
        SET_ELEMENT_CONFIG_PROP("flare_component__auto_rotation", bool, elementConfig.autoRotation)
        SET_ELEMENT_CONFIG_PROP("flare_component__intensity", float, elementConfig.intensity)
        SET_ELEMENT_CONFIG_PROP("flare_component__roundness", float, elementConfig.roundness)
        SET_ELEMENT_CONFIG_PROP("flare_component__side_count", int, elementConfig.sideCount)
        SET_ELEMENT_CONFIG_PROP("flare_component__tint", Point3, elementConfig.tint)
        SET_ELEMENT_CONFIG_PROP("flare_component__use_light_color", bool, elementConfig.useLightColor)
        SET_ELEMENT_CONFIG_PROP("flare_component__texture", ecs::string, elementConfig.texture)
        float rotationOffsetDeg = RAD_TO_DEG * elementConfig.rotationOffset;
        SET_ELEMENT_CONFIG_PROP("flare_component__rotation_offset", float, rotationOffsetDeg)
        elementConfig.rotationOffset = DEG_TO_RAD * rotationOffsetDeg;
        float preRotationOffsetDeg = RAD_TO_DEG * elementConfig.preRotationOffset;
        SET_ELEMENT_CONFIG_PROP("flare_component__pre_rotation_offset", float, preRotationOffsetDeg)
        elementConfig.preRotationOffset = DEG_TO_RAD * preRotationOffsetDeg;

#undef SET_ELEMENT_CONFIG_PROP

        if (componentsHandled != numComponents)
        {
          for (const auto &property : configObj)
            if (ACCEPTED_FLARE_COMPONENT_PROPS.count(property.first) == 0)
              logerr("Unknown lens flare component (flare config / component index / component property): %s / %d / %s",
                lens_flare_config__name, elementInd, property.first);
        }

        config.elements.push_back(eastl::move(elementConfig));
      }
      if (config.elements.size() > 0)
        configs.push_back(eastl::move(config));
    });
  prepareConfigBuffers(eastl::move(configs));
}

template <typename Callable>
static void schedule_update_flares_renderer_ecs_query(Callable);

static void schedule_flares_update()
{
  schedule_update_flares_renderer_ecs_query([](LensFlareRenderer &lens_flare_renderer) { lens_flare_renderer.markConfigsDirty(); });
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_REQUIRE(const ecs::string &lens_flare_config__name,
  const float &lens_flare_config__smooth_screen_fadeout_distance,
  const float &lens_flare_config__exposure_reduction,
  const Point2 &lens_flare_config__scale,
  const float &lens_flare_config__intensity,
  bool lens_flare_config__use_occlusion,
  const ecs::Array &lens_flare_config__elements)
ECS_TRACK(lens_flare_config__name,
  lens_flare_config__smooth_screen_fadeout_distance,
  lens_flare_config__exposure_reduction,
  lens_flare_config__scale,
  lens_flare_config__intensity,
  lens_flare_config__use_occlusion,
  lens_flare_config__elements)
static void lens_flare_config_on_appear_es(const ecs::Event &) { schedule_flares_update(); }

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(const ecs::string &lens_flare_config__name)
static void lens_flare_config_on_disappear_es(const ecs::Event &) { schedule_flares_update(); }

ECS_TAG(render)
ECS_REQUIRE(const LensFlareRenderer &lens_flare_renderer)
static void register_lens_flare_for_postfx_es(const RegisterPostfxResources &evt)
{
  (evt.get<0>().root() / "lens_flare")
    .readTexture("lens_flares")
    .atStage(dabfg::Stage::PS)
    .bindToShaderVar("lens_flare_tex")
    .optional();
  (evt.get<0>().root() / "lens_flare").readBlob<int>("has_lens_flares").optional().bindToShaderVar("lens_flares_enabled");
}
