// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/rtr.h>
#include <render/denoiser.h>
#include <3d/dag_resPtr.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_bindless.h>
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_shaderConstants.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <shaders/dag_shaderBlock.h>
#include <bvh/bvh.h>
#include <perfMon/dag_statDrv.h>
#include <image/dag_texPixel.h>
#include <math/dag_frustum.h>
#include <math/integer/dag_IPoint2.h>
#include <ioSys/dag_dataBlock.h>
#include <EASTL/algorithm.h>
#include <EASTL/optional.h>
#include <imgui/imgui.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>
#include <render/spheres_consts.hlsli>
#include "shaders/rtr_constants.hlsli"


#if _TARGET_PC
namespace
{
struct PostmortemTraceScope
{
  PostmortemTraceScope() { d3d::driver_command(Drv3dCommand::SET_GPU_POSTMORTEM_DATA_TRACE_ENABLED, (void *)(uintptr_t)1); }
  ~PostmortemTraceScope() { d3d::driver_command(Drv3dCommand::SET_GPU_POSTMORTEM_DATA_TRACE_ENABLED, (void *)(uintptr_t)0); }
};
} // namespace

#define PUSH_AFTERMATH_MARKER(name) d3d::driver_command(Drv3dCommand::AFTERMATH_MARKER, (void *)#name)
#define GPU_POSTMORTEM_TRACE_SCOPE  PostmortemTraceScope postmortemTraceScope;
#else
#define PUSH_AFTERMATH_MARKER(name)
#define GPU_POSTMORTEM_TRACE_SCOPE
#endif

