// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/rtao/rtao_api.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_computeShaders.h>
#include <bvh/bvh.h>
#include <perfMon/dag_statDrv.h>
#include <math/integer/dag_IPoint2.h>
#include <ioSys/dag_dataBlock.h>
#include <EASTL/shared_ptr.h>
#include <imgui/imgui.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>

namespace rtao
{

static ComputeShaderElement *trace = nullptr;

static int inv_proj_tmVarId = -1;
static int rtao_hit_dist_paramsVarId = -1;
static int rtao_resolutionIVarId = -1;
static int rtao_res_mulVarId = -1;
static int rtao_checkerboardVarId = -1;

static constexpr float ray_length_min = 0.01f;
static constexpr float ray_length_max = 10;
static constexpr float distance_factor_min = 0.01;
static constexpr float distance_factor_max = 5;
static constexpr float scatter_factor_min = 0;
static constexpr float scatter_factor_max = 50;
static constexpr float roughness_factor_min = -50.0;
static constexpr float roughness_factor_max = 0;

static float ray_length = -1;
static float distance_factor = 1;
static float scatter_factor = 20.0;
static float roughness_factor = -25.0;
static int max_stabilized_frame_num = 30;

using SharedNodeParams = eastl::shared_ptr<const NodeParams>;

#if DAGOR_DBGLEVEL > 0
static denoiser::DebugImage debug_image;
#endif

void initialize(bool half_res)
{
  if (!bvh::is_available())
    return;

  rtao_hit_dist_paramsVarId = get_shader_variable_id("rtao_hit_dist_params");
  rtao_resolutionIVarId = get_shader_variable_id("rtao_resolutionI");
  inv_proj_tmVarId = get_shader_variable_id("inv_proj_tm");
  rtao_res_mulVarId = get_shader_variable_id("rtao_res_mul");
  rtao_checkerboardVarId = get_shader_variable_id("rtao_checkerboard");
  ShaderGlobal::set_int(rtao_res_mulVarId, half_res ? 2 : 1);
  if (!trace)
    trace = new_compute_shader("rt_ao");

  denoiser::resolution_config.initRTAO(half_res);

  if (ray_length <= 0)
  {
    auto gfx = ::dgs_get_settings()->getBlockByNameEx("graphics");
    ray_length = gfx->getReal("rtaoRayLength", 0.95f);
    distance_factor = gfx->getReal("rtaoDistanceFactor", 1);
    scatter_factor = gfx->getReal("rtaoScatterFactor", 20);
    roughness_factor = gfx->getReal("rtaoRoughnessFactor", -25);

    ray_length = clamp(ray_length, ray_length_min, ray_length_max);
    distance_factor = clamp(distance_factor, distance_factor_min, distance_factor_max);
    scatter_factor = clamp(scatter_factor, scatter_factor_min, scatter_factor_max);
    roughness_factor = clamp(roughness_factor, roughness_factor_min, roughness_factor_max);
  }
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
  denoiser::resolution_config.closeRTAO();

  safe_delete(trace);
#if DAGOR_DBGLEVEL > 0
  debug_image.close();
#endif
}

denoiser::ParamsHandle<FrameParams> create_frame_params(dafg::Registry registry)
{
  return registry.createBlob<FrameParams>("rtao_frame_params").handle();
}

inline int divide_up(int x, int y) { return (x + y - 1) / y; }

static Point4 hit_dist_params() { return Point4(ray_length, distance_factor, scatter_factor, roughness_factor); }

static void render_noisy(const NodeParams &node, const FrameParams &params, bool checkerboard, const IPoint2 &resolution)
{
  TIME_D3D_PROFILE(rtao::render_noisy)

  bvh::bind_resources(params.ctxId, resolution.x);

  ShaderGlobal::set_int(rtao_checkerboardVarId, checkerboard ? 1 : 0);
  ShaderGlobal::set_float4(rtao_hit_dist_paramsVarId, hit_dist_params());
  ShaderGlobal::set_int4(rtao_resolutionIVarId, resolution.x, resolution.y, 0, 0);

  denoiser::trace_per_view(node.viewState, denoiser::TraceView{params.projTm}, params.lensView, [&](const denoiser::TraceView &view) {
    ShaderGlobal::set_float4x4(inv_proj_tmVarId, inverse44(view.projTm));
    trace->dispatchThreads(divide_up(resolution.x, checkerboard ? 2 : 1), resolution.y, 1);
  });

  bvh::unbind_resources();
}

static dafg::NodeHandle make_trace_node(const SharedNodeParams &params)
{
  return dafg::root().registerNode("rtao_trace", DAFG_PP_NODE_SRC, [params](dafg::Registry registry) {
    if (params->declareInputs)
      params->declareInputs(registry);
    denoiser::order_after_prepare(registry);
    auto frameParamsHndl = registry.readBlob<FrameParams>("rtao_frame_params").handle();
    auto denoiserParamsHndl = denoiser::create_ao_params(registry);
    auto outputHndl = denoiser::create_ao_trace_output(registry, "rtao_target");
    return [params, frameParamsHndl, denoiserParamsHndl, outputHndl]() {
      const FrameParams &fp = frameParamsHndl.ref();
      denoiser::AOParams &dp = denoiserParamsHndl.ref();
      dp.render = fp.render && fp.ctxId && trace && outputHndl;
      if (!dp.render)
        return;
      const bool checkerboard = fp.checkerboard && !denoiser::is_ray_reconstruction_enabled();
      dp.hitDistParams = hit_dist_params();
      dp.performanceMode = fp.performanceMode;
      dp.checkerboard = checkerboard;
      dp.maxStabilizedFrameNum = max_stabilized_frame_num;
      if (params->setShaderState)
        params->setShaderState();
      render_noisy(*params, fp, checkerboard, denoiser::texture_size(outputHndl->get()));
      if (params->resetShaderState)
        params->resetShaderState();
    };
  });
}

#if DAGOR_DBGLEVEL > 0
static dafg::NodeHandle make_debug_node()
{
  return dafg::root().registerNode("rtao_debug", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    auto outputHndl = denoiser::read_ao_output(registry);
    auto frameParamsHndl = registry.readBlob<FrameParams>("rtao_frame_params").handle();
    return [outputHndl, frameParamsHndl]() {
      debug_image.nodeRan();
      // a frame without a trace leaves the output unwritten, the window keeps its last copy
      if (outputHndl && frameParamsHndl.ref().render && imgui_window_is_visible("Render", "RTAO"))
        denoiser::copy_for_debug(outputHndl->get(), debug_image, "rtao_debug_image");
    };
  });
}
#endif

