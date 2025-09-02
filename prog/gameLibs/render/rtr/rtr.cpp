// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/rtr.h>
#include <render/denoiser.h>
#include <3d/dag_resPtr.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_bindless.h>
#include <drv/3d/dag_info.h>
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
static int rtr_resolutionIVarId = -1;
static int rtr_bindless_slotVarId = -1;
static int rtr_output_typeVarId = -1;
static int rtr_denoisedVarId = -1;
static int rtr_validation_textureVarId = -1;
static int rtr_shadowVarId = -1;
static int rtr_use_csmVarId = -1;
static int rtr_use_rrVarId = -1;
static int rtr_fom_shadowVarId = -1;
static int rtr_res_mulVarId = -1;
static int rtr_checkerboardVarId = -1;
static int rtr_probe_cycleVarId = -1;
static int rtr_probe_periodVarId = -1;
static int rtr_probe_debug_sizeVarId = -1;
static int rtr_probes_wVarId = -1;
static int rtr_probes_hVarId = -1;
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
static bool use_anti_firefly = false;
static bool show_probes = false;
static bool fix_probes = false;
static float probe_size = 0.05;

static int probe_cycle = 0;
static int probe_period = 1;

static d3d::SamplerHandle linear_sampler = d3d::INVALID_SAMPLER_HANDLE;

static UniqueTexHolder rtr_probes;
static UniqueBufHolder rtr_probe_locations;

static PostFxRenderer *validation_renderer;

static TEXTUREID validation_texture_id = BAD_TEXTUREID;
static TEXTUREID tiles_id = BAD_TEXTUREID;
static TEXTUREID rtr_tex_unfiltered_id = BAD_TEXTUREID;
static TEXTUREID denoised_reflection_id = BAD_TEXTUREID;
static TEXTUREID decoded_denoised_reflection_id = BAD_TEXTUREID;
static float image_debug_hmul = 1;

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
  rtr_resolutionIVarId = get_shader_variable_id("rtr_resolutionI");
  rtr_targetVarId = get_shader_variable_id("rtr_target");
  inv_proj_tmVarId = get_shader_variable_id("inv_proj_tm");
  rtr_bindless_slotVarId = get_shader_variable_id("rtr_bindless_slot");
  rtr_output_typeVarId = get_shader_variable_id("rtr_output_type");
  rtr_denoisedVarId = get_shader_variable_id("rtr_denoised");
  rtr_validation_textureVarId = get_shader_variable_id("rtr_validation_texture");
  rtr_res_mulVarId = get_shader_variable_id("rtr_res_mul");
  rtr_shadowVarId = get_shader_variable_id("rtr_shadow", true);
  rtr_use_csmVarId = get_shader_variable_id("rtr_use_csm", true);
  rtr_use_rrVarId = get_shader_variable_id("rtr_use_rr", true);
  rtr_fom_shadowVarId = get_shader_variable_id("rtr_fom_shadow", true);
  rtr_checkerboardVarId = get_shader_variable_id("rtr_checkerboard", true);
  rtr_probe_cycleVarId = get_shader_variable_id("rtr_probe_cycle", true);
  rtr_probe_periodVarId = get_shader_variable_id("rtr_probe_period", true);
  rtr_probe_debug_sizeVarId = get_shader_variable_id("rtr_probe_debug_size", true);
  rtr_probes_wVarId = get_shader_variable_id("rtr_probes_w", true);
  rtr_probes_hVarId = get_shader_variable_id("rtr_probes_h", true);
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
    probeLocation = new_compute_shader("rt_reflection_probe_location");

  denoiser::resolution_config.initRTR(half_res, checkerboard);

  d3d::SamplerInfo si;
  si.mip_map_mode = d3d::MipMapMode::Point;
  si.filter_mode = d3d::FilterMode::Linear;
  si.address_mode_u = d3d::AddressMode::Clamp;
  si.address_mode_v = d3d::AddressMode::Clamp;
  linear_sampler = d3d::request_sampler(si);
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
  ShaderGlobal::set_color4(rtr_water_hit_dist_paramsVarId, waterHitDistParams);
}

