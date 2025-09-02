// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/daFG.h>
#include <render/objectMotionBlur.h>
#include <render/world/frameGraphHelpers.h>

#include <daECS/core/entitySystem.h>
#include <daECS/core/componentType.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <render/renderSettings.h>
#include <render/daFrameGraph/ecs/frameGraphNode.h>
#include <render/renderEvent.h>

ECS_TAG(render)
static void calc_resolution_for_motion_blur_es(const SetResolutionEvent &evt)
{
  int tileCountX = ceilf((float)evt.renderingResolution.x / objectmotionblur::TILE_SIZE);
  int tileCountY = ceilf((float)evt.renderingResolution.y / objectmotionblur::TILE_SIZE);

  if (evt.type == SetResolutionEvent::Type::DYNAMIC_RESOLUTION)
  {
    dafg::set_dynamic_resolution("object_motionblur_tilesize", IPoint2{tileCountX, tileCountY});
  }
  else
  {
    dafg::set_resolution("object_motionblur_tilesize", IPoint2{tileCountX, tileCountY});
  }
}

dafg::NodeHandle makeObjectMotionBlurNode(objectmotionblur::MotionBlurSettings &settings)
{
  auto motionBlurNs = dafg::root() / "motion_blur" / "object_motion_blur";
  return motionBlurNs.registerNode("object_motion_blur_node", DAFG_PP_NODE_SRC, [settings](dafg::Registry registry) {
    objectmotionblur::on_settings_changed(settings);

    registry.readTexture("depth_for_transparency").atStage(dafg::Stage::CS).bindToShaderVar("depth_gbuf");
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("depth_gbuf_samplerstate");
    registry.readTexture("gbuf_2").atStage(dafg::Stage::CS).bindToShaderVar("material_gbuf").optional();
    registry.readTexture("motion_vecs").atStage(dafg::Stage::CS).bindToShaderVar("resolved_motion_vectors").optional();
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("resolved_motion_vectors_samplerstate").optional();

    auto sourceTargetHndl =
      registry.readTexture("color_target").atStage(dafg::Stage::COMPUTE).useAs(dafg::Usage::SHADER_RESOURCE).handle();

    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    registry.create("motion_blur_clamp_sampler", dafg::History::No)
      .blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo))
      .bindToShaderVar("object_motion_blur_in_tex_samplerstate");

    registry
      .createTexture2d("object_motion_blur_tile_max_tex", dafg::History::No,
        {TEXCF_UNORDERED | TEXFMT_G16R16F, registry.getResolution<2>("object_motionblur_tilesize")})
      .atStage(dafg::Stage::CS)
      .bindToShaderVar("object_motion_blur_tile_max_tex");

    registry
      .createTexture2d("object_motion_blur_neighbor_max", dafg::History::No,
        {TEXCF_UNORDERED | TEXFMT_G16R16F, registry.getResolution<2>("object_motionblur_tilesize")})
      .atStage(dafg::Stage::CS)
      .bindToShaderVar("object_motion_blur_neighbor_max");

    registry
      .createTexture2d("object_motion_blur_flattened_vel_tex", dafg::History::No,
        {TEXCF_UNORDERED | TEXFMT_R11G11B10F, registry.getResolution<2>("main_view")})
      .atStage(dafg::Stage::CS)
      .bindToShaderVar("object_motion_blur_flattened_vel_tex");
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.filter_mode = d3d::FilterMode::Point;
      registry.create("object_motion_blur_flattened_vel_sampler", dafg::History::No)
        .blob(d3d::request_sampler(smpInfo))
        .bindToShaderVar("object_motion_blur_flattened_vel_tex_samplerstate");
    }

    uint32_t targetFormat = get_frame_render_target_format();
    auto resultTexHndl = registry
                           .createTexture2d("color_target_done", dafg::History::No,
                             {TEXCF_UNORDERED | targetFormat, registry.getResolution<2>("main_view")})
                           .atStage(dafg::Stage::CS)
                           .bindToShaderVar("object_motion_blur_out_tex")
                           .handle();

    auto frameTimeHndl = registry.readBlob<float>("frame_delta_time").handle();
    registry.readBlob<TMatrix4>("motion_vec_reproject_tm").bindToShaderVar("motion_vec_reproject_tm");

    return [sourceTargetHndl, resultTexHndl, frameTimeHndl]() {
      objectmotionblur::apply(nullptr, sourceTargetHndl.d3dResId(), resultTexHndl.view(), safediv(1.0f, frameTimeHndl.ref()));
    };
  });
}

