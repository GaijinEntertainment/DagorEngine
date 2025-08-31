// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/rtao.h>
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

namespace rtao
{

static ComputeShaderElement *trace = nullptr;

static int inv_proj_tmVarId = -1;
static int rtao_targetVarId = -1;
static int rtao_hit_dist_paramsVarId = -1;
static int rtao_resolutionIVarId = -1;
static int ssao_texVarId = -1;
static int rtao_bindless_slotVarId = -1;
static int rtao_res_mulVarId = -1;
static int rtao_checkerboardVarId = -1;
static int downsampled_close_depth_texVarId = -1;

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

TEXTUREID denoised_ao_id_for_debug = BAD_TEXTUREID;

void get_required_persistent_texture_descriptors(denoiser::TexInfoMap &persistent_textures)
{
  denoiser::get_required_persistent_texture_descriptors_for_ao(persistent_textures);
}

void get_required_transient_texture_descriptors(denoiser::TexInfoMap &transient_textures)
{
  denoiser::get_required_transient_texture_descriptors_for_ao(transient_textures);
}

void initialize(bool half_res)
{
  if (!bvh::is_available())
    return;

  rtao_hit_dist_paramsVarId = get_shader_variable_id("rtao_hit_dist_params");
  rtao_resolutionIVarId = get_shader_variable_id("rtao_resolutionI");
  rtao_targetVarId = get_shader_variable_id("rtao_target");
  inv_proj_tmVarId = get_shader_variable_id("inv_proj_tm");
  ssao_texVarId = get_shader_variable_id("ssao_tex");
  rtao_bindless_slotVarId = get_shader_variable_id("rtao_bindless_slot");
  rtao_res_mulVarId = get_shader_variable_id("rtao_res_mul");
  rtao_checkerboardVarId = get_shader_variable_id("rtao_checkerboard");
  ShaderGlobal::set_int(rtao_res_mulVarId, half_res ? 2 : 1);
  downsampled_close_depth_texVarId = get_shader_variable_id("downsampled_close_depth_tex");
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

  ShaderGlobal::set_int(rtao_bindless_slotVarId, -1);
}

inline int divide_up(int x, int y) { return (x + y - 1) / y; }

void render_noisy(bvh::ContextId context_id, const TMatrix4 &proj_tm, TEXTUREID half_depth, TextureIDPair rtao_tex_unfiltered,
  bool checkerboard)
{
  TIME_D3D_PROFILE(rtao::render_noisy)
  TextureInfo ti;
  rtao_tex_unfiltered.getTex()->getinfo(ti);

  bvh::bind_resources(context_id, ti.w);

  ShaderGlobal::set_texture(rtao_targetVarId, rtao_tex_unfiltered.getId());
  ShaderGlobal::set_int(rtao_checkerboardVarId, checkerboard ? 1 : 0);

  const Point4 hitDistParams = Point4(ray_length, distance_factor, scatter_factor, roughness_factor);
  ShaderGlobal::set_color4(rtao_hit_dist_paramsVarId, hitDistParams);
  ShaderGlobal::set_int4(rtao_resolutionIVarId, ti.w, ti.h, 0, 0);

  ShaderGlobal::set_texture(downsampled_close_depth_texVarId, half_depth);
  ShaderGlobal::set_float4x4(inv_proj_tmVarId, inverse44(proj_tm));

  bvh::bind_tlas_stage(context_id, STAGE_CS);
  trace->dispatchThreads(divide_up(ti.w, checkerboard ? 2 : 1), ti.h, 1);
  bvh::unbind_tlas_stage(STAGE_CS);

  d3d::resource_barrier(ResourceBarrierDesc(rtao_tex_unfiltered.getTex2D(), RB_STAGE_ALL_SHADERS | RB_RO_SRV, 0, 0));
}

void render(bvh::ContextId context_id, const TMatrix4 &proj_tm, bool performance_mode, TEXTUREID half_depth,
  const denoiser::TexMap &textures, bool checkerboard)
{
  TIME_D3D_PROFILE(rtao::render);

  G_ASSERT_RETURN(!denoiser::is_ray_reconstruction_enabled() || !checkerboard, );

  Point4 hitDistParams = Point4(ray_length, distance_factor, scatter_factor, roughness_factor);

  denoiser::AODenoiser params;
  params.hitDistParams = hitDistParams;
  params.performanceMode = performance_mode;
  params.halfResolution = denoiser::resolution_config.rtao.isHalfRes;
  params.textures = textures;
  params.checkerboard = checkerboard;
  params.maxStabilizedFrameNum = max_stabilized_frame_num;

  ACQUIRE_DENOISER_TEXTURE(params, rtao_tex_unfiltered);

  if (!denoiser::is_ray_reconstruction_enabled())
  {
    ACQUIRE_DENOISER_TEXTURE(params, denoised_ao);
    denoised_ao_id_for_debug = denoised_ao_id;
  }

  render_noisy(context_id, proj_tm, half_depth, TextureIDPair{rtao_tex_unfiltered, rtao_tex_unfiltered_id}, checkerboard);

  denoiser::denoise_ao(params);
}

#if DAGOR_DBGLEVEL > 0
static void imguiWindow()
{
  if (denoised_ao_id_for_debug == BAD_TEXTUREID)
    return;

  if (auto denoised_ao = acquire_managed_tex(denoised_ao_id_for_debug); denoised_ao)
  {
    ImGui::SliderFloat("Ray length", &ray_length, ray_length_min, ray_length_max);
    ImGui::SliderFloat("Distance factor", &distance_factor, distance_factor_min, distance_factor_max);
    ImGui::SliderFloat("Scatter factor", &scatter_factor, scatter_factor_min, scatter_factor_max);
    ImGui::SliderFloat("Roughness factor", &roughness_factor, roughness_factor_min, roughness_factor_max);
    ImGui::SliderInt("Max stabilized frames", &max_stabilized_frame_num, 0, denoiser::AODenoiser::MAX_HISTORY_FRAME_NUM);

    ImGuiDagor::Image(denoised_ao_id_for_debug, denoised_ao);

    release_managed_tex(denoised_ao_id_for_debug);
  }
}

REGISTER_IMGUI_WINDOW("Render", "RTAO", imguiWindow);
#endif

} // namespace rtao