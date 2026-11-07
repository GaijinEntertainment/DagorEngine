// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/rtr/rtr_api.h>
#include <3d/dag_resPtr.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_bindless.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <shaders/dag_shaderBlock.h>
#include <bvh/bvh.h>
#include <perfMon/dag_statDrv.h>
#include <math/integer/dag_IPoint2.h>
#include <EASTL/shared_ptr.h>
#include <imgui/imgui.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>
#include <render/spheres_consts.hlsli>
#include "shaders/rtr_constants.hlsli"

namespace rtr
{

static ComputeShaderElement *trace = nullptr;
static ComputeShaderElement *pathTrace = nullptr;
static ComputeShaderElement *probeLocation = nullptr;
static ComputeShaderElement *probeColor = nullptr;

static DynamicShaderHelper probeDebug;

static int inv_proj_tmVarId = -1;
static int rtr_frame_indexVarId = -1;
static int rtr_hit_dist_paramsVarId = -1;
static int rtr_rough_ray_lengthVarId = -1;
static int rtr_ray_limit_coeffVarId = -1;
static int rtr_ray_limit_powerVarId = -1;
static int rtr_water_hit_dist_paramsVarId = -1;
static int rtr_resolutionIVarId = -1;
static int rtr_output_resolutionIVarId = -1;
static int rtr_output_typeVarId = -1;
static int rtr_shadowVarId = -1;
static int rtr_res_mulVarId = -1;
static int rtr_checkerboardVarId = -1;
static int rtr_probe_debug_sizeVarId = -1;
static int rtr_probes_countVarId = -1;
static int rtr_usable_probes_countVarId = -1;
static int rtr_probe_tresholdVarId = -1;
static int rtr_probesVarId = -1;
static int rtr_probe_locationsVarId = -1;
static int rtr_probe_sampler_slotVarId = -1;
static int rtr_fix_probesVarId = -1;
static int rtr_probe_randomize_raysVarId = -1;
static int rtr_probe_debug_display_backVarId = -1;
static int rtr_blur_radiusVarId = -1;
static int rtr_pt_min_ray_lengthVarId = -1;
static int rtr_pt_max_ray_lengthVarId = -1;

static float water_ray_length = 5000;
static float gloss_ray_length = 1000;
static float rough_ray_length = 1000;
static float ray_limit_coeff = 0; // 0 means no limit
static float ray_limit_power = -2.5;
static float distance_factor = 0;
static float scatter_factor = 1;
static float roughness_factor = 0;
static float probe_treshold = 0.75;
static bool use_probes = false;
static float blur_radius = 0.03;

static float pt_min_ray_length = 100;
static float pt_max_ray_length = 400;

static float al_luminance_sigma_scale = 4.0f;
static float al_luminance_sensitivity = 3.0f;

static int max_reblur_stabilized_frame_num = 31;
static int max_relax_stabilized_frame_num = 30;
static int max_relax_fast_stabilized_frame_num = 6;

static bool performance_mode = true;
static bool use_anti_firefly = true;
static bool fix_probes = false;
static bool is_checkerboard = false;
static bool is_path_traced = false;

static IPoint2 max_rendering_resolution = IPoint2::ZERO;

using SharedNodeParams = eastl::shared_ptr<const NodeParams>;

// the other output types are the values of denoiser::ReflectionMethod
static constexpr int RTR_OUTPUT_RR = 255;

#if DAGOR_DBGLEVEL > 0
static bool blur_in_graph = false;
static bool show_validation = false;
static bool display_back_probes = false;
static bool show_probes = false;
static float probe_size = 0.05;
static int rtr_validation_textureVarId = -1;
static int rtr_validation_uv_scaleVarId = -1;
static ComputeShaderElement *decode = nullptr;
static ComputeShaderElement *blurOverlay = nullptr;
static PostFxRenderer *validation_renderer = nullptr;
static denoiser::DebugImage decoded_image;
static denoiser::DebugImage validation_image;
static denoiser::DebugImage blur_overlay_image;
static denoiser::DebugImage probe_debug_atlas;
static denoiser::DebugImage probe_debug_locations;
static bool show_unfiltered = false; // the debug view shows the trace output in place of the denoised one
static int blur_overlay = 0;         // the view of rtr_blur_debug over the frame: 0 off, 1 the lobe radius, 2 the hit distance
#endif

inline int divide_up(int x, int y) { return (x + y - 1) / y; }

inline int calc_tiles_width(int w, bool checkerboard)
{
  int checkerboardWidth = divide_up(w, 2);
  return divide_up(checkerboard ? checkerboardWidth : w, 8);
}

inline int calc_tiles_height(int h) { return divide_up(h, 8); }

static IPoint2 probe_atlas_size(const IPoint2 &rendering_resolution)
{
  const IPoint2 trace = denoiser::resolution_config.rtrHalfRes ? rendering_resolution / 2 : rendering_resolution;
  const IPoint2 grid(divide_up(trace.x, PROBE_AREA), divide_up(trace.y, PROBE_AREA));
  return IPoint2(grid.x * PROBE_LAYER_MUL * PROBE_RESOLUTION_WITH_BORDER, grid.y * PROBE_RESOLUTION_WITH_BORDER);
}

static IPoint2 atlas_grid(const IPoint2 &atlas_size)
{
  return IPoint2(atlas_size.x / (PROBE_LAYER_MUL * PROBE_RESOLUTION_WITH_BORDER), atlas_size.y / PROBE_RESOLUTION_WITH_BORDER);
}

static void apply_probe_atlas_resolution()
{
  if (trace && max_rendering_resolution != IPoint2::ZERO)
    dafg::set_resolution("rtr_probe_atlas", probe_atlas_size(max_rendering_resolution));
}

void set_resolution(const IPoint2 &max_rendering_resolution_)
{
  max_rendering_resolution = max_rendering_resolution_;
  apply_probe_atlas_resolution();
}

void set_dynamic_resolution(const IPoint2 &rendering_resolution)
{
  if (!trace || max_rendering_resolution == IPoint2::ZERO)
    return;
  dafg::set_dynamic_resolution("rtr_probe_atlas", probe_atlas_size(min(rendering_resolution, max_rendering_resolution)));
}

void initialize(bool half_res, bool checkerboard, bool path_traced)
{
  if (!bvh::is_available())
    return;

  rtr_frame_indexVarId = get_shader_variable_id("rtr_frame_index");
  rtr_hit_dist_paramsVarId = get_shader_variable_id("rtr_hit_dist_params");
  rtr_rough_ray_lengthVarId = get_shader_variable_id("rtr_rough_ray_length");
  rtr_ray_limit_coeffVarId = get_shader_variable_id("rtr_ray_limit_coeff", true);
  rtr_ray_limit_powerVarId = get_shader_variable_id("rtr_ray_limit_power", true);
  rtr_water_hit_dist_paramsVarId = get_shader_variable_id("rtr_water_hit_dist_params", true);
  rtr_resolutionIVarId = get_shader_variable_id("rtr_resolutionI");
  rtr_output_resolutionIVarId = get_shader_variable_id("rtr_output_resolutionI", true);
  inv_proj_tmVarId = get_shader_variable_id("inv_proj_tm");
  rtr_output_typeVarId = get_shader_variable_id("rtr_output_type");
  rtr_res_mulVarId = get_shader_variable_id("rtr_res_mul");
  rtr_shadowVarId = get_shader_variable_id("rtr_shadow", true);
  rtr_checkerboardVarId = get_shader_variable_id("rtr_checkerboard", true);
  rtr_probe_debug_sizeVarId = get_shader_variable_id("rtr_probe_debug_size", true);
  rtr_probes_countVarId = get_shader_variable_id("rtr_probes_count", true);
  rtr_usable_probes_countVarId = get_shader_variable_id("rtr_usable_probes_count", true);
  rtr_probe_tresholdVarId = get_shader_variable_id("rtr_probe_treshold", true);
  rtr_probesVarId = get_shader_variable_id("rtr_probes", true);
  rtr_probe_locationsVarId = get_shader_variable_id("rtr_probe_locations", true);
  rtr_probe_sampler_slotVarId = get_shader_variable_id("rtr_probe_sampler_slot", true);
  rtr_fix_probesVarId = get_shader_variable_id("rtr_fix_probes", true);
  rtr_probe_randomize_raysVarId = get_shader_variable_id("rtr_probe_randomize_rays", true);
  rtr_probe_debug_display_backVarId = get_shader_variable_id("rtr_probe_debug_display_back", true);
  rtr_blur_radiusVarId = get_shader_variable_id("rtr_blur_radius", true);
  rtr_pt_min_ray_lengthVarId = get_shader_variable_id("rtr_pt_min_ray_length", true);
  rtr_pt_max_ray_lengthVarId = get_shader_variable_id("rtr_pt_max_ray_length", true);
#if DAGOR_DBGLEVEL > 0
  rtr_validation_textureVarId = get_shader_variable_id("rtr_validation_texture");
  rtr_validation_uv_scaleVarId = get_shader_variable_id("rtr_validation_uv_scale");
#endif
  ShaderGlobal::set_int(rtr_res_mulVarId, half_res ? 2 : 1);
  is_checkerboard = checkerboard;

  is_path_traced = path_traced;
  if (!trace)
    trace = new_compute_shader("rt_reflection");
  if (!pathTrace)
    pathTrace = new_compute_shader("rt_reflection_pt", !is_path_traced);

  if (!probeColor)
    probeColor = new_compute_shader("rt_reflection_probe_color");

  if (!probeLocation)
  {
#if _TARGET_PC
    if (d3d::get_driver_desc().caps.hasAtomicInt64OnGroupShared)
      probeLocation = new_compute_shader("rt_reflection_probe_location_64bit_atomics");
    else
#endif
      probeLocation = new_compute_shader("rt_reflection_probe_location");
  }

  G_ASSERT(!denoiser::is_ray_reconstruction_enabled() || !checkerboard);
  denoiser::resolution_config.initRTR(half_res);

  d3d::SamplerInfo sampler;
  sampler.filter_mode = d3d::FilterMode::Linear;
  const d3d::SamplerHandle smpHandle = d3d::request_sampler(sampler);
  ShaderGlobal::set_sampler(get_shader_variable_id("rtr_probe_sampler", true), smpHandle);
  ShaderGlobal::set_int(rtr_probe_sampler_slotVarId, d3d::register_bindless_sampler(smpHandle));
  apply_probe_atlas_resolution();
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

void set_performance_mode(bool performance_mode_) { rtr::performance_mode = performance_mode_; }

void teardown()
{
  denoiser::resolution_config.closeRTR();
  use_probes = false;

  safe_delete(trace);
  safe_delete(pathTrace);
  safe_delete(probeColor);
  safe_delete(probeLocation);

  probeDebug.close();

#if DAGOR_DBGLEVEL > 0
  safe_delete(decode);
  safe_delete(blurOverlay);
  safe_delete(validation_renderer);
  decoded_image.close();
  validation_image.close();
  blur_overlay_image.close();
  probe_debug_atlas.close();
  probe_debug_locations.close();
#endif
}

void set_water_params()
{
  if (rtr_water_hit_dist_paramsVarId < 0)
    rtr_water_hit_dist_paramsVarId = get_shader_variable_id("rtr_water_hit_dist_params", true);
  Point4 waterHitDistParams = Point4(water_ray_length, distance_factor, scatter_factor, roughness_factor);
  ShaderGlobal::set_float4(rtr_water_hit_dist_paramsVarId, waterHitDistParams);
}

static Point4 hit_dist_params() { return Point4(gloss_ray_length, distance_factor, scatter_factor, roughness_factor); }

void set_rtr_hit_distance_params()
{
  if (rtr_hit_dist_paramsVarId < 0)
    rtr_hit_dist_paramsVarId = get_shader_variable_id("rtr_hit_dist_params", true);
  ShaderGlobal::set_float4(rtr_hit_dist_paramsVarId, hit_dist_params());
}

void set_ray_limit_params(float ray_limit_coeff_, float ray_limit_power_, float probe_treshold_, bool use_rtr_probes)
{
  ray_limit_coeff = ray_limit_coeff_;
  ray_limit_power = ray_limit_power_;
  probe_treshold = probe_treshold_;
  use_probes = ray_limit_coeff > 0 && use_rtr_probes;
  if (!use_probes)
    ShaderGlobal::set_int4(rtr_usable_probes_countVarId, 0, 0, 0, 0);
}

void set_use_anti_firefly(bool use_anti_firefly_) { use_anti_firefly = use_anti_firefly_; }

void set_blur_radius(float blur_radius_) { blur_radius = blur_radius_; }

denoiser::ParamsHandle<FrameParams> create_frame_params(dafg::Registry registry)
{
  return registry.createBlob<FrameParams>("rtr_frame_params").handle();
}

static void update_probes(BaseTexture *atlas, BaseTexture *locations, const IPoint2 &resolution, bool randomize_rays)
{
  TIME_D3D_PROFILE(rtr::update_probes);

  const IPoint2 grid = atlas_grid(denoiser::texture_size(atlas));
  const IPoint2 usable(min(divide_up(resolution.x, PROBE_AREA), grid.x), min(divide_up(resolution.y, PROBE_AREA), grid.y));

  ShaderGlobal::set_int4(rtr_probes_countVarId, grid.x, grid.y, 0, 0);
  ShaderGlobal::set_int4(rtr_usable_probes_countVarId, usable.x, usable.y, 0, 0);
  ShaderGlobal::set_int(rtr_probe_randomize_raysVarId, randomize_rays ? 1 : 0);
  ShaderGlobal::set_int(rtr_fix_probesVarId, fix_probes ? 1 : 0);

  // One thread is processing one tile
  probeLocation->dispatch(usable.x, usable.y, 1);

  // One group is processing one probe (PROBE_RESOLUTION * PROBE_RESOLUTION)
  probeColor->dispatch(usable.x, usable.y, PROBE_LAYER_MUL);

#if DAGOR_DBGLEVEL > 0
  if (show_probes)
  {
    denoiser::copy_for_debug(atlas, probe_debug_atlas, "rtr_probes_debug");
    denoiser::copy_for_debug(locations, probe_debug_locations, "rtr_probe_locations_debug");
  }
#else
  G_UNUSED(locations);
#endif
}

static void bind_params(const FrameParams &params, denoiser::ReflectionMethod method, const IPoint2 &resolution)
{
  const int tilesWidth = calc_tiles_width(resolution.x, is_checkerboard);
  const int tilesHeight = calc_tiles_height(resolution.y);

  bvh::bind_resources(params.ctxId, resolution.x);

  ShaderGlobal::set_int(rtr_frame_indexVarId, denoiser::get_frame_number());
  ShaderGlobal::set_float(rtr_rough_ray_lengthVarId, rough_ray_length);
  ShaderGlobal::set_float4(rtr_hit_dist_paramsVarId, hit_dist_params());
  ShaderGlobal::set_float(rtr_ray_limit_coeffVarId, ray_limit_coeff);
  ShaderGlobal::set_float(rtr_ray_limit_powerVarId, ray_limit_power);
  ShaderGlobal::set_float4(rtr_water_hit_dist_paramsVarId,
    Point4(water_ray_length, distance_factor, scatter_factor, roughness_factor));
  ShaderGlobal::set_int4(rtr_resolutionIVarId, resolution.x, resolution.y, tilesWidth, tilesHeight);
  // the readers take the output size from here, the glass and the water traces write rtr_resolutionI too
  ShaderGlobal::set_int4(rtr_output_resolutionIVarId, resolution.x, resolution.y, 0, 0);
  ShaderGlobal::set_int(rtr_output_typeVarId, denoiser::is_ray_reconstruction_enabled() ? RTR_OUTPUT_RR : (int)method);
  ShaderGlobal::set_int(rtr_shadowVarId, params.rtShadow ? 1 : 0);
  ShaderGlobal::set_int(rtr_checkerboardVarId, is_checkerboard ? 1 : 0);
  ShaderGlobal::set_float(rtr_probe_tresholdVarId, probe_treshold);
  ShaderGlobal::set_float(rtr_blur_radiusVarId, blur_radius);
  ShaderGlobal::set_float(rtr_pt_min_ray_lengthVarId, pt_min_ray_length);
  ShaderGlobal::set_float(rtr_pt_max_ray_lengthVarId, max(pt_max_ray_length, pt_min_ray_length));
}

static ComputeShaderElement *current_trace() { return is_path_traced ? pathTrace : trace; }

static void do_trace(const NodeParams &node, const FrameParams &params, const IPoint2 &resolution)
{
  TIME_D3D_PROFILE(rtr::trace);

  denoiser::trace_per_view(node.viewState, denoiser::TraceView{params.projTm}, params.lensView, [&](const denoiser::TraceView &view) {
    ShaderGlobal::set_float4x4(inv_proj_tmVarId, inverse44(view.projTm));
    current_trace()->dispatch(calc_tiles_width(resolution.x, is_checkerboard), calc_tiles_height(resolution.y), 1);
  });
}

static dafg::NodeHandle make_probe_node(const SharedNodeParams &params)
{
  return dafg::root().registerNode("rtr_probes", DAFG_PP_NODE_SRC, [params](dafg::Registry registry) {
    if (params->declareProbeInputs)
      params->declareProbeInputs(registry);
    auto frameParamsHndl = registry.readBlob<FrameParams>("rtr_frame_params").handle();
    auto resolution = denoiser::reflection_resolution(registry);
    auto atlasHndl = registry.create("rtr_probes")
                       .texture({TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, registry.getResolution<2>("rtr_probe_atlas")})
                       .withHistory(dafg::History::ClearZeroOnFirstFrame)
                       .clear(ResourceClearValue{})
                       .atStage(dafg::Stage::CS)
                       .bindToShaderVar("rtr_probes")
                       .handle();
    registry.readTextureHistory("rtr_probes").atStage(dafg::Stage::CS).bindToShaderVar("rtr_probes_prev");
    auto locationsHndl = registry.create("rtr_probe_locations")
                           .texture({TEXFMT_A32B32G32R32F | TEXCF_UNORDERED,
                             registry.getResolution<2>("rtr_probe_atlas", 1.f / PROBE_RESOLUTION_WITH_BORDER)})
                           .withHistory(dafg::History::ClearZeroOnFirstFrame)
                           .clear(ResourceClearValue{})
                           .atStage(dafg::Stage::CS)
                           .bindToShaderVar("rtr_probe_locations")
                           .handle();
    registry.readTextureHistory("rtr_probe_locations").atStage(dafg::Stage::CS).bindToShaderVar("rtr_probe_locations_prev");
    return [params, frameParamsHndl, resolution, atlasHndl, locationsHndl]() {
      const FrameParams &fp = frameParamsHndl.ref();
      if (!use_probes || !fp.render || !fp.ctxId || !probeLocation || !probeColor)
        return;
      if (params->setShaderState)
        params->setShaderState();
      const IPoint2 res = resolution.get();
      bind_params(fp, params->method, res);
      update_probes(atlasHndl.get(), locationsHndl.get(), res, fp.randomizeRays);
      bvh::unbind_resources();
      if (params->resetShaderState)
        params->resetShaderState();
    };
  });
}

static dafg::NodeHandle make_trace_node(const SharedNodeParams &params)
{
  return dafg::root().registerNode("rtr_trace", DAFG_PP_NODE_SRC, [params](dafg::Registry registry) {
    if (params->declareInputs)
      params->declareInputs(registry);
    denoiser::order_after_prepare(registry);
    auto frameParamsHndl = registry.readBlob<FrameParams>("rtr_frame_params").handle();
    auto denoiserParamsHndl = denoiser::create_reflection_params(registry);
    auto unfilteredHndl = denoiser::create_reflection_trace_output(registry, "rtr_target");
    denoiser::create_reflection_output(registry, "rtr_denoised", params->method);
    registry.readTexture("rtr_probes").atStage(dafg::Stage::CS).bindlessShaderVar("rtr_probes_tex_slot").optional();
    registry.readTexture("rtr_probe_locations").atStage(dafg::Stage::CS).bindlessShaderVar("rtr_probe_locations_tex_slot").optional();
    if (!denoiser::is_ray_reconstruction_enabled())
    {
      // with ray reconstruction the trace shades from the gbuffer
      const denoiser::Inputs inputs = denoiser::reflection_inputs(params->checkerboardDepth, params->smartDepth);
      denoiser::modify_view_z_for_reflection(registry, "denoiser_view_z", inputs);
      denoiser::read_normal_roughness(registry, "rt_nr", inputs); // the normals of the pixels the view z set selects
      if (params->smartDepth)
        denoiser::read_selected_depth(registry, "half_selected_depth_rtr");
    }
    return [params, frameParamsHndl, denoiserParamsHndl, unfilteredHndl]() {
      const FrameParams &fp = frameParamsHndl.ref();
      denoiser::ReflectionParams &dp = denoiserParamsHndl.ref();
      dp.render = fp.render && fp.ctxId && current_trace() && unfilteredHndl;
      if (!dp.render)
        return;
      G_ASSERT(ray_limit_coeff != 0 || params->method == denoiser::ReflectionMethod::Blur);

      const bool reblur = params->method == denoiser::ReflectionMethod::Reblur;
      dp.hitDistParams = hit_dist_params();
      dp.blurRadius = blur_radius;
      dp.blurMaxRoughness = probe_treshold;
      dp.antilagSettings = Point2(al_luminance_sigma_scale, al_luminance_sensitivity);
      dp.maxStabilizedFrameNum = reblur ? max_reblur_stabilized_frame_num : max_relax_stabilized_frame_num;
      dp.maxFastStabilizedFrameNum = max_relax_fast_stabilized_frame_num;
      dp.antiFirefly = use_anti_firefly;
      dp.performanceMode = performance_mode;
      dp.checkerboard = is_checkerboard;

      if (params->setShaderState)
        params->setShaderState();
      const IPoint2 resolution = denoiser::texture_size(unfilteredHndl->get());
      bind_params(fp, params->method, resolution);
      do_trace(*params, fp, resolution);
      bvh::unbind_resources();
      if (params->resetShaderState)
        params->resetShaderState();
    };
  });
}

#if DAGOR_DBGLEVEL > 0
static void decode_for_debug(BaseTexture *src, bool packed)
{
  if (!src)
    return;
  if (!decode)
    decode = new_compute_shader("rt_reflection_decode");

  // the decode averages 2x2 texels of the input, a checkerboard trace holds its output packed in the left half
  IPoint2 valid = denoiser::texture_size(src);
  if (packed)
    valid.x /= 2;
  const IPoint2 size = valid / 2;
  Texture *dst = denoiser::fit_debug_image(decoded_image, size, TEXCF_UNORDERED | TEXFMT_A16B16G16R16F, "rtr_decoded_debug");
  if (!dst)
    return;

  static int rtr_reflection_decode_srcVarId = get_shader_variable_id("rtr_reflection_decode_src");
  static int rtr_reflection_decode_dstVarId = get_shader_variable_id("rtr_reflection_decode_dst");

  ShaderGlobal::set_texture(rtr_reflection_decode_srcVarId, src);
  ShaderGlobal::set_texture(rtr_reflection_decode_dstVarId, dst);
  static int rtr_reflection_decode_sizeVarId = get_shader_variable_id("rtr_reflection_decode_size", true);
  ShaderGlobal::set_int4(rtr_reflection_decode_sizeVarId, valid.x, valid.y, 0, 0); // the shader takes the size from here

  decode->dispatchThreads(size.x, size.y, 1);

  ShaderGlobal::set_texture(rtr_reflection_decode_srcVarId, nullptr);
  ShaderGlobal::set_texture(rtr_reflection_decode_dstVarId, nullptr);
}

static void make_blur_overlay(BaseTexture *output, BaseTexture *trace)
{
  if (!blurOverlay)
    blurOverlay = new_compute_shader("rtr_blur_debug");

  const IPoint2 size = denoiser::texture_size(output);
  Texture *dst = denoiser::fit_debug_image(blur_overlay_image, size, TEXCF_UNORDERED | TEXFMT_A16B16G16R16F, "rtr_blur_overlay");
  if (!dst)
    return;

  static int rtr_blur_debug_lobeVarId = get_shader_variable_id("rtr_blur_debug_lobe");
  static int rtr_blur_debug_traceVarId = get_shader_variable_id("rtr_blur_debug_trace");
  static int rtr_blur_debug_dstVarId = get_shader_variable_id("rtr_blur_debug_dst");
  static int rtr_blur_debug_sizeVarId = get_shader_variable_id("rtr_blur_debug_size");

  ShaderGlobal::set_texture(rtr_blur_debug_lobeVarId, output);
  ShaderGlobal::set_texture(rtr_blur_debug_traceVarId, trace);
  ShaderGlobal::set_texture(rtr_blur_debug_dstVarId, dst);
  ShaderGlobal::set_int4(rtr_blur_debug_sizeVarId, size.x, size.y, blur_overlay, int(blur_radius * size.y));

  blurOverlay->dispatchThreads(size.x, size.y, 1);

  ShaderGlobal::set_texture(rtr_blur_debug_lobeVarId, nullptr);
  ShaderGlobal::set_texture(rtr_blur_debug_traceVarId, nullptr);
  ShaderGlobal::set_texture(rtr_blur_debug_dstVarId, nullptr);
}

static dafg::NodeHandle make_debug_node(bool validation)
{
  return dafg::root().registerNode("rtr_debug", DAFG_PP_NODE_SRC, [validation](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    auto outputHndl = denoiser::read_reflection_output(registry);
    auto unfilteredHndl = denoiser::read_reflection_trace_output(registry);
    auto validationOutputHndl = validation ? denoiser::read_reflection_validation(registry) : denoiser::OutputHandle{};
    auto frameParamsHndl = registry.readBlob<FrameParams>("rtr_frame_params").handle();
    return [outputHndl, unfilteredHndl, validationOutputHndl, frameParamsHndl]() {
      decoded_image.nodeRan();
      if (!frameParamsHndl.ref().render) // a frame without a trace leaves the outputs unwritten, the window keeps its last copies
        return;
      if (validationOutputHndl)
        denoiser::copy_for_debug(validationOutputHndl->get(), validation_image, "rtr_validation_debug");
      if (blur_in_graph && blur_overlay && outputHndl && unfilteredHndl)
        make_blur_overlay(outputHndl->get(), unfilteredHndl->get());
      if (!imgui_window_is_visible("Render", "RTR"))
        return;
      if (show_unfiltered && unfilteredHndl)
        decode_for_debug(unfilteredHndl->get(), is_checkerboard);
      else if (outputHndl)
        decode_for_debug(outputHndl->get(), false);
    };
  });
}
#endif

void register_nodes(const NodeParams &params, dag::Vector<dafg::NodeHandle> &nodes)
{
  SharedNodeParams shared = eastl::make_shared<NodeParams>(params);
  G_ASSERTF(!(use_probes && is_path_traced), "rtr: the probes and the path tracing are mutually exclusive");
  if (use_probes)
  {
    apply_probe_atlas_resolution();
    nodes.emplace_back(make_probe_node(shared));
  }
  nodes.emplace_back(make_trace_node(shared));
  denoiser::ReflectionNodeParams denoiseParams;
  denoiseParams.method = params.method;
  denoiseParams.checkerboardDepth = params.checkerboardDepth;
  denoiseParams.useSmartDepth = params.smartDepth;
  denoiseParams.motionVectorsName = params.motionVectorsName;
  denoiseParams.halfMotionVectorsName = params.halfMotionVectorsName;
#if DAGOR_DBGLEVEL > 0
  const bool blur = params.method == denoiser::ReflectionMethod::Blur;
  denoiseParams.validation = is_validation_layer_enabled() && !blur;
  blur_in_graph = blur && !denoiser::is_ray_reconstruction_enabled();
#endif
  nodes.emplace_back(denoiser::make_reflection_node(denoiseParams));
  nodes.emplace_back(denoiser::make_reflection_normal_roughness_node());
#if DAGOR_DBGLEVEL > 0
  nodes.emplace_back(make_debug_node(denoiseParams.validation));
#endif
}

bool is_validation_layer_enabled()
{
#if DAGOR_DBGLEVEL > 0
  return show_validation && !denoiser::is_ray_reconstruction_enabled();
#else
  return false;
#endif
}

void render_validation_layer()
{
#if DAGOR_DBGLEVEL > 0
  const denoiser::DebugImage &image = blur_in_graph ? blur_overlay_image : validation_image;
  if (!(blur_in_graph ? blur_overlay != 0 : is_validation_layer_enabled()) || !image.texture())
    return;

  if (!validation_renderer)
    validation_renderer = new PostFxRenderer("rtr_validation_renderer");

  ShaderGlobal::set_texture(rtr_validation_textureVarId, image.texture());
  ShaderGlobal::set_float4(rtr_validation_uv_scaleVarId, image.uvMax());
  validation_renderer->render();
  ShaderGlobal::set_texture(rtr_validation_textureVarId, nullptr);
#endif
}

void render_probes()
{
#if DAGOR_DBGLEVEL > 0
  if (!show_probes)
    return;

  TIME_D3D_PROFILE(rtr::render_probes);

  Texture *atlas = probe_debug_atlas.texture();
  Texture *locations = probe_debug_locations.texture();
  if (!atlas || !locations)
    return;

  if (!probeDebug.shader)
    probeDebug.init("rt_reflection_probe_debug", nullptr, 0, "rt_reflection_probe_debug");

  SCOPE_RESET_SHADER_BLOCKS;

  ShaderGlobal::set_texture(rtr_probesVarId, atlas);
  ShaderGlobal::set_texture(rtr_probe_locationsVarId, locations);
  ShaderGlobal::set_float(rtr_probe_debug_sizeVarId, probe_size);
  const IPoint2 copyGrid = atlas_grid(denoiser::texture_size(atlas));
  ShaderGlobal::set_int4(rtr_probes_countVarId, copyGrid.x, copyGrid.y, 0, 0);

  probeDebug.shader->setStates(0, true);

  IPoint4 probesToDraw = ShaderGlobal::get_int4(rtr_usable_probes_countVarId);
  d3d::draw_instanced(PRIM_TRILIST, 0, SPHERES_INDICES_TO_DRAW, probesToDraw.x * probesToDraw.y);

  ShaderGlobal::set_texture(rtr_probesVarId, nullptr);
  ShaderGlobal::set_texture(rtr_probe_locationsVarId, nullptr);
#endif
}

#if DAGOR_DBGLEVEL > 0
static void imguiWindow()
{
  if (!decoded_image.inGraph())
    return;
  ImGui::SliderFloat("Water ray length", &water_ray_length, 0.01f, 5000);
  ImGui::SliderFloat("Gloss ray length", &gloss_ray_length, 0.01f, 5000);
  ImGui::SliderFloat("Rough ray length", &rough_ray_length, 0.01f, 5000);
  ImGui::SliderFloat("Maximum ray limit coeff", &ray_limit_coeff, 0.01f, 5000);
  ImGui::SliderFloat("Maximum ray limit power", &ray_limit_power, 0.01f, 5);
  if (is_path_traced)
  {
    ImGui::SliderFloat("PT min ray length", &pt_min_ray_length, 0.01f, 5000);
    ImGui::SliderFloat("PT max ray length", &pt_max_ray_length, pt_min_ray_length, 5000);
    pt_max_ray_length = max(pt_max_ray_length, pt_min_ray_length);
  }

  if (blur_in_graph)
  {
    ImGui::SliderFloat("Max blur radius", &blur_radius, 0, 0.1f);
    ImGui::Checkbox("Show unfiltered", &show_unfiltered);
    ImGui::Combo("Overlay", &blur_overlay, "Off\0Lobe radius\0Hit distance\0");
    if (blur_overlay == 1)
      ImGui::TextUnformatted("green: blur up to the max radius, yellow to red: mips, a band per mip, magenta: no ray");
    else if (blur_overlay == 2)
      ImGui::TextUnformatted("red 1/16 m to blue 1 km, a band per octave, white: miss, magenta: no ray");
  }
  else if (!denoiser::is_ray_reconstruction_enabled())
  {
    ImGui::SliderFloat("Distance factor", &distance_factor, 0, 1);
    ImGui::SliderFloat("Scatter factor", &scatter_factor, 0, 2);
    ImGui::SliderFloat("Roughness factor", &roughness_factor, 0, 10);

    ImGui::SliderFloat("luminance sigma scale", &al_luminance_sigma_scale, 1, 5);
    ImGui::SliderFloat("luminance sensitivity", &al_luminance_sensitivity, 1, 5);

    ImGui::Checkbox("Performance mode", &performance_mode);
    ImGui::SliderInt("Max reblur stabilized frames", &max_reblur_stabilized_frame_num, 0,
      denoiser::ReflectionParams::REBLUR_MAX_HISTORY_FRAME_NUM);
    ImGui::SliderInt("Max relax stabilized frames", &max_relax_stabilized_frame_num, 0,
      denoiser::ReflectionParams::RELAX_MAX_HISTORY_FRAME_NUM);
    ImGui::SliderInt("Max relax fast stabilized frames", &max_relax_fast_stabilized_frame_num, 0, max_relax_stabilized_frame_num);

    ImGui::Checkbox("Show unfiltered", &show_unfiltered);
    ImGui::Checkbox("Show validation layer", &show_validation);
    ImGui::Checkbox("Use anti firefly", &use_anti_firefly);
  }
  ImGui::Checkbox("Show probes", &show_probes);
  if (show_probes)
  {
    ImGui::Checkbox("Fix probes", &fix_probes);
    ImGui::Checkbox("Display back probes", &display_back_probes);
    ShaderGlobal::set_int(rtr_probe_debug_display_backVarId, display_back_probes);
    ImGui::SliderFloat("Probe size", &probe_size, 0.01, 2);
  }

  if (Texture *tex = decoded_image.texture())
  {
    const Point2 uv = decoded_image.uvMax();
    ImGuiDagor::Image(tex, decoded_image.aspect(), ImVec2(0, 0), ImVec2(uv.x, uv.y));
  }
}

REGISTER_IMGUI_WINDOW("Render", "RTR", imguiWindow);
#endif

} // namespace rtr