namespace rtr
{

namespace TextureNames
{
static const char *decoded_denoised_reflection = "decoded_denoised_reflection";
}; // namespace TextureNames

static ComputeShaderElement *trace = nullptr;
static ComputeShaderElement *decode = nullptr;
static ComputeShaderElement *probeLocation = nullptr;
static ComputeShaderElement *probeColor = nullptr;

static DynamicShaderHelper probeDebug;

static int inv_proj_tmVarId = -1;
static int rtr_targetVarId = -1;
static int rtr_frame_indexVarId = -1;
static int rtr_hit_dist_paramsVarId = -1;
static int rtr_rough_ray_lengthVarId = -1;
static int rtr_ray_limit_coeffVarId = -1;
static int rtr_ray_limit_powerVarId = -1;
static int rtr_water_hit_dist_paramsVarId = -1;
static int rtr_resolutionVarId = -1;
static int rtr_resolutionIVarId = -1;
static int rtr_texture_resolutionVarId = -1;
static int rtr_uv_maxVarId = -1;
static int rtr_bindless_slotVarId = -1;
static int rtr_output_typeVarId = -1;
static int rtr_denoisedVarId = -1;
static int rtr_validation_textureVarId = -1;
static int rtr_shadowVarId = -1;
static int rtr_use_csmVarId = -1;
static int rtr_fom_shadowVarId = -1;
static int rtr_res_mulVarId = -1;
static int rtr_checkerboardVarId = -1;
static int rtr_probe_debug_sizeVarId = -1;
static int rtr_probes_countVarId = -1;
static int rtr_usable_probes_countVarId = -1;
static int rtr_probe_tresholdVarId = -1;
static int rtr_probes_tex_slotVarId = -1;
static int rtr_probe_locations_buf_slotVarId = -1;
static int rtr_probe_sampler_slotVarId = -1;
static int rt_nrVarId = -1;
static int cameraFovYVarId = 1;
static int downsampled_close_depth_texVarId = -1;
static int denoiser_glass_history_confidence_tweakVarId = -1;
static int denoiser_view_zVarId = -1;

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

static float al_luminance_sigma_scale = 4.0f;
static float al_luminance_sensitivity = 3.0f;

static int max_reblur_stabilized_frame_num = 31;
static int max_relax_stabilized_frame_num = 30;
static int max_relax_fast_stabilized_frame_num = 6;

static float glass_tweak_full = 0.3;
static float glass_tweak_half = 0.6;

static denoiser::ReflectionMethod output_type = denoiser::ReflectionMethod::Relax;

static bool performance_mode = true;
static bool show_validation = false;
static bool use_anti_firefly = true;
static bool show_probes = false;
static bool fix_probes = false;
static float probe_size = 0.05;

static d3d::SamplerHandle linear_sampler = d3d::INVALID_SAMPLER_HANDLE;

static UniqueTexHolder rtr_probes;
static UniqueBufHolder rtr_probe_locations;
struct BindlessProbes
{
  uint32_t probesTex, probesSampler, locationBuf;
  bool allocated;
} rtr_probes_bindless;

static PostFxRenderer *validation_renderer;

static BaseTexture *validation_texture_ptr = nullptr;
static BaseTexture *tiles_ptr = nullptr;
static BaseTexture *normal_roughness_ptr = nullptr;
static BaseTexture *view_z_ptr = nullptr;
static BaseTexture *half_depth_ptr = nullptr;
static BaseTexture *rtr_tex_unfiltered_ptr = nullptr;
static BaseTexture *rtr_basetex = nullptr;
static BaseTexture *denoised_reflection_ptr = nullptr;
static BaseTexture *decoded_denoised_reflection_ptr = nullptr;
static float image_debug_hmul = 1;

static bvh::ContextId current_context_id = nullptr;
static bool has_rt_shadow = false;
static bool has_csm_shadow = false;
static int tiles_width = 1;
static int tiles_height = 1;
static bool is_checkerboard = false;
static Point4 hit_dist_params;

inline int divide_up(int x, int y) { return (x + y - 1) / y; }

inline int calc_tiles_width(int w, bool checkerboard)
{
  int checkerboardWidth = divide_up(w, 2);
  return divide_up(checkerboard ? checkerboardWidth : w, 8);
}

inline int calc_tiles_height(int h) { return divide_up(h, 8); }

void initialize(denoiser::ReflectionMethod method, bool half_res, bool checkerboard)
{
  if (!bvh::is_available())
    return;

  rtr::output_type = method;
  rtr_frame_indexVarId = get_shader_variable_id("rtr_frame_index");
  rtr_hit_dist_paramsVarId = get_shader_variable_id("rtr_hit_dist_params");
  rtr_rough_ray_lengthVarId = get_shader_variable_id("rtr_rough_ray_length");
  rtr_ray_limit_coeffVarId = get_shader_variable_id("rtr_ray_limit_coeff", true);
  rtr_ray_limit_powerVarId = get_shader_variable_id("rtr_ray_limit_power", true);
  rtr_water_hit_dist_paramsVarId = get_shader_variable_id("rtr_water_hit_dist_params", true);
  rtr_resolutionVarId = get_shader_variable_id("rtr_resolution");
  rtr_resolutionIVarId = get_shader_variable_id("rtr_resolutionI");
  rtr_texture_resolutionVarId = get_shader_variable_id("rtr_texture_resolution");
  rtr_uv_maxVarId = get_shader_variable_id("rtr_uv_max");
  rtr_targetVarId = get_shader_variable_id("rtr_target");
  inv_proj_tmVarId = get_shader_variable_id("inv_proj_tm");
  rtr_bindless_slotVarId = get_shader_variable_id("rtr_bindless_slot");
  rtr_output_typeVarId = get_shader_variable_id("rtr_output_type");
  rtr_denoisedVarId = get_shader_variable_id("rtr_denoised");
  rtr_validation_textureVarId = get_shader_variable_id("rtr_validation_texture");
  rtr_res_mulVarId = get_shader_variable_id("rtr_res_mul");
  rtr_shadowVarId = get_shader_variable_id("rtr_shadow", true);
  rtr_use_csmVarId = get_shader_variable_id("rtr_use_csm", true);
  rtr_fom_shadowVarId = get_shader_variable_id("rtr_fom_shadow", true);
  rtr_checkerboardVarId = get_shader_variable_id("rtr_checkerboard", true);
  rtr_probe_debug_sizeVarId = get_shader_variable_id("rtr_probe_debug_size", true);
  rtr_probes_countVarId = get_shader_variable_id("rtr_probes_count", true);
  rtr_usable_probes_countVarId = get_shader_variable_id("rtr_usable_probes_count", true);
  rtr_probe_tresholdVarId = get_shader_variable_id("rtr_probe_treshold", true);
  rtr_probes_tex_slotVarId = get_shader_variable_id("rtr_probes_tex_slot", true);
  rtr_probe_locations_buf_slotVarId = get_shader_variable_id("rtr_probe_locations_buf_slot", true);
  rtr_probe_sampler_slotVarId = get_shader_variable_id("rtr_probe_sampler_slot", true);
  rt_nrVarId = get_shader_variable_id("rt_nr");
  cameraFovYVarId = get_shader_variable_id("cameraFovY", true);
  downsampled_close_depth_texVarId = get_shader_variable_id("downsampled_close_depth_tex");
  denoiser_glass_history_confidence_tweakVarId = get_shader_variable_id("denoiser_glass_history_confidence_tweak", true);
  denoiser_view_zVarId = get_shader_variable_id("denoiser_view_z", true);
  ShaderGlobal::set_int(rtr_res_mulVarId, half_res ? 2 : 1);

  if (!trace)
    trace = new_compute_shader("rt_reflection");

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

  denoiser::resolution_config.initRTR(half_res, checkerboard);

  d3d::SamplerInfo si;
  si.mip_map_mode = d3d::MipMapMode::Point;
  si.filter_mode = d3d::FilterMode::Linear;
  si.address_mode_u = d3d::AddressMode::Clamp;
  si.address_mode_v = d3d::AddressMode::Clamp;
  linear_sampler = d3d::request_sampler(si);

  if (!rtr_probes_bindless.allocated)
  {
    rtr_probes_bindless.probesTex = d3d::allocate_bindless_resource_range(D3DResourceType::TEX, 1);
    rtr_probes_bindless.locationBuf = d3d::allocate_bindless_resource_range(D3DResourceType::SBUF, 1);
    rtr_probes_bindless.allocated = true;
  }
}

void change_method(denoiser::ReflectionMethod method) { output_type = method; }

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