void set_rtr_hit_distance_params()
{
  if (rtr_hit_dist_paramsVarId < 0)
    rtr_hit_dist_paramsVarId = get_shader_variable_id("rtr_hit_dist_params", true);
  Point4 hitDistParams = Point4(gloss_ray_length, distance_factor, scatter_factor, roughness_factor);
  ShaderGlobal::set_color4(rtr_hit_dist_paramsVarId, hitDistParams);
}

void set_ray_limit_params(float ray_limit_coeff_, float ray_limit_power_, bool use_rtr_probes)
{
  ray_limit_coeff = ray_limit_coeff_;
  ray_limit_power = ray_limit_power_;
  use_probes = ray_limit_coeff > 0 && use_rtr_probes;
}

void set_use_anti_firefly(bool use_anti_firefly_) { use_anti_firefly = use_anti_firefly_; }

void update_probes(bvh::ContextId context_id, int w, int h)
{
  TIME_D3D_PROFILE(rtr::update_probes);

  int probeCountH = divide_up(w, PROBE_AREA);
  int probeCountV = divide_up(h, PROBE_AREA);
  int probeWidth = probeCountH * PROBE_RESOLUTION_WITH_BORDER;
  int probeHeight = probeCountV * PROBE_RESOLUTION_WITH_BORDER;

  if (rtr_probes)
  {
    TextureInfo ti;
    rtr_probes->getinfo(ti);
    if (ti.w != probeWidth || ti.h != probeHeight)
      rtr_probes.close();
  }

  if (rtr_probe_locations && rtr_probe_locations->getNumElements() != probeCountH * probeCountV)
    rtr_probe_locations.close();

  if (!rtr_probes)
  {
    rtr_probes = dag::create_tex(nullptr, probeWidth, probeHeight, TEXFMT_A16B16G16R16F | TEXCF_UNORDERED | TEXCF_CLEAR_ON_CREATE, 1,
      "rtr_probes");
    d3d::SamplerInfo sampler;
    sampler.filter_mode = d3d::FilterMode::Linear;
    ShaderGlobal::set_sampler(get_shader_variable_id("rtr_probe_sampler"), d3d::request_sampler(sampler));
  }
  if (!rtr_probe_locations)
    rtr_probe_locations = dag::buffers::create_ua_sr_structured(sizeof(Point4), probeCountH * probeCountV, "rtr_probe_locations",
      d3d::buffers::Init::Zero);

  probe_cycle = (probe_cycle + 1) % probe_period;

  ShaderGlobal::set_int(rtr_probe_cycleVarId, probe_cycle);
  ShaderGlobal::set_int(rtr_probe_periodVarId, probe_period);
  ShaderGlobal::set_int(rtr_probes_wVarId, probeCountH);
  ShaderGlobal::set_int(rtr_probes_hVarId, probeCountV);

  // One thread is processing one tile
  if (!fix_probes)
    probeLocation->dispatchThreads(probeCountH, probeCountV, 1);

  // One group is processing one probe (PROBE_RESOLUTION * PROBE_RESOLUTION)
  bvh::bind_tlas_stage(context_id, STAGE_CS);
  probeColor->dispatch(divide_up(probeCountH, probe_period), probeCountV, 1);
  bvh::bind_tlas_stage(context_id, STAGE_CS);
}

