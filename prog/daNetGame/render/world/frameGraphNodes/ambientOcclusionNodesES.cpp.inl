// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/shared_ptr.h>

#include <render/daFrameGraph/daFG.h>

#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>

#include <render/gtao.h>
#include <render/ssao.h>
#include <render/viewVecs.h>
#include <render/renderEvent.h>

#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/global_vars.h>
#include <render/world/dafgCameraRegistrator.h>

#include <drv/3d/dag_info.h>


ECS_TAG(render)
ECS_ON_EVENT(ResetAoNodes)
static void reset_ao_camera_nodes_es(const ResetAoNodes &evt,
  int &dng_ao_camera_nodes__w,
  int &dng_ao_camera_nodes__h,
  uint32_t &dng_ao_camera_nodes__creation_flags,
  bool &dng_ao_camera_nodes__useGTAO,
  const ecs::string &dafg_camera_registrator__name)
{
  dng_ao_camera_nodes__w = evt.aoW;
  dng_ao_camera_nodes__h = evt.aoH;
  dng_ao_camera_nodes__creation_flags = evt.creationFlags;
  dng_ao_camera_nodes__useGTAO = evt.useGtao;

  ShaderGlobal::set_int(use_contact_shadowsVarId,
    dng_ao_camera_nodes__creation_flags & (SSAO_USE_CONTACT_SHADOWS | SSAO_USE_CONTACT_SHADOWS_SIMPLIFIED) ? 1 : 0);

  recreate_camera_registrator_nodes(dafg_camera_registrator__name);
}

static auto register_ssao_sampler(dafg::Registry registry)
{
  d3d::SamplerInfo smpInfo;
  smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
  smpInfo.border_color = d3d::BorderColor::Color::OpaqueWhite;
  return registry.create("ssao_sampler").blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo));
}

struct PreparedAo
{
  dafg::Stage stage;
  dafg::Usage usage;
  uint32_t cflags;
};

static inline auto prepare_ao(const uint32_t creation_flags)
{
  uint32_t cflags = ssao_detail::creation_flags_to_format(ssao_detail::consider_shader_assumes(creation_flags));
  bool useCompute = d3d::should_use_compute_for_image_processing({cflags});

  cflags |= (useCompute ? 0 : TEXCF_RTARGET);
  dafg::Stage stage = (useCompute ? dafg::Stage::CS : dafg::Stage::PS);
  dafg::Usage usage = (useCompute ? dafg::Usage::SHADER_RESOURCE : dafg::Usage::COLOR_ATTACHMENT);

  return PreparedAo{stage, usage, cflags};
}

