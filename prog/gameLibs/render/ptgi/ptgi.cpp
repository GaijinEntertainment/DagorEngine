// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/ptgi/ptgi_api.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_postFxRenderer.h>
#include <bvh/bvh.h>
#include <perfMon/dag_statDrv.h>
#include <math/integer/dag_IPoint2.h>
#include <EASTL/shared_ptr.h>
#include <imgui/imgui.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>

namespace ptgi
{

static ComputeShaderElement *trace = nullptr;

#if DAGOR_DBGLEVEL > 0
static constexpr float ray_length_min = 0.01f;
static constexpr float ray_length_max = 200;
static constexpr float distance_factor_min = 0.01;
static constexpr float distance_factor_max = 5;
static constexpr float scatter_factor_min = 0;
static constexpr float scatter_factor_max = 50;
static constexpr float roughness_factor_min = -50.0;
static constexpr float roughness_factor_max = 0;
static bool mul_debug_by_albedo = true;
static bool decode_tonemap = true;
static bool show_validation = false;
static ComputeShaderElement *decode = nullptr;
static PostFxRenderer *validation_renderer = nullptr;
static denoiser::DebugImage decoded_image;
static denoiser::DebugImage validation_image;
#endif

static int inv_proj_tmVarId = -1;
static int ptgi_resolutionIVarId = -1;
static int ptgi_res_mulVarId = -1;
static int ptgi_hit_dist_paramsVarId = -1;
static int ptgi_max_bouncesVarId = -1;
static int ptgi_checkerboardVarId = -1;
static int ptgi_boostVarId = -1;
static int ptgi_boost_rangeVarId = -1;
static int ptgi_use_rrVarId = -1;
static int rt_half_rateVarId = -1;
static int ptgi_validation_textureVarId = -1;
static int ptgi_validation_uv_scaleVarId = -1;

static float ray_length = 100;
static float distance_factor = 1;
static float scatter_factor = 20.0;
static float roughness_factor = -25.0;
static float boost = 0;
static float boost_range = 0;
static int max_stabilized_frame_num = denoiser::GIParams::MAX_HISTORY_FRAME_NUM;
static bool anti_firefly = true;
static bool second_chance_history = false;
static bool second_chance_debug = false;

using SharedNodeParams = eastl::shared_ptr<const NodeParams>;

void initialize(bool half_res)
{
  if (!bvh::is_available())
    return;

  inv_proj_tmVarId = get_shader_variable_id("inv_proj_tm");
  ptgi_resolutionIVarId = get_shader_variable_id("ptgi_resolutionI");
  ptgi_res_mulVarId = get_shader_variable_id("ptgi_res_mul");
  ptgi_hit_dist_paramsVarId = get_shader_variable_id("ptgi_hit_dist_params");
  ptgi_max_bouncesVarId = get_shader_variable_id("ptgi_max_bounces");
  ptgi_checkerboardVarId = get_shader_variable_id("ptgi_checkerboard");
  ptgi_boostVarId = get_shader_variable_id("ptgi_boost");
  ptgi_boost_rangeVarId = get_shader_variable_id("ptgi_boost_range");
  ptgi_use_rrVarId = get_shader_variable_id("ptgi_use_rr");
  rt_half_rateVarId = get_shader_variable_id("rt_half_rate", true);
  ptgi_validation_textureVarId = get_shader_variable_id("ptgi_validation_texture");
  ptgi_validation_uv_scaleVarId = get_shader_variable_id("ptgi_validation_uv_scale");

  denoiser::resolution_config.initPTGI(half_res);
  ShaderGlobal::set_int(ptgi_res_mulVarId, half_res ? 2 : 1);

  if (!trace)
    trace = new_compute_shader("rt_gi");
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
  denoiser::resolution_config.closePTGI();
  safe_delete(trace);
#if DAGOR_DBGLEVEL > 0
  safe_delete(decode);
  safe_delete(validation_renderer);
  decoded_image.close();
  validation_image.close();
#endif
}

void set_second_chance_history(bool second_chance_history_) { second_chance_history = second_chance_history_; }

denoiser::ParamsHandle<FrameParams> create_frame_params(dafg::Registry registry)
{
  return registry.createBlob<FrameParams>("ptgi_frame_params").handle();
}

inline int divide_up(int x, int y) { return (x + y - 1) / y; }

static Point4 hit_dist_params() { return Point4(ray_length, distance_factor, scatter_factor, roughness_factor); }

static void do_trace(const FrameParams &params, bool checkerboard, const IPoint2 &resolution)
{
  bvh::bind_resources(params.ctxId, resolution.x);

  ShaderGlobal::set_int4(ptgi_resolutionIVarId, resolution.x, resolution.y, 0, 0);

  ShaderGlobal::set_float4x4(inv_proj_tmVarId, inverse44(params.projTm));

  ShaderGlobal::set_int(ptgi_max_bouncesVarId, params.quality == Quality::Low ? 3 : params.quality == Quality::Medium ? 4 : 5);

  const bool useRR = denoiser::is_ptgi_ray_reconstruction_enabled();
  ShaderGlobal::set_int(ptgi_checkerboardVarId, checkerboard ? 1 : 0);
  ShaderGlobal::set_int(ptgi_use_rrVarId, useRR ? 1 : 0);

  const bool halfRate = params.halfRate && useRR;
  ShaderGlobal::set_int(rt_half_rateVarId, halfRate ? 1 : 0);

  ShaderGlobal::set_float4(ptgi_hit_dist_paramsVarId, hit_dist_params());

  if (boost_range > 0)
  {
    ShaderGlobal::set_float(ptgi_boostVarId, boost);
    ShaderGlobal::set_float(ptgi_boost_rangeVarId, boost_range);
  }
  else if (params.inCockpit)
  {
    ShaderGlobal::set_float(ptgi_boostVarId, 0.5);
    ShaderGlobal::set_float(ptgi_boost_rangeVarId, 5);
  }
  else
  {
    ShaderGlobal::set_float(ptgi_boostVarId, 0);
    ShaderGlobal::set_float(ptgi_boost_rangeVarId, 0);
  }

  {
    TIME_D3D_PROFILE(ptgi::render_noisy)
    const int threadsX = divide_up(resolution.x, (checkerboard || halfRate) ? 2 : 1);
    // Half rate uses two threads for each 2x2 quad, so the quad row on an odd bottom edge needs both
    const int threadsY = halfRate ? divide_up(resolution.y, 2) * 2 : resolution.y;
    trace->dispatchThreads(threadsX, threadsY, 1);
  }

  bvh::unbind_resources();
}

static dafg::NodeHandle make_trace_node(const SharedNodeParams &params)
{
  return dafg::root().registerNode("ptgi_trace", DAFG_PP_NODE_SRC, [params](dafg::Registry registry) {
    if (params->declareInputs)
      params->declareInputs(registry);
    denoiser::order_after_prepare(registry);
    auto frameParamsHndl = registry.readBlob<FrameParams>("ptgi_frame_params").handle();
    auto denoiserParamsHndl = denoiser::create_gi_params(registry);
    auto outputHndl = denoiser::create_gi_trace_output(registry, "ptgi_target");
    // a half res trace takes the normal and the roughness from the denoiser prepare, a full res one from the gbuffer
    if (denoiser::resolution_config.ptgiHalfRes && !denoiser::is_ptgi_ray_reconstruction_enabled())
      denoiser::read_normal_roughness(registry, "rt_nr", denoiser::Inputs::Half);
    return [params, frameParamsHndl, denoiserParamsHndl, outputHndl]() {
      const FrameParams &fp = frameParamsHndl.ref();
      denoiser::GIParams &dp = denoiserParamsHndl.ref();
      dp.render = fp.render && fp.ctxId && trace && outputHndl;
      if (!dp.render)
        return;
      const bool checkerboard = !denoiser::is_ptgi_ray_reconstruction_enabled();
      dp.hitDistParams = hit_dist_params();
      dp.checkerboard = checkerboard;
      dp.maxStabilizedFrameNum = max_stabilized_frame_num;
      dp.antiFirefly = anti_firefly;
      dp.secondChanceDebug = second_chance_debug;
      if (params->setShaderState)
        params->setShaderState();
      do_trace(fp, checkerboard, denoiser::texture_size(outputHndl->get()));
      if (params->resetShaderState)
        params->resetShaderState();
    };
  });
}

#if DAGOR_DBGLEVEL > 0
static void decode_for_debug(BaseTexture *src)
{
  if (!src)
    return;
  if (!decode)
    decode = new_compute_shader("rt_gi_decode");

  // the decode takes every res_mul-th texel of the input
  const int resMul = denoiser::resolution_config.ptgiHalfRes ? 2 : 1;
  const IPoint2 valid = denoiser::texture_size(src);
  const IPoint2 size = valid / resMul;
  Texture *dst = denoiser::fit_debug_image(decoded_image, size, TEXCF_UNORDERED | TEXFMT_A16B16G16R16F, "ptgi_decoded_debug");
  if (!dst)
    return;

  static int ptgi_decode_srcVarId = get_shader_variable_id("ptgi_decode_src");
  static int ptgi_decode_dstVarId = get_shader_variable_id("ptgi_decode_dst");
  static int ptgi_decode_albedo_mulVarId = get_shader_variable_id("ptgi_decode_albedo_mul");
  static int ptgi_decode_tonemapVarId = get_shader_variable_id("ptgi_decode_tonemap");

  ShaderGlobal::set_texture(ptgi_decode_srcVarId, src);
  ShaderGlobal::set_texture(ptgi_decode_dstVarId, dst);
  ShaderGlobal::set_int(ptgi_decode_albedo_mulVarId, mul_debug_by_albedo ? 1 : 0);
  ShaderGlobal::set_int(ptgi_decode_tonemapVarId, decode_tonemap ? 1 : 0);
  static int ptgi_decode_sizeVarId = get_shader_variable_id("ptgi_decode_size", true);
  ShaderGlobal::set_int4(ptgi_decode_sizeVarId, valid.x, valid.y, 0, 0); // the shader takes the size from here

  decode->dispatchThreads(size.x, size.y, 1);

  ShaderGlobal::set_texture(ptgi_decode_srcVarId, nullptr);
  ShaderGlobal::set_texture(ptgi_decode_dstVarId, nullptr);
}

static dafg::NodeHandle make_debug_node(const SharedNodeParams &params, bool validation)
{
  return dafg::root().registerNode("ptgi_debug", DAFG_PP_NODE_SRC, [params, validation](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    if (params->declareDebugInputs)
      params->declareDebugInputs(registry);
    auto outputHndl = denoiser::read_gi_output(registry);
    auto validationOutputHndl = validation ? denoiser::read_gi_validation(registry) : denoiser::OutputHandle{};
    auto frameParamsHndl = registry.readBlob<FrameParams>("ptgi_frame_params").handle();
    return [outputHndl, validationOutputHndl, frameParamsHndl]() {
      decoded_image.nodeRan();
      if (!frameParamsHndl.ref().render) // a frame without a trace leaves the outputs unwritten, the window keeps its last copies
        return;
      if (validationOutputHndl)
        denoiser::copy_for_debug(validationOutputHndl->get(), validation_image, "ptgi_validation_debug");
      if (outputHndl && imgui_window_is_visible("Render", "PTGI"))
        decode_for_debug(outputHndl->get());
    };
  });
}
#endif

void register_nodes(const NodeParams &params, dag::Vector<dafg::NodeHandle> &nodes)
{
  SharedNodeParams shared = eastl::make_shared<NodeParams>(params);
  nodes.emplace_back(make_trace_node(shared));
  denoiser::GINodeParams denoiseParams;
  denoiseParams.motionVectorsName = params.motionVectorsName;
  denoiseParams.halfMotionVectorsName = params.halfMotionVectorsName;
#if DAGOR_DBGLEVEL > 0
  denoiseParams.validation = is_validation_layer_enabled();
#endif
  denoiseParams.secondChanceHistory = is_second_chance_history_enabled();
  nodes.emplace_back(denoiser::make_gi_node(denoiseParams));
  nodes.emplace_back(denoiser::make_gi_view_z_node());
#if DAGOR_DBGLEVEL > 0
  nodes.emplace_back(make_debug_node(shared, denoiseParams.validation));
#endif
}

bool is_validation_layer_enabled()
{
#if DAGOR_DBGLEVEL > 0
  return show_validation && !denoiser::is_ptgi_ray_reconstruction_enabled();
#else
  return false;
#endif
}

bool is_second_chance_history_enabled() { return second_chance_history && !denoiser::is_ptgi_ray_reconstruction_enabled(); }

void render_validation_layer()
{
#if DAGOR_DBGLEVEL > 0
  if (!is_validation_layer_enabled() || !validation_image.texture())
    return;

  if (!validation_renderer)
    validation_renderer = new PostFxRenderer("ptgi_validation_renderer");

  ShaderGlobal::set_texture(ptgi_validation_textureVarId, validation_image.texture());
  ShaderGlobal::set_float4(ptgi_validation_uv_scaleVarId, validation_image.uvMax());
  validation_renderer->render();
  ShaderGlobal::set_texture(ptgi_validation_textureVarId, nullptr);
#endif
}

#if DAGOR_DBGLEVEL > 0
static void imguiWindow()
{
  if (!decoded_image.inGraph())
    return;
  if (!denoiser::is_ptgi_ray_reconstruction_enabled())
  {
    ImGui::Checkbox("Multiply by albedo", &mul_debug_by_albedo);
    ImGui::Checkbox("Tonemap", &decode_tonemap);
  }
  ImGui::SliderFloat("Boost", &boost, 0, 2);
  ImGui::SliderFloat("Boost range", &boost_range, 0, 10);
  ImGui::SliderFloat("Ray length", &ray_length, ray_length_min, ray_length_max);
  if (!denoiser::is_ptgi_ray_reconstruction_enabled())
  {
    ImGui::SliderFloat("Distance factor", &distance_factor, distance_factor_min, distance_factor_max);
    ImGui::SliderFloat("Scatter factor", &scatter_factor, scatter_factor_min, scatter_factor_max);
    ImGui::SliderFloat("Roughness factor", &roughness_factor, roughness_factor_min, roughness_factor_max);
    ImGui::SliderInt("Max stabilized frames", &max_stabilized_frame_num, 0, denoiser::GIParams::MAX_HISTORY_FRAME_NUM);
    ImGui::Checkbox("Anti firefly", &anti_firefly);
    ImGui::Checkbox("Second chance history", &second_chance_history);
    if (second_chance_history)
      ImGui::Checkbox("Show second chance", &second_chance_debug);
    ImGui::Checkbox("Show validation layer", &show_validation);
  }

  if (Texture *tex = decoded_image.texture())
  {
    const Point2 uv = decoded_image.uvMax();
    ImGuiDagor::Image(tex, decoded_image.aspect(), ImVec2(0, 0), ImVec2(uv.x, uv.y));
  }
}

REGISTER_IMGUI_WINDOW("Render", "PTGI", imguiWindow);
#endif

} // namespace ptgi
