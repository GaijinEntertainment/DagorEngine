// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/rtsm.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_computeShaders.h>
#include <bvh/bvh.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_bindless.h>
#include <render/denoiser.h>
#include <perfMon/dag_statDrv.h>
#include <math/dag_frustum.h>
#include <math/integer/dag_IPoint2.h>
#include <ioSys/dag_dataBlock.h>
#include <EASTL/algorithm.h>
#include <EASTL/optional.h>
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
static int use_precomputed_dynamic_lightsVarId = -1;
static int inv_proj_tmVarId = -1;
static int combined_shadowsVarId = -1;
static int rtsm_valueVarId = -1;
static int rtsm_translucencyVarId = -1;
static int rtsm_denoisedVarId = -1;
static int rtsm_dynamic_lightsVarId = -1;
static int rtsm_has_nukeVarId = -1;
static int rtsm_bindless_slotVarId = -1;
static int rtsm_dynamic_light_radiusVarId = -1;
static int rtsm_dynamic_soft_shadowsVarId = -1;
static int rtsm_qualityVarId = -1;
static int rtsm_tree_max_distanceVarId = -1;

static int max_stabilized_frame_count = 5;

static bool dynamic_light_shadows = false;

static RenderMode render_mode = RenderMode::Hard;

TEXTUREID denoised_rtsm_id_for_debug = BAD_TEXTUREID;

namespace TextureNames
{
const char *rtsm_dynamic_lights = "rtsm_dynamic_lights";
}

