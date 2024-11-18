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

namespace rtao
{

static ComputeShaderElement *trace = nullptr;

static UniqueTex ao_value;
static UniqueTex denoised_ao;

static int inv_proj_tmVarId = -1;
static int rtao_targetVarId = -1;
static int rtao_frame_indexVarId = -1;
static int rtao_hit_dist_paramsVarId = -1;
static int rtao_resolutionIVarId = -1;
static int ssao_texVarId = -1;
static int rtao_bindless_slotVarId = -1;
static int rtao_res_mulVarId = -1;
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

void initialize(bool half_res)
{
  if (!bvh::is_available())
    return;

  rtao_frame_indexVarId = get_shader_variable_id("rtao_frame_index");
  rtao_hit_dist_paramsVarId = get_shader_variable_id("rtao_hit_dist_params");
  rtao_resolutionIVarId = get_shader_variable_id("rtao_resolutionI");
  rtao_targetVarId = get_shader_variable_id("rtao_target");
  inv_proj_tmVarId = get_shader_variable_id("inv_proj_tm");
  ssao_texVarId = get_shader_variable_id("ssao_tex");
  rtao_bindless_slotVarId = get_shader_variable_id("rtao_bindless_slot");
  rtao_res_mulVarId = get_shader_variable_id("rtao_res_mul");
  ShaderGlobal::set_int(rtao_res_mulVarId, half_res ? 2 : 1);
  downsampled_close_depth_texVarId = get_shader_variable_id("downsampled_close_depth_tex");
  if (!trace)
    trace = new_compute_shader("rt_ao");

  ao_value.close();
  denoised_ao.close();

  denoiser::make_ao_maps(ao_value, denoised_ao, half_res);

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
  safe_delete(trace);

  ao_value.close();
  denoised_ao.close();

  ShaderGlobal::set_int(rtao_bindless_slotVarId, -1);
}

void render(bvh::ContextId context_id, const TMatrix4 &proj_tm, bool performance_mode, TEXTUREID half_depth)
{
  TIME_D3D_PROFILE(rtao::render);

  TextureInfo ti;
  denoised_ao->getinfo(ti);

  bvh::bind_resources(context_id, ti.w);

  Point4 hitDistParams = Point4(ray_length, distance_factor, scatter_factor, roughness_factor);

  ShaderGlobal::set_texture(rtao_targetVarId, ao_value.getTexId());
  ShaderGlobal::set_int(rtao_frame_indexVarId, denoiser::get_frame_number());
  ShaderGlobal::set_color4(rtao_hit_dist_paramsVarId, hitDistParams);
  ShaderGlobal::set_int4(rtao_resolutionIVarId, ti.w, ti.h, 0, 0);

  ShaderGlobal::set_texture(downsampled_close_depth_texVarId, half_depth);
  ShaderGlobal::set_float4x4(inv_proj_tmVarId, inverse44(proj_tm));

  trace->dispatchThreads(ti.w / 2, ti.h, 1);

  denoiser::AODenoiser params;
  params.hitDistParams = hitDistParams;
  params.denoisedAo = denoised_ao.getTex2D();
  params.aoValue = ao_value.getTex2D();
  params.performanceMode = performance_mode;
  denoiser::denoise_ao(params);

  d3d::resource_barrier(ResourceBarrierDesc(denoised_ao.getTex2D(), RB_STAGE_ALL_SHADERS | RB_RO_SRV, 0, 0));
}

#if DAGOR_DBGLEVEL > 0
static void imguiWindow()
{
  if (!denoised_ao)
    return;

  ImGui::SliderFloat("Ray length", &ray_length, ray_length_min, ray_length_max);
  ImGui::SliderFloat("Distance factor", &distance_factor, distance_factor_min, distance_factor_max);
  ImGui::SliderFloat("Scatter factor", &scatter_factor, scatter_factor_min, scatter_factor_max);
  ImGui::SliderFloat("Roughness factor", &roughness_factor, roughness_factor_min, roughness_factor_max);

  ImGuiDagor::Image(denoised_ao.getTexId(), denoised_ao.getTex2D());
}

REGISTER_IMGUI_WINDOW("Render", "RTAO", imguiWindow);
#endif

} // namespace rtao