  safe_delete(trace);
  safe_delete(decode);
  safe_delete(probeColor);
  safe_delete(probeLocation);
  safe_delete(validation_renderer);

  if (rtr_probes_bindless.allocated)
  {
    d3d::free_bindless_resource_range(D3DResourceType::TEX, rtr_probes_bindless.probesTex, 1);
    d3d::free_bindless_resource_range(D3DResourceType::SBUF, rtr_probes_bindless.locationBuf, 1);
    rtr_probes_bindless.allocated = false;
  }
  rtr_probes.close();
  rtr_probe_locations.close();

  probeDebug.close();

  ShaderGlobal::set_int(rtr_bindless_slotVarId, -1);
}

void set_water_params()
{
  if (rtr_water_hit_dist_paramsVarId < 0)
    rtr_water_hit_dist_paramsVarId = get_shader_variable_id("rtr_water_hit_dist_params", true);
  Point4 waterHitDistParams = Point4(water_ray_length, distance_factor, scatter_factor, roughness_factor);
  ShaderGlobal::set_float4(rtr_water_hit_dist_paramsVarId, waterHitDistParams);
}

void set_rtr_hit_distance_params()
{
  if (rtr_hit_dist_paramsVarId < 0)
    rtr_hit_dist_paramsVarId = get_shader_variable_id("rtr_hit_dist_params", true);
  Point4 hitDistParams = Point4(gloss_ray_length, distance_factor, scatter_factor, roughness_factor);
  ShaderGlobal::set_float4(rtr_hit_dist_paramsVarId, hitDistParams);
}

void set_ray_limit_params(float ray_limit_coeff_, float ray_limit_power_, float probe_treshold_, bool use_rtr_probes)
{
  ray_limit_coeff = ray_limit_coeff_;
  ray_limit_power = ray_limit_power_;
  probe_treshold = probe_treshold_;
  use_probes = ray_limit_coeff > 0 && use_rtr_probes;
  if (!use_probes)
  {
    ShaderGlobal::set_int(rtr_probe_locations_buf_slotVarId, -1);
    ShaderGlobal::set_int(rtr_probes_tex_slotVarId, -1);
    ShaderGlobal::set_int(rtr_probe_sampler_slotVarId, -1);
  }
}

void set_use_anti_firefly(bool use_anti_firefly_) { use_anti_firefly = use_anti_firefly_; }