void render(bvh::ContextId context_id, const TMatrix4 &proj_tm, bool rt_shadow, bool csm_shadow, const denoiser::TexMap &textures,
  bool checkerboard)
{
  TIME_D3D_PROFILE(rtr::render);

  G_ASSERT(ray_limit_coeff != 0);

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

  G_ASSERT_RETURN(denoiser::is_ray_reconstruction_enabled() || tiles != textures.end(), );
  G_ASSERT_RETURN(denoiser::is_ray_reconstruction_enabled() || denoised_reflection != textures.end(), );
  G_ASSERT_RETURN(denoiser::is_ray_reconstruction_enabled() || normal_roughness != textures.end(), );
  G_ASSERT_RETURN(denoiser::is_ray_reconstruction_enabled() || view_z != textures.end(), );
  G_ASSERT_RETURN(denoiser::is_ray_reconstruction_enabled() || half_depth != textures.end(), );
  G_ASSERT_RETURN(reflection_value != textures.end(), );

  auto id_or_bad = [&](auto name) { return name != textures.end() ? name->second.getId() : BAD_TEXTUREID; };

  validation_texture_id = id_or_bad(validation);
  tiles_id = id_or_bad(tiles);
  rtr_tex_unfiltered_id = id_or_bad(reflection_value);
  denoised_reflection_id = id_or_bad(denoised_reflection);
  decoded_denoised_reflection_id = id_or_bad(decoded_denoised_reflection);
  image_debug_hmul = (checkerboard ? 2 : 1);

  TextureInfo ti;
  reflection_value->second.getTex2D()->getinfo(ti);

  int tw = calc_tiles_width(ti.w, checkerboard);
  int th = calc_tiles_height(ti.h);

  bvh::bind_resources(context_id, ti.w);

  Point4 hitDistParams = Point4(gloss_ray_length, distance_factor, scatter_factor, roughness_factor);
  Point4 waterHitDistParams = Point4(water_ray_length, distance_factor, scatter_factor, roughness_factor);

  ShaderGlobal::set_texture(denoiser_view_zVarId, denoiser::is_ray_reconstruction_enabled() ? BAD_TEXTUREID : id_or_bad(view_z));
  ShaderGlobal::set_texture(rtr_denoisedVarId, denoised_reflection_id);
  ShaderGlobal::set_texture(rtr_targetVarId, id_or_bad(reflection_value));
  ShaderGlobal::set_int(rtr_frame_indexVarId, denoiser::get_frame_number());
  ShaderGlobal::set_real(rtr_rough_ray_lengthVarId, rough_ray_length);
  ShaderGlobal::set_color4(rtr_hit_dist_paramsVarId, hitDistParams);
  ShaderGlobal::set_real(rtr_ray_limit_coeffVarId, ray_limit_coeff);
  ShaderGlobal::set_real(rtr_ray_limit_powerVarId, ray_limit_power);
  ShaderGlobal::set_color4(rtr_water_hit_dist_paramsVarId, waterHitDistParams);
  ShaderGlobal::set_int4(rtr_resolutionIVarId, ti.w, ti.h, tw, th);
  ShaderGlobal::set_float4x4(inv_proj_tmVarId, inverse44(proj_tm));
  constexpr int RTR_OUTPUT_RR = 2;
  ShaderGlobal::set_int(rtr_output_typeVarId, denoiser::is_ray_reconstruction_enabled() ? RTR_OUTPUT_RR : (int)output_type);
  ShaderGlobal::set_texture(rt_nrVarId, id_or_bad(normal_roughness));
  ShaderGlobal::set_texture(downsampled_close_depth_texVarId, id_or_bad(half_depth));
  ShaderGlobal::set_int(rtr_shadowVarId, rt_shadow ? 1 : 0);
  ShaderGlobal::set_int(rtr_use_csmVarId, csm_shadow ? 1 : 0);
  ShaderGlobal::set_int(rtr_use_rrVarId, denoiser::is_ray_reconstruction_enabled() ? 1 : 0);
  ShaderGlobal::set_int(rtr_checkerboardVarId, checkerboard ? 1 : 0);
  ShaderGlobal::set_real(denoiser_glass_history_confidence_tweakVarId,
    denoiser::resolution_config.rtr.isHalfRes ? glass_tweak_half : glass_tweak_full);

  if (use_probes)
    update_probes(context_id, ti.w, ti.h);

  {
    // const float clear[] = { 0, 1, 0, 1 };
    // d3d::clear_rwtexf(reflection_value.getTex2D(), clear, 0, 0);
    // d3d::clear_rwtexf(denoised_reflection.getTex2D(), clear, 0, 0);

    TIME_D3D_PROFILE(rtr::noisy_image);

    bvh::bind_tlas_stage(context_id, STAGE_CS);

    trace->dispatch(tw, th, 1);
    d3d::resource_barrier(ResourceBarrierDesc(reflection_value->second.getTex2D(), RB_NONE, 0, 0));

    bvh::unbind_tlas_stage(STAGE_CS);
  }

  denoiser::ReflectionDenoiser params;
  params.method = output_type;
  params.hitDistParams = hitDistParams;
  params.maxStabilizedFrameNum =
    output_type == denoiser::ReflectionMethod::Reblur ? max_reblur_stabilized_frame_num : max_relax_stabilized_frame_num;
  params.maxFastStabilizedFrameNum = max_relax_fast_stabilized_frame_num;
  params.antilagSettings = Point2(al_luminance_sigma_scale, al_luminance_sensitivity);
  params.halfResolution = denoiser::resolution_config.rtr.isHalfRes;
  params.antiFirefly = use_anti_firefly;
  params.performanceMode = performance_mode;
  params.checkerboard = checkerboard;
  params.textures = textures;
  denoiser::denoise_reflection(params);
}

