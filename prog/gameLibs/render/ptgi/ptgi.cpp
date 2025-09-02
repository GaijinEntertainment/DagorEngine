// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/ptgi.h>
#include <render/denoiser.h>
#include <3d/dag_resPtr.h>
#include <drv/3d/dag_bindless.h>
#include <shaders/dag_computeShaders.h>
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
#include <render/daFrameGraph/daFG.h>

namespace ptgi
{

namespace TextureNames
{
static const char *decoded_denoised_gi = "decoded_denoised_gi";
}; // namespace TextureNames

static ComputeShaderElement *trace = nullptr;
static ComputeShaderElement *decode = nullptr;

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
#endif

static int inv_proj_tmVarId = -1;
static int ptgi_targetVarId = -1;
static int ptgi_resolutionIVarId = -1;
static int ptgi_res_mulVarId = -1;
static int ptgi_hit_dist_paramsVarId = -1;
static int rt_nrVarId = -1;
static int ptgi_max_bouncesVarId = -1;
static int ptgi_checkerboardVarId = -1;
static int ptgi_bindless_slotVarId = -1;
static int ptgi_boostVarId = -1;
static int ptgi_boost_rangeVarId = -1;
static int ptgi_use_rrVarId = -1;
static int downsampled_close_depth_texVarId = -1;

static float ray_length = 100;
static float distance_factor = 1;
static float scatter_factor = 20.0;
static float roughness_factor = -25.0;
static float boost = 0;
static float boost_range = 0;
static int max_stabilized_frame_num = denoiser::GIDenoiser::MAX_HISTORY_FRAME_NUM;

TEXTUREID denoised_gi_id_for_debug = BAD_TEXTUREID;
TEXTUREID decoded_denoised_gi_id_for_debug = BAD_TEXTUREID;

void get_required_persistent_texture_descriptors(denoiser::TexInfoMap &persistent_textures)
{
  denoiser::get_required_persistent_texture_descriptors_for_gi(persistent_textures);

#if DAGOR_DBGLEVEL > 0
  if (!denoiser::is_ray_reconstruction_enabled())
  {
    auto denoised_gi = persistent_textures.find(denoiser::GIDenoiser::TextureNames::denoised_gi);
    if (denoised_gi == persistent_textures.end())
    {
      logerr("denoised_gi was not found in requested ptgi textures!");
      return;
    }
    TextureInfo sourceTi = denoised_gi->second;
    auto &ti = persistent_textures[TextureNames::decoded_denoised_gi];
    ti.w = denoiser::resolution_config.ptgi.isHalfRes ? sourceTi.w / 2 : sourceTi.w;
    ti.h = denoiser::resolution_config.ptgi.isHalfRes ? sourceTi.h / 2 : sourceTi.h;
    ti.mipLevels = 1;
    ti.type = D3DResourceType::TEX;
    ti.cflg = TEXCF_UNORDERED | TEXFMT_A16B16G16R16F;
  }
#endif
}

void get_required_transient_texture_descriptors(denoiser::TexInfoMap &transient_textures)
{
  denoiser::get_required_transient_texture_descriptors_for_gi(transient_textures);
}

void initialize(bool half_res)
{
  if (!bvh::is_available())
    return;

  inv_proj_tmVarId = get_shader_variable_id("inv_proj_tm");
  ptgi_targetVarId = get_shader_variable_id("ptgi_target");
  ptgi_resolutionIVarId = get_shader_variable_id("ptgi_resolutionI");
  ptgi_res_mulVarId = get_shader_variable_id("ptgi_res_mul");
  ptgi_hit_dist_paramsVarId = get_shader_variable_id("ptgi_hit_dist_params");
  downsampled_close_depth_texVarId = get_shader_variable_id("downsampled_close_depth_tex");
  rt_nrVarId = get_shader_variable_id("rt_nr");
  ptgi_max_bouncesVarId = get_shader_variable_id("ptgi_max_bounces");
  ptgi_checkerboardVarId = get_shader_variable_id("ptgi_checkerboard");
  ptgi_bindless_slotVarId = get_shader_variable_id("ptgi_bindless_slot");
  ptgi_boostVarId = get_shader_variable_id("ptgi_boost");
  ptgi_boost_rangeVarId = get_shader_variable_id("ptgi_boost_range");
  ptgi_use_rrVarId = get_shader_variable_id("ptgi_use_rr");

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
  safe_delete(decode);
  turn_off();
}

void turn_off() { ShaderGlobal::set_int(ptgi_bindless_slotVarId, -1); }

inline int divide_up(int x, int y) { return (x + y - 1) / y; }

void render(bvh::ContextId context_id, const TMatrix4 &proj_tm, TEXTUREID depth, bool in_cockpit, const denoiser::TexMap &textures,
  Quality quality, bool checkerboard)
{
  TIME_D3D_PROFILE(ptgi::render);

  G_ASSERT_RETURN(!denoiser::is_ray_reconstruction_enabled() || !checkerboard, );

  auto denoised_gi = textures.find(denoiser::GIDenoiser::TextureNames::denoised_gi);
  auto gi_value = textures.find(denoiser::GIDenoiser::TextureNames::ptgi_tex_unfiltered);
  auto decoded_denoised_gi = textures.find(TextureNames::decoded_denoised_gi);
  auto normal_roughness =
    textures.find(denoiser::resolution_config.ptgi.isHalfRes ? denoiser::TextureNames::denoiser_half_normal_roughness
                                                             : denoiser::TextureNames::denoiser_normal_roughness);

  G_ASSERT_RETURN(!denoiser::resolution_config.ptgi.isHalfRes || normal_roughness != textures.end(), );
  G_ASSERT_RETURN(gi_value != textures.end(), );

  auto id_or_bad = [&](auto name) { return name != textures.end() ? name->second.getId() : BAD_TEXTUREID; };

  if (!denoiser::is_ray_reconstruction_enabled())
  {
    denoised_gi_id_for_debug = id_or_bad(denoised_gi);
    decoded_denoised_gi_id_for_debug = id_or_bad(decoded_denoised_gi);
  }
  else
  {
    denoised_gi_id_for_debug = decoded_denoised_gi_id_for_debug = BAD_TEXTUREID;
  }

  Point4 hitDistParams = Point4(ray_length, distance_factor, scatter_factor, roughness_factor);

  denoiser::GIDenoiser params;
  params.hitDistParams = hitDistParams;
  params.textures = textures;
  params.checkerboard = checkerboard;
  params.halfResolution = denoiser::resolution_config.ptgi.isHalfRes;
  params.maxStabilizedFrameNum = max_stabilized_frame_num;

  TextureInfo ti;
  gi_value->second.getTex()->getinfo(ti);

  bvh::bind_resources(context_id, ti.w);

  ShaderGlobal::set_texture(ptgi_targetVarId, gi_value->second.getId());
  ShaderGlobal::set_texture(rt_nrVarId, denoiser::resolution_config.ptgi.isHalfRes ? normal_roughness->second.getId() : BAD_TEXTUREID);

  ShaderGlobal::set_int4(ptgi_resolutionIVarId, ti.w, ti.h, 0, 0);

  ShaderGlobal::set_texture(downsampled_close_depth_texVarId, depth);
  ShaderGlobal::set_float4x4(inv_proj_tmVarId, inverse44(proj_tm));

  ShaderGlobal::set_int(ptgi_max_bouncesVarId, quality == Quality::Low ? 3 : quality == Quality::Medium ? 4 : 5);

  ShaderGlobal::set_int(ptgi_checkerboardVarId, checkerboard ? 1 : 0);
  ShaderGlobal::set_int(ptgi_use_rrVarId, ::denoiser::is_ray_reconstruction_enabled() ? 1 : 0);

  ShaderGlobal::set_color4(ptgi_hit_dist_paramsVarId, hitDistParams);

  if (boost_range > 0)
  {
    ShaderGlobal::set_real(ptgi_boostVarId, boost);
    ShaderGlobal::set_real(ptgi_boost_rangeVarId, boost_range);
  }
  else if (in_cockpit)
  {
    ShaderGlobal::set_real(ptgi_boostVarId, 0.5);
    ShaderGlobal::set_real(ptgi_boost_rangeVarId, 5);
  }
  else
  {
    ShaderGlobal::set_real(ptgi_boostVarId, 0);
    ShaderGlobal::set_real(ptgi_boost_rangeVarId, 0);
  }

  bvh::bind_tlas_stage(context_id, STAGE_CS);
  {
    TIME_D3D_PROFILE(ptgi::render_noisy)
    trace->dispatchThreads(divide_up(ti.w, checkerboard ? 2 : 1), ti.h, 1);
  }
  bvh::unbind_tlas_stage(STAGE_CS);

  d3d::resource_barrier(ResourceBarrierDesc(gi_value->second.getTex2D(), RB_STAGE_ALL_SHADERS | RB_RO_SRV, 0, 0));

  denoiser::denoise_gi(params);

#if DAGOR_DBGLEVEL > 0
  if (!decode)
    decode = new_compute_shader("rt_gi_decode");

  static int ptgi_decode_srcVarId = get_shader_variable_id("ptgi_decode_src");
  static int ptgi_decode_dstVarId = get_shader_variable_id("ptgi_decode_dst");
  static int ptgi_decode_albedo_mulVarId = get_shader_variable_id("ptgi_decode_albedo_mul");
  static int ptgi_decode_tonemapVarId = get_shader_variable_id("ptgi_decode_tonemap");

  ShaderGlobal::set_texture(ptgi_decode_srcVarId, denoised_gi_id_for_debug);
  ShaderGlobal::set_texture(ptgi_decode_dstVarId, decoded_denoised_gi_id_for_debug);
  ShaderGlobal::set_int(ptgi_decode_albedo_mulVarId, mul_debug_by_albedo ? 1 : 0);
  ShaderGlobal::set_int(ptgi_decode_tonemapVarId, decode_tonemap ? 1 : 0);

  decode->dispatchThreads(ti.w, ti.h, 1);
#endif
}

#if DAGOR_DBGLEVEL > 0
static void imguiWindow()
{
  if (denoised_gi_id_for_debug == BAD_TEXTUREID)
    return;

  if (auto denoised_ao = acquire_managed_tex(denoised_gi_id_for_debug); denoised_ao)
  {
    ImGui::Checkbox("Multiply by albedo", &mul_debug_by_albedo);
    ImGui::Checkbox("Tonemap", &decode_tonemap);
    ImGui::SliderFloat("Boost", &boost, 0, 2);
    ImGui::SliderFloat("Boost range", &boost_range, 0, 10);
    ImGui::SliderFloat("Ray length", &ray_length, ray_length_min, ray_length_max);
    ImGui::SliderFloat("Distance factor", &distance_factor, distance_factor_min, distance_factor_max);
    ImGui::SliderFloat("Scatter factor", &scatter_factor, scatter_factor_min, scatter_factor_max);
    ImGui::SliderFloat("Roughness factor", &roughness_factor, roughness_factor_min, roughness_factor_max);
    ImGui::SliderInt("Max stabilized frames", &max_stabilized_frame_num, 0, denoiser::GIDenoiser::MAX_HISTORY_FRAME_NUM);

    if (auto decoded_denoised_gi_tex = acquire_managed_tex(decoded_denoised_gi_id_for_debug))
    {
      ImGuiDagor::Image(decoded_denoised_gi_id_for_debug, decoded_denoised_gi_tex);
      release_managed_tex(decoded_denoised_gi_id_for_debug);
    }

    release_managed_tex(denoised_gi_id_for_debug);
  }
}

REGISTER_IMGUI_WINDOW("Render", "PTGI", imguiWindow);
#endif

} // namespace ptgi