void update_probes(IPoint2 res_size, IPoint2 resolution)
{
  TIME_D3D_PROFILE(rtr::update_probes);

  int probeCountH = divide_up(res_size.x, PROBE_AREA);
  int probeCountV = divide_up(res_size.y, PROBE_AREA);
  int probeWidth = probeCountH * PROBE_RESOLUTION_WITH_BORDER;
  int probeHeight = probeCountV * PROBE_RESOLUTION_WITH_BORDER;
  int usableProbeCountH = divide_up(resolution.x, PROBE_AREA);
  int usableProbeCountV = divide_up(resolution.y, PROBE_AREA);

  if (rtr_probes)
  {
    TextureInfo ti;
    rtr_probes->getinfo(ti);
    if (ti.w != probeWidth || ti.h != probeHeight)
    {
      d3d::update_bindless_resources_to_null(D3DResourceType::TEX, rtr_probes_bindless.probesTex, 1);
      rtr_probes.close();
    }
  }

  if (rtr_probe_locations && rtr_probe_locations->getNumElements() / 4 != probeCountH * probeCountV)
  {
    d3d::update_bindless_resources_to_null(D3DResourceType::SBUF, rtr_probes_bindless.locationBuf, 1);
    rtr_probe_locations.close();
  }

  if (!rtr_probes)
  {
    rtr_probes = dag::create_tex(nullptr, probeWidth, probeHeight, TEXFMT_A16B16G16R16F | TEXCF_UNORDERED | TEXCF_CLEAR_ON_CREATE, 1,
      "rtr_probes", RESTAG_RTR);
    d3d::SamplerInfo sampler;
    sampler.filter_mode = d3d::FilterMode::Linear;
    auto smpHandle = d3d::request_sampler(sampler);
    ShaderGlobal::set_sampler(get_shader_variable_id("rtr_probe_sampler"), smpHandle);
    rtr_probes_bindless.probesSampler = d3d::register_bindless_sampler(smpHandle);
  }

  if (!rtr_probe_locations)
    rtr_probe_locations = dag::buffers::create_ua_sr_byte_address(sizeof(Point4) / 4 * probeCountH * probeCountV,
      "rtr_probe_locations", d3d::buffers::Init::Zero, RESTAG_RTR);

  ShaderGlobal::set_int4(rtr_probes_countVarId, probeCountH, probeCountV, 0, 0);
  ShaderGlobal::set_int4(rtr_usable_probes_countVarId, usableProbeCountH, usableProbeCountV, 0, 0);

  // One thread is processing one tile
  if (!fix_probes)
    probeLocation->dispatch(usableProbeCountH, usableProbeCountV, 1);

  d3d::set_cs_constbuffer_register_count(256);
  // One group is processing one probe (PROBE_RESOLUTION * PROBE_RESOLUTION)
  probeColor->dispatch(usableProbeCountH, usableProbeCountV, 1);
  d3d::set_cs_constbuffer_register_count(0);

  d3d::resource_barrier({rtr_probe_locations.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL});
  ShaderGlobal::set_int(rtr_probe_locations_buf_slotVarId, rtr_probes_bindless.locationBuf);
  d3d::update_bindless_resource(D3DResourceType::SBUF, rtr_probes_bindless.locationBuf, rtr_probe_locations.getBuf());

  d3d::resource_barrier({rtr_probes.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 1});
  ShaderGlobal::set_int(rtr_probes_tex_slotVarId, rtr_probes_bindless.probesTex);
  ShaderGlobal::set_int(rtr_probe_sampler_slotVarId, rtr_probes_bindless.probesSampler);
  d3d::update_bindless_resource(D3DResourceType::TEX, rtr_probes_bindless.probesTex, rtr_probes.getTex2D());
}

static IPoint2 get_resolution()
{
  return denoiser::resolution_config.rtr.isHalfRes ? denoiser::resolution_config.dynRes.halfRes
                                                   : denoiser::resolution_config.dynRes.res;
}

