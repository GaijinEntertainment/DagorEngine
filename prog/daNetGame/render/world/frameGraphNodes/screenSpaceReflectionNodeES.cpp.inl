// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/daFG.h>

#include <screenSpaceReflectionNodes_api.h>

#include "frameGraphNodes.h"

#include <render/cameraParams.h>
#include <render/cameraInCamera/cameraInCamera.h>

#include <daECS/core/componentTypes.h>
#include <daECS/core/entitySystem.h>

#include <render/renderEvent.h>
#include <render/world/dafgCameraRegistrator.h>
#include <render/world/frameGraphHelpers.h>
#include <frustumCulling/frustumPlanes.h>
#include <shaders/dag_shaders.h>

#define INSIDE_RENDERER 1
#include "../private_worldRenderer.h"

ECS_REGISTER_RELOCATABLE_TYPE(SsrNodesConfig, nullptr)

extern uint32_t get_gi_history_frames();

static ShaderVariableInfo ssr_denoiser_tileVarId("ssr_denoiser_tile", true);
static ShaderVariableInfo filter_high_luminance_changes("filter_high_luminance_changes", true);

// Both counters are in frames: only ssr_history_state_node updates them, and it is not multiplexed.
int invalidate_ssr_history_frames = 0;
int luminance_filter_frames = 0;

void invalidate_ssr_history(int frames) { invalidate_ssr_history_frames = ::max(invalidate_ssr_history_frames, frames); }

static inline bool update_is_history_valid()
{
  if (get_gi_history_frames() == 1)
    invalidate_ssr_history(1);

  const bool valid = invalidate_ssr_history_frames == 0;
  if (invalidate_ssr_history_frames > 0)
    --invalidate_ssr_history_frames;
  return valid;
}

void luminance_filter_ssr_history(int frames) { luminance_filter_frames = ::max(luminance_filter_frames, frames); }
static inline void update_is_luminance_filter_required()
{
  ShaderGlobal::set_int(filter_high_luminance_changes, luminance_filter_frames > 0);
  if (luminance_filter_frames > 0)
    luminance_filter_frames--;
}

static SsrNodesConfig resolve_ssr_config(SsrNodesConfig cfg)
{
  ssr_pass::get_real_quality_and_fmt(cfg.fmt, cfg.quality);
  if (cfg.quality != SSRQuality::Compute)
    cfg.fmt |= TEXCF_RTARGET;

  return cfg;
}

static render::ssr::TargetDesc ssr_target_desc(const SsrNodesConfig &cfg)
{
  return {.fmt = cfg.fmt, .quality = cfg.quality, .isFullres = cfg.isFullres};
}

static render::ssr::ResourceNames dng_ssr_resource_names()
{
  return {
    .closeDepthResName = "close_depth",
    .normalsResName = "downsampled_far_normals",
    .closeDepthSamplerResName = "close_depth_sampler",
    .normalsSamplerResName = "downsampled_normals_sampler",
    .motionVectorsSamplerResName = "downsampled_motion_vectors_tex_sampler",
    .gbufSamplerResName = "gbuf_sampler",
  };
}

ECS_TAG(render)
ECS_ON_EVENT(OnCameraMainViewNodeConstruction)
static void create_ssr_camera_nodes_es(const OnCameraMainViewNodeConstruction &evt, const SsrNodesConfig &dng_ssr_camera_nodes__config)
{
  if (dng_ssr_camera_nodes__config.w <= 0 || dng_ssr_camera_nodes__config.h <= 0)
    return;

  const SsrNodesConfig cfg = resolve_ssr_config(dng_ssr_camera_nodes__config);

  evt.nodes->push_back(
    render::ssr::make_resource_provider_node(dafg::root(), {
                                                             .target = ssr_target_desc(cfg),
                                                             .samplerAddressMode = d3d::AddressMode::Clamp,
                                                             .samplerBorderColor = d3d::BorderColor::Color::TransparentBlack,
                                                           }));

  evt.nodes->push_back(dafg::register_node("ssr_history_state_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.multiplex(dafg::multiplexing::Mode::None);
    registry.executionHas(dafg::SideEffects::External);
    auto historyValidHndl = registry.createBlob<bool>("ssr_history_valid").handle();
    return [historyValidHndl]() {
      historyValidHndl.ref() = update_is_history_valid();
      update_is_luminance_filter_required();
    };
  }));

  evt.nodes->push_back(render::ssr::make_denoiser_node(dafg::root(), {
                                                                       .resourceNames = dng_ssr_resource_names(),
                                                                       .traceViewNsName = "view0",
                                                                       .target = ssr_target_desc(cfg),
                                                                       .useSimpleDenoiser = cfg.denoiserType == SSR_DENOISER_SIMPLE,
                                                                       .readMaterialGbuf = true,
                                                                     }));
}

// Project specific RAII context.
struct SsrTraceContext
{
  SsrTraceContext(const CameraParams &camera, const ssr_pass::CameraState &, const render::ssr::FrameParams &, bool is_main_view) :
    camcam{is_main_view, camera}
  {
    auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
    if (wr.hasSSRAlternateReflections())
      wr.setGILightsToShader(false /*allow_frustum_lights*/);
  }

  camera_in_camera::ApplyPostfxState camcam;
};

ECS_TAG(render)
ECS_ON_EVENT(OnCameraPerViewNodeConstruction)
static void create_ssr_camera_view_nodes_es(const OnCameraPerViewNodeConstruction &evt,
  const SsrNodesConfig &dng_ssr_camera_nodes__config)
{
  if (dng_ssr_camera_nodes__config.w <= 0 || dng_ssr_camera_nodes__config.h <= 0)
    return;

  const SsrNodesConfig cfg = resolve_ssr_config(dng_ssr_camera_nodes__config);

  auto ns = dafg::root() / evt.viewNsName;

  evt.nodes->push_back(ns.registerNode("ssr_params_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.readBlob("after_prepare_lights_node_token");
    auto historyValidHndl = registry.readBlob<bool>("ssr_history_valid").handle();
    auto paramsHndl = registry.createBlob<render::ssr::FrameParams>(render::ssr::frame_params_blob_name).handle();
    render::ssr::create_camera_state_blob(registry);
    return [historyValidHndl, paramsHndl]() {
      render::ssr::FrameParams params;
      params.historyValid = historyValidHndl.ref();

      paramsHndl.ref() = params;
    };
  }));

  evt.nodes->push_back(
    render::ssr::make_trace_node<SsrTraceContext, OrderingToken>(ns, {
                                                                       .resourceNames = dng_ssr_resource_names(),
                                                                       .target = ssr_target_desc(cfg),
                                                                       .useSimpleDenoiser = cfg.denoiserType == SSR_DENOISER_SIMPLE,
                                                                       .readMaterialGbuf = true,
                                                                       .isMainView = evt.isMainView,
                                                                     }));
}

ECS_TAG(render)
ECS_ON_EVENT(ResetSsrNodes)
static void reset_ssr_camera_nodes_es(
  const ResetSsrNodes &evt, SsrNodesConfig &dng_ssr_camera_nodes__config, const ecs::string &dafg_camera_registrator__name)
{
  dng_ssr_camera_nodes__config = evt.config;

  recreate_camera_registrator_nodes(dafg_camera_registrator__name);
}
