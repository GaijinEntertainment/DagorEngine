// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <ecs/render/shaderVar.h>
#include <render/resourceSlot/ecs/nodeHandleWithSlotsAccess.h>
#include <shaders/dag_postFxRenderer.h>
#include <render/resourceSlot/registerAccess.h>

template <typename Callable>
static void vhs_camera_preset_ecs_query(ecs::EntityId, Callable);
template <typename Callable>
static void get_vhs_camera_presets_ecs_query(Callable);
template <typename Callable>
static void get_vhs_camera_shader_vars_ecs_query(Callable);

ECS_ON_EVENT(on_appear)
ECS_REQUIRE(ecs::Tag vhs_camera__isPreset)
static void vhs_camera_preset_add_es(const ecs::Event &, ecs::EntityId eid, bool vhs_camera__isActive)
{
  if (!vhs_camera__isActive)
    return;
  get_vhs_camera_presets_ecs_query([eid](ecs::EidList &vhs_camera__activePresets) { vhs_camera__activePresets.push_back(eid); });
}

ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(ecs::Tag vhs_camera__isPreset)
static void vhs_camera_preset_remove_es(const ecs::Event &, ecs::EntityId eid)
{
  get_vhs_camera_presets_ecs_query([eid](ecs::EidList &vhs_camera__activePresets) {
    vhs_camera__activePresets.erase(eastl::remove(vhs_camera__activePresets.begin(), vhs_camera__activePresets.end(), eid),
      vhs_camera__activePresets.end());
  });
}

ECS_TRACK(vhs_camera__isActive)
ECS_REQUIRE(ecs::Tag vhs_camera__isPreset)
static void vhs_camera_preset_active_track_es(const ecs::Event &, ecs::EntityId eid, bool vhs_camera__isActive)
{
  get_vhs_camera_presets_ecs_query([eid, vhs_camera__isActive](ecs::EidList &vhs_camera__activePresets) {
    if (vhs_camera__isActive)
      vhs_camera__activePresets.push_back(eid);
    else
      vhs_camera__activePresets.erase(eastl::remove(vhs_camera__activePresets.begin(), vhs_camera__activePresets.end(), eid),
        vhs_camera__activePresets.end());
  });
}

static void vhs_camera_set_shader_params(float vhs_camera__resolutionDownscale,
  float vhs_camera__saturationMultiplier,
  float vhs_camera__noiseIntensity,
  float vhs_camera__dynamicRangeMin,
  float vhs_camera__dynamicRangeMax,
  float vhs_camera__dynamicRangeGamma,
  float vhs_camera__scanlineHeight,
  ShaderVar vhs_camera__downscale,
  ShaderVar vhs_camera__saturation,
  ShaderVar vhs_camera__noise_strength,
  ShaderVar vhs_camera__dynamic_range_min,
  ShaderVar vhs_camera__dynamic_range_max,
  ShaderVar vhs_camera__dynamic_range_gamma,
  ShaderVar vhs_camera__scanline_height,
  resource_slot::NodeHandleWithSlotsAccess &vhsCamera)
{
  vhs_camera__downscale.set_real(vhs_camera__resolutionDownscale);
  vhs_camera__saturation.set_real(vhs_camera__saturationMultiplier);
  vhs_camera__noise_strength.set_real(vhs_camera__noiseIntensity);
  vhs_camera__dynamic_range_min.set_real(vhs_camera__dynamicRangeMin);
  vhs_camera__dynamic_range_max.set_real(vhs_camera__dynamicRangeMax);
  vhs_camera__dynamic_range_gamma.set_real(vhs_camera__dynamicRangeGamma);
  vhs_camera__scanline_height.set_real(vhs_camera__scanlineHeight);
  vhsCamera = resource_slot::register_access("vhs_camera", DABFG_PP_NODE_SRC,
    {resource_slot::Update{"postfx_input_slot", "vhs_frame", 150}},
    [vhs_camera__resolutionDownscale](resource_slot::State slotsState, dabfg::Registry registry) {
      registry.createTexture2d(slotsState.resourceToCreateFor("postfx_input_slot"), dabfg::History::No,
        {TEXFMT_R11G11B10F | TEXCF_RTARGET, registry.getResolution<2>("post_fx", vhs_camera__resolutionDownscale)});
      registry.requestRenderPass().color({slotsState.resourceToCreateFor("postfx_input_slot")});
      registry.readTexture(slotsState.resourceToReadFrom("postfx_input_slot")).atStage(dabfg::Stage::PS).bindToShaderVar("frame_tex");

      return [shader = PostFxRenderer("vhs_camera")] { shader.render(); };
    });
}

