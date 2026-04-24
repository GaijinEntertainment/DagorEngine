// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/skies.h>
#include <render/fx/auroraBorealis.h>

#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <render/renderEvent.h>
#include <render/world/frameGraphHelpers.h>
#include <render/viewVecs.h>
#include <render/daFrameGraph/ecs/frameGraphNode.h>
#include <render/world/defaultVrsSettings.h>
#include <render/skies.h>

ECS_DECLARE_BOXED_TYPE(AuroraBorealis);
ECS_REGISTER_BOXED_TYPE(AuroraBorealis, nullptr);
ECS_AUTO_REGISTER_COMPONENT(AuroraBorealis, "aurora_borealis", nullptr, 0);

ECS_ON_EVENT(on_appear, ChangeRenderFeatures)
ECS_TRACK(*)
void aurora_borealis_es(const ecs::Event &,
  AuroraBorealis &aurora_borealis,
  bool aurora_borealis__enabled,
  bool aurora_borealis__is_night,
  float aurora_borealis__tex_res_multiplier,
  float aurora_borealis__top_height,
  Point3 aurora_borealis__top_color,
  float aurora_borealis__bottom_height,
  Point3 aurora_borealis__bottom_color,
  float aurora_borealis__brightness,
  float aurora_borealis__luminance,
  float aurora_borealis__speed,
  float aurora_borealis__ripples_scale,
  float aurora_borealis__ripples_speed,
  float aurora_borealis__ripples_strength,
  dafg::NodeHandle &aurora_borealis__init,
  dafg::NodeHandle &aurora_borealis__render,
  dafg::NodeHandle &aurora_borealis__apply)
{
  AuroraBorealisParams params = {};
  params.enabled = aurora_borealis__enabled && aurora_borealis__is_night;
  params.top_height = aurora_borealis__top_height;
  params.top_color = aurora_borealis__top_color;
  params.bottom_height = aurora_borealis__bottom_height;
  params.bottom_color = aurora_borealis__bottom_color;
  params.brightness = aurora_borealis__brightness;
  params.luminance = aurora_borealis__luminance;
  params.speed = aurora_borealis__speed;
  params.ripples_scale = aurora_borealis__ripples_scale;
  params.ripples_speed = aurora_borealis__ripples_speed;
  params.ripples_strength = aurora_borealis__ripples_strength;

  aurora_borealis.setParams(params);

  if (!params.enabled)
  {
    aurora_borealis__init = {};
    aurora_borealis__render = {};
    aurora_borealis__apply = {};
    return;
  }

  if (!aurora_borealis__init || !aurora_borealis__render || !aurora_borealis__apply)
  {
    uint32_t texFmt = aurora_borealis.texFmt();
    aurora_borealis__init = dafg::register_node("aurora_borealis__init", DAFG_PP_NODE_SRC,
      [texFmt, aurora_borealis__tex_res_multiplier](dafg::Registry registry) {
        registry.multiplex(dafg::multiplexing::Mode::None);

        registry.createTexture2d("aurora_borealis_tex",
          {texFmt | TEXCF_RTARGET, registry.getResolution<2>("main_view", aurora_borealis__tex_res_multiplier)});
      });

    aurora_borealis__render =
      dafg::register_node("aurora_borealis__render", DAFG_PP_NODE_SRC, [&aurora_borealis](dafg::Registry registry) {
        registry.multiplex(dafg::multiplexing::Mode::CameraInCamera);

        registry.requestRenderPass().color({"aurora_borealis_tex"});
        registry.readTexture("far_downsampled_depth").atStage(dafg::Stage::POST_RASTER).bindToShaderVar("downsampled_far_depth_tex");

        auto camera = read_camera_in_camera(registry);
        auto currentCameraHndl = CameraViewShvars{camera}.bindViewVecs().toHandle();

        return [currentCameraHndl, &aurora_borealis](const dafg::multiplexing::Index &multiplexing_index) {
          camera_in_camera::ApplyPostfxState camcam{multiplexing_index, currentCameraHndl.ref()};

          aurora_borealis.beforeRender();
        };
      });

    aurora_borealis__apply =
      dafg::register_node("aurora_borealis__apply", DAFG_PP_NODE_SRC, [&aurora_borealis](dafg::Registry registry) {
        registry.readBlob<OrderingToken>("skies_rendered_token"); // after sky
        registry.createBlob<OrderingToken>("aurora_borealis");    // and before clouds

        registry.readTexture("aurora_borealis_tex").atStage(dafg::Stage::PS).bindToShaderVar("aurora_borealis_tex");

        registry.requestRenderPass()
          .color({"opaque_with_envi"})
          .depthRoAndBindToShaderVars({"opaque_depth_with_water_before_clouds"}, {})
          .vrsRate(VRS_RATE_TEXTURE_NAME);

        return [&aurora_borealis]() { aurora_borealis.render(); };
      });
  }
}

ECS_TAG(render)
ECS_NO_ORDER
static inline void aurora_borealis_switch_es(
  const ecs::UpdateStageInfoAct &, bool &aurora_borealis__is_night, float aurora_borealis__night_sun_cos)
{
  if (DaSkies *skies = get_daskies())
  {
    Point3 sunDir = skies->getSunDir();
    aurora_borealis__is_night = sunDir.y < aurora_borealis__night_sun_cos;
  }
}