void initialize(RenderMode rm, bool have_dynamic_light_shadows)
{
  if (!bvh::is_available())
    return;

  ::rtsm::render_mode = rm;
  denoiser::resolution_config.initRTSM();
  dynamic_light_shadows = have_dynamic_light_shadows;

  rt_shadow_resolutionVarId = get_shader_variable_id("rt_shadow_resolution");
  rt_shadow_resolutionIVarId = get_shader_variable_id("rt_shadow_resolutionI");
  world_view_posVarId = get_shader_variable_id("world_view_pos");
  rtsm_render_modeVarId = get_shader_variable_id("rtsm_render_mode");
  use_precomputed_dynamic_lightsVarId = get_shader_variable_id("use_precomputed_dynamic_lights", true);
  inv_proj_tmVarId = get_shader_variable_id("inv_proj_tm");
  combined_shadowsVarId = get_shader_variable_id("combined_shadows");
  rtsm_valueVarId = get_shader_variable_id("rtsm_value");
  rtsm_translucencyVarId = get_shader_variable_id("rtsm_translucency");
  rtsm_denoisedVarId = get_shader_variable_id("rtsm_denoised");
  rtsm_dynamic_lightsVarId = get_shader_variable_id("rtsm_dynamic_lights");
  rtsm_has_nukeVarId = get_shader_variable_id("rtsm_has_nuke");
  rtsm_bindless_slotVarId = get_shader_variable_id("rtsm_bindless_slot");
  rtsm_dynamic_light_radiusVarId = get_shader_variable_id("rtsm_dynamic_light_radius");
  rtsm_dynamic_soft_shadowsVarId = get_shader_variable_id("rtsm_dynamic_soft_shadows");
  rtsm_qualityVarId = get_shader_variable_id("rtsm_quality");
  rtsm_tree_max_distanceVarId = get_shader_variable_id("rtsm_tree_max_distance");

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
  denoiser::resolution_config.closeRTSM();
  safe_delete(trace);
  safe_delete(traceFillLow);
  safe_delete(traceFillHigh);
  safe_delete(traceFillHighPrepass);
  safe_delete(traceDynamic);

  ShaderGlobal::set_int(rtsm_bindless_slotVarId, -1);
  ShaderGlobal::set_texture(rtsm_valueVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(rtsm_translucencyVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(rtsm_denoisedVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(rtsm_dynamic_lightsVarId, BAD_TEXTUREID);
}

void get_required_persistent_texture_descriptors(denoiser::TexInfoMap &persistent_textures)
{
  switch (render_mode)
  {
    case RenderMode::Hard:
    {
      auto &ti = persistent_textures[denoiser::ShadowDenoiser::TextureNames::rtsm_shadows_denoised];
      ti.w = denoiser::resolution_config.rtsm.width;
      ti.h = denoiser::resolution_config.rtsm.height;
      ti.mipLevels = 1;
      ti.type = D3DResourceType::TEX;
      ti.cflg = TEXCF_UNORDERED | TEXFMT_A8R8G8B8;
    }
    break;
    case RenderMode::Denoised: denoiser::get_required_persistent_texture_descriptors_for_rtsm(persistent_textures, false); break;
    case RenderMode::DenoisedTranslucent:
      denoiser::get_required_persistent_texture_descriptors_for_rtsm(persistent_textures, true);
      break;
  }

  if (dynamic_light_shadows)
  {
    auto &ti = persistent_textures[TextureNames::rtsm_dynamic_lights];
    ti.w = denoiser::resolution_config.rtsm.width;
    ti.h = denoiser::resolution_config.rtsm.height;
    ti.mipLevels = 1;
    ti.type = D3DResourceType::TEX;
    ti.cflg = TEXCF_UNORDERED | TEXFMT_R11G11B10F;
  }
}

void get_required_transient_texture_descriptors(denoiser::TexInfoMap &transient_textures)
{
  switch (render_mode)
  {
    case RenderMode::Hard: break;
    case RenderMode::Denoised: denoiser::get_required_transient_texture_descriptors_for_rtsm(transient_textures, false); break;
    case RenderMode::DenoisedTranslucent:
      denoiser::get_required_transient_texture_descriptors_for_rtsm(transient_textures, true);
      break;
  }
}

void render(bvh::ContextId context_id, const Point3 &view_pos, const Point3 &light_dir, const TMatrix4 &proj_tm,
  const denoiser::TexMap &textures, float tree_max_distance, bool has_nuke, bool has_dynamic_lights, Texture *csm_texture,
  d3d::SamplerHandle csm_sampler, Texture *vsm_texture, d3d::SamplerHandle vsm_sampler, int quality_mode)
{
  if (!trace)
    return;
  G_ASSERT(!denoiser::is_ray_reconstruction_enabled() || quality_mode == 0);

  TIME_D3D_PROFILE(rtsm::render);

  denoiser::ShadowDenoiser params;
  params.lightDirection = light_dir;
  params.maxStabilizedFrameNum = max_stabilized_frame_count;
  params.csmTexture = csm_texture;
  params.csmSampler = csm_sampler;
  params.vsmTexture = vsm_texture;
  params.vsmSampler = vsm_sampler;
  params.textures = textures;

  TextureInfo ti;
  if (render_mode == RenderMode::Hard)
  {
    ACQUIRE_DENOISER_TEXTURE(params, rtsm_shadows_denoised);
    ShaderGlobal::set_texture(rtsm_valueVarId, rtsm_shadows_denoised_id);
    rtsm_shadows_denoised->getinfo(ti);
  }
  else
  {
    ACQUIRE_DENOISER_TEXTURE(params, rtsm_value);
    ShaderGlobal::set_texture(rtsm_valueVarId, rtsm_value_id);
    rtsm_value->getinfo(ti);
  }

  if (!denoiser::is_ray_reconstruction_enabled())
  {
    ACQUIRE_DENOISER_TEXTURE(params, rtsm_shadows_denoised);
    ACQUIRE_OPTIONAL_DENOISER_TEXTURE(params, rtsm_translucency);

    denoised_rtsm_id_for_debug = rtsm_shadows_denoised_id;

    ShaderGlobal::set_texture(rtsm_translucencyVarId, rtsm_translucency_id);
    ShaderGlobal::set_texture(rtsm_denoisedVarId, rtsm_shadows_denoised_id);
  }

  bvh::bind_resources(context_id, ti.w);

  ShaderGlobal::set_color4(rt_shadow_resolutionVarId, ti.w, ti.h);
  ShaderGlobal::set_int4(rt_shadow_resolutionIVarId, ti.w, ti.h, 0, 0);
  ShaderGlobal::set_color4(world_view_posVarId, view_pos);
  ShaderGlobal::set_int(rtsm_has_nukeVarId, has_nuke ? 1 : 0);
  ShaderGlobal::set_int(use_precomputed_dynamic_lightsVarId, (has_dynamic_lights && dynamic_light_shadows) ? 1 : 0);
  ShaderGlobal::set_float4x4(inv_proj_tmVarId, inverse44(proj_tm));
  ShaderGlobal::set_real(rtsm_tree_max_distanceVarId, tree_max_distance);

  if (denoiser::is_ray_reconstruction_enabled())
    ShaderGlobal::set_int(rtsm_render_modeVarId, 3);
  else
    switch (render_mode)
    {
      case RenderMode::Hard: ShaderGlobal::set_int(rtsm_render_modeVarId, 0); break;
      case RenderMode::Denoised: ShaderGlobal::set_int(rtsm_render_modeVarId, 1); break;
      case RenderMode::DenoisedTranslucent: ShaderGlobal::set_int(rtsm_render_modeVarId, 2); break;
    }

  bvh::bind_tlas_stage(context_id, ShaderStage::STAGE_CS);

  ShaderGlobal::set_int(rtsm_qualityVarId, quality_mode);

  switch (quality_mode)
  {
    case 0:
    {
      TIME_D3D_PROFILE(rtsm::render_noisy_high)
      trace->dispatchThreads(ti.w, ti.h, 1);
    }
      bvh::unbind_tlas_stage(ShaderStage::STAGE_CS);
      break;
    case 1:
    {
      TIME_D3D_PROFILE(rtsm::render_noisy_medium)
      trace->dispatchThreads(ti.w / 2, ti.h, 1);
    }
      bvh::unbind_tlas_stage(ShaderStage::STAGE_CS); // If TLAS is not unset it breaks bindings of next dispatch (PS5 only)
      {
        TIME_D3D_PROFILE(rtsm::fill_medium)
        traceFillHighPrepass->dispatchThreads(ti.w / 2, ti.h, 1);
        traceFillHigh->dispatchThreads(ti.w / 2, ti.h, 1);
      }
      break;
    case 2:
    {
      TIME_D3D_PROFILE(rtsm::render_noisy_low)
      trace->dispatchThreads(ti.w / 2, ti.h, 1);
    }
      bvh::unbind_tlas_stage(ShaderStage::STAGE_CS);
      {
        TIME_D3D_PROFILE(rtsm::fill_low)
        traceFillLow->dispatchThreads(ti.w / 2, ti.h, 1);
      }
      break;
  }


  if (render_mode == RenderMode::Hard)
  {
    ACQUIRE_DENOISER_TEXTURE(params, rtsm_shadows_denoised);
    d3d::resource_barrier(ResourceBarrierDesc(rtsm_shadows_denoised, RB_STAGE_ALL_SHADERS | RB_RO_SRV, 0, 0));
  }
  else
    denoiser::denoise_shadow(params);
}

void render_dynamic_light_shadows(bvh::ContextId context_id, const Point3 &view_pos, TextureIDPair dynamic_lighting_texture,
  float light_radius, bool soft_shadow, bool has_nuke)
{
  ShaderGlobal::set_texture(rtsm_dynamic_lightsVarId,
    dynamic_lighting_texture.getTex2D() ? dynamic_lighting_texture.getId() : BAD_TEXTUREID);

  if (!dynamic_lighting_texture.getTex2D())
    return;

  G_ASSERT(traceDynamic);
  G_ASSERT(dynamic_lighting_texture.getTex2D());

  TIME_D3D_PROFILE(rtsm::render_dynamic);

  TextureInfo ti;
  dynamic_lighting_texture.getTex2D()->getinfo(ti);

  bvh::bind_resources(context_id, ti.w);

  ShaderGlobal::set_int4(rt_shadow_resolutionIVarId, ti.w, ti.h, 0, 0);
  ShaderGlobal::set_color4(rt_shadow_resolutionVarId, ti.w, ti.h);
  ShaderGlobal::set_color4(world_view_posVarId, view_pos);
  ShaderGlobal::set_int(use_precomputed_dynamic_lightsVarId, 1);
  ShaderGlobal::set_int(rtsm_has_nukeVarId, has_nuke ? 1 : 0);
  ShaderGlobal::set_real(rtsm_dynamic_light_radiusVarId, light_radius);
  ShaderGlobal::set_int(rtsm_dynamic_soft_shadowsVarId, soft_shadow ? 1 : 0);

  const float zero[4] = {0, 0, 0, 0};
  d3d::clear_rwtexf(dynamic_lighting_texture.getTex2D(), zero, 0, 0);

  bvh::bind_tlas_stage(context_id, ShaderStage::STAGE_CS);
  traceDynamic->dispatchThreads(ti.w, ti.h, 1);
  bvh::unbind_tlas_stage(ShaderStage::STAGE_CS);
}

void turn_off()
{
  ShaderGlobal::set_int(use_precomputed_dynamic_lightsVarId, 0);
  ShaderGlobal::set_int(rtsm_bindless_slotVarId, -1);
}

#if DAGOR_DBGLEVEL > 0
static void imguiWindow()
{
  if (denoised_rtsm_id_for_debug == BAD_TEXTUREID)
    return;

  if (auto denoised_rtsm = acquire_managed_tex(denoised_rtsm_id_for_debug); denoised_rtsm)
  {
    ImGui::SliderInt("Max stabilized frame count", &max_stabilized_frame_count, 0, 7);

    ImGuiDagor::Image(denoised_rtsm_id_for_debug, denoised_rtsm);

    release_managed_tex(denoised_rtsm_id_for_debug);
  }
}

REGISTER_IMGUI_WINDOW("Render", "RTSM", imguiWindow);
#endif

} // namespace rtsm