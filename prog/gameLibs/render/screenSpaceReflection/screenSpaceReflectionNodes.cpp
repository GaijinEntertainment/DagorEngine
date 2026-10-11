// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "screenSpaceReflectionNodes_api.h"
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_sampler.h>
#include <drv/3d/dag_decl.h>
#include <drv/3d/dag_viewScissor.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <startup/dag_globalSettings.h>
#include <render/viewVecs.h>
#include <EASTL/unique_ptr.h>

namespace render::ssr
{

static constexpr const char *render_resources_blob_name = "ssr_render_resources";

static float target_resolution_scale(const TargetDesc &target) { return target.isFullres ? 1.f : 0.5f; }

// The trace and the denoise pass can live in different namespaces, the params blob always belongs to the trace.
static dafg::NameSpaceRequest trace_name_space(dafg::Registry registry, const char *trace_view_ns_name)
{
  return trace_view_ns_name ? registry.root() / trace_view_ns_name : registry.currNameSpace();
}

namespace detail
{

// The project can have a normals history while the pass that would read it does not run.
static bool use_normals_history(const InputParameters &params)
{
  return params.readNormalsHistory && ssr_needs_downsampled_normals_history(params.target.quality);
}

void declare_inputs(dafg::Registry registry, const ResourceNames &names, const InputParameters &params)
{
  const dafg::Stage stage = params.stage;

  if (params.readPrevFrame)
  {
    registry.readTextureHistory("prev_frame_tex").atStage(stage).bindToShaderVar("prev_frame_tex").optional();
    if (names.prevFrameSamplerResName)
    {
      registry.read(names.prevFrameSamplerResName)
        .blob<d3d::SamplerHandle>()
        .bindToShaderVar("prev_frame_tex_samplerstate")
        .optional();
    }
  }

  G_ASSERT(names.closeDepthResName);
  registry.readTexture(names.closeDepthResName).atStage(stage).bindToShaderVar("downsampled_close_depth_tex").optional();
  registry.readTextureHistory(names.closeDepthResName).atStage(stage).bindToShaderVar("prev_downsampled_close_depth_tex").optional();
  if (names.closeDepthSamplerResName)
  {
    registry.read(names.closeDepthSamplerResName)
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("downsampled_close_depth_tex_samplerstate")
      .bindToShaderVar("prev_downsampled_close_depth_tex_samplerstate")
      .optional();
  }

  G_ASSERT(names.normalsResName);
  registry.readTexture(names.normalsResName).atStage(stage).bindToShaderVar("downsampled_normals").optional();
  if (use_normals_history(params))
    registry.readTextureHistory(names.normalsResName).atStage(stage).bindToShaderVar("prev_downsampled_normals").optional();
  if (names.normalsSamplerResName)
    registry.read(names.normalsSamplerResName)
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("downsampled_normals_samplerstate")
      .optional();

  if (names.motionVectorsResName)
    registry.readTexture(names.motionVectorsResName).atStage(stage).bindToShaderVar(names.motionVectorsVar).optional();
  if (names.motionVectorsSamplerResName)
  {
    registry.read(names.motionVectorsSamplerResName)
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("downsampled_motion_vectors_tex_samplerstate")
      .bindToShaderVar("prev_downsampled_motion_vectors_tex_samplerstate")
      .optional();
  }

  if (params.readCheckerboardDepth && names.checkerboardDepthResName)
  {
    registry.readTexture(names.checkerboardDepthResName)
      .atStage(stage)
      .bindToShaderVar("downsampled_checkerboard_depth_tex")
      .optional();
  }

  // At full res, the ssr passes need the full gbuffer's normals and depth.
  if (params.target.isFullres)
  {
    registry.read("gbuf_1").texture().atStage(stage).bindToShaderVar("normal_gbuf");
    registry.readTexture("gbuf_depth").atStage(stage).bindToShaderVar("depth_gbuf");
  }

  if (params.readMaterialGbuf || params.target.isFullres)
    registry.read("gbuf_2").texture().atStage(stage).bindToShaderVar("material_gbuf").optional();

  // The whole gbuffer shares one sampler, so bind every var of it through a single request.
  if (names.gbufSamplerResName)
  {
    registry.read(names.gbufSamplerResName)
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("material_gbuf_samplerstate")
      .bindToShaderVar("normal_gbuf_samplerstate")
      .bindToShaderVar("depth_gbuf_samplerstate")
      .optional();
  }
}

TraceResources declare_trace_resources(dafg::Registry registry, const TraceNodeParameters &params)
{
  declare_inputs(registry, params.resourceNames,
    {
      .target = params.target,
      .stage = dafg::Stage::PS_OR_CS,
      .readPrevFrame = true,
      .readCheckerboardDepth = true,
      .readMaterialGbuf = params.readMaterialGbuf,
      .readNormalsHistory = params.readNormalsHistory,
    });

  registry.historyFor("ssr_target").texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("ssr_prev_target");

  // The compute trace writes the target through a UAV, the pixel one renders into it.
  const dafg::Usage usage =
    params.target.quality == SSRQuality::Compute ? dafg::Usage::SHADER_RESOURCE : dafg::Usage::COLOR_ATTACHMENT;

  return {
    .target = registry.modifyTexture("ssr_target_before_denoise").atStage(dafg::Stage::PS_OR_CS).useAs(usage).handle(),
    // FrameParams and CameraState must be provided by the project.
    .frameParams = registry.readBlob<FrameParams>(frame_params_blob_name).handle(),
    .ssrCamera = registry.modifyBlob<ssr_pass::CameraState>(camera_state_blob_name).handle(),
    .prevSsrCamera = registry.readBlobHistory<ssr_pass::CameraState>(camera_state_blob_name).handle(),
    .camera = registry.readBlob<CameraParams>("current_camera").handle(),
    .renderResources = registry.readBlob<ssr_pass::RenderResources *>(render_resources_blob_name).handle(),
    .resolution = registry.getResolution<2>("main_view", target_resolution_scale(params.target)),
    .targetMipCount = ssr_pass::get_mip_count(params.target.quality),
    .useSimpleDenoiser = use_simple_denoiser(params.target, params.useSimpleDenoiser),
    .isMainView = params.isMainView,
  };
}

void prepare_ssr(const TraceResources &res)
{
  static ShaderVariableInfo ssr_denoiser_typeVarId("ssr_denoiser_type", true);
  ShaderGlobal::set_int(ssr_denoiser_typeVarId, res.useSimpleDenoiser ? 1 : 0);

  ssr_pass::CameraState &ssrCamera = res.ssrCamera.ref();
  if (!res.frameParams.ref().rebase)
  {
    const CameraParams &camera = res.camera.ref();
    ssrCamera.viewTm = camera.viewTm;
    ssrCamera.projTm = camera.jitterProjTm;
    ssrCamera.worldPos = camera.cameraWorldPos;
  }
  ssrCamera.viewVecs = calc_view_vecs(ssrCamera.viewTm, ssrCamera.projTm);
}

void render_ssr(const TraceResources &res, const dafg::multiplexing::Index &multiplexing_index)
{
  const FrameParams &frameParams = res.frameParams.ref();
  BaseTexture *ssrTex = res.target.get();
  const IPoint2 resolution = res.resolution.get();

  const ssr_pass::TraceParams params = {
    .width = resolution.x,
    .height = resolution.y,
    .targetMipCount = res.targetMipCount,
    .callId = (int)(::dagor_frame_no() + multiplexing_index.viewport + multiplexing_index.subSample + multiplexing_index.superSample),
    .isHistoryValid = frameParams.historyValid,
  };

  ssr_pass::RenderResources &resources = *res.renderResources.ref();

  SCOPE_RENDER_TARGET;
  if (resources.compute)
    d3d::set_rwtex(STAGE_CS, 0, ssrTex, 0, 0);
  else
  {
    d3d::set_render_target({}, DepthAccess::RW, {{ssrTex, 0, 0}});
    d3d::setviewscissor(0, 0, params.width, params.height);
  }

  ssr_pass::trace(resources, params, res.ssrCamera.ref(), res.prevSsrCamera.ref());

  if (resources.compute)
    d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);

