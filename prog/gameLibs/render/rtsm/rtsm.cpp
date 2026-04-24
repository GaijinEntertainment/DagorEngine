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
static int inv_proj_tmVarId = -1;
static int combined_shadowsVarId = -1;
static int rtsm_valueVarId = -1;
static int rtsm_translucencyVarId = -1;
static int rtsm_denoisedVarId = -1;
static int precomputed_dynamic_lightsVarId = -1;
static int use_precomputed_dynamic_lightsVarId = -1;
static int rtsm_has_nukeVarId = -1;
static int rtsm_bindless_slotVarId = -1;
static int rtsm_dynamic_light_radiusVarId = -1;
static int rtsm_dynamic_soft_shadowsVarId = -1;
static int rtsm_qualityVarId = -1;
static int denoiser_view_zVarId = -1;

static int max_stabilized_frame_count = 5;

static bool dynamic_light_shadows = false;

static RenderMode render_mode = RenderMode::Hard;

Texture *denoised_rtsm_for_debug = nullptr;

namespace TextureNames
{
const char *precomputed_dynamic_lights = "precomputed_dynamic_lights";
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
  inv_proj_tmVarId = get_shader_variable_id("inv_proj_tm");
  combined_shadowsVarId = get_shader_variable_id("combined_shadows");
  rtsm_valueVarId = get_shader_variable_id("rtsm_value");
  rtsm_translucencyVarId = get_shader_variable_id("rtsm_translucency", true);
  rtsm_denoisedVarId = get_shader_variable_id("rtsm_denoised");
  precomputed_dynamic_lightsVarId = get_shader_variable_id("precomputed_dynamic_lights");
  use_precomputed_dynamic_lightsVarId = get_shader_variable_id("use_precomputed_dynamic_lights", true);
  rtsm_has_nukeVarId = get_shader_variable_id("rtsm_has_nuke");
  rtsm_bindless_slotVarId = get_shader_variable_id("rtsm_bindless_slot");
  rtsm_dynamic_light_radiusVarId = get_shader_variable_id("rtsm_dynamic_light_radius");
  rtsm_dynamic_soft_shadowsVarId = get_shader_variable_id("rtsm_dynamic_soft_shadows");
  rtsm_qualityVarId = get_shader_variable_id("rtsm_quality");
  denoiser_view_zVarId = get_shader_variable_id("denoiser_view_z", true);

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

static void clean_shvars()
{
  ShaderGlobal::set_texture(rtsm_valueVarId, nullptr);
  ShaderGlobal::set_texture(rtsm_translucencyVarId, nullptr);
  ShaderGlobal::set_texture(rtsm_denoisedVarId, nullptr);
  ShaderGlobal::set_texture(denoiser_view_zVarId, nullptr);
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
  clean_shvars();
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

static void set_shvars(const RTSMContext &ctx, const TMatrix4 &inv_proj_tm, Point3 view_pos)
{
  ShaderGlobal::set_texture(rtsm_valueVarId, ctx.rtsmValueTex);

  if (!denoiser::is_ray_reconstruction_enabled())
  {
    ShaderGlobal::set_texture(rtsm_translucencyVarId, ctx.rtsmTranslucencyTex);
    ShaderGlobal::set_texture(rtsm_denoisedVarId, ctx.rtsmShadowsDenoisedTex);
  }

  auto view_z = ctx.denoiserParams.textures.find(denoiser::TextureNames::denoiser_view_z);
  ShaderGlobal::set_texture(denoiser_view_zVarId, (view_z != ctx.denoiserParams.textures.end() ? view_z->second : nullptr));

  bvh::bind_resources(ctx.ctxId, ctx.resolution.x);

  ShaderGlobal::set_float4(rt_shadow_resolutionVarId, ctx.resolution.x, ctx.resolution.y);
  ShaderGlobal::set_int4(rt_shadow_resolutionIVarId, ctx.resolution.x, ctx.resolution.y, 0, 0);
  ShaderGlobal::set_float4(world_view_posVarId, view_pos);
  ShaderGlobal::set_int(rtsm_has_nukeVarId, ctx.hasNuke ? 1 : 0);
  ShaderGlobal::set_float4x4(inv_proj_tmVarId, inv_proj_tm);

  if (denoiser::is_ray_reconstruction_enabled())
    ShaderGlobal::set_int(rtsm_render_modeVarId, 3);
  else
    switch (render_mode)
    {
      case RenderMode::Hard: ShaderGlobal::set_int(rtsm_render_modeVarId, 0); break;
      case RenderMode::Denoised: ShaderGlobal::set_int(rtsm_render_modeVarId, 1); break;
      case RenderMode::DenoisedTranslucent: ShaderGlobal::set_int(rtsm_render_modeVarId, 2); break;
    }

  ShaderGlobal::set_int(rtsm_qualityVarId, ctx.qualityMode);
}

eastl::optional<RTSMContext> prepare(bvh::ContextId context_id, const Point3 &light_dir, const denoiser::TexMap &textures,
  bool has_nuke, Texture *csm_texture, d3d::SamplerHandle csm_sampler, Texture *vsm_texture, d3d::SamplerHandle vsm_sampler,
  int quality_mode)
{
  if (!trace)
    return eastl::nullopt;

  TIME_D3D_PROFILE(rtsm::prepare);

  G_ASSERT(!denoiser::is_ray_reconstruction_enabled() || quality_mode == 0);

  denoiser::ShadowDenoiser params;
  params.lightDirection = light_dir;
  params.maxStabilizedFrameNum = max_stabilized_frame_count;
  params.csmTexture = csm_texture;
  params.csmSampler = csm_sampler;
  params.vsmTexture = vsm_texture;
  params.vsmSampler = vsm_sampler;
  params.textures = textures;

  Texture *rtsmValueTex = nullptr;
  Texture *rtsmShadowsDenoisedTex = nullptr;
  Texture *rtsmTranslucencyTex = nullptr;

  if (render_mode == RenderMode::Hard)
  {
    ACQUIRE_DENOISER_TEXTURE(params, rtsm_shadows_denoised, eastl::nullopt);
    rtsmValueTex = rtsm_shadows_denoised;
  }
  else
  {
    ACQUIRE_DENOISER_TEXTURE(params, rtsm_value, eastl::nullopt);
    rtsmValueTex = rtsm_value;
  }

  const IPoint2 &resolution = denoiser::resolution_config.dynRes.res;

  if (!denoiser::is_ray_reconstruction_enabled())
  {
    ACQUIRE_DENOISER_TEXTURE(params, rtsm_shadows_denoised, eastl::nullopt);
    ACQUIRE_OPTIONAL_DENOISER_TEXTURE(params, rtsm_translucency);

    denoised_rtsm_for_debug = rtsm_shadows_denoised;
    rtsmShadowsDenoisedTex = rtsm_shadows_denoised;
    rtsmTranslucencyTex = rtsm_translucency;
  }

  return RTSMContext{.denoiserParams = eastl::move(params),
    .resolution = resolution,
    .ctxId = context_id,
    .rtsmValueTex = rtsmValueTex,
    .rtsmTranslucencyTex = rtsmTranslucencyTex,
    .rtsmShadowsDenoisedTex = rtsmShadowsDenoisedTex,
    .qualityMode = quality_mode,
    .hasNuke = has_nuke};
}

void do_trace(const RTSMContext &ctx, const TMatrix4 &inv_proj_tm, const Point3 &view_pos)
{
  TIME_D3D_PROFILE(rtsm::do_trace);
  const IPoint2 &resolution = ctx.resolution;

  set_shvars(ctx, inv_proj_tm, view_pos);

  switch (ctx.qualityMode)
  {
    case 0:
    {
      TIME_D3D_PROFILE(rtsm::render_noisy_high)
      trace->dispatchThreads(resolution.x, resolution.y, 1);
    }
    break;
    case 1:
    {
      TIME_D3D_PROFILE(rtsm::render_noisy_medium)
      trace->dispatchThreads(resolution.x / 2, resolution.y, 1);
    }
      {
        TIME_D3D_PROFILE(rtsm::fill_medium)
        traceFillHighPrepass->dispatchThreads(resolution.x / 2, resolution.y, 1);
        traceFillHigh->dispatchThreads(resolution.x / 2, resolution.y, 1); // uses RT, requires TLAS
      }
      break;
    case 2:
    {
      TIME_D3D_PROFILE(rtsm::render_noisy_low)
      trace->dispatchThreads(resolution.x / 2, resolution.y, 1);
    }
      {
        TIME_D3D_PROFILE(rtsm::fill_low)
        traceFillLow->dispatchThreads(resolution.x / 2, resolution.y, 1);
      }
      break;
  }
}

void denoise(const RTSMContext &ctx)
{
  TIME_D3D_PROFILE(rtsm::denoise);

  if (render_mode == RenderMode::Hard)
  {
    ACQUIRE_DENOISER_TEXTURE(ctx.denoiserParams, rtsm_shadows_denoised, );
    d3d::resource_barrier(ResourceBarrierDesc(rtsm_shadows_denoised, RB_STAGE_ALL_SHADERS | RB_RO_SRV, 0, 0));
  }
  else
    denoiser::denoise_shadow(ctx.denoiserParams);

  clean_shvars();
}

void render(bvh::ContextId context_id, const Point3 &view_pos, const Point3 &light_dir, const TMatrix4 &proj_tm,
  const denoiser::TexMap &textures, bool has_nuke, Texture *csm_texture, d3d::SamplerHandle csm_sampler, Texture *vsm_texture,
  d3d::SamplerHandle vsm_sampler, int quality_mode)
{
  if (!trace)
    return;
  G_ASSERT(!denoiser::is_ray_reconstruction_enabled() || quality_mode == 0);

  TIME_D3D_PROFILE(rtsm::render);

  eastl::optional<RTSMContext> ctx =
    prepare(context_id, light_dir, textures, has_nuke, csm_texture, csm_sampler, vsm_texture, vsm_sampler, quality_mode);

  if (!ctx)
    return;

  do_trace(*ctx, inverse44(proj_tm), view_pos);
  denoise(*ctx);
}

void render_dynamic_light_shadows(bvh::ContextId context_id, const Point3 &view_pos, Texture *dynamic_lighting_texture,
  float light_radius, bool soft_shadow, bool has_nuke)
{
  if (!dynamic_lighting_texture)
    return;

  ShaderGlobal::set_texture(precomputed_dynamic_lightsVarId, dynamic_lighting_texture);
  ShaderGlobal::set_int(use_precomputed_dynamic_lightsVarId, 1);
  G_ASSERT(traceDynamic);

  TIME_D3D_PROFILE(rtsm::render_dynamic);

  TextureInfo ti;
  dynamic_lighting_texture->getinfo(ti);

  bvh::bind_resources(context_id, ti.w);

  ShaderGlobal::set_int4(rt_shadow_resolutionIVarId, ti.w, ti.h, 0, 0);
  ShaderGlobal::set_float4(rt_shadow_resolutionVarId, ti.w, ti.h);
  ShaderGlobal::set_float4(world_view_posVarId, view_pos);
  ShaderGlobal::set_int(rtsm_has_nukeVarId, has_nuke ? 1 : 0);
  ShaderGlobal::set_float(rtsm_dynamic_light_radiusVarId, light_radius);
  ShaderGlobal::set_int(rtsm_dynamic_soft_shadowsVarId, soft_shadow ? 1 : 0);

  const float zero[4] = {0, 0, 0, 0};
  d3d::clear_rwtexf(dynamic_lighting_texture, zero, 0, 0);
  traceDynamic->dispatchThreads(ti.w, ti.h, 1);
}

void turn_off() { ShaderGlobal::set_int(rtsm_bindless_slotVarId, -1); }

#if DAGOR_DBGLEVEL > 0
static void imguiWindow()
{
  if (!denoised_rtsm_for_debug)
    return;

  ImGui::SliderInt("Max stabilized frame count", &max_stabilized_frame_count, 0, 7);
  ImGuiDagor::Image(denoised_rtsm_for_debug);
}

REGISTER_IMGUI_WINDOW("Render", "RTSM", imguiWindow);
#endif

} // namespace rtsm