dafg::NodeHandle makeDummyObjectMotionBlurNode()
{
  auto motionBlurNs = dafg::root() / "motion_blur" / "object_motion_blur";
  return motionBlurNs.registerNode("dummy", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.requestRenderPass().color({registry.rename("color_target", "color_target_done", dafg::History::No).texture()});
  });
}

template <typename Callable>
inline void object_motion_blur_node_init_ecs_query(ecs::EntityId eid, Callable c);

ECS_TAG(render)
ECS_ON_EVENT(OnRenderSettingsReady)
ECS_TRACK(render_settings__motionBlurStrength)
ECS_REQUIRE(float render_settings__motionBlurStrength, bool render_settings__bare_minimum)
static void init_object_motion_blur_es(
  const ecs::Event &, float render_settings__motionBlurStrength, bool render_settings__bare_minimum)
{
  object_motion_blur_node_init_ecs_query(g_entity_mgr->getSingletonEntity(ECS_HASH("object_motion_blur")),
    [render_settings__motionBlurStrength, render_settings__bare_minimum](dafg::NodeHandle &object_motion_blur__render_node,
      int object_motion_blur__max_blur_px, int object_motion_blur__max_samples, float object_motion_blur__strength,
      float object_motion_blur__vignette_strength, float object_motion_blur__ramp_strength, float object_motion_blur__ramp_cutoff) {
      objectmotionblur::MotionBlurSettings settings;
      settings.externalTextures = true;

      settings.maxBlurPx = object_motion_blur__max_blur_px;
      settings.maxSamples = object_motion_blur__max_samples;
      // Multiply by 2 to make 50% the default, while allowing users to increase it
      settings.strength = object_motion_blur__strength * (2.f * render_settings__motionBlurStrength / 100.f);
      settings.vignetteStrength = object_motion_blur__vignette_strength;
      settings.rampStrength = object_motion_blur__ramp_strength;
      settings.rampCutoff = object_motion_blur__ramp_cutoff;

      const bool isEnabled = (settings.strength > 0 && object_motion_blur__vignette_strength > 0 && !render_settings__bare_minimum);
      object_motion_blur__render_node = isEnabled ? makeObjectMotionBlurNode(settings) : makeDummyObjectMotionBlurNode();
    });
}

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(dafg::NodeHandle object_motion_blur__render_node)
static void close_object_motion_blur_es(const ecs::Event &) { objectmotionblur::on_settings_changed({}); }

template <typename Callable>
inline void object_motion_blur_init_settings_ecs_query(ecs::EntityId eid, Callable c);

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void object_motion_blur_settings_appear_es(const ecs::Event &,
  int object_motion_blur_settings__max_blur_px,
  int object_motion_blur_settings__max_samples,
  float object_motion_blur_settings__strength,
  float object_motion_blur_settings__vignette_strength,
  float object_motion_blur_settings__ramp_strength,
  float object_motion_blur_settings__ramp_cutoff)
{
  object_motion_blur_init_settings_ecs_query(g_entity_mgr->getSingletonEntity(ECS_HASH("object_motion_blur")),
    [&](int &object_motion_blur__max_blur_px, int &object_motion_blur__max_samples, float &object_motion_blur__strength,
      float &object_motion_blur__vignette_strength, float &object_motion_blur__ramp_strength, float &object_motion_blur__ramp_cutoff) {
      object_motion_blur__max_blur_px = object_motion_blur_settings__max_blur_px;
      object_motion_blur__max_samples = object_motion_blur_settings__max_samples;
      object_motion_blur__strength = object_motion_blur_settings__strength;
      object_motion_blur__vignette_strength = object_motion_blur_settings__vignette_strength;
      object_motion_blur__ramp_strength = object_motion_blur_settings__ramp_strength;
      object_motion_blur__ramp_cutoff = object_motion_blur_settings__ramp_cutoff;
    });
}