ECS_TAG(render)
ECS_ON_EVENT(OnCameraMainViewNodeConstruction)
static void create_gtao_camera_main_view_nodes_es(const OnCameraMainViewNodeConstruction &evt,
  const int dng_ao_camera_nodes__w,
  const int dng_ao_camera_nodes__h,
  const uint32_t dng_ao_camera_nodes__creation_flags,
  const bool dng_ao_camera_nodes__useGTAO)
{
  if (dng_ao_camera_nodes__w <= 0 || dng_ao_camera_nodes__h <= 0)
    return;

  if (!dng_ao_camera_nodes__useGTAO)
    return;

  const auto preparedAo = prepare_ao(dng_ao_camera_nodes__creation_flags);
  const auto &stage = preparedAo.stage;
  const auto &usage = preparedAo.usage;
  const auto &cflags = preparedAo.cflags;

  auto perCameraResources = dafg::register_node("gtao_node_per_camera_res_node", DAFG_PP_NODE_SRC,
    [gtao_flags = dng_ao_camera_nodes__creation_flags, cflags, stage, usage, w = dng_ao_camera_nodes__w, h = dng_ao_camera_nodes__h](
      dafg::Registry registry) {
      const auto halfMainViewRes = registry.getResolution<2>("main_view", 0.5f);

      registry.create("ssao_tex_before_filter").texture({cflags, halfMainViewRes}).atStage(stage).useAs(usage);

      register_ssao_sampler(registry);

      registry.create("ssao_tmp_tex").texture({cflags, halfMainViewRes});


      auto gtaoRenderer = registry.createBlob<GTAORenderer *>("gtao_renderer").handle();
      return [gtaoRenderer, gtao = eastl::make_unique<GTAORenderer>(w, h, 1, gtao_flags, false, false)]() {
        gtaoRenderer.ref() = gtao.get();
      };
    });

  auto gtaoFilter = dafg::register_node("gtao_filter_node", DAFG_PP_NODE_SRC, [stage, usage](dafg::Registry registry) {
    auto bindShaderVar = [&registry, stage](const char *shader_var_name, const char *tex_name) {
      return registry.read(tex_name).texture().atStage(stage).bindToShaderVar(shader_var_name);
    };

    auto bindHistoryShaderVar = [&registry, stage](const char *shader_var_name, const char *tex_name) {
      return registry.historyFor(tex_name).texture().atStage(stage).bindToShaderVar(shader_var_name);
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

    auto ssaoHndl = registry.renameTexture("ssao_tex_before_filter", "ssao_tex")
                      .withHistory(dafg::History::ClearZeroOnFirstFrame)
                      .atStage(stage)
                      .useAs(usage)
                      .handle();
    auto ssaoHistHndl = registry.historyFor("ssao_tex").texture().atStage(stage).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    auto ssaoTmpHndl = registry.modifyTexture("ssao_tmp_tex").atStage(stage).useAs(usage).handle();

    read_gbuffer_material_only(registry);

    auto subFrameSampleHndl = registry.readBlob<SubFrameSample>("sub_frame_sample").handle();

    auto gtaoRenderer = registry.renameBlob<GTAORenderer *>("gtao_renderer", "gtao_filter_renderer").handle();
    auto camera = registry.readBlob<CameraParams>("current_camera");
    CameraViewShvars{camera}.bindViewVecs();

    return [ssaoHndl, ssaoHistHndl, ssaoTmpHndl, subFrameSampleHndl, gtaoRenderer] {
      BaseTexture *ssaoTex = ssaoHndl.get();
      BaseTexture *prevSsaoTex = ssaoHistHndl.get();
      BaseTexture *ssaoTmpTex = ssaoTmpHndl.get();

      gtaoRenderer.ref()->applyGTAOFilter(ssaoTex, prevSsaoTex, ssaoTmpTex, subFrameSampleHndl.ref());
    };
  });

  evt.nodes->push_back(eastl::move(perCameraResources));
  evt.nodes->push_back(eastl::move(gtaoFilter));
}

ECS_TAG(render)
ECS_ON_EVENT(OnCameraPerViewNodeConstruction)
static void create_gtao_camera_view_nodes_es(const OnCameraPerViewNodeConstruction &evt,
  const int dng_ao_camera_nodes__w,
  const int dng_ao_camera_nodes__h,
  const uint32_t dng_ao_camera_nodes__creation_flags,
  const bool dng_ao_camera_nodes__useGTAO)
{
  if (dng_ao_camera_nodes__w <= 0 || dng_ao_camera_nodes__h <= 0)
    return;

  if (!dng_ao_camera_nodes__useGTAO)
    return;

  const auto preparedAo = prepare_ao(dng_ao_camera_nodes__creation_flags);
  const auto &stage = preparedAo.stage;
  const auto &usage = preparedAo.usage;

  auto ns = dafg::root() / evt.viewNsName;
  auto gtao = ns.registerNode("gtao_node", DAFG_PP_NODE_SRC,
    [is_main_view = evt.isMainView, stage, usage, view_ns = evt.viewNsName](dafg::Registry registry) {
      registry.createBlob<OrderingToken>("after_gtao_node_token");
      registry.readBlob("after_prepare_lights_node_token");
      registry.readBlob("after_esm_ao_node_token").optional();

      use_volfog_shadow(registry, dafg::Stage::PS); // volfog shadow is integrated into SSAO/GTAO pass as additional shadow

      auto bindShaderVar = [&registry, stage](const char *shader_var_name, const char *tex_name) {
        return registry.read(tex_name).texture().atStage(stage).bindToShaderVar(shader_var_name);
      };

      auto bindHistoryShaderVar = [&registry, stage](const char *shader_var_name, const char *tex_name) {
        return registry.historyFor(tex_name).texture().atStage(stage).bindToShaderVar(shader_var_name);
      };

      bindShaderVar("downsampled_close_depth_tex", "close_depth");
      registry.read("close_depth_sampler")
        .blob<d3d::SamplerHandle>()
        .bindToShaderVar("downsampled_close_depth_tex_samplerstate")
        .bindToShaderVar("prev_downsampled_close_depth_tex_samplerstate");

      bindHistoryShaderVar("prev_downsampled_close_depth_tex", "close_depth");

      bindShaderVar("downsampled_normals", "downsampled_normals");
      registry.read("downsampled_normals_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("downsampled_normals_samplerstate");

      auto ssaoHndl = registry.modifyTexture("ssao_tex_before_filter").atStage(stage).useAs(usage).handle();

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

      auto camera = read_camera_view(registry, view_ns);
      auto cameraHndl = CameraViewShvars{camera}.bindViewVecs().toHandle();
      auto prevCameraHndl = read_camera_view_history(registry, view_ns).handle();

      auto gtaoRenderer = registry.readBlob<GTAORenderer *>("gtao_renderer").handle();
      return [is_main_view, ssaoHndl, cameraHndl, prevCameraHndl, gtaoRenderer]() {
        camera_in_camera::ApplyPostfxState camcam{is_main_view, cameraHndl.ref(), prevCameraHndl.ref()};

        const auto &camera = cameraHndl.ref();
        BaseTexture *ssaoTex = ssaoHndl.get();

        TextureInfo aoInfo;
        ssaoTex->getinfo(aoInfo);
        if (gtaoRenderer.ref()->getWidth() != (int)aoInfo.w || gtaoRenderer.ref()->getHeight() != (int)aoInfo.h)
          gtaoRenderer.ref()->changeResolution(aoInfo.w, aoInfo.h);

        gtaoRenderer.ref()->renderGTAO(camera.viewTm, camera.jitterProjTm, ssaoTex, &camera.cameraWorldPos);
      };
    });

  evt.nodes->push_back(eastl::move(gtao));
}

ECS_TAG(render)
ECS_ON_EVENT(OnCameraMainViewNodeConstruction)
static void create_ssao_camera_main_view_nodes_es(const OnCameraMainViewNodeConstruction &evt,
  const int dng_ao_camera_nodes__w,
  const int dng_ao_camera_nodes__h,
  const uint32_t dng_ao_camera_nodes__creation_flags,
  const bool dng_ao_camera_nodes__useGTAO)
{
  if (dng_ao_camera_nodes__w <= 0 || dng_ao_camera_nodes__h <= 0)
    return;

  if (dng_ao_camera_nodes__useGTAO)
    return;

  auto perCameraResources = dafg::register_node("ssao_per_camera_res_node", DAFG_PP_NODE_SRC,
    [ssao_flags = dng_ao_camera_nodes__creation_flags, w = dng_ao_camera_nodes__w, h = dng_ao_camera_nodes__h](
      dafg::Registry registry) {
      const auto halfMainViewRes = registry.getResolution<2>("main_view", 0.5f);

      registry.create("ssao_tex")
        .texture(
          {ssao_detail::creation_flags_to_format(ssao_detail::consider_shader_assumes(ssao_flags)) | TEXCF_RTARGET, halfMainViewRes})
        .withHistory(dafg::History::ClearZeroOnFirstFrame)
        .atStage(dafg::Stage::PS)
        .useAs(dafg::Usage::COLOR_ATTACHMENT);

      register_ssao_sampler(registry);

      registry.create("ssao_history_sampler").blob<d3d::SamplerHandle>(d3d::request_sampler({}));

      registry.create("ssao_tmp_tex")
        .texture(
          {ssao_detail::creation_flags_to_format(ssao_detail::consider_shader_assumes(ssao_flags)) | TEXCF_RTARGET, halfMainViewRes});

      auto ssaoRenderer = registry.createBlob<SSAORenderer *>("ssao_renderer").handle();

      return [ssaoRenderer, ssao = eastl::make_unique<SSAORenderer>(w, h, 1, ssao_flags, false, "", false)]() {
        ssaoRenderer.ref() = ssao.get();
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

    auto camera = registry.readBlob<CameraParams>("current_camera");
    CameraViewShvars{camera}.bindViewVecs();

    auto ssaoRenderer = registry.renameBlob<SSAORenderer *>("ssao_renderer", "ssao_blur_renderer").handle();

    return [ssaoHndl, ssaoTmpHndl, ssaoRenderer] {
      BaseTexture *ssaoTex = ssaoHndl.get();
      BaseTexture *ssaoTmpTex = ssaoTmpHndl.get();

      ssaoRenderer.ref()->applySSAOBlur(ssaoTex, ssaoTmpTex);
    };
  });

  evt.nodes->push_back(eastl::move(perCameraResources));
  evt.nodes->push_back(eastl::move(ssaoBlur));
}

ECS_TAG(render)
ECS_ON_EVENT(OnCameraPerViewNodeConstruction)
static void create_ssao_camera_view_nodes_es(const OnCameraPerViewNodeConstruction &evt,
  const int dng_ao_camera_nodes__w,
  const int dng_ao_camera_nodes__h,
  const bool dng_ao_camera_nodes__useGTAO)
{
  if (dng_ao_camera_nodes__w <= 0 || dng_ao_camera_nodes__h <= 0)
    return;

  if (dng_ao_camera_nodes__useGTAO)
    return;

  dafg::NameSpace view_ns = dafg::root() / evt.viewNsName;
  auto ssao = view_ns.registerNode("ssao_node", DAFG_PP_NODE_SRC,
    [is_main_view = evt.isMainView, view_ns = evt.viewNsName](dafg::Registry registry) {
      strict_camera_view_ordering(registry, is_main_view);

      registry.createBlob<OrderingToken>("after_ssao_node_token");
      registry.readBlob("after_prepare_lights_node_token");
      registry.readBlob("after_esm_ao_node_token").optional();

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

      auto camera = read_camera_view(registry, view_ns);
      auto cameraHndl = CameraViewShvars{camera}.bindViewVecs().toHandle();
      auto prevCameraHndl = read_camera_view_history(registry, view_ns).handle();
      auto subFrameSampleHndl = registry.readBlob<SubFrameSample>("sub_frame_sample").handle();

      auto ssaoRenderer = registry.modifyBlob<SSAORenderer *>("ssao_renderer").handle();
      return [is_main_view, ssaoHndl, ssaoHistHndl, depthHndl, cameraHndl, prevCameraHndl, subFrameSampleHndl, ssaoRenderer]() {
        camera_in_camera::ApplyPostfxState camcam{is_main_view, cameraHndl.ref(), prevCameraHndl.ref()};
        const auto &camera = cameraHndl.ref();

        BaseTexture *ssaoTex = ssaoHndl.get();
        BaseTexture *prevSsaoTex = ssaoHistHndl.get();
        BaseTexture *closeDownsampledDepth = depthHndl.get();

        const bool needRTClear = is_main_view;
        ssaoRenderer.ref()->renderSSAO(camera.viewTm, camera.jitterProjTm, closeDownsampledDepth, ssaoTex, prevSsaoTex,
          &camera.cameraWorldPos, subFrameSampleHndl.ref(), needRTClear);
      };
    });

  evt.nodes->push_back(eastl::move(ssao));
}
