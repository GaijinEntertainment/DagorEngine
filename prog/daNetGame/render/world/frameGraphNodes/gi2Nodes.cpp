// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/daFG.h>

#include <daGI2/daGI2.h>
#include <util/dag_convar.h>
#include <render/viewVecs.h>

#define INSIDE_RENDERER 1
#include "../private_worldRenderer.h"
#include "../global_vars.h"
#include "frameGraphNodes.h"
#include <render/world/frameGraphHelpers.h>
#include <drv/3d/dag_renderTarget.h>


dafg::NodeHandle makeGiCalcNode()
{
  return dafg::register_node("gi_before_frame_lit", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("combine_shadows_node");
    registry.createBlob<OrderingToken>("gi_before_frame_lit_token", dafg::History::No);

    // fixme: add ssao dependence
    auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
    if (wr.hasFeature(FeatureRenderFlags::COMBINED_SHADOWS))
    {
      registry.readTexture("combined_shadows").atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("combined_shadows");
      registry.read("combined_shadows_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("combined_shadows_samplerstate");
    }
    read_gbuffer(registry);
    read_gbuffer_depth(registry);
    registry.requestState().setFrameBlock("global_frame");
    registry.read("motion_vecs").texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("motion_gbuf").optional();
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("motion_gbuf_samplerstate").optional();

    registry.readTexture("close_depth").atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("downsampled_close_depth_tex").optional();
    registry.read("close_depth_sampler")
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("downsampled_close_depth_tex_samplerstate")
      .optional();

    registry.readTexture("checkerboard_depth")
      .atStage(dafg::Stage::PS_OR_CS)
      .bindToShaderVar("downsampled_checkerboard_depth_tex")
      .optional();
    registry.read("checkerboard_depth_sampler")
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("downsampled_checkerboard_depth_tex_samplerstate")
      .optional();

    registry.readTextureHistory("close_depth").atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("gi_prev_downsampled_close_depth_tex");
    registry.read("close_depth_sampler")
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("gi_prev_downsampled_close_depth_tex_samplerstate");

    registry.read("downsampled_normals").texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("downsampled_normals").optional();
    registry.read("downsampled_normals_sampler")
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("downsampled_normals_samplerstate")
      .optional();
    registry.readTextureHistory("prev_frame_tex").atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("prev_frame_tex").optional();
    registry.read("prev_frame_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("prev_frame_tex_samplerstate").optional();

    registry.readTexture("far_downsampled_depth").atStage(dafg::Stage::COMPUTE).bindToShaderVar("downsampled_far_depth_tex");
    registry.historyFor("far_downsampled_depth")
      .texture()
      .atStage(dafg::Stage::PS_OR_CS)
      .bindToShaderVar("prev_downsampled_far_depth_tex");
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      smpInfo.filter_mode = d3d::FilterMode::Point;
      registry.create("gi_far_downsampled_depth_sampler", dafg::History::No)
        .blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo))
        .bindToShaderVar("downsampled_far_depth_tex_samplerstate")
        .bindToShaderVar("prev_downsampled_far_depth_tex_samplerstate");
    }
    auto hasAnyDynamicLights = registry.readBlob<bool>("has_any_dynamic_lights").handle();
    auto currentCameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    return [hasAnyDynamicLights, currentCameraHndl] {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

      if (!wr.daGI2)
        return;

      set_inv_globtm_to_shader(currentCameraHndl.ref().viewTm, currentCameraHndl.ref().jitterProjTm, false);

      if (wr.lights.hasClusteredLights() && hasAnyDynamicLights.get())
        wr.lights.setBuffersToShader();
      wr.daGI2->beforeFrameLit(wr.canChangeAltitudeUnexpectedly ? 0 : wr.giDynamicQuality);
    };
  });
}

dafg::NodeHandle makeGiFeedbackNode()
{
  return dafg::register_node("gi_after_frame_lit", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("resolve_gbuffer_node");
    registry.executionHas(dafg::SideEffects::External);
    auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
    if (wr.hasFeature(FeatureRenderFlags::FULL_DEFERRED))
    {
      // fixme: add ssao dependence
      if (wr.hasFeature(FeatureRenderFlags::COMBINED_SHADOWS))
      {
        registry.readTexture("combined_shadows").atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("combined_shadows");
        registry.read("combined_shadows_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("combined_shadows_samplerstate");
      }

      eastl::optional<dafg::VirtualResourceHandle<BaseTexture, true, false>> currentAmbientHndl;
      if (wr.hasFeature(FeatureRenderFlags::DEFERRED_LIGHT))
      {
        registry.readTexture("current_ambient").atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("current_ambient");
        registry.read("current_ambient_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("current_ambient_samplerstate");
      }

      if (wr.hasFeature(FeatureRenderFlags::SSAO))
      {
        registry.readTexture("ssao_tex").atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("ssao_tex").optional();
        registry.read("ssao_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("ssao_tex_samplerstate").optional();
      }

      read_gbuffer(registry);
      read_gbuffer_depth(registry);
    }
    registry.read("close_depth").texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("downsampled_close_depth_tex");
    registry.read("close_depth_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("downsampled_close_depth_tex_samplerstate");

    // far_downsampled_depth is needed for HZB occlusion
    registry.read("far_downsampled_depth").texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("downsampled_far_depth_tex");
    registry.read("far_downsampled_depth_sampler")
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("downsampled_far_depth_tex_samplerstate");
    registry.requestState().setFrameBlock("global_frame");

    auto hasAnyDynamicLights = registry.readBlob<bool>("has_any_dynamic_lights").handle();
    auto currentCameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    return [hasAnyDynamicLights, currentCameraHndl]() {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

      if (!wr.daGI2)
        return;

      set_inv_globtm_to_shader(currentCameraHndl.ref().viewTm, currentCameraHndl.ref().jitterProjTm, false);
      const auto flags =
        wr.hasFeature(FeatureRenderFlags::FULL_DEFERRED) ? DaGI::FrameData(DaGI::FrameHasAll) : DaGI::FrameData(DaGI::FrameHasDepth);

      if (flags & DaGI::FrameHasLitScene && wr.lights.hasClusteredLights() && hasAnyDynamicLights.get())
        wr.lights.setBuffersToShader();

      const bool allowUpdateFromGbuf = !camera_in_camera::is_lens_render_active();
      wr.daGI2->afterFrameRendered(flags, allowUpdateFromGbuf);
    };
  });
}

dafg::NodeHandle makeGiScreenDebugDepthNode()
{
  return dafg::register_node("gi_screen_debug_depth_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.requestRenderPass().color({"frame_with_debug"});
    registry.readTexture("depth_for_postfx").atStage(dafg::Stage::PS).bindToShaderVar("depth_gbuf");
    return [] {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      if (!wr.daGI2)
        return;
      wr.daGI2->debugRenderScreenDepth();
    };
  });
}

dafg::NodeHandle makeGiScreenDebugNode()
{
  return dafg::register_node("gi_screen_debug_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.requestRenderPass().color({"frame_with_debug"});
    registry.readTexture("depth_for_postfx").atStage(dafg::Stage::PS).bindToShaderVar("depth_gbuf");
    return [] {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      if (!wr.daGI2)
        return;
      wr.daGI2->debugRenderScreen();
    };
  });
}
