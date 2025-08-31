// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/shared_ptr.h>

#include <render/daFrameGraph/daFG.h>

#include <render/gtao.h>
#include <render/ssao.h>
#include <render/viewVecs.h>

#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/global_vars.h>
#include "frameGraphNodes.h"

static auto register_ssao_sampler(dafg::Registry registry)
{
  d3d::SamplerInfo smpInfo;
  smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
  smpInfo.border_color = d3d::BorderColor::Color::OpaqueWhite;
  return registry.create("ssao_sampler", dafg::History::No).blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo));
}

static eastl::array<dafg::NodeHandle, 3> gen_gtao_node(int w, int h, uint32_t gtao_flags)
{
  auto perCameraResources =
    dafg::register_node("gtao_node_per_camera_res_node", DAFG_PP_NODE_SRC, [gtao_flags, w, h](dafg::Registry registry) {
      const auto halfMainViewRes = registry.getResolution<2>("main_view", 0.5f);

      registry.create("ssao_tex_before_filter", dafg::History::No)
        .texture(
          {ssao_detail::creation_flags_to_format(ssao_detail::consider_shader_assumes(gtao_flags)) | TEXCF_RTARGET, halfMainViewRes})
        .atStage(dafg::Stage::PS)
        .useAs(dafg::Usage::COLOR_ATTACHMENT);

      register_ssao_sampler(registry);

      registry.create("ssao_tmp_tex", dafg::History::No)
        .texture(
          {ssao_detail::creation_flags_to_format(ssao_detail::consider_shader_assumes(gtao_flags)) | TEXCF_RTARGET, halfMainViewRes});


      auto gtaoRenderer = registry.createBlob<GTAORenderer *>("gtao_renderer", dafg::History::No).handle();
      return [gtaoRenderer, gtao = eastl::make_unique<GTAORenderer>(w, h, 1, gtao_flags, false, false)]() {
        gtaoRenderer.ref() = gtao.get();
      };
    });

  auto gtao = dafg::register_node("gtao_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("prepare_lights_node");
    registry.orderMeBefore("combine_shadows_node");

    use_volfog_shadow(registry, dafg::Stage::PS); // volfog shadow is integrated into SSAO/GTAO pass as additional shadow

    auto bindShaderVar = [&registry](const char *shader_var_name, const char *tex_name) {
      return registry.read(tex_name).texture().atStage(dafg::Stage::PS).bindToShaderVar(shader_var_name);
    };

    auto bindHistoryShaderVar = [&registry](const char *shader_var_name, const char *tex_name) {
      return registry.historyFor(tex_name).texture().atStage(dafg::Stage::PS).bindToShaderVar(shader_var_name);
    };

    bindShaderVar("downsampled_close_depth_tex", "close_depth");
    registry.read("close_depth_sampler")
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("downsampled_close_depth_tex_samplerstate")
      .bindToShaderVar("prev_downsampled_close_depth_tex_samplerstate");

    bindHistoryShaderVar("prev_downsampled_close_depth_tex", "close_depth");

    bindShaderVar("downsampled_normals", "downsampled_normals");
    registry.read("downsampled_normals_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("downsampled_normals_samplerstate");

    auto ssaoHndl =
      registry.modifyTexture("ssao_tex_before_filter").atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT).handle();

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

    registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
    auto cameraHndl = read_camera_in_camera(registry).handle();
    auto prevCameraHndl = read_history_camera_in_camera(registry).handle();

    auto gtaoRenderer = registry.readBlob<GTAORenderer *>("gtao_renderer").handle();
    return [ssaoHndl, cameraHndl, prevCameraHndl, gtaoRenderer](const dafg::multiplexing::Index &multiplexing_index) {
      camera_in_camera::ApplyPostfxState camcam{multiplexing_index, cameraHndl.ref(), prevCameraHndl.ref()};

      const auto &camera = cameraHndl.ref();
      const bool isMainView = multiplexing_index.subCamera == 0;
      if (isMainView)
        set_viewvecs_to_shader(camera.viewTm, camera.jitterProjTm);

      ManagedTexView ssaoTex = ssaoHndl.view();

      gtaoRenderer.ref()->renderGTAO(camera.viewTm, camera.jitterProjTm, &ssaoTex, &camera.cameraWorldPos);
    };
  });

  auto gtaoFilter = dafg::register_node("gtao_filter_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto bindShaderVar = [&registry](const char *shader_var_name, const char *tex_name) {
      return registry.read(tex_name).texture().atStage(dafg::Stage::PS).bindToShaderVar(shader_var_name);
    };

    auto bindHistoryShaderVar = [&registry](const char *shader_var_name, const char *tex_name) {
      return registry.historyFor(tex_name).texture().atStage(dafg::Stage::PS).bindToShaderVar(shader_var_name);
    };

    bindShaderVar("downsampled_close_depth_tex", "close_depth");
    registry.read("close_depth_sampler")
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("downsampled_close_depth_tex_samplerstate")
      .bindToShaderVar("prev_downsampled_close_depth_tex_samplerstate");

    bindHistoryShaderVar("prev_downsampled_close_depth_tex", "close_depth");

    bindShaderVar("downsampled_motion_vectors_tex", "downsampled_motion_vectors_tex").optional();
    bindHistoryShaderVar("prev_downsampled_motion_vectors_tex", "downsampled_motion_vectors_tex").optional();
    registry.read("downsampled_motion_vectors_tex_sampler")
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("downsampled_motion_vectors_tex_samplerstate")
      .bindToShaderVar("prev_downsampled_motion_vectors_tex_samplerstate")
      .optional();

    auto ssaoHndl = registry.renameTexture("ssao_tex_before_filter", "ssao_tex", dafg::History::ClearZeroOnFirstFrame)
                      .atStage(dafg::Stage::PS)
                      .useAs(dafg::Usage::COLOR_ATTACHMENT)
                      .handle();
    auto ssaoHistHndl =
      registry.historyFor("ssao_tex").texture().atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    auto ssaoTmpHndl = registry.modifyTexture("ssao_tmp_tex").atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT).handle();

    read_gbuffer_material_only(registry);

    auto subFrameSampleHndl = registry.readBlob<SubFrameSample>("sub_frame_sample").handle();

    auto gtaoRenderer = registry.renameBlob<GTAORenderer *>("gtao_renderer", "gtao_filter_renderer", dafg::History::No).handle();
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();

    return [cameraHndl, ssaoHndl, ssaoHistHndl, ssaoTmpHndl, subFrameSampleHndl, gtaoRenderer] {
      const auto &camera = cameraHndl.ref();
      set_viewvecs_to_shader(camera.viewTm, camera.jitterProjTm);

      ManagedTexView ssaoTex = ssaoHndl.view();
      ManagedTexView prevSsaoTex = ssaoHistHndl.view();
      ManagedTexView ssaoTmpTex = ssaoTmpHndl.view();

      gtaoRenderer.ref()->applyGTAOFilter(&ssaoTex, &prevSsaoTex, &ssaoTmpTex, subFrameSampleHndl.ref());
    };
  });

  return {eastl::move(perCameraResources), eastl::move(gtao), eastl::move(gtaoFilter)};
}