bool prepare(bvh::ContextId context_id, bool rt_shadow, bool csm_shadow, const denoiser::TexMap &textures, bool checkerboard)
{
  TIME_D3D_PROFILE(rtr::prepare);

  G_ASSERT(ray_limit_coeff != 0);

  current_context_id = context_id;
  has_csm_shadow = csm_shadow;
  has_rt_shadow = rt_shadow;

  hit_dist_params = Point4(gloss_ray_length, distance_factor, scatter_factor, roughness_factor);

  auto tiles = textures.find(denoiser::ReflectionDenoiser::TextureNames::rtr_sample_tiles);
  auto denoised_reflection = textures.find(denoiser::ReflectionDenoiser::TextureNames::rtr_tex);
  auto reflection_value = textures.find(denoiser::ReflectionDenoiser::TextureNames::rtr_tex_unfiltered);
  auto validation = textures.find(denoiser::ReflectionDenoiser::TextureNames::rtr_validation);
  auto decoded_denoised_reflection = textures.find(TextureNames::decoded_denoised_reflection);
  auto normal_roughness =
    textures.find(denoiser::resolution_config.rtr.isHalfRes ? denoiser::TextureNames::denoiser_half_normal_roughness
                                                            : denoiser::TextureNames::denoiser_normal_roughness);
  auto view_z = textures.find(denoiser::resolution_config.rtr.isHalfRes ? denoiser::TextureNames::denoiser_half_view_z
                                                                        : denoiser::TextureNames::denoiser_view_z);
  auto half_depth = textures.find(denoiser::TextureNames::half_depth);

  G_ASSERT_RETURN(denoiser::is_ray_reconstruction_enabled() || tiles != textures.end(), false);
  G_ASSERT_RETURN(denoiser::is_ray_reconstruction_enabled() || denoised_reflection != textures.end(), false);
  G_ASSERT_RETURN(denoiser::is_ray_reconstruction_enabled() || normal_roughness != textures.end(), false);
  G_ASSERT_RETURN(denoiser::is_ray_reconstruction_enabled() || view_z != textures.end(), false);
  G_ASSERT_RETURN(denoiser::is_ray_reconstruction_enabled() || half_depth != textures.end(), false);
  G_ASSERT_RETURN(reflection_value != textures.end(), false);

  auto basetex_or_null = [&](auto name) { return name != textures.end() ? name->second : nullptr; };

  validation_texture_ptr = basetex_or_null(validation);
  tiles_ptr = basetex_or_null(tiles);
  normal_roughness_ptr = basetex_or_null(normal_roughness);
  view_z_ptr = basetex_or_null(view_z);
  half_depth_ptr = basetex_or_null(half_depth);
  rtr_tex_unfiltered_ptr = basetex_or_null(reflection_value);
  rtr_basetex = basetex_or_null(reflection_value);
  denoised_reflection_ptr = basetex_or_null(denoised_reflection);
  decoded_denoised_reflection_ptr = basetex_or_null(decoded_denoised_reflection);
  image_debug_hmul = (checkerboard ? 2 : 1);

  const IPoint2 &resolution = get_resolution();

  tiles_width = calc_tiles_width(resolution.x, checkerboard);
  tiles_height = calc_tiles_height(resolution.y);
  is_checkerboard = checkerboard;

  return true;
}

