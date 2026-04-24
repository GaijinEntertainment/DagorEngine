// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <ecs/render/shaderVar.h>
#include <render/resourceSlot/ecs/nodeHandleWithSlotsAccess.h>
#include <shaders/dag_postFxRenderer.h>
#include <render/resourceSlot/registerAccess.h>
#include <render/vhsCameraCommon/vhsCameraCommon.h>

template <typename Callable>
static void vhs_camera_preset_ecs_query(ecs::EntityManager &manager, ecs::EntityId, Callable);
template <typename Callable>
static void reinit_vhs_camera_fg_node_ecs_query(ecs::EntityManager &manager, Callable);

static void reinit_vhs_camera_fg_node(resource_slot::NodeHandleWithSlotsAccess &vhsCamera, float vhs_camera__resolutionDownscale)
{
  vhsCamera =
    resource_slot::register_access("vhs_camera", DAFG_PP_NODE_SRC, {resource_slot::Update{"postfx_input_slot", "vhs_frame", 150}},
      [vhs_camera__resolutionDownscale](resource_slot::State slotsState, dafg::Registry registry) {
        registry.createTexture2d(slotsState.resourceToCreateFor("postfx_input_slot"),
          {TEXFMT_R11G11B10F | TEXCF_RTARGET, registry.getResolution<2>("post_fx", vhs_camera__resolutionDownscale)});
        registry.requestRenderPass().color({slotsState.resourceToCreateFor("postfx_input_slot")});
        registry.readTexture(slotsState.resourceToReadFrom("postfx_input_slot")).atStage(dafg::Stage::PS).bindToShaderVar("frame_tex");

        return [shader = PostFxRenderer("vhs_camera")] { shader.render(); };
      });
}

ECS_REQUIRE(ecs::Tag vhs_camera__isPreset)
ECS_REQUIRE(eastl::true_type vhs_camera__isActive)
ECS_REQUIRE(eastl::true_type camera__active = true)
ECS_TRACK(vhs_camera__resolutionDownscale)
static void vhs_camera_fg_node_reinit_on_res_downscale_change_es(
  const ecs::Event &, ecs::EntityManager &manager, const ecs::EntityId eid, const float vhs_camera__resolutionDownscale)
{
  reinit_vhs_camera_fg_node_ecs_query(manager, [&](ecs::EidList &vhs_camera__activePresets, ShaderVar vhs_camera__downscale,
                                                 resource_slot::NodeHandleWithSlotsAccess &vhsCamera) {
    if (!vhs_camera__activePresets.empty() && vhs_camera__activePresets.back() == eid)
    {
      reinit_vhs_camera_fg_node(vhsCamera, vhs_camera__resolutionDownscale);
      vhs_camera__downscale.set_float(vhs_camera__resolutionDownscale);
    }
  });
}

ECS_ON_EVENT(on_appear)
static void vhs_camera_init_es(const ecs::Event &, resource_slot::NodeHandleWithSlotsAccess &vhsCamera) { vhsCamera.reset(); }

ECS_TRACK(vhs_camera__activePresets)
static void vhs_camera_node_parameters_track_es(const ecs::Event &,
  ecs::EntityManager &manager,
  const ecs::EidList &vhs_camera__activePresets,
  ShaderVar vhs_camera__downscale,
  ShaderVar vhs_camera__saturation,
  ShaderVar vhs_camera__noise_strength,
  ShaderVar vhs_camera__noise_saturation,
  ShaderVar vhs_camera__dynamic_range_min,
  ShaderVar vhs_camera__dynamic_range_max,
  ShaderVar vhs_camera__dynamic_range_gamma,
  ShaderVar vhs_camera__scanline_height,
  ShaderVar vhs_camera__noise_uv_scale,
  ShaderVar vhs_camera__chromatic_aberration_intensity,
  ShaderVar vhs_camera__chromatic_aberration_start,
  resource_slot::NodeHandleWithSlotsAccess &vhsCamera)
{
  if (vhs_camera__activePresets.empty())
  {
    vhsCamera.reset();
    return;
  }
  vhs_camera_preset_ecs_query(manager, vhs_camera__activePresets.back(),
    [&](float vhs_camera__resolutionDownscale, float vhs_camera__saturationMultiplier, float vhs_camera__noiseIntensity,
      float vhs_camera__dynamicRangeMin, float vhs_camera__dynamicRangeMax, float vhs_camera__dynamicRangeGamma,
      float vhs_camera__scanlineHeight, float vhs_camera__noiseUvScale, float vhs_camera__chromaticAberrationIntensity,
      float vhs_camera__chromaticAberrationStart, float vhs_camera__noiseSaturation) {
      vhs_camera_set_shader_params(vhs_camera__resolutionDownscale, vhs_camera__saturationMultiplier, vhs_camera__noiseIntensity,
        vhs_camera__dynamicRangeMin, vhs_camera__dynamicRangeMax, vhs_camera__dynamicRangeGamma, vhs_camera__scanlineHeight,
        vhs_camera__noiseUvScale, vhs_camera__chromaticAberrationIntensity, vhs_camera__chromaticAberrationStart,
        vhs_camera__noiseSaturation, vhs_camera__downscale, vhs_camera__saturation, vhs_camera__noise_strength,
        vhs_camera__noise_saturation, vhs_camera__dynamic_range_min, vhs_camera__dynamic_range_max, vhs_camera__dynamic_range_gamma,
        vhs_camera__scanline_height, vhs_camera__noise_uv_scale, vhs_camera__chromatic_aberration_intensity,
        vhs_camera__chromatic_aberration_start);

      reinit_vhs_camera_fg_node(vhsCamera, vhs_camera__resolutionDownscale);
    });
}
