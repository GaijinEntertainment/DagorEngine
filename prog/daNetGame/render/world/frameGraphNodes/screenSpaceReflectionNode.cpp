// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/daFG.h>

#include <render/screenSpaceReflections.h>

#include "frameGraphNodes.h"

#include "render/world/cameraParams.h"

#include <render/world/frameGraphHelpers.h>
#include <frustumCulling/frustumPlanes.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_computeShaders.h>

extern uint32_t get_gi_history_frames();

static ShaderVariableInfo ssr_denoiser_typeVarId("ssr_denoiser_type", true);
static ShaderVariableInfo ssr_denoiser_tileVarId("ssr_denoiser_tile", true);

int invalidate_ssr_history_frames = 0;
static bool is_history_valid = false;

void invalidate_ssr_history(int frames) { invalidate_ssr_history_frames = ::max(invalidate_ssr_history_frames, frames); }

static inline void update_is_history_valid()
{
  if (get_gi_history_frames() == 1)
    invalidate_ssr_history(1);

  is_history_valid = invalidate_ssr_history_frames == 0;
  if (invalidate_ssr_history_frames > 0)
    --invalidate_ssr_history_frames;
}

eastl::fixed_vector<dafg::NodeHandle, 3> makeScreenSpaceReflectionNodes(
  int w, int h, bool is_fullres, int denoiser_type, uint32_t fmt, SSRQuality ssr_quality)
{
  ScreenSpaceReflections::getRealQualityAndFmt(fmt, ssr_quality);
  if (ssr_quality != SSRQuality::Compute)
    fmt |= TEXCF_RTARGET;

  eastl::fixed_vector<dafg::NodeHandle, 3> result;

  if (!is_fullres)
  {
    w /= 2;
    h /= 2;
  }

  result.push_back(
    dafg::register_node("ssr_node_camera_res_provider", DAFG_PP_NODE_SRC, [fmt, is_fullres, ssr_quality](dafg::Registry registry) {
      registry.create("ssr_target_before_denoise", dafg::History::No)
        .texture({fmt, registry.getResolution<2>("main_view", is_fullres ? 1.0f : 0.5f),
          (uint32_t)ScreenSpaceReflections::getMipCount(ssr_quality)});

      {
        d3d::SamplerInfo smpInfo;
        smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
        smpInfo.border_color = d3d::BorderColor::Color::TransparentBlack;
        registry.create("ssr_target_sampler", dafg::History::No).blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo));
      }
    }));

  result.push_back(
    dafg::register_node("ssr_node", DAFG_PP_NODE_SRC, [fmt, ssr_quality, w, h, is_fullres, denoiser_type](dafg::Registry registry) {
      registry.orderMeAfter("prepare_lights_node");
      registry.orderMeBefore("combine_shadows_node");

      registry.readTextureHistory("prev_frame_tex").atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("prev_frame_tex");
      registry.read("prev_frame_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("prev_frame_tex_samplerstate");

      auto bindShaderVar = [&registry](const char *shader_var_name, const char *tex_name) {
        return registry.read(tex_name).texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar(shader_var_name);
      };

      auto bindHistoryShaderVar = [&registry](const char *shader_var_name, const char *tex_name) {
        return registry.historyFor(tex_name).texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar(shader_var_name);
      };

      const dafg::Usage createdResUsage = dafg::Usage::SHADER_RESOURCE;

      bindShaderVar("downsampled_close_depth_tex", "close_depth");
      bindHistoryShaderVar("prev_downsampled_close_depth_tex", "close_depth");
      registry.read("close_depth_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("prev_downsampled_close_depth_tex_samplerstate");
      registry.read("close_depth_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("downsampled_close_depth_tex_samplerstate");

      bindShaderVar("downsampled_normals", "downsampled_normals");
      registry.read("downsampled_normals_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("downsampled_normals_samplerstate");
      bindHistoryShaderVar("prev_downsampled_normals", "downsampled_normals");

      if (is_fullres)
      {
        read_gbuffer(registry, dafg::Stage::PS_OR_CS, readgbuffer::NORMAL);
        read_gbuffer_depth(registry, dafg::Stage::PS_OR_CS);
      }

      // Only used when available
      bindShaderVar("downsampled_motion_vectors_tex", "downsampled_motion_vectors_tex").optional();
      registry.read("downsampled_motion_vectors_tex_sampler")
        .blob<d3d::SamplerHandle>()
        .bindToShaderVar("downsampled_motion_vectors_tex_samplerstate")
        .bindToShaderVar("prev_downsampled_motion_vectors_tex_samplerstate")
        .optional();

      auto ssrTargetHndl =
        registry.modifyTexture("ssr_target_before_denoise").atStage(dafg::Stage::PS_OR_CS).useAs(createdResUsage).handle();
      auto ssrTargetHistHndl =
        registry.historyFor("ssr_target").texture().atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();

      registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
      auto cameraHndl = read_camera_in_camera(registry).handle();
      auto subFrameSampleHndl = registry.readBlob<SubFrameSample>("sub_frame_sample").handle();

      read_gbuffer_material_only(registry);

      use_camera_in_camera_jitter_frustum_plane_shader_vars(registry);

      return [ssrTargetHndl, ssrTargetHistHndl, cameraHndl, subFrameSampleHndl, denoiser_type,
               ssr = eastl::make_shared<ScreenSpaceReflections>(w, h, 1, fmt, ssr_quality, SSRFlag::None)](
               dafg::multiplexing::Index multiplexing_index) {
        camera_in_camera::ApplyPostfxState camcam{multiplexing_index, cameraHndl.ref()};
        const auto &camera = cameraHndl.ref();

        SubFrameSample subFrameSample = subFrameSampleHndl.ref();
        ShaderGlobal::set_int(ssr_denoiser_typeVarId, denoiser_type == SSR_DENOISER_SIMPLE ? 1 : 0);
        ManagedTexView ssrTex = ssrTargetHndl.view();

        TextureInfo info;
        ssrTex->getinfo(info);
        // Copmute version of SSR requires explicit resolution change, because dispatch size depends on it.
        // Pixel version worked well without this resolution change, but let it be here for all versions for safety.
        ssr->changeDynamicResolution(info.w, info.h);

        ManagedTexView prevSsrTex = ssrTargetHistHndl.view();
        int callId = ::dagor_frame_no() + multiplexing_index.viewport + multiplexing_index.subSample + multiplexing_index.superSample;

        if (multiplexing_index == dafg::multiplexing::Index{})
          update_is_history_valid();
        ssr->setHistoryValid(is_history_valid);

        ssr->render(camera.viewTm, camera.jitterProjTm, camera.cameraWorldPos, subFrameSample, ssrTex, prevSsrTex, ssrTex, callId);
      };
    }));

  if (denoiser_type == SSR_DENOISER_NONE)
  {
    // for LQ ssr
    result.push_back(dafg::register_node("ssr_publish_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      registry.renameTexture("ssr_target_before_denoise", "ssr_target", dafg::History::ClearZeroOnFirstFrame);
      return [] {};
    }));
  }

  if (denoiser_type == SSR_DENOISER_SIMPLE)
  {
    result.push_back(
      dafg::register_node("ssr_temporal_denoiser_node", DAFG_PP_NODE_SRC, [fmt, w, h, is_fullres](dafg::Registry registry) {
        auto bindShaderVar = [&registry](const char *shader_var_name, const char *tex_name) {
          return registry.read(tex_name).texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar(shader_var_name);
        };

        auto bindHistoryShaderVar = [&registry](const char *shader_var_name, const char *tex_name) {
          return registry.historyFor(tex_name).texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar(shader_var_name);
        };

        bindShaderVar("downsampled_close_depth_tex", "close_depth");
        bindHistoryShaderVar("prev_downsampled_close_depth_tex", "close_depth");
        registry.read("close_depth_sampler")
          .blob<d3d::SamplerHandle>()
          .bindToShaderVar("prev_downsampled_close_depth_tex_samplerstate");
        registry.read("close_depth_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("downsampled_close_depth_tex_samplerstate");

        bindShaderVar("downsampled_normals", "downsampled_normals");
        registry.read("downsampled_normals_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("downsampled_normals_samplerstate");
        bindHistoryShaderVar("prev_downsampled_normals", "downsampled_normals");

        if (is_fullres)
        {
          read_gbuffer(registry, dafg::Stage::PS_OR_CS, readgbuffer::NORMAL);
          read_gbuffer_depth(registry, dafg::Stage::PS_OR_CS);
        }

        // Only used when available
        bindShaderVar("downsampled_motion_vectors_tex", "downsampled_motion_vectors_tex").optional();
        registry.read("downsampled_motion_vectors_tex_sampler")
          .blob<d3d::SamplerHandle>()
          .bindToShaderVar("downsampled_motion_vectors_tex_samplerstate")
          .optional();

        registry.create("ssr_target", dafg::History::ClearZeroOnFirstFrame)
          .texture({fmt, registry.getResolution<2>("main_view", is_fullres ? 1.0f : 0.5f), 1})
          .atStage(dafg::Stage::PS_OR_CS)
          .bindToShaderVar("ssr_target");
        registry.read("ssr_target_sampler")
          .blob<d3d::SamplerHandle>()
          .bindToShaderVar("ssr_target_before_denoise_samplerstate")
          .bindToShaderVar("ssr_prev_target_samplerstate");

        bindHistoryShaderVar("ssr_prev_target", "ssr_target");
        bindShaderVar("ssr_target_before_denoise", "ssr_target_before_denoise");
        read_gbuffer_material_only(registry);

        use_jitter_frustum_plane_shader_vars(registry);

        return
          [w, h, ssr_temporal_denoise_cs = eastl::unique_ptr<ComputeShaderElement>(new_compute_shader("ssr_temporal_denoise_cs"))]() {
            ssr_temporal_denoise_cs->dispatchThreads(w, h, 1);
          };
      }));
  }

  return result;
}