void bind_params()
{
  const IPoint2 resolution = get_resolution();

  bvh::bind_resources(current_context_id, resolution.x);

  Point4 waterHitDistParams = Point4(water_ray_length, distance_factor, scatter_factor, roughness_factor);

  ShaderGlobal::set_texture(denoiser_view_zVarId, denoiser::is_ray_reconstruction_enabled() ? nullptr : view_z_ptr);
  ShaderGlobal::set_texture(rtr_denoisedVarId, denoised_reflection_ptr);
  ShaderGlobal::set_texture(rtr_targetVarId, rtr_tex_unfiltered_ptr);
  ShaderGlobal::set_int(rtr_frame_indexVarId, denoiser::get_frame_number());
  ShaderGlobal::set_float(rtr_rough_ray_lengthVarId, rough_ray_length);
  ShaderGlobal::set_float4(rtr_hit_dist_paramsVarId, hit_dist_params);
  ShaderGlobal::set_float(rtr_ray_limit_coeffVarId, ray_limit_coeff);
  ShaderGlobal::set_float(rtr_ray_limit_powerVarId, ray_limit_power);
  ShaderGlobal::set_float4(rtr_water_hit_dist_paramsVarId, waterHitDistParams);
  ShaderGlobal::set_float4(rtr_resolutionVarId, resolution.x, resolution.y, 0, 0);
  ShaderGlobal::set_int4(rtr_resolutionIVarId, resolution.x, resolution.y, tiles_width, tiles_height);
  ShaderGlobal::set_int4(rtr_texture_resolutionVarId, denoiser::resolution_config.rtr.width, denoiser::resolution_config.rtr.height, 0,
    0);
  Point2 uvMax = Point2(1, 1);
  if (denoiser::resolution_config.rtr.width != resolution.x || denoiser::resolution_config.rtr.height != resolution.y)
  {
    // Avoid samples outside the valid area
    uvMax.x = (resolution.x - 0.6f) / denoiser::resolution_config.rtr.width;
    uvMax.y = (resolution.y - 0.6f) / denoiser::resolution_config.rtr.height;
  }
  ShaderGlobal::set_float4(rtr_uv_maxVarId, uvMax);
  constexpr int RTR_OUTPUT_RR = 2;
  ShaderGlobal::set_int(rtr_output_typeVarId, denoiser::is_ray_reconstruction_enabled() ? RTR_OUTPUT_RR : (int)output_type);
  ShaderGlobal::set_texture(rt_nrVarId, normal_roughness_ptr);
  ShaderGlobal::set_texture(downsampled_close_depth_texVarId, half_depth_ptr);
  ShaderGlobal::set_int(rtr_shadowVarId, has_rt_shadow ? 1 : 0);
  ShaderGlobal::set_int(rtr_use_csmVarId, has_csm_shadow ? 1 : 0);
  ShaderGlobal::set_int(rtr_checkerboardVarId, is_checkerboard ? 1 : 0);
  ShaderGlobal::set_float(denoiser_glass_history_confidence_tweakVarId,
    denoiser::resolution_config.rtr.isHalfRes ? glass_tweak_half : glass_tweak_full);
  ShaderGlobal::set_float(rtr_probe_tresholdVarId, probe_treshold);
}

void unbind_params()
{
  ShaderGlobal::set_texture(denoiser_view_zVarId, nullptr);
  ShaderGlobal::set_texture(rtr_denoisedVarId, nullptr);
  ShaderGlobal::set_texture(rtr_targetVarId, nullptr);

  ShaderGlobal::set_texture(rt_nrVarId, nullptr);
  ShaderGlobal::set_texture(downsampled_close_depth_texVarId, nullptr);
}

void do_update_probes()
{
  G_ASSERT(use_probes);
  const IPoint2 &resolution = get_resolution();
  update_probes(IPoint2{denoiser::resolution_config.rtr.width, denoiser::resolution_config.rtr.height}, resolution);

  PUSH_AFTERMATH_MARKER(rtr_update_probes_completed);
}

void do_trace(const TMatrix4 &proj_tm)
{
  TIME_D3D_PROFILE(rtr::trace);

  if (denoiser::is_ray_reconstruction_enabled())
  {
    const float clear[] = {0, 0, 0, 1};
    d3d::clear_rwtexf(rtr_basetex, clear, 0, 0);
  }

  ShaderGlobal::set_float4x4(inv_proj_tmVarId, inverse44(proj_tm));
  d3d::set_cs_constbuffer_register_count(128 + 32);
  trace->dispatch(tiles_width, tiles_height, 1);
  d3d::set_cs_constbuffer_register_count(0);
  d3d::resource_barrier(ResourceBarrierDesc(rtr_basetex, RB_NONE, 0, 0));

  PUSH_AFTERMATH_MARKER(rtr_do_trace_completed);
}

void denoise(const denoiser::TexMap &textures)
{
  TIME_D3D_PROFILE(rtr::denoise);

  denoiser::ReflectionDenoiser denoiser_params;
  denoiser_params.method = output_type;
  denoiser_params.hitDistParams = hit_dist_params;
  denoiser_params.maxStabilizedFrameNum =
    output_type == denoiser::ReflectionMethod::Reblur ? max_reblur_stabilized_frame_num : max_relax_stabilized_frame_num;
  denoiser_params.maxFastStabilizedFrameNum = max_relax_fast_stabilized_frame_num;
  denoiser_params.antilagSettings = Point2(al_luminance_sigma_scale, al_luminance_sensitivity);
  denoiser_params.halfResolution = denoiser::resolution_config.rtr.isHalfRes;
  denoiser_params.antiFirefly = use_anti_firefly;
  denoiser_params.performanceMode = performance_mode;
  denoiser_params.checkerboard = is_checkerboard;
  denoiser_params.textures = textures;

  denoiser::denoise_reflection(denoiser_params);

  PUSH_AFTERMATH_MARKER(rtr_denoise_completed);
}