void register_nodes(const NodeParams &params, dag::Vector<dafg::NodeHandle> &nodes)
{
  SharedNodeParams shared = eastl::make_shared<NodeParams>(params);
  nodes.emplace_back(make_trace_node(shared));
  denoiser::AONodeParams denoiseParams;
  denoiseParams.motionVectorsName = params.motionVectorsName;
  denoiseParams.halfMotionVectorsName = params.halfMotionVectorsName;
  nodes.emplace_back(denoiser::make_ao_node(denoiseParams));
#if DAGOR_DBGLEVEL > 0
  nodes.emplace_back(make_debug_node());
#endif
}

#if DAGOR_DBGLEVEL > 0
static void imguiWindow()
{
  if (!debug_image.inGraph())
    return;
  ImGui::SliderFloat("Ray length", &ray_length, ray_length_min, ray_length_max);
  ImGui::SliderFloat("Distance factor", &distance_factor, distance_factor_min, distance_factor_max);
  ImGui::SliderFloat("Scatter factor", &scatter_factor, scatter_factor_min, scatter_factor_max);
  if (!denoiser::is_ray_reconstruction_enabled())
  {
    ImGui::SliderFloat("Roughness factor", &roughness_factor, roughness_factor_min, roughness_factor_max);
    ImGui::SliderInt("Max stabilized frames", &max_stabilized_frame_num, 0, denoiser::AOParams::MAX_HISTORY_FRAME_NUM);
  }

  if (Texture *tex = debug_image.texture())
  {
    const Point2 uv = debug_image.uvMax();
    ImGuiDagor::Image(tex, debug_image.aspect(), ImVec2(0, 0), ImVec2(uv.x, uv.y));
  }
}

REGISTER_IMGUI_WINDOW("Render", "RTAO", imguiWindow);
#endif

} // namespace rtao
