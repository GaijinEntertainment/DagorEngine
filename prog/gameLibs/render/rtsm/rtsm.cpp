// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/rtsm.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_computeShaders.h>
#include <bvh/bvh.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_bindless.h>
#include <render/denoiser.h>
#include <perfMon/dag_statDrv.h>
#include <image/dag_texPixel.h>
#include <math/dag_frustum.h>
#include <math/integer/dag_IPoint2.h>
#include <ioSys/dag_dataBlock.h>
#include <EASTL/algorithm.h>
#include <EASTL/optional.h>

#include "sobol_256_4d.h"
#include "scrambling_ranking_128x128_2d_1spp.h"

namespace rtsm
{

static ComputeShaderElement *trace = nullptr;
static ComputeShaderElement *traceDynamic = nullptr;

static UniqueTex finalShadowMap;
static UniqueTexHolder shadowValueMap;
static UniqueTexHolder shadowTranslucencyMap;
static UniqueTexHolder scramblingRankingTexture;
static UniqueTexHolder sobolTexture;

static UniqueTexHolder dynamicLightingTexture;

static int rt_shadow_resolutionVarId = -1;
static int rt_shadow_resolutionIVarId = -1;
static int world_view_posVarId = -1;
static int rtsm_render_modeVarId = -1;
static int rtsm_frame_indexVarId = -1;
static int use_precomputed_dynamic_lightsVarId = -1;
static int inv_proj_tmVarId = -1;
static int combined_shadowsVarId = -1;
static int rtsm_valueVarId = -1;
static int rtsm_translucencyVarId = -1;
static int rtsm_denoisedVarId = -1;
static int rtsm_has_nukeVarId = -1;
static int rtsm_bindless_slotVarId = -1;

static RenderMode render_mode = RenderMode::Hard;

void initialize(int width, int height, RenderMode rm, bool dynamic_light_shadows)
{
  if (!bvh::is_available())
    return;

  ::rtsm::render_mode = rm;

  rt_shadow_resolutionVarId = get_shader_variable_id("rt_shadow_resolution");
  rt_shadow_resolutionIVarId = get_shader_variable_id("rt_shadow_resolutionI");
  world_view_posVarId = get_shader_variable_id("world_view_pos");
  rtsm_render_modeVarId = get_shader_variable_id("rtsm_render_mode");
  rtsm_frame_indexVarId = get_shader_variable_id("rtsm_frame_index");
  use_precomputed_dynamic_lightsVarId = get_shader_variable_id("use_precomputed_dynamic_lights", true);
  inv_proj_tmVarId = get_shader_variable_id("inv_proj_tm");
  combined_shadowsVarId = get_shader_variable_id("combined_shadows");
  rtsm_valueVarId = get_shader_variable_id("rtsm_value");
  rtsm_translucencyVarId = get_shader_variable_id("rtsm_translucency");
  rtsm_denoisedVarId = get_shader_variable_id("rtsm_denoised");
  rtsm_has_nukeVarId = get_shader_variable_id("rtsm_has_nuke");
  rtsm_bindless_slotVarId = get_shader_variable_id("rtsm_bindless_slot");

  if (!trace)
    trace = new_compute_shader("rt_shadows");

  if (!traceDynamic)
    traceDynamic = new_compute_shader("rt_direct_lights");

  dynamicLightingTexture.close();
  finalShadowMap.close();
  shadowValueMap.close();
  shadowTranslucencyMap.close();

  if (dynamic_light_shadows)
    dynamicLightingTexture = dag::create_tex(nullptr, width, height, TEXCF_UNORDERED | TEXFMT_R11G11B10F, 1, "rtsm_dynamic_lights");

  UniqueTex shadowValueMapCreate, shadowTranslucencyMapCreate;

  switch (render_mode)
  {
    case RenderMode::Hard:
      finalShadowMap = dag::create_tex(nullptr, width, height, TEXCF_UNORDERED | TEXFMT_A8R8G8B8, 1, "rtsm_shadows");
      break;
    case RenderMode::Denoised: denoiser::make_shadow_maps(shadowValueMapCreate, finalShadowMap); break;
    case RenderMode::DenoisedTranslucent:
      denoiser::make_shadow_maps(shadowValueMapCreate, shadowTranslucencyMapCreate, finalShadowMap);
      break;
  }

  if (shadowValueMapCreate)
    shadowValueMap = eastl::move(shadowValueMapCreate);
  if (shadowTranslucencyMapCreate)
    shadowTranslucencyMap = eastl::move(shadowTranslucencyMapCreate);

  dynamicLightingTexture.setVar();
  shadowValueMap.setVar();
  shadowTranslucencyMap.setVar();

  ShaderGlobal::set_texture(rtsm_valueVarId, shadowValueMap);
  ShaderGlobal::set_texture(rtsm_translucencyVarId, shadowTranslucencyMap);
  ShaderGlobal::set_texture(rtsm_denoisedVarId, finalShadowMap);

  if (!scramblingRankingTexture)
  {
    static_assert(sizeof(scrambling_ranking_128x128_2d_1spp) == 128 * 128 * 4);
    TexImage32 *image = TexImage32::create(128, 128, tmpmem);
    memcpy(image->getPixels(), scrambling_ranking_128x128_2d_1spp, sizeof(scrambling_ranking_128x128_2d_1spp));
    scramblingRankingTexture = dag::create_tex(image, 128, 128, TEXFMT_R8G8B8A8, 1, "scrambling_ranking_texture");
    scramblingRankingTexture.setVar();
    memfree(image, tmpmem);
  }

  if (!sobolTexture)
  {
    static_assert(sizeof(sobol_256_4d) == 256 * 1 * 4);
    TexImage32 *image = TexImage32::create(256, 1, tmpmem);
    memcpy(image->getPixels(), sobol_256_4d, sizeof(sobol_256_4d));
    sobolTexture = dag::create_tex(image, 256, 1, TEXFMT_R8G8B8A8, 1, "sobol_texture");
    sobolTexture.setVar();
    memfree(image, tmpmem);
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
  safe_delete(traceDynamic);

  dynamicLightingTexture.close();
  finalShadowMap.close();
  shadowValueMap.close();
  shadowTranslucencyMap.close();
  sobolTexture.close();
  scramblingRankingTexture.close();

  ShaderGlobal::set_int(rtsm_bindless_slotVarId, -1);
}

void render(bvh::ContextId context_id, const Point3 &view_pos, const TMatrix4 &proj_tm, bool has_nuke, bool has_dynamic_lights,
  Texture *csm_texture, d3d::SamplerHandle csm_sampler)
{
  G_ASSERT(trace);
  G_ASSERT(finalShadowMap);

  TIME_D3D_PROFILE(rtsm::render);

  TextureInfo ti;
  finalShadowMap->getinfo(ti);

  bvh::bind_resources(context_id, ti.w);

  ShaderGlobal::set_color4(rt_shadow_resolutionVarId, ti.w, ti.h);
  ShaderGlobal::set_int4(rt_shadow_resolutionIVarId, ti.w, ti.h, 0, 0);
  ShaderGlobal::set_color4(world_view_posVarId, view_pos);
  ShaderGlobal::set_int(rtsm_frame_indexVarId, denoiser::get_frame_number());
  ShaderGlobal::set_int(rtsm_has_nukeVarId, has_nuke ? 1 : 0);
  ShaderGlobal::set_int(use_precomputed_dynamic_lightsVarId, dynamicLightingTexture && has_dynamic_lights ? 1 : 0);
  ShaderGlobal::set_float4x4(inv_proj_tmVarId, inverse44(proj_tm));

  switch (render_mode)
  {
    case RenderMode::Hard: ShaderGlobal::set_int(rtsm_render_modeVarId, 0); break;
    case RenderMode::Denoised: ShaderGlobal::set_int(rtsm_render_modeVarId, 1); break;
    case RenderMode::DenoisedTranslucent: ShaderGlobal::set_int(rtsm_render_modeVarId, 2); break;
  }

  trace->dispatchThreads(ti.w, ti.h, 1);

  denoiser::ShadowDenoiser params;
  params.denoisedShadowMap = finalShadowMap.getTex2D();
  params.shadowValue = shadowValueMap.getTex2D();
  params.shadowTranslucency = render_mode == RenderMode::DenoisedTranslucent ? shadowTranslucencyMap.getTex2D() : nullptr;
  params.csmTexture = csm_texture;
  params.csmSampler = csm_sampler;

  denoiser::denoise_shadow(params);
}

void render_dynamic_light_shadows(bvh::ContextId context_id, const Point3 &view_pos, bool has_nuke)
{
  if (!dynamicLightingTexture)
    return;

  G_ASSERT(traceDynamic);
  G_ASSERT(dynamicLightingTexture);

  TIME_D3D_PROFILE(rtsm::render_dynamic);

  TextureInfo ti;
  dynamicLightingTexture->getinfo(ti);

  bvh::bind_resources(context_id, ti.w);

  ShaderGlobal::set_int4(rt_shadow_resolutionIVarId, ti.w, ti.h, 0, 0);
  ShaderGlobal::set_color4(rt_shadow_resolutionVarId, ti.w, ti.h);
  ShaderGlobal::set_color4(world_view_posVarId, view_pos);
  ShaderGlobal::set_int(use_precomputed_dynamic_lightsVarId, 1);
  ShaderGlobal::set_int(rtsm_has_nukeVarId, has_nuke ? 1 : 0);

  const float zero[4] = {0, 0, 0, 0};
  d3d::clear_rwtexf(dynamicLightingTexture.getTex2D(), zero, 0, 0);
  traceDynamic->dispatchThreads(ti.w, ti.h, 1);
}

void turn_off()
{
  ShaderGlobal::set_int(use_precomputed_dynamic_lightsVarId, 0);
  ShaderGlobal::set_int(rtsm_bindless_slotVarId, -1);
}

TextureIDPair get_output_texture() { return TextureIDPair(finalShadowMap.getTex2D(), finalShadowMap.getTexId()); }

} // namespace rtsm