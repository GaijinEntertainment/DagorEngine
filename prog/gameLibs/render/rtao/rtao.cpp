// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/rtao.h>
#include <render/denoiser.h>
#include <3d/dag_resPtr.h>
#include <drv/3d/dag_bindless.h>
#include <drv/3d/dag_rwResource.h>
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
static int rtao_resolutionVarId = -1;
static int rtao_resolutionIVarId = -1;
static int rtao_texture_resolutionVarId = -1;
static int rtao_uv_maxVarId = -1;
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

Texture *denoised_ao_for_debug = nullptr;

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
  rtao_resolutionVarId = get_shader_variable_id("rtao_resolution");
  rtao_resolutionIVarId = get_shader_variable_id("rtao_resolutionI");
  rtao_texture_resolutionVarId = get_shader_variable_id("rtao_texture_resolution");
  rtao_uv_maxVarId = get_shader_variable_id("rtao_uv_max");
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

void render_noisy(bvh::ContextId context_id, const TMatrix4 &proj_tm, Texture *half_depth, Texture *rtao_tex_unfiltered,
  bool checkerboard)
{
  TIME_D3D_PROFILE(rtao::render_noisy)
  const IPoint2 &resolution =
    denoiser::resolution_config.rtao.isHalfRes ? denoiser::resolution_config.dynRes.halfRes : denoiser::resolution_config.dynRes.res;

  bvh::bind_resources(context_id, resolution.x);

  ShaderGlobal::set_texture(rtao_targetVarId, rtao_tex_unfiltered);
  ShaderGlobal::set_int(rtao_checkerboardVarId, checkerboard ? 1 : 0);

  const Point4 hitDistParams = Point4(ray_length, distance_factor, scatter_factor, roughness_factor);
  ShaderGlobal::set_float4(rtao_hit_dist_paramsVarId, hitDistParams);
  ShaderGlobal::set_float4(rtao_resolutionVarId, resolution.x, resolution.y, 0, 0);
  ShaderGlobal::set_int4(rtao_resolutionIVarId, resolution.x, resolution.y, 0, 0);
  ShaderGlobal::set_int4(rtao_texture_resolutionVarId, denoiser::resolution_config.rtao.width, denoiser::resolution_config.rtao.height,
    0, 0);
  Point2 uvMax = Point2(1, 1);
  if (denoiser::resolution_config.rtao.width != resolution.x || denoiser::resolution_config.rtao.height != resolution.y)
  {
    // Avoid samples outside the valid area
    uvMax.x = (resolution.x - 0.6f) / denoiser::resolution_config.rtao.width;
    uvMax.y = (resolution.y - 0.6f) / denoiser::resolution_config.rtao.height;
  }
  ShaderGlobal::set_float4(rtao_uv_maxVarId, uvMax);

  ShaderGlobal::set_texture(downsampled_close_depth_texVarId, half_depth);
  ShaderGlobal::set_float4x4(inv_proj_tmVarId, inverse44(proj_tm));

  trace->dispatchThreads(divide_up(resolution.x, checkerboard ? 2 : 1), resolution.y, 1);

  d3d::resource_barrier(ResourceBarrierDesc(rtao_tex_unfiltered, RB_STAGE_ALL_SHADERS | RB_RO_SRV, 0, 0));

  ShaderGlobal::set_texture(downsampled_close_depth_texVarId, nullptr);
  ShaderGlobal::set_texture(rtao_targetVarId, nullptr);
}

bool do_trace(bvh::ContextId context_id, const TMatrix4 &proj_tm, Texture *half_depth, const denoiser::TexMap &textures,
  bool checkerboard)
{
  TIME_D3D_PROFILE(rtao::trace);
  G_ASSERT_RETURN(!denoiser::is_ray_reconstruction_enabled() || !checkerboard, false);

  ACQUIRE_DENOISER_TEXTURE_BASE(DENOISER_TEX_NAMES(denoiser::AODenoiser), textures, rtao_tex_unfiltered, false);

  if (!denoiser::is_ray_reconstruction_enabled())
  {
    ACQUIRE_DENOISER_TEXTURE_BASE(DENOISER_TEX_NAMES(denoiser::AODenoiser), textures, denoised_ao, false);
    denoised_ao_for_debug = denoised_ao;
  }

  render_noisy(context_id, proj_tm, half_depth, rtao_tex_unfiltered, checkerboard);

  return true;
}

void denoise(bool performance_mode, const denoiser::TexMap &textures, bool checkerboard)
{
  TIME_D3D_PROFILE(rtao::denoise);
  Point4 hitDistParams = Point4(ray_length, distance_factor, scatter_factor, roughness_factor);

  denoiser::AODenoiser params;
  params.hitDistParams = hitDistParams;
  params.performanceMode = performance_mode;
  params.halfResolution = denoiser::resolution_config.rtao.isHalfRes;
  params.textures = textures;
  params.checkerboard = checkerboard;
  params.maxStabilizedFrameNum = max_stabilized_frame_num;

  denoiser::denoise_ao(params);
}

void render(bvh::ContextId context_id, const TMatrix4 &proj_tm, bool performance_mode, Texture *half_depth,
  const denoiser::TexMap &textures, bool checkerboard)
{
  TIME_D3D_PROFILE(rtao::render);

  if (!do_trace(context_id, proj_tm, half_depth, textures, checkerboard))
    return;

  denoise(performance_mode, textures, checkerboard);
}

#if DAGOR_DBGLEVEL > 0
static void imguiWindow()
{
  if (!denoised_ao_for_debug)
    return;

  ImGui::SliderFloat("Ray length", &ray_length, ray_length_min, ray_length_max);
  ImGui::SliderFloat("Distance factor", &distance_factor, distance_factor_min, distance_factor_max);
  ImGui::SliderFloat("Scatter factor", &scatter_factor, scatter_factor_min, scatter_factor_max);
  ImGui::SliderFloat("Roughness factor", &roughness_factor, roughness_factor_min, roughness_factor_max);
  ImGui::SliderInt("Max stabilized frames", &max_stabilized_frame_num, 0, denoiser::AODenoiser::MAX_HISTORY_FRAME_NUM);

  ImGuiDagor::Image(denoised_ao_for_debug);
}

REGISTER_IMGUI_WINDOW("Render", "RTAO", imguiWindow);
#endif

} // namespace rtao