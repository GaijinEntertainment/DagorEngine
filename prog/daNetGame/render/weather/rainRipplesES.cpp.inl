// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/coreEvents.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <perfMon/dag_statDrv.h>
#include <ecs/render/shaderVar.h>
#include <ecs/render/postfx_renderer.h>
#include <render/renderEvent.h>
#include <render/renderSettings.h>
#include <render/daFrameGraph/ecs/frameGraphNode.h>
#include <render/world/frameGraphHelpers.h>
#include <drv/3d/dag_renderStates.h>

// we use this event localy to reduce amount of code which init/terminate rain ripples and track settings
ECS_BROADCAST_EVENT_TYPE(OnRainToggle, bool /*enabled*/)
ECS_REGISTER_EVENT(OnRainToggle)

ECS_ON_EVENT(on_appear)
ECS_REQUIRE(ecs::Tag rain_tag)
static void rain_ripples_appear_es(const ecs::Event &, ecs::EntityManager &manager) { manager.broadcastEvent(OnRainToggle(true)); }

ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(ecs::Tag rain_tag)
static void rain_ripples_disappear_es(const ecs::Event &, ecs::EntityManager &manager) { manager.broadcastEvent(OnRainToggle(false)); }

ECS_TRACK(*)
ECS_ON_EVENT(OnRainToggle)
static void rain_ripples_toggle_es(const ecs::Event &event,
  const ecs::string &render_settings__fftWaterQuality,
  bool render_settings__fullDeferred,
  bool render_settings__rain_ripples_enabled,
  bool render_settings__bare_minimum,
  ecs::EntityManager &manager)
{
  bool rainEnabled = false;
  if (event.is<OnRainToggle>())
    rainEnabled = static_cast<const OnRainToggle &>(event).get<0>();
  else
    rainEnabled = manager.getSingletonEntity(ECS_HASH("rain_ripples")) != ecs::INVALID_ENTITY_ID;

  bool useRipplePostEffect = rainEnabled && render_settings__rain_ripples_enabled && !render_settings__bare_minimum;
  bool useRippleOnWater = useRipplePostEffect && render_settings__fullDeferred &&
                          (render_settings__fftWaterQuality == "high" || render_settings__fftWaterQuality == "ultra");

  if (!useRipplePostEffect)
    manager.destroyEntity(manager.getSingletonEntity(ECS_HASH("rain_ripples")));
  else
    manager.getOrCreateSingletonEntity(ECS_HASH("rain_ripples"));

  ShaderGlobal::set_int(get_shader_glob_var_id("water_rain_ripples_enabled", true), useRippleOnWater);
}

template <typename Callable>
static void get_rain_ripple_renderer_ecs_query(ecs::EntityManager &manager, Callable c);

ECS_TAG(render)
ECS_ON_EVENT(on_appear, ChangeRenderFeatures)
static void create_rain_ripples_node_es(const ecs::Event &evt, dafg::NodeHandle &rain_ripples_node)
{
  if (auto *changedFeatures = evt.cast<ChangeRenderFeatures>())
  {
    if (!(changedFeatures->isFeatureChanged(CAMERA_IN_CAMERA) || changedFeatures->isFeatureChanged(GBUFFER_PACKED_NORMALS)))
      return;
  }

  bool isNormalsPacked = renderer_has_feature(FeatureRenderFlags::GBUFFER_PACKED_NORMALS);
  auto ns = isNormalsPacked ? dafg::root() / "opaque" / "mixing" : dafg::root() / "opaque" / "statics";
  rain_ripples_node = ns.registerNode("rain_ripples_node", DAFG_PP_NODE_SRC, [isNormalsPacked](dafg::Registry registry) {
    registry.allowAsyncPipelines()
      .requestRenderPass()
      .depthRoAndBindToShaderVars("gbuf_depth", {"depth_gbuf"})
      .color({isNormalsPacked ? "unpacked_normals" : "gbuf_1"})
      .vrsRate(VRS_RATE_TEXTURE_NAME);

    if (isNormalsPacked)
      registry.readTexture("gbuf_2").atStage(dafg::Stage::PS).bindToShaderVar("material_gbuf");

    request_common_opaque_state(registry);

    return [](const dafg::multiplexing::Index &multiplexing_index) {
      const camera_in_camera::ApplyMasterState camcam(multiplexing_index);
      get_rain_ripple_renderer_ecs_query(*g_entity_mgr, [](ShaderVar zn_zfar, const PostFxRenderer &rain_ripples_shader) {
        Color4 zn_zfar_value = ShaderGlobal::get_float4(zn_zfar);
        d3d::set_depth_bounds(0, 50.f / zn_zfar_value.g);
        rain_ripples_shader.render();
        d3d::set_depth_bounds(0, 1.0f);
      });
    };
  });
}

// it' rain entity, not rain ripples
ECS_ON_EVENT(on_appear)
static void set_rain_ripples_size_es(const ecs::Event &, float rain_ripples__size)
{
  ShaderGlobal::set_float(get_shader_glob_var_id("rain_ripples_max_radius", true), rain_ripples__size);
  ShaderGlobal::set_float(get_shader_glob_var_id("droplets_scale", true), rain_ripples__size * 20.f);
}