bool is_validation_layer_enabled() { return show_validation; }

void render_validation_layer()
{
  if (!show_validation || validation_texture_id == BAD_TEXTUREID)
    return;

  if (!validation_renderer)
    validation_renderer = new PostFxRenderer("rtr_validation_renderer");

  ShaderGlobal::set_texture(rtr_validation_textureVarId, validation_texture_id);
  validation_renderer->render();
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

  ShaderGlobal::set_real(rtr_probe_debug_sizeVarId, probe_size);

  probeDebug.shader->setStates(0, true);

  d3d::draw_instanced(PRIM_TRILIST, 0, SPHERES_INDICES_TO_DRAW, rtr_probe_locations->getNumElements());
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
  ShaderGlobal::set_real(denoiser_glass_history_confidence_tweakVarId,
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

  if (denoised_reflection_id != BAD_TEXTUREID)
  {
    if (!decode)
      decode = new_compute_shader("rt_reflection_decode");

    static int rtr_reflection_decode_srcVarId = get_shader_variable_id("rtr_reflection_decode_src");
    static int rtr_reflection_decode_dstVarId = get_shader_variable_id("rtr_reflection_decode_dst");

    if (auto decoded_denoised_reflection_tex = acquire_managed_tex(decoded_denoised_reflection_id))
    {
      ImGui::GetWindowDrawList()->AddCallback(
        [](const ImDrawList *, const ImDrawCmd *) {
          if (auto decoded_denoised_reflection_tex = acquire_managed_tex(decoded_denoised_reflection_id))
          {
            TextureInfo ti;
            decoded_denoised_reflection_tex->getinfo(ti);

            ShaderGlobal::set_texture(rtr_reflection_decode_srcVarId,
              show_unfiltered ? rtr_tex_unfiltered_id : denoised_reflection_id);
            ShaderGlobal::set_texture(rtr_reflection_decode_dstVarId, decoded_denoised_reflection_id);

            decode->dispatchThreads(ti.w, ti.h, 1);

            release_managed_tex(decoded_denoised_reflection_id);
          }
        },
        nullptr);

      ImGuiDagor::Image(decoded_denoised_reflection_id, decoded_denoised_reflection_tex);
      release_managed_tex(decoded_denoised_reflection_id);
    }
  }
}

REGISTER_IMGUI_WINDOW("Render", "RTR", imguiWindow);
#endif

void turn_off() { ShaderGlobal::set_int(rtr_bindless_slotVarId, -1); }

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