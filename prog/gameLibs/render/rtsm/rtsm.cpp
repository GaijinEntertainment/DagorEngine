// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/rtsm/rtsm_api.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_computeShaders.h>
#include <bvh/bvh.h>
#include <drv/3d/dag_rwResource.h>
#include <perfMon/dag_statDrv.h>
#include <math/integer/dag_IPoint2.h>
#include <EASTL/shared_ptr.h>
#include <imgui/imgui.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>

namespace rtsm
{

static ComputeShaderElement *trace = nullptr;
static ComputeShaderElement *traceFillLow = nullptr;
static ComputeShaderElement *traceFillHigh = nullptr;
static ComputeShaderElement *traceFillHighPrepass = nullptr;
static ComputeShaderElement *traceDynamic = nullptr;

static int rt_shadow_resolutionVarId = -1;
static int rt_shadow_resolutionIVarId = -1;
static int world_view_posVarId = -1;
static int rtsm_render_modeVarId = -1;
static int inv_proj_tmVarId = -1;
static int precomputed_dynamic_lightsVarId = -1;
static int render_direct_lights_use_tilingVarId = -1;
static int render_direct_lights_screen_sizeVarId = -1;
static int use_precomputed_dynamic_lightsVarId = -1;
static int rtsm_has_nukeVarId = -1;
static int rtsm_dynamic_light_radiusVarId = -1;
static int rtsm_dynamic_soft_shadowsVarId = -1;
static int rtsm_qualityVarId = -1;

static int max_stabilized_frame_count = 5;

using SharedNodeParams = eastl::shared_ptr<const NodeParams>;

#if DAGOR_DBGLEVEL > 0
static denoiser::DebugImage debug_image;
#endif

void initialize()
{
  if (!bvh::is_available())
    return;

  rt_shadow_resolutionVarId = get_shader_variable_id("rt_shadow_resolution");
  rt_shadow_resolutionIVarId = get_shader_variable_id("rt_shadow_resolutionI");
  world_view_posVarId = get_shader_variable_id("world_view_pos");
  rtsm_render_modeVarId = get_shader_variable_id("rtsm_render_mode");
  inv_proj_tmVarId = get_shader_variable_id("inv_proj_tm");
  precomputed_dynamic_lightsVarId = get_shader_variable_id("precomputed_dynamic_lights");
  render_direct_lights_use_tilingVarId = get_shader_variable_id("render_direct_lights_use_tiling", true);
  render_direct_lights_screen_sizeVarId = get_shader_variable_id("render_direct_lights_screen_size", true);
  use_precomputed_dynamic_lightsVarId = get_shader_variable_id("use_precomputed_dynamic_lights", true);
  rtsm_has_nukeVarId = get_shader_variable_id("rtsm_has_nuke");
  rtsm_dynamic_light_radiusVarId = get_shader_variable_id("rtsm_dynamic_light_radius");
  rtsm_dynamic_soft_shadowsVarId = get_shader_variable_id("rtsm_dynamic_soft_shadows");
  rtsm_qualityVarId = get_shader_variable_id("rtsm_quality");

  if (!trace)
    trace = new_compute_shader("rt_shadows");

  if (!traceFillLow)
    traceFillLow = new_compute_shader("rt_shadows_fill_low");

  if (!traceFillHigh)
    traceFillHigh = new_compute_shader("rt_shadows_fill_high");

  if (!traceFillHighPrepass)
    traceFillHighPrepass = new_compute_shader("rt_shadows_fill_high_prepass");

  if (!traceDynamic)
    traceDynamic = new_compute_shader("rt_direct_lights");
}

template <typename T>
static void safe_delete(T *&ptr)
{
  if (ptr)
  {
    delete ptr;
    ptr = nullptr;
  }
}

void teardown()
{
  safe_delete(trace);
  safe_delete(traceFillLow);
  safe_delete(traceFillHigh);
  safe_delete(traceFillHighPrepass);
  safe_delete(traceDynamic);
#if DAGOR_DBGLEVEL > 0
  debug_image.close();
#endif
}

denoiser::ParamsHandle<FrameParams> create_frame_params(dafg::Registry registry)
{
  return registry.createBlob<FrameParams>("rtsm_frame_params").handle();
}

static void dispatch_trace(int quality, const IPoint2 &resolution)
{
  switch (quality)
  {
    case 0:
    {
      TIME_D3D_PROFILE(rtsm::render_noisy_high)
      trace->dispatchThreads(resolution.x, resolution.y, 1);
    }
    break;
    case 1:
    {
      TIME_D3D_PROFILE(rtsm::render_noisy_medium)
      trace->dispatchThreads(resolution.x / 2, resolution.y, 1);
    }
      {
        TIME_D3D_PROFILE(rtsm::fill_medium)
        traceFillHighPrepass->dispatchThreads(resolution.x / 2, resolution.y, 1);
        traceFillHigh->dispatchThreads(resolution.x / 2, resolution.y, 1); // uses RT, requires TLAS
      }
      break;
    case 2:
    {
      TIME_D3D_PROFILE(rtsm::render_noisy_low)
      trace->dispatchThreads(resolution.x / 2, resolution.y, 1);
    }
      {
        TIME_D3D_PROFILE(rtsm::fill_low)
        traceFillLow->dispatchThreads(resolution.x / 2, resolution.y, 1);
      }
      break;
  }
}

static void do_trace(const NodeParams &node, const FrameParams &params, const IPoint2 &resolution)
{
  TIME_D3D_PROFILE(rtsm::trace);

  ShaderGlobal::set_float4(rt_shadow_resolutionVarId, resolution.x, resolution.y);
  ShaderGlobal::set_int4(rt_shadow_resolutionIVarId, resolution.x, resolution.y, 0, 0);
  ShaderGlobal::set_int(rtsm_has_nukeVarId, params.hasNuke ? 1 : 0);
  ShaderGlobal::set_int(rtsm_render_modeVarId, denoiser::is_ray_reconstruction_enabled() ? 1 : 0);
  ShaderGlobal::set_int(rtsm_qualityVarId, params.quality);

  bvh::bind_resources(params.ctxId, resolution.x);

  denoiser::trace_per_view(node.viewState, denoiser::TraceView{params.projTm, params.viewPos}, params.lensView,
    [&](const denoiser::TraceView &view) {
      ShaderGlobal::set_float4(world_view_posVarId, view.viewPos);
      ShaderGlobal::set_float4x4(inv_proj_tmVarId, inverse44(view.projTm));
      dispatch_trace(params.quality, resolution);
    });

  bvh::unbind_resources();
}

static dafg::NodeHandle make_trace_node(const SharedNodeParams &params)
{
  return dafg::root().registerNode("rtsm_trace", DAFG_PP_NODE_SRC, [params](dafg::Registry registry) {
    if (params->declareInputs)
      params->declareInputs(registry);
    denoiser::order_after_prepare(registry);
    auto frameParamsHndl = registry.readBlob<FrameParams>("rtsm_frame_params").handle();
    auto denoiserParamsHndl = denoiser::create_shadow_params(registry);
    auto valueHndl = denoiser::create_shadow_trace_output(registry, "rtsm_value");
    denoiser::modify_view_z_for_shadow(registry, "denoiser_view_z");
    return [params, frameParamsHndl, denoiserParamsHndl, valueHndl]() {
      const FrameParams &fp = frameParamsHndl.ref();
      denoiser::ShadowParams &dp = denoiserParamsHndl.ref();
      dp.render = fp.render && fp.ctxId && trace && valueHndl;
      if (!dp.render)
        return;
      G_ASSERT(!denoiser::is_ray_reconstruction_enabled() || fp.quality == 0);
      dp.lightDirection = fp.lightDir;
      dp.maxStabilizedFrameNum = max_stabilized_frame_count;
      dp.csmTexture = fp.csmTexture;
      dp.csmSampler = fp.csmSampler;
      dp.vsmTexture = fp.vsmTexture;
      dp.vsmSampler = fp.vsmSampler;
      if (params->setShaderState)
        params->setShaderState();
      do_trace(*params, fp, denoiser::texture_size(valueHndl->get()));
      if (params->resetShaderState)
        params->resetShaderState();
    };
  });
}

#if DAGOR_DBGLEVEL > 0
static dafg::NodeHandle make_debug_node()
{
  return dafg::root().registerNode("rtsm_debug", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    auto outputHndl = denoiser::read_shadow_output(registry);
    auto frameParamsHndl = registry.readBlob<FrameParams>("rtsm_frame_params").handle();
    return [outputHndl, frameParamsHndl]() {
      debug_image.nodeRan();
      // a frame without a trace leaves the output unwritten, the window keeps its last copy
      if (outputHndl && frameParamsHndl.ref().render && imgui_window_is_visible("Render", "RTSM"))
        denoiser::copy_for_debug(outputHndl->get(), debug_image, "rtsm_debug_image");
    };
  });
}
#endif

