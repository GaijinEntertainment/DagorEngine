// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/shared_ptr.h>

#include <render/daBfg/bfg.h>

#include <render/gtao.h>
#include <render/ssao.h>

#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>
#include "frameGraphNodes.h"

static auto register_ssao_sampler(dabfg::Registry registry)
{
  d3d::SamplerInfo smpInfo;
  smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
  smpInfo.border_color = d3d::BorderColor::Color::OpaqueWhite;
  return registry.create("ssao_sampler", dabfg::History::No).blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo));
}

static dabfg::NodeHandle gen_gtao_node(int w, int h, uint32_t gtao_flags)
{
  return dabfg::register_node("gtao_node", DABFG_PP_NODE_SRC, [gtao_flags, w, h](dabfg::Registry registry) {
    registry.orderMeAfter("prepare_lights_node");
    registry.orderMeBefore("combine_shadows_node");

    use_volfog_shadow(registry, dabfg::Stage::PS); // volfog shadow is integrated into SSAO/GTAO pass as additional shadow

    auto bindShaderVar = [&registry](const char *shader_var_name, const char *tex_name) {
      return registry.read(tex_name).texture().atStage(dabfg::Stage::PS).bindToShaderVar(shader_var_name);
    };

    auto bindHistoryShaderVar = [&registry](const char *shader_var_name, const char *tex_name) {
      return registry.historyFor(tex_name).texture().atStage(dabfg::Stage::PS).bindToShaderVar(shader_var_name);
    };

    bindShaderVar("downsampled_close_depth_tex", "close_depth");
    registry.read("close_depth_sampler")
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("downsampled_close_depth_tex_samplerstate")
      .bindToShaderVar("prev_downsampled_close_depth_tex_samplerstate");

    bindHistoryShaderVar("prev_downsampled_close_depth_tex", "close_depth");

    bindShaderVar("downsampled_normals", "downsampled_normals");
    registry.read("downsampled_normals_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("downsampled_normals_samplerstate");

    const auto halfMainViewRes = registry.getResolution<2>("main_view", 0.5f);

    auto ssaoHndl =
      registry.create("ssao_tex", dabfg::History::ClearZeroOnFirstFrame)
        .texture(
          {ssao_detail::creation_flags_to_format(ssao_detail::consider_shader_assumes(gtao_flags)) | TEXCF_RTARGET, halfMainViewRes})
        .atStage(dabfg::Stage::PS)
        .useAs(dabfg::Usage::COLOR_ATTACHMENT)
        .handle();
    register_ssao_sampler(registry);

    auto ssaoHistHndl =
      registry.historyFor("ssao_tex").texture().atStage(dabfg::Stage::PS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();

    auto ssaoTmpHndl =
      registry.create("ssao_tmp_tex", dabfg::History::No)
        .texture(
          {ssao_detail::creation_flags_to_format(ssao_detail::consider_shader_assumes(gtao_flags)) | TEXCF_RTARGET, halfMainViewRes})
        .atStage(dabfg::Stage::PS)
        .useAs(dabfg::Usage::COLOR_ATTACHMENT)
        .handle();

    // Only used when available
    bindShaderVar("downsampled_motion_vectors_tex", "downsampled_motion_vectors_tex").optional();
    bindHistoryShaderVar("prev_downsampled_motion_vectors_tex", "downsampled_motion_vectors_tex").optional();
    registry.read("downsampled_motion_vectors_tex_sampler")
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("downsampled_motion_vectors_tex_samplerstate")
      .bindToShaderVar("prev_downsampled_motion_vectors_tex_samplerstate")
      .optional();
    bindShaderVar("fom_shadows_cos", "fom_shadows_cos").optional();
    bindShaderVar("fom_shadows_sin", "fom_shadows_sin").optional();
    registry.read("fom_shadows_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("fom_shadows_cos_samplerstate").optional();
    read_gbuffer_material_only(registry);

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto subFrameSampleHndl = registry.readBlob<SubFrameSample>("sub_frame_sample").handle();
    return [ssaoHndl, ssaoHistHndl, ssaoTmpHndl, cameraHndl, subFrameSampleHndl,
             gtao = eastl::make_unique<GTAORenderer>(w, h, 1, gtao_flags, false)] {
      ManagedTexView ssaoTex = ssaoHndl.view();
      ssaoTex->disableSampler();

      ManagedTexView ssaoTmpTex = ssaoTmpHndl.view();
      ssaoTmpTex->disableSampler();

      ManagedTexView prevSsaoTex = ssaoHistHndl.view();

      const auto &camera = cameraHndl.ref();
      gtao->render(camera.viewTm, camera.jitterProjTm, nullptr, &ssaoTex, &prevSsaoTex, &ssaoTmpTex, camera.cameraWorldPos,
        subFrameSampleHndl.ref());
    };
  });
}

static dabfg::NodeHandle gen_ssao_node(int w, int h, uint32_t ssao_flags)
{
  return dabfg::register_node("ssao_node", DABFG_PP_NODE_SRC, [ssao_flags, w, h](dabfg::Registry registry) {
    registry.orderMeAfter("prepare_lights_node");
    registry.orderMeBefore("combine_shadows_node");

    use_volfog_shadow(registry, dabfg::Stage::PS); // volfog shadow is integrated into SSAO/GTAO pass as additional shadow

    auto bindShaderVar = [&registry](const char *shader_var_name, const char *tex_name) {
      return registry.read(tex_name).texture().atStage(dabfg::Stage::PS).bindToShaderVar(shader_var_name);
    };

    auto bindHistoryShaderVar = [&registry](const char *shader_var_name, const char *tex_name) {
      return registry.historyFor(tex_name).texture().atStage(dabfg::Stage::PS).bindToShaderVar(shader_var_name);
    };

    auto depthHndl = bindShaderVar("downsampled_close_depth_tex", "close_depth").handle();
    registry.read("close_depth_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("downsampled_close_depth_tex_samplerstate");

    bindHistoryShaderVar("prev_downsampled_close_depth_tex", "close_depth");
    registry.read("close_depth_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("prev_downsampled_close_depth_tex_samplerstate");

    // Only used on some presets
    bindShaderVar("downsampled_normals", "downsampled_normals").optional();
    registry.read("downsampled_normals_sampler")
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("downsampled_normals_samplerstate")
      .optional();

    const auto halfMainViewRes = registry.getResolution<2>("main_view", 0.5f);

    auto ssaoHndl =
      registry.create("ssao_tex", dabfg::History::ClearZeroOnFirstFrame)
        .texture(
          {ssao_detail::creation_flags_to_format(ssao_detail::consider_shader_assumes(ssao_flags)) | TEXCF_RTARGET, halfMainViewRes})
        .atStage(dabfg::Stage::PS)
        .useAs(dabfg::Usage::COLOR_ATTACHMENT)
        .handle();
    register_ssao_sampler(registry).bindToShaderVar("ssao_tex_samplerstate");

    auto ssaoHistHndl =
      registry.historyFor("ssao_tex").texture().atStage(dabfg::Stage::PS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();
    registry.create("ssao_history_sampler", dabfg::History::No)
      .blob<d3d::SamplerHandle>(d3d::request_sampler({}))
      .bindToShaderVar("ssao_prev_tex_samplerstate");

    auto ssaoTmpHndl =
      registry.create("ssao_tmp_tex", dabfg::History::No)
        .texture(
          {ssao_detail::creation_flags_to_format(ssao_detail::consider_shader_assumes(ssao_flags)) | TEXCF_RTARGET, halfMainViewRes})
        .atStage(dabfg::Stage::PS)
        .useAs(dabfg::Usage::COLOR_ATTACHMENT)
        .handle();

    // Only used when available
    bindShaderVar("downsampled_motion_vectors_tex", "downsampled_motion_vectors_tex").optional();
    bindHistoryShaderVar("prev_downsampled_motion_vectors_tex", "downsampled_motion_vectors_tex").optional();
    registry.read("downsampled_motion_vectors_tex_sampler")
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("downsampled_motion_vectors_tex_samplerstate")
      .bindToShaderVar("prev_downsampled_motion_vectors_tex_samplerstate")
      .optional();
    bindShaderVar("fom_shadows_cos", "fom_shadows_cos").optional();
    bindShaderVar("fom_shadows_sin", "fom_shadows_sin").optional();
    registry.read("fom_shadows_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("fom_shadows_cos_samplerstate").optional();

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto subFrameSampleHndl = registry.readBlob<SubFrameSample>("sub_frame_sample").handle();
    return [ssaoHndl, ssaoHistHndl, ssaoTmpHndl, depthHndl, cameraHndl, subFrameSampleHndl,
             ssao = eastl::make_unique<SSAORenderer>(w, h, 1, ssao_flags, false)] {
      const auto &camera = cameraHndl.ref();

      ManagedTexView ssaoTex = ssaoHndl.view();
      ssaoTex->disableSampler();

      ManagedTexView ssaoTmpTex = ssaoTmpHndl.view();
      ssaoTmpTex->disableSampler();

      ManagedTexView prevSsaoTex = ssaoHistHndl.view();

      ManagedTexView closeDownsampledDepth = depthHndl.view();

      ssao->render(camera.viewTm, camera.jitterProjTm, closeDownsampledDepth.getTex2D(), &ssaoTex, &prevSsaoTex, &ssaoTmpTex,
        camera.cameraWorldPos, subFrameSampleHndl.ref());
    };
  });
}

dabfg::NodeHandle makeAmbientOcclusionNode(AoAlgo algo, int w, int h, uint32_t flags)
{
  switch (algo)
  {
    case AoAlgo::GTAO: return gen_gtao_node(w, h, flags);
    case AoAlgo::SSAO: return gen_ssao_node(w, h, flags);
    default: return dabfg::NodeHandle();
  }
}