ECS_REQUIRE(ecs::Tag vhs_camera__isPreset)
ECS_REQUIRE(eastl::true_type vhs_camera__isActive)
ECS_REQUIRE(eastl::true_type camera__active = true)
ECS_TRACK(*)
static void vhs_camera_preset_params_track_es(const ecs::Event &,
  ecs::EntityId eid,
  float vhs_camera__resolutionDownscale,
  float vhs_camera__saturationMultiplier,
  float vhs_camera__noiseIntensity,
  float vhs_camera__dynamicRangeMin,
  float vhs_camera__dynamicRangeMax,
  float vhs_camera__dynamicRangeGamma,
  float vhs_camera__scanlineHeight)
{
  get_vhs_camera_shader_vars_ecs_query(
    [=](ecs::EidList &vhs_camera__activePresets, ShaderVar vhs_camera__downscale, ShaderVar vhs_camera__saturation,
      ShaderVar vhs_camera__noise_strength, ShaderVar vhs_camera__dynamic_range_min, ShaderVar vhs_camera__dynamic_range_max,
      ShaderVar vhs_camera__dynamic_range_gamma, ShaderVar vhs_camera__scanline_height,
      resource_slot::NodeHandleWithSlotsAccess &vhsCamera) {
      if (!vhs_camera__activePresets.empty() && vhs_camera__activePresets.back() == eid)
      {
        vhs_camera_set_shader_params(vhs_camera__resolutionDownscale, vhs_camera__saturationMultiplier, vhs_camera__noiseIntensity,
          vhs_camera__dynamicRangeMin, vhs_camera__dynamicRangeMax, vhs_camera__dynamicRangeGamma, vhs_camera__scanlineHeight,
          vhs_camera__downscale, vhs_camera__saturation, vhs_camera__noise_strength, vhs_camera__dynamic_range_min,
          vhs_camera__dynamic_range_max, vhs_camera__dynamic_range_gamma, vhs_camera__scanline_height, vhsCamera);
      }
    });
}

ECS_ON_EVENT(on_appear)
static void vhs_camera_init_es(const ecs::Event &, resource_slot::NodeHandleWithSlotsAccess &vhsCamera) { vhsCamera.reset(); }

ECS_TRACK(vhs_camera__activePresets)
static void vhs_camera_node_parameters_track_es(const ecs::Event &,
  const ecs::EidList &vhs_camera__activePresets,
  ShaderVar vhs_camera__downscale,
  ShaderVar vhs_camera__saturation,
  ShaderVar vhs_camera__noise_strength,
  ShaderVar vhs_camera__dynamic_range_min,
  ShaderVar vhs_camera__dynamic_range_max,
  ShaderVar vhs_camera__dynamic_range_gamma,
  ShaderVar vhs_camera__scanline_height,
  resource_slot::NodeHandleWithSlotsAccess &vhsCamera)
{
  if (vhs_camera__activePresets.empty())
  {
    vhsCamera.reset();
    return;
  }
  vhs_camera_preset_ecs_query(vhs_camera__activePresets.back(),
    [&](float vhs_camera__resolutionDownscale, float vhs_camera__saturationMultiplier, float vhs_camera__noiseIntensity,
      float vhs_camera__dynamicRangeMin, float vhs_camera__dynamicRangeMax, float vhs_camera__dynamicRangeGamma,
      float vhs_camera__scanlineHeight) {
      vhs_camera_set_shader_params(vhs_camera__resolutionDownscale, vhs_camera__saturationMultiplier, vhs_camera__noiseIntensity,
        vhs_camera__dynamicRangeMin, vhs_camera__dynamicRangeMax, vhs_camera__dynamicRangeGamma, vhs_camera__scanlineHeight,
        vhs_camera__downscale, vhs_camera__saturation, vhs_camera__noise_strength, vhs_camera__dynamic_range_min,
        vhs_camera__dynamic_range_max, vhs_camera__dynamic_range_gamma, vhs_camera__scanline_height, vhsCamera);
    });
}