  ssr_pass::build_mip_chain(resources, ssrTex, params.width, params.height);
}

} // namespace detail

dafg::VirtualResourceHandle<ssr_pass::CameraState, false, false> create_camera_state_blob(dafg::Registry registry)
{
  return registry.createBlob<ssr_pass::CameraState>(camera_state_blob_name).withHistory().handle();
}

dafg::NodeHandle make_resource_provider_node(dafg::NameSpace ns, const ResourceProviderParameters &params)
{
  return ns.registerNode("ssr_node_camera_res_provider", DAFG_PP_NODE_SRC, [params](dafg::Registry registry) {
    registry.create("ssr_target_before_denoise")
      .texture({params.target.fmt, registry.getResolution<2>("main_view", target_resolution_scale(params.target)),
        (uint32_t)ssr_pass::get_mip_count(params.target.quality)});

    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = params.samplerAddressMode;
    smpInfo.border_color = params.samplerBorderColor;
    registry.create("ssr_target_sampler").blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo));

    auto resourcesHndl = registry.createBlob<ssr_pass::RenderResources *>(render_resources_blob_name).handle();
    auto resources = eastl::make_unique<ssr_pass::RenderResources>(ssr_pass::create_render_resources(params.target.quality));

    return [resourcesHndl, resources = eastl::move(resources)] { resourcesHndl.ref() = resources.get(); };
  });
}