static eastl::array<dafg::NodeHandle, 3> gen_ssao_node(int w, int h, uint32_t ssao_flags)
{
  auto perCameraResources =
    dafg::register_node("ssao_per_camera_res_node", DAFG_PP_NODE_SRC, [ssao_flags, w, h](dafg::Registry registry) {
      const auto halfMainViewRes = registry.getResolution<2>("main_view", 0.5f);

      registry.create("ssao_tex", dafg::History::ClearZeroOnFirstFrame)
        .texture(
          {ssao_detail::creation_flags_to_format(ssao_detail::consider_shader_assumes(ssao_flags)) | TEXCF_RTARGET, halfMainViewRes})
        .atStage(dafg::Stage::PS)
        .useAs(dafg::Usage::COLOR_ATTACHMENT);

      register_ssao_sampler(registry);

      registry.create("ssao_history_sampler", dafg::History::No).blob<d3d::SamplerHandle>(d3d::request_sampler({}));

      registry.create("ssao_tmp_tex", dafg::History::No)
        .texture(
          {ssao_detail::creation_flags_to_format(ssao_detail::consider_shader_assumes(ssao_flags)) | TEXCF_RTARGET, halfMainViewRes});

      auto ssaoRenderer = registry.createBlob<SSAORenderer *>("ssao_renderer", dafg::History::No).handle();

      return [ssaoRenderer, ssao = eastl::make_unique<SSAORenderer>(w, h, 1, ssao_flags, false, "", false)]() {
        ssaoRenderer.ref() = ssao.get();
      };
    });

  auto ssao = dafg::register_node("ssao_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("prepare_lights_node");
    registry.orderMeBefore("combine_shadows_node");

    use_volfog_shadow(registry, dafg::Stage::PS); // volfog shadow is integrated into SSAO/GTAO pass as additional shadow

    read_gbuffer_material_only(registry);

    auto bindShaderVar = [&registry](const char *shader_var_name, const char *tex_name) {
      return registry.read(tex_name).texture().atStage(dafg::Stage::PS).bindToShaderVar(shader_var_name);
    };

    auto bindHistoryShaderVar = [&registry](const char *shader_var_name, const char *tex_name) {
      return registry.historyFor(tex_name).texture().atStage(dafg::Stage::PS).bindToShaderVar(shader_var_name);
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

    auto ssaoHndl = registry.modifyTexture("ssao_tex").atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT).handle();
    registry.readBlob<d3d::SamplerHandle>("ssao_sampler").bindToShaderVar("ssao_tex_samplerstate");

    auto ssaoHistHndl =
      registry.historyFor("ssao_tex").texture().atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    registry.readBlob<d3d::SamplerHandle>("ssao_history_sampler").bindToShaderVar("ssao_prev_tex_samplerstate");

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

    registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
    auto cameraHndl = read_camera_in_camera(registry).handle();
    auto prevCameraHndl = read_history_camera_in_camera(registry).handle();
    auto subFrameSampleHndl = registry.readBlob<SubFrameSample>("sub_frame_sample").handle();

    auto ssaoRenderer = registry.modifyBlob<SSAORenderer *>("ssao_renderer").handle();
    return [ssaoHndl, ssaoHistHndl, depthHndl, cameraHndl, prevCameraHndl, subFrameSampleHndl, ssaoRenderer](
             const dafg::multiplexing::Index &multiplexing_index) {
      camera_in_camera::ApplyPostfxState camcam{multiplexing_index, cameraHndl.ref(), prevCameraHndl.ref()};
      const auto &camera = cameraHndl.ref();

      const bool isMainView = multiplexing_index.subCamera == 0;
      if (isMainView)
        set_viewvecs_to_shader(camera.viewTm, camera.jitterProjTm);

      ManagedTexView ssaoTex = ssaoHndl.view();

      ManagedTexView prevSsaoTex = ssaoHistHndl.view();

      ManagedTexView closeDownsampledDepth = depthHndl.view();

      const bool needRTClear = isMainView;
      ssaoRenderer.ref()->renderSSAO(camera.viewTm, camera.jitterProjTm, closeDownsampledDepth.getTex2D(), &ssaoTex, &prevSsaoTex,
        &camera.cameraWorldPos, subFrameSampleHndl.ref(), needRTClear);
    };
  });

  auto ssaoBlur = dafg::register_node("ssao_blur_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto bindShaderVar = [&registry](const char *shader_var_name, const char *tex_name) {
      return registry.read(tex_name).texture().atStage(dafg::Stage::PS).bindToShaderVar(shader_var_name);
    };

    auto bindHistoryShaderVar = [&registry](const char *shader_var_name, const char *tex_name) {
      return registry.historyFor(tex_name).texture().atStage(dafg::Stage::PS).bindToShaderVar(shader_var_name);
    };

    bindShaderVar("downsampled_close_depth_tex", "close_depth");
    registry.read("close_depth_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("downsampled_close_depth_tex_samplerstate");

    bindHistoryShaderVar("prev_downsampled_close_depth_tex", "close_depth");
    registry.read("close_depth_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("prev_downsampled_close_depth_tex_samplerstate");

    auto ssaoHndl = registry.modifyTexture("ssao_tex").atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT).handle();
    registry.readBlob<d3d::SamplerHandle>("ssao_sampler").bindToShaderVar("ssao_tex_samplerstate");

    auto ssaoTmpHndl = registry.modifyTexture("ssao_tmp_tex").atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT).handle();

    // Only used when available
    bindShaderVar("downsampled_motion_vectors_tex", "downsampled_motion_vectors_tex").optional();
    bindHistoryShaderVar("prev_downsampled_motion_vectors_tex", "downsampled_motion_vectors_tex").optional();

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();

    auto ssaoRenderer = registry.renameBlob<SSAORenderer *>("ssao_renderer", "ssao_blur_renderer", dafg::History::No).handle();

    return [ssaoHndl, ssaoTmpHndl, cameraHndl, ssaoRenderer] {
      const auto &camera = cameraHndl.ref();
      set_viewvecs_to_shader(camera.viewTm, camera.jitterProjTm);

      ManagedTexView ssaoTex = ssaoHndl.view();
      ManagedTexView ssaoTmpTex = ssaoTmpHndl.view();

      ssaoRenderer.ref()->applySSAOBlur(&ssaoTex, &ssaoTmpTex);
    };
  });

  return {eastl::move(perCameraResources), eastl::move(ssao), eastl::move(ssaoBlur)};
}

eastl::array<dafg::NodeHandle, 3> makeAmbientOcclusionNodes(AoAlgo algo, int w, int h, uint32_t flags)
{
  ShaderGlobal::set_int(use_contact_shadowsVarId, flags & (SSAO_USE_CONTACT_SHADOWS | SSAO_USE_CONTACT_SHADOWS_SIMPLIFIED) ? 1 : 0);
  switch (algo)
  {
    case AoAlgo::GTAO: return gen_gtao_node(w, h, flags);
    case AoAlgo::SSAO: return gen_ssao_node(w, h, flags);
    default: return {};
  }
}