void render(bvh::ContextId context_id, const TMatrix4 &proj_tm, bool rt_shadow, bool csm_shadow, const denoiser::TexMap &textures,
  bool checkerboard)
{
  GPU_POSTMORTEM_TRACE_SCOPE;

  if (!prepare(context_id, rt_shadow, csm_shadow, textures, checkerboard))
    return;

  bind_params();
  {
    if (use_probes)
      do_update_probes();

    do_trace(proj_tm);
  }
  unbind_params();

  denoise(textures);
}

bool is_validation_layer_enabled() { return show_validation; }

void render_validation_layer()
{
  if (!show_validation || !validation_texture_ptr)
    return;

  if (!validation_renderer)
    validation_renderer = new PostFxRenderer("rtr_validation_renderer");

  ShaderGlobal::set_texture(rtr_validation_textureVarId, validation_texture_ptr);
  validation_renderer->render();
  ShaderGlobal::set_texture(rtr_validation_textureVarId, nullptr);
}

void render_probes()
{
  if (!show_probes)
    return;

  TIME_D3D_PROFILE(rtr::render_probes);

  if (!rtr_probes || !rtr_probe_locations)
    return;

  if (!probeDebug.shader)
    probeDebug.init("rt_reflection_probe_debug", nullptr, 0, "rt_reflection_probe_debug");

  SCOPE_RESET_SHADER_BLOCKS;

  ShaderGlobal::set_float(rtr_probe_debug_sizeVarId, probe_size);

  probeDebug.shader->setStates(0, true);

  IPoint4 probesToDraw = ShaderGlobal::get_int4(rtr_usable_probes_countVarId);
  d3d::draw_instanced(PRIM_TRILIST, 0, SPHERES_INDICES_TO_DRAW, probesToDraw.x * probesToDraw.y);
}


#if DAGOR_DBGLEVEL > 0

static bool show_unfiltered = false;

inline const char *operator*(denoiser::ReflectionMethod mode)
{
  switch (mode)
  {
    case denoiser::ReflectionMethod::Reblur: return "Reblur";
    case denoiser::ReflectionMethod::Relax: return "Relax";
    default: return "Unknown";
  }
}

static void imguiWindow()
{
  ImGui::SliderFloat("Water ray length", &water_ray_length, 0.01f, 5000);
  ImGui::SliderFloat("Gloss ray length", &gloss_ray_length, 0.01f, 5000);
  ImGui::SliderFloat("Rough ray length", &rough_ray_length, 0.01f, 5000);
  ImGui::SliderFloat("Maximum ray limit coeff", &ray_limit_coeff, 0.01f, 5000);
  ImGui::SliderFloat("Maximum ray limit power", &ray_limit_power, 0.01f, 5);
  ImGui::SliderFloat("Distance factor", &distance_factor, 0, 1);
  ImGui::SliderFloat("Scatter factor", &scatter_factor, 0, 2);
  ImGui::SliderFloat("Roughness factor", &roughness_factor, 0, 10);

  ImGui::SliderFloat("luminance sigma scale", &al_luminance_sigma_scale, 1, 5);
  ImGui::SliderFloat("luminance sensitivity", &al_luminance_sensitivity, 1, 5);

  ImGui::SliderInt("Max reblur stabilized frames", &max_reblur_stabilized_frame_num, 0,
    denoiser::ReflectionDenoiser::REBLUR_MAX_HISTORY_FRAME_NUM);
  ImGui::SliderInt("Max relax stabilized frames", &max_relax_stabilized_frame_num, 0,
    denoiser::ReflectionDenoiser::RELAX_MAX_HISTORY_FRAME_NUM);
  ImGui::SliderInt("Max relax fast stabilized frames", &max_relax_fast_stabilized_frame_num, 0, max_relax_stabilized_frame_num);

  ImGui::SliderFloat("Glass tweak", denoiser::resolution_config.rtr.isHalfRes ? &glass_tweak_half : &glass_tweak_full, 0, 1);
  ShaderGlobal::set_float(denoiser_glass_history_confidence_tweakVarId,
    denoiser::resolution_config.rtr.isHalfRes ? glass_tweak_half : glass_tweak_full);

  using RDM = denoiser::ReflectionMethod;
  ImGui::Text("Denoiser: %s", *output_type);

  if (output_type == RDM::Reblur)
    ImGui::Checkbox("Performance mode", &performance_mode);

  ImGui::Checkbox("Show validation layer", &show_validation);
  ImGui::Checkbox("Use anti firefly", &use_anti_firefly);
  ImGui::Checkbox("Show probes", &show_probes);
  if (show_probes)
  {
    ImGui::Checkbox("Fix probes", &fix_probes);
    ImGui::SliderFloat("Probe size", &probe_size, 0.01, 2);
  }

  ImGui::Separator();
  ImGui::Checkbox("Show unfiltered", &show_unfiltered);

  if (denoised_reflection_ptr)
  {
    if (!decode)
      decode = new_compute_shader("rt_reflection_decode");

    static int rtr_reflection_decode_srcVarId = get_shader_variable_id("rtr_reflection_decode_src");
    static int rtr_reflection_decode_dstVarId = get_shader_variable_id("rtr_reflection_decode_dst");

    ImGui::GetWindowDrawList()->AddCallback(
      [](const ImDrawList *, const ImDrawCmd *) {
        if (decoded_denoised_reflection_ptr)
        {
          TextureInfo ti;
          decoded_denoised_reflection_ptr->getinfo(ti);

          ShaderGlobal::set_texture(rtr_reflection_decode_srcVarId,
            show_unfiltered ? rtr_tex_unfiltered_ptr : denoised_reflection_ptr);
          ShaderGlobal::set_texture(rtr_reflection_decode_dstVarId, decoded_denoised_reflection_ptr);

          decode->dispatchThreads(ti.w, ti.h, 1);

          ShaderGlobal::set_texture(rtr_reflection_decode_srcVarId, nullptr);
          ShaderGlobal::set_texture(rtr_reflection_decode_dstVarId, nullptr);
        }
      },
      nullptr);

    ImGuiDagor::Image(decoded_denoised_reflection_ptr);
  }
}