dafg::NodeHandle make_denoiser_node(dafg::NameSpace ns, const DenoiserParameters &params)
{
  if (!detail::use_simple_denoiser(params.target, params.useSimpleDenoiser))
  {
    return ns.registerNode("ssr_publish_node", DAFG_PP_NODE_SRC, [params](dafg::Registry registry) {
      const bool isCompute = params.target.quality == SSRQuality::Compute;
      registry.renameTexture("ssr_target_before_denoise", "ssr_target")
        .withHistory(dafg::History::ClearZeroOnFirstFrame)
        .atStage(dafg::Stage::PS_OR_CS)
        .useAs(isCompute ? dafg::Usage::SHADER_RESOURCE : dafg::Usage::COLOR_ATTACHMENT);
    });
  }

  return ns.registerNode("ssr_temporal_denoiser_node", DAFG_PP_NODE_SRC, [params](dafg::Registry registry) {
    // The node only dispatches ssr_temporal_denoise_cs, so every access is CS-stage.
    constexpr dafg::Stage stage = dafg::Stage::CS;
    detail::declare_inputs(registry, params.resourceNames,
      {.target = params.target,
        .stage = stage,
        .readMaterialGbuf = params.readMaterialGbuf,
        .readNormalsHistory = params.readNormalsHistory});

    registry.readTexture("ssr_target_before_denoise").atStage(stage).bindToShaderVar("ssr_target_before_denoise");

    const auto resolution = registry.getResolution<2>("main_view", target_resolution_scale(params.target));
    registry.create("ssr_target")
      .texture({params.target.fmt | TEXCF_UNORDERED, resolution, 1})
      .withHistory(dafg::History::ClearZeroOnFirstFrame)
      .atStage(stage)
      .bindToShaderVar("ssr_target");
    registry.readTextureHistory("ssr_target").atStage(stage).bindToShaderVar("ssr_prev_target");
    registry.read("ssr_target_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("ssr_prev_target_samplerstate");

    const dafg::NameSpaceRequest traceNs = trace_name_space(registry, params.traceViewNsName);
    auto frameParamsHndl = traceNs.readBlob<FrameParams>(frame_params_blob_name).handle();
    auto ssrCameraHndl = traceNs.readBlob<ssr_pass::CameraState>(camera_state_blob_name).handle();
    auto prevSsrCameraHndl = traceNs.readBlobHistory<ssr_pass::CameraState>(camera_state_blob_name).handle();

    return [frameParamsHndl, ssrCameraHndl, prevSsrCameraHndl, resolution,
             cs = eastl::unique_ptr<ComputeShaderElement>(new_compute_shader("ssr_temporal_denoise_cs"))]() {
      const FrameParams &frameParams = frameParamsHndl.ref();

      static ShaderVariableInfo force_ignore_historyVarId("force_ignore_history", true);
      const int prevIgnoreHistory = ShaderGlobal::get_int(force_ignore_historyVarId);
      STATE_GUARD(ShaderGlobal::set_int(force_ignore_historyVarId, VALUE), !frameParams.historyValid || prevIgnoreHistory,
        prevIgnoreHistory);

      ssr_pass::apply_reprojection(ssrCameraHndl.ref(), prevSsrCameraHndl.ref());

      const IPoint2 res = resolution.get();
      cs->dispatchThreads(res.x, res.y, 1);
    };
  });
}

} // namespace render::ssr
