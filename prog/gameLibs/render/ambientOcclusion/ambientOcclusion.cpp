// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ambientOcclusion_api.h"

#include <render/daFrameGraph/daFG.h>
#include <render/subFrameSample.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_sampler.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_consts.h>
#include <shaders/dag_shaders.h>
#include <EASTL/optional.h>
#include <EASTL/unique_ptr.h>

namespace ambient_occlusion
{

static constexpr const char *ao_tex = "ssao_tex";
static constexpr const char *ao_tmp_tex = "ssao_tmp_tex";

static constexpr const char *ao_node_state = "ao_node_state";
static constexpr const char *ao_filter_node_state = "ao_filter_node_state";

static constexpr const char *ao_sampler = "ssao_sampler";

static TargetDesc make_target_desc(uint32_t creation_flags, bool allow_compute)
{
  const uint32_t format = ao_common::creation_flags_to_format(ao_common::consider_shader_assumes(creation_flags));
  const bool useCompute = allow_compute && d3d::should_use_compute_for_image_processing({format});

  return TargetDesc{useCompute ? dafg::Stage::CS : dafg::Stage::PS, format | (useCompute ? TEXCF_UNORDERED : TEXCF_RTARGET)};
}

TargetDesc make_ssao_target_desc(uint32_t creation_flags) { return make_target_desc(creation_flags, false); }

TargetDesc make_gtao_target_desc(uint32_t creation_flags) { return make_target_desc(creation_flags, true); }

namespace
{

using TexHandle = dafg::VirtualResourceHandle<BaseTexture, true, false>;
using ConstTexHandle = dafg::VirtualResourceHandle<const BaseTexture, true, false>;
using SubFrameSampleHandle = dafg::VirtualResourceHandle<const SubFrameSample, false, true>;

void declare_ao_targets(dafg::Registry registry, const TargetDesc &target, bool clear_ao_tex)
{
  const auto halfMainViewRes = registry.getResolution<2>("main_view", 0.5f);

  auto aoTexReq = registry.create(ao_tex).texture({target.cflags, halfMainViewRes}).withHistory(dafg::History::ClearZeroOnFirstFrame);

  if (clear_ao_tex)
  {
    auto clearedAoTexReq = eastl::move(aoTexReq).clear(ResourceClearValue{.asFloat = {1.0f, 1.0f, 1.0f, 1.0f}});
    // It's required to request the render pass, so the clear above is triggered every frame.
    registry.requestRenderPass().color({clearedAoTexReq});
  }
  else
  {
    eastl::move(aoTexReq).atStage(target.stage).useAs(target.usage());
  }

  registry.create(ao_tmp_tex).texture({target.cflags, halfMainViewRes});
}

void declare_ao_samplers(dafg::Registry registry)
{
  d3d::SamplerInfo aoSmpInfo;
  aoSmpInfo.address_mode_u = aoSmpInfo.address_mode_v = aoSmpInfo.address_mode_w = d3d::AddressMode::Border;
  aoSmpInfo.border_color = d3d::BorderColor::Color::OpaqueWhite;
  const d3d::SamplerHandle sampler = d3d::request_sampler(aoSmpInfo);

  registry.create(ao_sampler).blob<d3d::SamplerHandle>(sampler);
}

TexHandle modify_ao_tex(dafg::Registry registry, const TargetDesc &target)
{
  return registry.modifyTexture(ao_tex).atStage(target.stage).useAs(target.usage()).handle();
}

TexHandle modify_ao_tmp_tex(dafg::Registry registry, const TargetDesc &target)
{
  return registry.modifyTexture(ao_tmp_tex).atStage(target.stage).useAs(target.usage()).handle();
}

ConstTexHandle read_ao_tex_history(dafg::Registry registry, const TargetDesc &target, const char *shader_var = nullptr)
{
  auto request = registry.historyFor(ao_tex).texture().atStage(target.stage);
  if (shader_var)
    return eastl::move(request).bindToShaderVar(shader_var).handle();
  return eastl::move(request).useAs(dafg::Usage::SHADER_RESOURCE).handle();
}

void bind_texture(dafg::Registry registry, const char *tex_name, dafg::Stage stage, const char *shader_var,
  const char *alias_shader_var)
{
  auto request = registry.read(tex_name).texture().atStage(stage).bindToShaderVar(shader_var);
  if (alias_shader_var)
    eastl::move(request).bindToShaderVar(alias_shader_var);
}

void bind_optional_texture(dafg::Registry registry, const char *tex_name, dafg::Stage stage, const char *shader_var,
  const char *alias_shader_var)
{
  auto request = registry.read(tex_name).texture().atStage(stage).bindToShaderVar(shader_var).optional();
  if (alias_shader_var)
    eastl::move(request).bindToShaderVar(alias_shader_var);
}

void declare_depth_and_motion_inputs(dafg::Registry registry, const ResourceNames &names, dafg::Stage stage)
{
  if (names.closeDepthTex)
  {
    bind_texture(registry, names.closeDepthTex, stage, "downsampled_close_depth_tex", names.closeDepthAliasShaderVar);
    registry.historyFor(names.closeDepthTex).texture().atStage(stage).bindToShaderVar("prev_downsampled_close_depth_tex").optional();
  }
  if (names.closeDepthSampler)
  {
    registry.readBlob<d3d::SamplerHandle>(names.closeDepthSampler)
      .bindToShaderVar("downsampled_close_depth_tex_samplerstate")
      .bindToShaderVar("prev_downsampled_close_depth_tex_samplerstate");
  }

  if (names.motionVectorsTex)
  {
    bind_optional_texture(registry, names.motionVectorsTex, stage, "downsampled_motion_vectors_tex",
      names.motionVectorsAliasShaderVar);
    if (names.motionVectorsHistoryShaderVar)
    {
      registry.historyFor(names.motionVectorsTex)
        .texture()
        .atStage(stage)
        .bindToShaderVar(names.motionVectorsHistoryShaderVar)
        .optional();
    }
  }
  if (names.motionVectorsSampler)
  {
    registry.readBlob<d3d::SamplerHandle>(names.motionVectorsSampler)
      .bindToShaderVar("downsampled_motion_vectors_tex_samplerstate")
      .bindToShaderVar("prev_downsampled_motion_vectors_tex_samplerstate")
      .optional();
  }
}

void declare_normals_and_fom_shadows(dafg::Registry registry, const ResourceNames &names, dafg::Stage stage)
{
  if (names.normalsTex)
    registry.read(names.normalsTex).texture().atStage(stage).bindToShaderVar("downsampled_normals").optional();
  if (names.normalsSampler)
    registry.readBlob<d3d::SamplerHandle>(names.normalsSampler).bindToShaderVar("downsampled_normals_samplerstate").optional();

  if (names.fomShadowsCosTex)
    registry.read(names.fomShadowsCosTex).texture().atStage(stage).bindToShaderVar("fom_shadows_cos").optional();
  if (names.fomShadowsSinTex)
    registry.read(names.fomShadowsSinTex).texture().atStage(stage).bindToShaderVar("fom_shadows_sin").optional();
  if (names.fomShadowsSampler)
    registry.readBlob<d3d::SamplerHandle>(names.fomShadowsSampler).bindToShaderVar("fom_shadows_cos_samplerstate").optional();
}

void declare_contact_shadows_and_gbuffer(dafg::Registry registry, const ResourceNames &names, dafg::Stage stage,
  bool material_gbuffer_before_root_publish)
{
  if (names.contactShadowTex)
    registry.root().read(names.contactShadowTex).texture().atStage(stage).useAs(dafg::Usage::SHADER_RESOURCE).optional();

  if (!names.materialGbufferTex)
    return;

  // This namespace selection should be removed, when we have the gbuffer publish nodes at the similar places in the projects.
  auto gbufNs = [&] { return material_gbuffer_before_root_publish ? registry.root() / "opaque" / "decals" : registry.root(); };

  gbufNs().read(names.materialGbufferTex).texture().atStage(stage).bindToShaderVar("material_gbuf").optional();
  if (names.materialGbufferSampler)
    gbufNs().read(names.materialGbufferSampler).blob<d3d::SamplerHandle>().bindToShaderVar("material_gbuf_samplerstate").optional();
}

void declare_wait_tokens(dafg::Registry registry, const eastl::array<const char *, 3> &token_names)
{
  for (const char *token : token_names)
  {
    if (token)
      registry.readBlob(token).optional();
  }
}

eastl::optional<SubFrameSampleHandle> read_sub_frame_sample(dafg::Registry registry, const ResourceNames &names)
{
  if (!names.subFrameSampleBlob)
    return eastl::nullopt;
  return registry.readBlob<SubFrameSample>(names.subFrameSampleBlob).optional().handle();
}

SubFrameSample resolve_sub_frame_sample(const eastl::optional<SubFrameSampleHandle> &handle)
{
  const SubFrameSample *subFrameSample = handle ? handle->get() : nullptr;
  return subFrameSample ? *subFrameSample : SubFrameSample::Single;
}

} // anonymous namespace

static const Point4 &get_camera_view_vec_lt(const CameraParams &c) { return c.viewVecs.viewVecLT; }
static const Point4 &get_camera_view_vec_rt(const CameraParams &c) { return c.viewVecs.viewVecRT; }
static const Point4 &get_camera_view_vec_lb(const CameraParams &c) { return c.viewVecs.viewVecLB; }
static const Point4 &get_camera_view_vec_rb(const CameraParams &c) { return c.viewVecs.viewVecRB; }

detail::CameraHandle detail::read_current_camera(dafg::Registry registry)
{
  return registry.readBlob<CameraParams>(current_camera_blob)
    .bindToShaderVar<get_camera_view_vec_lt>("view_vecLT")
    .bindToShaderVar<get_camera_view_vec_rt>("view_vecRT")
    .bindToShaderVar<get_camera_view_vec_lb>("view_vecLB")
    .bindToShaderVar<get_camera_view_vec_rb>("view_vecRB")
    .handle();
}

detail::CameraHandle detail::read_prev_camera(dafg::Registry registry)
{
  return registry.readBlobHistory<CameraParams>(current_camera_blob).handle();
}

dafg::NodeHandle make_ssao_prepare_node(const dafg::NameSpace &ns, const SsaoPrepareParameters &params)
{
  return ns.registerNode("ssao_prepare", DAFG_PP_NODE_SRC,
    [params = eastl::make_unique<const SsaoPrepareParameters>(params)](dafg::Registry registry) {
      declare_ao_targets(registry, params->target, /*clear_ao_tex*/ true);
      declare_ao_samplers(registry);

      auto stateHndl = registry.createBlob<detail::SsaoNodeState *>(ao_node_state).handle();

      auto state = eastl::make_unique<detail::SsaoNodeState>();
      state->resources = ssao_pass::create_render_resources(params->creationFlags);

      return [stateHndl, state = eastl::move(state)]() { stateHndl.ref() = state.get(); };
    });
}

dafg::NodeHandle make_gtao_prepare_node(const dafg::NameSpace &ns, const GtaoPrepareParameters &params)
{
  return ns.registerNode("gtao_prepare", DAFG_PP_NODE_SRC,
    [params = eastl::make_unique<const GtaoPrepareParameters>(params)](dafg::Registry registry) {
      declare_ao_targets(registry, params->target, /*clear_ao_tex*/ false);
      declare_ao_samplers(registry);

      auto stateHndl = registry.createBlob<detail::GtaoNodeState *>(ao_node_state).handle();

      auto state = eastl::make_unique<detail::GtaoNodeState>();
      state->resources = gtao_pass::create_render_resources(params->creationFlags);

      return [stateHndl, state = eastl::move(state)]() { stateHndl.ref() = state.get(); };
    });
}

eastl::unique_ptr<const detail::SsaoRenderState> detail::declare_ssao_render(dafg::Registry registry,
  const SsaoRenderParameters &params)
{
  const ResourceNames &names = params.names;
  const dafg::Stage stage = params.target.stage;

  registry.requestRenderPass().color({ao_tex});
  read_ao_tex_history(registry, params.target, "ssao_prev_tex");
  auto subFrameSample = read_sub_frame_sample(registry, names);

  G_ASSERT(names.closeDepthTex);
  auto nodeState = registry.readBlob<detail::SsaoNodeState *>(ao_node_state).handle();

  declare_depth_and_motion_inputs(registry, names, stage);
  if (names.farDepthTex)
    registry.read(names.farDepthTex).texture().atStage(stage).bindToShaderVar("downsampled_far_depth_tex");
  declare_normals_and_fom_shadows(registry, names, stage);
  declare_contact_shadows_and_gbuffer(registry, names, stage, params.materialGbufferBeforeRootPublish);
  declare_wait_tokens(registry, params.waitTokenNames);

  registry.readBlob<d3d::SamplerHandle>(ao_sampler)
    .bindToShaderVar("ssao_tex_samplerstate")
    .bindToShaderVar("ssao_prev_tex_samplerstate");

  return eastl::make_unique<detail::SsaoRenderState>(
    detail::SsaoRenderState{subFrameSample, nodeState, params.setViewFromMultiplexing, params.isMainView});
}

void detail::render_ssao(const detail::SsaoRenderState &state, dafg::multiplexing::Index multiplexing_index, const CameraParams &view)
{
  detail::SsaoNodeState &nodeState = *state.nodeState.ref();
  if (state.setViewFromMultiplexing)
    nodeState.frameState.setCurrentView(multiplexing_index.viewport);

  ssao_pass::resolve(nodeState.resources, nodeState.frameState.current(), /*reprojection*/ nullptr, view.viewTm, view.jitterProjTm,
    &view.cameraWorldPos, resolve_sub_frame_sample(state.subFrameSample), state.isMainView);
}

eastl::unique_ptr<const detail::GtaoRenderState> detail::declare_gtao_render(dafg::Registry registry,
  const GtaoRenderParameters &params)
{
  const ResourceNames &names = params.names;
  const dafg::Stage stage = params.target.stage;

  eastl::optional<TexHandle> aoTex;
  if (stage == dafg::Stage::CS)
    aoTex = modify_ao_tex(registry, params.target);
  else
    registry.requestRenderPass().color({ao_tex});

  auto subFrameSample = read_sub_frame_sample(registry, names);
  auto nodeState = registry.readBlob<detail::GtaoNodeState *>(ao_node_state).handle();
  auto resolution = registry.getResolution<2>("main_view", 0.5f);

  declare_depth_and_motion_inputs(registry, names, stage);
  if (names.farDepthTex)
    registry.read(names.farDepthTex).texture().atStage(stage).bindToShaderVar("downsampled_far_depth_tex");
  declare_normals_and_fom_shadows(registry, names, stage);
  declare_contact_shadows_and_gbuffer(registry, names, stage, params.materialGbufferBeforeRootPublish);
  declare_wait_tokens(registry, params.waitTokenNames);

  return eastl::make_unique<detail::GtaoRenderState>(
    detail::GtaoRenderState{subFrameSample, aoTex, nodeState, resolution, params.setViewFromMultiplexing, params.isMainView});
}

void detail::render_gtao(const detail::GtaoRenderState &state, dafg::multiplexing::Index multiplexing_index, const CameraParams &view)
{
  detail::GtaoNodeState &nodeState = *state.nodeState.ref();
  if (state.setViewFromMultiplexing)
    nodeState.frameState.setCurrentView(multiplexing_index.viewport);

  const IPoint2 res = state.resolution.get();

  // the gtao_target is plain resource declaration, not dshl variable, which blocks daFG from setting it automatically for now.
  if (state.aoTex)
    d3d::set_rwtex(STAGE_CS, 0, state.aoTex->get(), 0, 0);

  gtao_pass::resolve(nodeState.resources, nodeState.frameState.current(), /*reprojection*/ nullptr, view.viewTm, view.jitterProjTm,
    &view.cameraWorldPos, resolve_sub_frame_sample(state.subFrameSample), state.isMainView, res.x, res.y);

  if (state.aoTex)
    d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
}

dafg::NodeHandle make_ssao_filter_node(const dafg::NameSpace &ns, const SsaoFilterParameters &params)
{
  return ns.registerNode("ssao_filter", DAFG_PP_NODE_SRC,
    [params = eastl::make_unique<const SsaoFilterParameters>(params)](dafg::Registry registry) {
      auto aoTex = modify_ao_tex(registry, params->target);
      auto aoTmpTex = modify_ao_tmp_tex(registry, params->target);
      auto nodeState = registry.renameBlob<detail::SsaoNodeState *>(ao_node_state, ao_filter_node_state).handle();
      auto resolution = registry.getResolution<2>("main_view", 0.5f);

      declare_depth_and_motion_inputs(registry, params->names, params->target.stage);
      registry.readBlob<d3d::SamplerHandle>(ao_sampler).bindToShaderVar("ssao_tex_samplerstate");

      return [aoTex, aoTmpTex, nodeState, resolution]() {
        detail::SsaoNodeState &state = *nodeState.ref();
        const IPoint2 res = resolution.get();

        BaseTexture *ssaoTex = aoTex.get();
        BaseTexture *tmpTex = aoTmpTex.get();

        ssao_pass::two_pass_blur(state.resources, state.frameState.current().frameNo, ssaoTex, tmpTex, res.x, res.y);
      };
    });
}

dafg::NodeHandle make_gtao_filter_node(const dafg::NameSpace &ns, const GtaoFilterParameters &params)
{
  return ns.registerNode("gtao_filter", DAFG_PP_NODE_SRC,
    [params = eastl::make_unique<const GtaoFilterParameters>(params)](dafg::Registry registry) {
      auto aoTex = modify_ao_tex(registry, params->target);
      auto aoHistTex = read_ao_tex_history(registry, params->target);
      auto aoTmpTex = modify_ao_tmp_tex(registry, params->target);
      auto nodeState = registry.renameBlob<detail::GtaoNodeState *>(ao_node_state, ao_filter_node_state).handle();
      auto resolution = registry.getResolution<2>("main_view", 0.5f);

      declare_depth_and_motion_inputs(registry, params->names, params->target.stage);
      registry.readBlob<d3d::SamplerHandle>(ao_sampler).bindToShaderVar("ssao_tex_samplerstate");
      declare_contact_shadows_and_gbuffer(registry, params->names, params->target.stage, params->materialGbufferBeforeRootPublish);
      detail::read_current_camera(registry);

      return [aoTex, aoHistTex, aoTmpTex, nodeState, resolution]() {
        detail::GtaoNodeState &state = *nodeState.ref();
        const IPoint2 res = resolution.get();

        BaseTexture *rawTex = aoTex.get();
        BaseTexture *prevGtaoTex = aoHistTex.get();
        BaseTexture *spatialTex = aoTmpTex.get();

        gtao_pass::spatiotemporal_filter(state.resources.filter, rawTex, prevGtaoTex, spatialTex, res.x, res.y);
      };
    });
}

} // namespace ambient_occlusion