REGISTER_IMGUI_WINDOW("Render", "RTR", imguiWindow);
#endif

void turn_off() { ShaderGlobal::set_int(rtr_bindless_slotVarId, -1); }

void denoise_noop(const denoiser::TexMap &textures)
{
  denoiser::ReflectionDenoiser params;
  params.method = output_type;
  params.hitDistParams = hit_dist_params;
  params.maxStabilizedFrameNum =
    output_type == denoiser::ReflectionMethod::Reblur ? max_reblur_stabilized_frame_num : max_relax_stabilized_frame_num;
  params.maxFastStabilizedFrameNum = max_relax_fast_stabilized_frame_num;
  params.antilagSettings = Point2(al_luminance_sigma_scale, al_luminance_sensitivity);
  params.halfResolution = denoiser::resolution_config.rtr.isHalfRes;
  params.antiFirefly = use_anti_firefly;
  params.performanceMode = performance_mode;
  params.checkerboard = is_checkerboard;
  params.textures = textures;

  denoiser::denoise_reflection_noop(params);
}

void get_required_persistent_texture_descriptors(denoiser::TexInfoMap &persistent_textures)
{
  denoiser::get_required_persistent_texture_descriptors_for_rtr(persistent_textures, output_type);
#if DAGOR_DBGLEVEL > 0
  if (!denoiser::is_ray_reconstruction_enabled())
  {
    auto denoised_reflection = persistent_textures.find(denoiser::ReflectionDenoiser::TextureNames::rtr_tex);
    if (denoised_reflection == persistent_textures.end())
    {
      logerr("rtr_tex was not found in requested rtr textures!");
      return;
    }
    TextureInfo sourceTi = denoised_reflection->second;
    auto &ti = persistent_textures[TextureNames::decoded_denoised_reflection];
    ti.w = sourceTi.w / 2;
    ti.h = sourceTi.h / 2;
    ti.mipLevels = 1;
    ti.type = D3DResourceType::TEX;
    ti.cflg = TEXCF_UNORDERED | TEXFMT_A16B16G16R16F;
  }
#endif
}

void get_required_transient_texture_descriptors(denoiser::TexInfoMap &transient_textures)
{
  denoiser::get_required_transient_texture_descriptors_for_rtr(transient_textures, output_type);
}

} // namespace rtr