void register_nodes(const NodeParams &params, dag::Vector<dafg::NodeHandle> &nodes)
{
  SharedNodeParams shared = eastl::make_shared<NodeParams>(params);
  nodes.emplace_back(make_trace_node(shared));
  denoiser::ShadowNodeParams denoiseParams;
  denoiseParams.motionVectorsName = params.motionVectorsName;
  nodes.emplace_back(denoiser::make_shadow_node(denoiseParams));
#if DAGOR_DBGLEVEL > 0
  nodes.emplace_back(make_debug_node());
#endif
}

void render_dynamic_light_shadows(bvh::ContextId context_id, const Point3 &view_pos, Texture *dynamic_lighting_texture,
  float light_radius, bool soft_shadow, bool has_nuke)
{
  if (!dynamic_lighting_texture)
    return;

  ShaderGlobal::set_texture(precomputed_dynamic_lightsVarId, dynamic_lighting_texture);
  ShaderGlobal::set_int(render_direct_lights_use_tilingVarId, 0);
  ShaderGlobal::set_int(use_precomputed_dynamic_lightsVarId, 1);
  G_ASSERT(traceDynamic);

  TIME_D3D_PROFILE(rtsm::render_dynamic);

  TextureInfo ti;
  dynamic_lighting_texture->getinfo(ti);

  bvh::bind_resources(context_id, ti.w);

  ShaderGlobal::set_int4(rt_shadow_resolutionIVarId, ti.w, ti.h, 0, 0);
  ShaderGlobal::set_float4(rt_shadow_resolutionVarId, ti.w, ti.h);
  ShaderGlobal::set_int4(render_direct_lights_screen_sizeVarId, ti.w, ti.h, 0, 0);
  ShaderGlobal::set_float4(world_view_posVarId, view_pos);
  ShaderGlobal::set_int(rtsm_has_nukeVarId, has_nuke ? 1 : 0);
  ShaderGlobal::set_float(rtsm_dynamic_light_radiusVarId, light_radius);
  ShaderGlobal::set_int(rtsm_dynamic_soft_shadowsVarId, soft_shadow ? 1 : 0);

  const float zero[4] = {0, 0, 0, 0};
  d3d::clear_rwtexf(dynamic_lighting_texture, zero, 0, 0);
  traceDynamic->dispatchThreads(ti.w, ti.h, 1);

  bvh::unbind_resources();
}

#if DAGOR_DBGLEVEL > 0
static void imguiWindow()
{
  if (!debug_image.inGraph())
    return;
  if (!denoiser::is_ray_reconstruction_enabled())
    ImGui::SliderInt("Max stabilized frame count", &max_stabilized_frame_count, 0, denoiser::ShadowParams::MAX_HISTORY_FRAME_NUM);
  if (Texture *tex = debug_image.texture())
  {
    const Point2 uv = debug_image.uvMax();
    ImGuiDagor::Image(tex, debug_image.aspect(), ImVec2(0, 0), ImVec2(uv.x, uv.y));
  }
}

REGISTER_IMGUI_WINDOW("Render", "RTSM", imguiWindow);
#endif

} // namespace rtsm
