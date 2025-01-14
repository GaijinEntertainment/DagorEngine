// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <stdio.h>
#include <vecmath/dag_vecMath.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <3d/dag_textureIDHolder.h>
#include <drv/3d/dag_resetDevice.h>
#include <shaders/dag_postFxRenderer.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_shaders.h>
#include <render/scopeRenderTarget.h>
#include <3d/dag_lockTexture.h>
#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_autoFuncProf.h>
#include <render/set_reprojection.h>
#include <render/viewVecs.h>
#include <daSkies2/daScattering.h>
#include "preparedSkies.h"
#include <math/dag_Point3.h>
#include "shaders/clouds2/cloud_settings.hlsli"
#include "shaders/atmosphere/texture_sizes.hlsli"

#define GLOBAL_VARS_LIST                                 \
  VAR(skies_globtm, false)                               \
  VAR(skies_panoramic_scattering, false)                 \
  VAR(skies_mie_phase_consts, true)                      \
  VAR(skies_mie_extrapolating_coef, true)                \
  VAR(skies_planet_radius, false)                        \
  VAR(skies_atmosphere_radius, false)                    \
  VAR(prepare_origin, false)                             \
  VAR(prepare_resolution, false)                         \
  VAR(preparedScatteringDistToTc, false)                 \
  VAR(prev_skies_frustum_scattering, false)              \
  VAR(prev_skies_frustum_scattering_samplerstate, false) \
  VAR(skies_froxels_prev_dist, false)                    \
  VAR(skies_frustum_scattering_frame, false)             \
  VAR(skies_froxels_resolution, false)                   \
  VAR(skies_froxels_dist, false)                         \
  VAR(skies_frustum_scattering_last_tz, true)            \
  VAR(skies_continue_temporal, false)                    \
  VAR(skies_min_max_horizon_mu, false)                   \
  VAR(skies_view_lut_tex, false)                         \
  VAR(skies_view_lut_tex_samplerstate, false)            \
  VAR(skies_view_lut_mie_tex, false)                     \
  VAR(skies_view_lut_mie_tex_samplerstate, false)        \
  VAR(prev_skies_view_lut_tex, false)                    \
  VAR(prev_skies_view_lut_tex_samplerstate, false)       \
  VAR(prev_skies_view_lut_mie_tex, true)                 \
  VAR(prev_skies_view_lut_mie_tex_samplerstate, true)    \
  VAR(prev_skies_min_max_horizon_mu, false)              \
  VAR(preparedLoss, false)                               \
  VAR(skies_frustum_scattering, true)                    \
  VAR(skies_frustum_scattering_samplerstate, true)       \
  VAR(skies_render_screen_split, true)


#define VAR(a, opt) static ShaderVariableInfo a##VarId(#a, opt);
GLOBAL_VARS_LIST
#undef VAR

static bool checkComputeUse() { return d3d::get_driver_desc().shaderModel >= 5.0_sm; }

void DaScattering::close()
{
  skies_ms_texture.close();
  skies_irradiance_texture.close();
  skies_transmittance_texture.close();
  gpuAtmosphereCb.close();
}

void DaScattering::initCommon()
{
  if (skies_generate_transmittance_ps.getElem())
    return;

  skies_render_screen.init("skies_render_screen");
  if (checkComputeUse())
  {
#define INIT(a) a.reset(new_compute_shader(#a, false))
    INIT(skies_view_lut_tex_prepare_cs);
    INIT(skies_prepare_transmittance_for_altitude_cs);
    INIT(skies_approximate_ms_lut_cs);
    INIT(skies_generate_transmittance_cs);
    INIT(indirect_irradiance_ms_cs);
    INIT(skies_integrate_froxel_scattering_cs);
#undef INIT
  }
  else
  {
#define INIT(a) a.init(#a)
    INIT(skies_view_lut_tex_prepare_ps);
    INIT(skies_generate_transmittance_ps);
    INIT(skies_prepare_transmittance_for_altitude_ps);
    INIT(skies_approximate_ms_lut_ps);
    INIT(indirect_irradiance_ms);
    INIT(skies_integrate_froxel_scattering_ps);
#undef INIT
  }
}

void DaScattering::initMsApproximation()
{
  d3d::SamplerHandle clampSamplerHndl;
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = d3d::AddressMode::Clamp;
    smpInfo.address_mode_v = d3d::AddressMode::Clamp;
    smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    clampSamplerHndl = d3d::request_sampler(smpInfo);
  }

  uint32_t fmt = TEXFMT_A16B16G16R16F;
  if (checkComputeUse() && d3d::get_driver_desc().issues.hasBrokenComputeFormattedOutput)
    fmt = TEXFMT_A32B32G32R32F;
  const uint32_t flags = checkComputeUse() ? TEXCF_UNORDERED : TEXCF_RTARGET;
  if (!skies_transmittance_texture)
  {
    skies_transmittance_texture =
      dag::create_tex(NULL, TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT, flags | fmt, 1, "skies_transmittance_texture");
    skies_transmittance_texture->disableSampler();
    ShaderGlobal::set_sampler(::get_shader_variable_id("skies_transmittance_texture_samplerstate"), clampSamplerHndl);
  }

  if (!skies_irradiance_texture)
  {
    skies_irradiance_texture =
      dag::create_tex(NULL, IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT, flags | fmt, 1, "skies_irradiance_texture");
    skies_irradiance_texture->disableSampler();
    ShaderGlobal::set_sampler(::get_shader_variable_id("skies_irradiance_texture_samplerstate"), clampSamplerHndl);
  }

  if (!skies_ms_texture)
  {
    skies_ms_texture =
      dag::create_tex(NULL, SKIES_MULTIPLE_SCATTERING_APPROX, SKIES_MULTIPLE_SCATTERING_APPROX, flags | fmt, 1, "skies_ms_texture");
    skies_ms_texture->disableSampler();
    ShaderGlobal::set_sampler(::get_shader_variable_id("skies_ms_texture_samplerstate"), clampSamplerHndl);
  }
}

void DaScattering::init()
{
  initCommon();
  initMsApproximation();
}

PreparedSkies *create_prepared_skies(const char *base_name, const PreparedSkiesParams &params)
{
  PreparedSkies *skies = new PreparedSkies;
  const bool reprojection = (params.reprojection == PreparedSkiesParams::Reprojection::ON);
  skies->panoramic = params.panoramic == PreparedSkiesParams::Panoramic::ON;
  skies->rangeScale = params.scatteringRangeScale < 0 ? -1.0f : clamp(params.scatteringRangeScale, 0.01f, 2.5f);
  uint32_t fmtOut = TEXFMT_A16B16G16R16F;
  uint32_t fmtLoss = TEXFMT_R11G11B10F;
  const uint32_t flg = checkComputeUse() ? TEXCF_UNORDERED : TEXCF_RTARGET;
  if (!(d3d::get_texformat_usage(fmtLoss) & flg))
    fmtLoss = TEXFMT_A16B16G16R16F;
  if (checkComputeUse() && d3d::get_driver_desc().issues.hasBrokenComputeFormattedOutput)
    fmtOut = fmtLoss = TEXFMT_A32B32G32R32F;
  const float defaultGoodDist = 80000.f, defaultFullDist = 300000.f;
  skies->goodQualityDist = skies->rangeScale < 0 ? MAX_CLOUDS_DIST_LIGHT_KM * 1000 / 1.4 : skies->rangeScale * defaultGoodDist;
  skies->fullDist = skies->rangeScale < 0 ? skies->goodQualityDist * 1.4 : skies->rangeScale * defaultFullDist;
  skies->minRange = params.minScatteringRange;
  String name;
  {
    const int prepared_texsize = 32 * clamp(params.transmittanceColorQuality, 1, 4);
    name.printf(128, "%s_loss", base_name);
    skies->preparedLoss = dag::create_tex(NULL, prepared_texsize, prepared_texsize, flg | fmtLoss, 1, name);
    skies->preparedLoss->disableSampler();
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = d3d::AddressMode::Clamp;
    smpInfo.address_mode_v = d3d::AddressMode::Clamp;
    smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    ShaderGlobal::set_sampler(::get_shader_variable_id("preparedLoss_samplerstate"), d3d::request_sampler(smpInfo));
  }
  const int wMul = skies->panoramic ? 3 : 1;
  // if (params.scatteringScreenQuality > 0)
  {
    const int scatteringQuality = clamp(params.scatteringScreenQuality, 1, 8);
    const int slices = clamp(params.scatteringDepthSlices, 8, 128);
    for (int i = 0; i < (reprojection ? skies->scatteringVolume.size() : 1); ++i)
    {
      name.printf(0, "%s_skies_frustum_scattering%d", base_name, i);
      skies->scatteringVolume[i] = dag::create_voltex(16 * wMul * scatteringQuality, (skies->panoramic ? 12 : 24) * scatteringQuality,
        slices, fmtOut | flg, 1, name.c_str());
      skies->scatteringVolume[i]->disableSampler();
      d3d::resource_barrier({skies->scatteringVolume[i].getVolTex(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
    }
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = skies->panoramic ? d3d::AddressMode::Wrap : d3d::AddressMode::Clamp;
    smpInfo.address_mode_v = d3d::AddressMode::Clamp;
    smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    skies->scatteringVolumeSampler = d3d::request_sampler(smpInfo);
    skies->skiesLastTcZ = (slices - 1.5) / slices;
  }
  if (params.skiesLUTQuality > 0)
  {
    const int skyQuality = clamp(params.skiesLUTQuality, 1, 8);
    int w = (wMul * 32 * skyQuality + 7) & ~7, h = (18 * skyQuality + 7) & ~7;
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      if (skies->panoramic)
        smpInfo.address_mode_u = d3d::AddressMode::Wrap;
      d3d::SamplerHandle sampler = d3d::request_sampler(smpInfo);
      ShaderGlobal::set_sampler(skies_view_lut_tex_samplerstateVarId, sampler);
      ShaderGlobal::set_sampler(prev_skies_view_lut_tex_samplerstateVarId, sampler);
      ShaderGlobal::set_sampler(skies_view_lut_mie_tex_samplerstateVarId, sampler);
      ShaderGlobal::set_sampler(prev_skies_view_lut_mie_tex_samplerstateVarId, sampler);
    }
    for (int i = 0; i < (reprojection ? skies->skiesLutTex.size() : 1); ++i)
    {
      name.printf(0, "%s_skies_lut_tex_%d", base_name, i);
      skies->skiesLutTex[i] = dag::create_tex(NULL, w, h, fmtOut | flg, 1, name.c_str()); // noAlphaHdrFmt
      skies->skiesLutTex[i]->disableSampler();

      name.printf(0, "%s_skies_lut_mie_tex_%d", base_name, i);
      skies->skiesLutMieTex[i].close();
      skies->skiesLutMieTex[i] = dag::create_tex(NULL, w, h, fmtOut | flg, 1, name.c_str());
      skies->skiesLutMieTex[i]->disableSampler();
    }
    skies->skiesLutRestartTemporal = true;
    skies->resetGen = 0;
  }

  return skies;
}

void destroy_prepared_skies(PreparedSkies *&skies) { del_it(skies); }

void use_prepared_skies(PreparedSkies *skies, PreparedSkies *panorama_skies)
{
  if (!skies)
    return;
  ShaderGlobal::set_texture(preparedLossVarId, skies->preparedLoss);
  ShaderGlobal::set_texture(skies_frustum_scatteringVarId, skies->scatteringVolume[skies->frame & 1]);
  ShaderGlobal::set_sampler(skies_frustum_scattering_samplerstateVarId, skies->scatteringVolumeSampler);
  if (!panorama_skies)
  {
    ShaderGlobal::set_texture(skies_view_lut_texVarId, skies->skiesLutTex[skies->frame & 1]);
    ShaderGlobal::set_texture(skies_view_lut_mie_texVarId, skies->skiesLutMieTex[skies->frame & 1]);
  }
  else
  {
    ShaderGlobal::set_texture(skies_view_lut_texVarId, BAD_TEXTUREID);
    ShaderGlobal::set_texture(skies_view_lut_mie_texVarId, BAD_TEXTUREID);
  }
  if (skies->scatteringVolume[0])
  {
    TextureInfo info;
    skies->scatteringVolume[0]->getinfo(info, 0);
    ShaderGlobal::set_int4(skies_froxels_resolutionVarId, info.w, info.h, info.d, 0);
    ShaderGlobal::set_real(skies_froxels_distVarId, skies->froxelsMaxDist);
  }
  ShaderGlobal::set_float4x4(skies_globtmVarId, skies->prevGlobTm);
  ShaderGlobal::set_color4(prepare_originVarId, P3D(skies->preparedScatteringOrigin), 0);
  ShaderGlobal::set_color4(preparedScatteringDistToTcVarId, skies->precomputedTcDist);
  ShaderGlobal::set_int(skies_panoramic_scatteringVarId, skies->panoramic ? 1 : 0);
  ShaderGlobal::set_color4(skies_min_max_horizon_muVarId, skies->skiesParams);
  ShaderGlobal::set_real(skies_frustum_scattering_last_tzVarId,
    (skies->panoramic || panorama_skies != nullptr) ? 1.0f : skies->skiesLastTcZ);
}

void DaScattering::prepareFrustumScattering(PreparedSkies &skies, PreparedSkies *panorama_skies, const Point3 &primary_dir,
  const TMatrix &view_tm, const TMatrix4 &proj_tm, bool restart_temporal)
{
  if (!skies.scatteringVolume[0])
    return;
  ScopeReprojection reprojectionScope;
  restart_temporal |= !skies.scatteringVolume[1];
  alignas(16) Point4 viewVecLT, viewVecRT, viewVecLB, viewVecRB;
  // TODO: replace with calc_view_vecs() and proper set_viewvecs_to_shader()
  set_viewvecs_to_shader(viewVecLT, viewVecRT, viewVecLB, viewVecRB, view_tm, proj_tm);
  set_reprojection(view_tm, proj_tm, skies.prevProjTm, skies.prevWorldPos, skies.prevGlobTm, skies.prevViewVecLT, skies.prevViewVecRT,
    skies.prevViewVecLB, skies.prevViewVecRB, nullptr);
  ShaderGlobal::set_real(skies_frustum_scattering_last_tzVarId,
    (skies.panoramic || panorama_skies != nullptr) ? 1.0f : skies.skiesLastTcZ);
  if (skies.scatteringVolume[1])
  {
    skies.frame++;
    skies.frame &= (1 << 24) - 1; // mantissa
  }
  else
    skies.frame = 0;
  const int cFrame = skies.frame & 1;
  ShaderGlobal::set_int(skies_frustum_scattering_frameVarId, skies.frame);
  ShaderGlobal::set_int(skies_continue_temporalVarId, restart_temporal ? 0 : 1);
  ShaderGlobal::set_int(skies_panoramic_scatteringVarId, skies.panoramic ? 1 : 0);

  const double RG = getEarthRadius();
  const double rRG = RG / (RG + max(0.01, skies.prevWorldPos.y * 0.001));
  const float muHoriz = -safe_sqrt(1.0 - rRG * rRG);
  const float lenLT = length(Point3::xyz(viewVecLT)), lenLB = length(Point3::xyz(viewVecLB)), lenRT = length(Point3::xyz(viewVecRT)),
              lenRB = length(Point3::xyz(viewVecRB));
  auto minL = [](float y, float len) { return min(y, y / len); };
  auto maxL = [](float y, float len) { return max(y, y / len); };
  const float muMin =
    max(-1.f, min(min(minL(viewVecLT.y, lenLT), minL(viewVecLB.y, lenLB)), min(minL(viewVecRT.y, lenRT), minL(viewVecRB.y, lenRB))));
  const float muMax =
    min(1.0f, max(max(maxL(viewVecLT.y, lenLT), maxL(viewVecLB.y, lenLB)), max(maxL(viewVecRT.y, lenRT), maxL(viewVecRB.y, lenRB))));

  float belowHorizonPartTc = 0.5;
  float muMed = muHoriz;
  if (muHoriz <= muMin)
  {
    belowHorizonPartTc = 0;
    muMed = muMin;
  }
  else if (muHoriz >= muMax)
  {
    belowHorizonPartTc = 1;
    muMed = muMax;
  }
  else
    belowHorizonPartTc = (muHoriz - muMin) / max(1e-6f, muMax - muMin);

  // there was also a pole check and a horizon (being too far from the view angle) check, but it seems they are not needed anymore
  // these checks also changed parametrization to isEncodingInvalid==true, probably it has fixed some bugs/or increase quality
  // but likely they are not effectively working anymore after new volumetric scattering is introduced
  // isEncodingInvalid makes LUT to be very ineffective, causing significant jump of quality
  const bool isEncodingInvalid = muMax == muMin;
  skies.skiesParams = Color4{muMin, muMed, muMax, isEncodingInvalid ? -1 : belowHorizonPartTc};
  ShaderGlobal::set_color4(skies_min_max_horizon_muVarId, skies.skiesParams);
  ShaderGlobal::set_color4(prev_skies_min_max_horizon_muVarId, skies.prevSkiesParams);
  skies.prevSkiesParams = skies.skiesParams;
  ShaderGlobal::set_texture(prev_skies_view_lut_texVarId, skies.skiesLutTex[1 - cFrame]);
  ShaderGlobal::set_texture(prev_skies_view_lut_mie_texVarId, skies.skiesLutMieTex[1 - cFrame]);
  ShaderGlobal::set_texture(skies_view_lut_texVarId, skies.skiesLutTex[cFrame]);
  // #endif
  if (skies.skiesLutTex[0] && !panorama_skies)
  {
    TIME_D3D_PROFILE(trace_sky);
    if (skies.skiesLutRestartTemporal)
    {
      skies.skiesLutRestartTemporal = false;
      restart_temporal = true;
    }

    TextureInfo info;
    skies.skiesLutTex[cFrame]->getinfo(info, 0);
    ShaderGlobal::set_color4(prepare_resolutionVarId, info.w, info.h, 0, 0);
    ShaderGlobal::set_int(skies_continue_temporalVarId,
      restart_temporal || dot(primary_dir, skies.preparedScatteringPrimaryLight) < 0.9 ? 0 : 1);
    if (skies_view_lut_tex_prepare_cs)
    {
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), skies.skiesLutTex[cFrame].getTex2D());
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 1, VALUE, 0, 0), skies.skiesLutMieTex[cFrame].getTex2D());
      skies_view_lut_tex_prepare_cs->dispatchThreads(info.w, info.h, 1);
    }
    else
    {
      SCOPE_RENDER_TARGET;
      d3d::set_render_target(skies.skiesLutTex[cFrame].getTex2D(), 0);
      d3d::set_render_target(1, skies.skiesLutMieTex[cFrame].getTex2D(), 0);
      skies_view_lut_tex_prepare_ps.render();
    }
    d3d::resource_barrier({skies.skiesLutTex[cFrame].getTex2D(), RB_RO_SRV | RB_STAGE_ALL_GRAPHICS | RB_STAGE_COMPUTE, 0, 0});
    d3d::resource_barrier({skies.skiesLutMieTex[cFrame].getTex2D(), RB_RO_SRV | RB_STAGE_ALL_GRAPHICS | RB_STAGE_COMPUTE, 0, 0});
  }
  else if (skies.skiesLutTex[0] && panorama_skies && !skies.skiesLutRestartTemporal)
  {
    skies.skiesLutRestartTemporal = true;
  }

  {
    TIME_D3D_PROFILE(scatteringVolumeRender)
    // static int zn_zfarVarId = get_shader_variable_id("zn_zfar", true);
    // debug("zn_zfar = %@", ShaderGlobal::get_color4_fast(zn_zfarVarId));
    ShaderGlobal::set_texture(prev_skies_frustum_scatteringVarId, skies.scatteringVolume[1 - cFrame]);
    ShaderGlobal::set_sampler(prev_skies_frustum_scattering_samplerstateVarId, skies.scatteringVolumeSampler);
    ShaderGlobal::set_int(skies_continue_temporalVarId, restart_temporal ? 0 : 1);

    TextureInfo info;
    skies.scatteringVolume[0]->getinfo(info, 0);
    float maxDist = skies.goodQualityDist;
    if (skies.rangeScale > 0)
    {
      const TMatrix4 globtmInv = inverse44(skies.prevGlobTm).transpose();
      const Point4 ltFar = Point4(-1, -1, 0, 1) * globtmInv, rbFar = Point4(+1, +1, 0, 1) * globtmInv;
      const float maxFarDist = (ltFar.w < 1e-6 || rbFar.w < 1e-6) ? skies.goodQualityDist * 1.333f : // that's assumption that we have
                                                                                                     // goodQualityDist == zfar +
                                                                                                     // corners
                                 max(length(Point3::xyz(ltFar)) / ltFar.w, length(Point3::xyz(rbFar)) / rbFar.w);
      // Driver3dPerspective p;
      // G_VERIFY(d3d::getpersp(p));//const float maxViewLenDistDist = max(max(lenLT, lenLB), max(lenRT, lenRB));
      // debug("zfr dist  %@ vs %f*%f = %f",length(Point3::xyz(ltFar))/max(1e-6f, ltFar.w), maxViewLenDistDist, p.zf,
      // maxViewLenDistDist*p.zf);

      // static const float froxels_dist_mul = info.d/(info.d-0.5);//so we have zfar at center of latest texel
      maxDist = max(skies.minRange, maxFarDist * info.d / (info.d - 0.5f) * skies.rangeScale);
    }
    ShaderGlobal::set_int4(skies_froxels_resolutionVarId, info.w, info.h, info.d, 0);
    ShaderGlobal::set_real(skies_froxels_distVarId, maxDist);
    ShaderGlobal::set_real(skies_froxels_prev_distVarId, skies.froxelsMaxDist);
    skies.froxelsMaxDist = maxDist;

    if (skies_integrate_froxel_scattering_cs)
    {
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), skies.scatteringVolume[cFrame].getVolTex());
      skies_integrate_froxel_scattering_cs->dispatchThreads(info.w, info.h, 1);
    }
    else
    {
      SCOPE_RENDER_TARGET;
      d3d::set_render_target();
      d3d::set_render_target(0, skies.scatteringVolume[cFrame].getVolTex(), d3d::RENDER_TO_WHOLE_ARRAY, 0);
      skies_integrate_froxel_scattering_ps.getElem()->setStates();
      d3d::draw_instanced(PRIM_TRILIST, 0, 1, info.d);
    }
    d3d::resource_barrier({skies.scatteringVolume[cFrame].getVolTex(), RB_RO_SRV | RB_STAGE_ALL_GRAPHICS | RB_STAGE_COMPUTE, 0, 0});
    ShaderGlobal::set_texture(skies_frustum_scatteringVarId, skies.scatteringVolume[cFrame]);
    ShaderGlobal::set_sampler(skies_frustum_scattering_samplerstateVarId, skies.scatteringVolumeSampler);
  }
  use_prepared_skies(&skies, panorama_skies);
}

inline bool equal(const Point3 &a, const Point3 &b)
{
  return is_relative_equal_float(a.x, b.x) && is_relative_equal_float(a.y, b.y) && is_relative_equal_float(a.z, b.z);
}

bool DaScattering::prepareSkyForAltitude(const Point3 &origin, float tolerance, uint32_t gen, PreparedSkies *skies,
  const TMatrix &view_tm, const TMatrix4 &proj_tm, uint32_t prepare_frame, PreparedSkies *panorama_skies, const Point3 &primary_dir,
  const Point3 &secondary_dir)
{
  if (!skies)
    return false;

  const uint32_t cGen = get_d3d_full_reset_counter();
  const bool resetted = skies->resetGen != cGen;
  const bool alreadyPreparedThisFrame = (prepare_frame == skies->lastSkiesPrepareFrame);
  if (!resetted && alreadyPreparedThisFrame)
    return false;
  skies->resetGen = cGen;
  skies->lastSkiesPrepareFrame = prepare_frame;
  bool updateScattering = true;
  bool resetTemporalScattering = resetted;
  if (skies->panoramic)
  {
    if (resetted || skies->scatteringGeneration != gen || !equal(skies->preparedScatteringPrimaryLight, primary_dir) ||
        !equal(skies->preparedScatteringSecondaryLight, secondary_dir) ||
        lengthSq(origin - skies->preparedScatteringOrigin) > tolerance)
    {
      resetTemporalScattering = true;
      updateScattering = true;
    }
    else
      updateScattering = false;
  }
  if (updateScattering)
  {
    ShaderGlobal::set_color4(prepare_originVarId, P3D(origin), 0);
    prepareFrustumScattering(*skies, panorama_skies, primary_dir, view_tm, proj_tm, resetTemporalScattering);
    skies->preparedScatteringOrigin = origin;
    skies->preparedScatteringPrimaryLight = primary_dir;
    skies->preparedScatteringSecondaryLight = secondary_dir;
  }
  const bool transmittanceChanged =
    fabsf(origin.y - skies->transmittanceAltitude) > tolerance || gen != skies->scatteringGeneration || resetted;

  skies->scatteringGeneration = gen;
  if (!transmittanceChanged || !skies->preparedLoss)
    return updateScattering;
  skies->transmittanceAltitude = origin.y;
  const float offset = max(0.f, origin.y - 1000.f * getAtmosphereAltitudeKm());
  const float sq = getPreparedDistSq();
  const float goodQualityScale = sq / skies->goodQualityDist;
  const float badQualityScale = (1 - sq) / (skies->fullDist - skies->goodQualityDist);
  skies->precomputedTcDist = Color4(goodQualityScale, -offset * goodQualityScale, badQualityScale,
    1 - skies->fullDist * badQualityScale - offset * badQualityScale);
  TextureInfo ti;
  skies->preparedLoss->getinfo(ti, 0);
  ShaderGlobal::set_color4(prepare_originVarId, P3D(origin), 0);
  ShaderGlobal::set_color4(preparedScatteringDistToTcVarId, skies->precomputedTcDist);
  ShaderGlobal::set_color4(prepare_resolutionVarId, ti.w, ti.h, 0, 0);

  TIME_D3D_PROFILE(prepareLoss);
  if (skies_prepare_transmittance_for_altitude_cs)
  {
    // need to clear it to avoid full black result in PIX // TODO: check if we still need this workaround
    d3d::clear_rwtexf(skies->preparedLoss.getTex2D(), ZERO_PTR<float>(), 0, 0);
    d3d::resource_barrier({skies->preparedLoss.getTex2D(), RB_FLUSH_UAV | RB_STAGE_ALL_SHADERS | RB_SOURCE_STAGE_COMPUTE, 0, 0});

    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), skies->preparedLoss.getTex2D());
    skies_prepare_transmittance_for_altitude_cs->dispatchThreads(ti.w, ti.h, 1);
  }
  else
  {
    SCOPE_RENDER_TARGET;
    d3d::set_render_target(skies->preparedLoss.getTex2D(), 0);
    skies_prepare_transmittance_for_altitude_ps.render();
  }
  d3d::resource_barrier({skies->preparedLoss.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});

  return true;
}

void DaScattering::renderSky()
{
  TIME_D3D_PROFILE(renderSkies);

  if (d3d::get_driver_desc().caps.hasTileBasedArchitecture && VariableMap::isVariablePresent(skies_render_screen_splitVarId) &&
      !ShaderGlobal::is_var_assumed(skies_render_screen_splitVarId))
  {
    ShaderGlobal::set_int(skies_render_screen_splitVarId, 1);
    skies_render_screen.render();
    ShaderGlobal::set_int(skies_render_screen_splitVarId, 2);
    skies_render_screen.render();
    ShaderGlobal::set_int(skies_render_screen_splitVarId, 3);
    skies_render_screen.render();
    ShaderGlobal::set_int(skies_render_screen_splitVarId, 0);
  }
  else
    skies_render_screen.render();
}

void DaScattering::setParams(const SkyAtmosphereParams &params)
{
  if (current == params)
    return;
  current = params;
  setParamsToShader();
}

void DaScattering::startGpuReadback()
{
  if (!skies_irradiance_texture)
    return;
  int stride; // start copy to CPU
  if (skies_irradiance_texture->lockimg(nullptr, stride, 0, TEXLOCK_READ | TEXLOCK_NOSYSLOCK))
    skies_irradiance_texture->unlockimg();
  gpuReadbackStarted = true;
  // todo: implement event/fence
}

bool DaScattering::finishGpuReadback()
{
  if (!gpuReadbackStarted || !skies_irradiance_texture)
    return false;
  TIME_PROFILE(skies_updateCPUIrradiance);
  if (auto lockedTex = lock_texture_ro(skies_irradiance_texture.getTex2D(), 0, TEXLOCK_READ | TEXLOCK_NOSYSLOCK))
  {
#if DAGOR_DBGLEVEL > 0
    debug("updateCPUIrradiance from GPU");
#endif
    updateCPUIrradiance(reinterpret_cast<const uint16_t *>(lockedTex.get()), lockedTex.getByteStride());
    lockedTex.close(); // unlock for resource barrier
    d3d::resource_barrier({skies_irradiance_texture.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
    gpuReadbackStarted = false;
    return true;
  }
  return false;
}

bool DaScattering::updateColors() { return finishGpuReadback(); }

void DaScattering::precompute_ms_approximation()
{
  initMsApproximation();
  TIME_D3D_PROFILE(precompute_ms_approximation);
  if (skies_generate_transmittance_cs)
  {
    TIME_D3D_PROFILE(prepare_transmittance_cs);
    // step 1 - transmittance
    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), skies_transmittance_texture.getTex2D());
    skies_generate_transmittance_cs->dispatchThreads(TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT, 1);
  }
  else
  {
    TIME_D3D_PROFILE(prepare_transmittance_ps);
    SCOPE_RENDER_TARGET;
    // step 1 - transmittance
    d3d::set_render_target(skies_transmittance_texture.getTex2D(), 0, 0);
    skies_generate_transmittance_ps.render(); // todo: use compute if available
  }
  d3d::resource_barrier({skies_transmittance_texture.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});

  // step 2 (MS)
  if (skies_approximate_ms_lut_cs)
  {
    TIME_D3D_PROFILE(prepare_ms_cs);
    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), skies_ms_texture.getTex2D());
    skies_approximate_ms_lut_cs->dispatch(SKIES_MULTIPLE_SCATTERING_APPROX, SKIES_MULTIPLE_SCATTERING_APPROX, 1);
  }
  else
  {
    SCOPE_RENDER_TARGET;
    TIME_D3D_PROFILE(prepare_ms_ps);
    d3d::set_render_target(skies_ms_texture.getTex2D(), 0);
    skies_approximate_ms_lut_ps.render();
  }
  d3d::resource_barrier({skies_ms_texture.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
  if (indirect_irradiance_ms_cs)
  {
    TIME_D3D_PROFILE(prepare_irradiance);
    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), skies_irradiance_texture.getTex2D());
    indirect_irradiance_ms_cs->dispatch(IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT, 1);
  }
  else
  {
    TIME_D3D_PROFILE(prepare_irradiance);
    SCOPE_RENDER_TARGET;
    d3d::set_render_target(skies_irradiance_texture.getTex2D(), 0, 0); // todo: use compute if available
    indirect_irradiance_ms.render();
  }
  d3d::resource_barrier({skies_irradiance_texture.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
  startGpuReadback();
}

void DaScattering::precompute(bool invalidateCpu)
{
  setParamsToShader();
  precompute_ms_approximation();

  if (invalidateCpu)
    invalidateCPUData();
  finishGpuReadback(); // just in case
  multipleScatteringGeneration++;
}

#include <math/dag_mathUtils.h>
#include <math/dag_hlsl_floatx.h>
#include "cpu_definitions.hlsli"
#include "shaders/atmosphere/atmosphere_params.hlsli"
void DaScattering::setParamsToShader()
{
  setCPUConsts();
  AtmosphereParameters atmosphere = getAtmosphere();
  ShaderGlobal::set_color4(skies_mie_extrapolating_coefVarId, P3D(atmosphere.mie_extrapolation_coef), 0);
  ShaderGlobal::set_color4(skies_mie_phase_constsVarId, P4D(atmosphere.mie_phase_consts));
  ShaderGlobal::set_real(skies_planet_radiusVarId, atmosphere.bottom_radius);
  ShaderGlobal::set_real(skies_atmosphere_radiusVarId, atmosphere.top_radius);

  // Recreating buffer to avoid an error message related to incorret usage of the persistent buffer.
  // This is ok because, we recreate it only several times during initialization
  gpuAtmosphereCb.close();
  gpuAtmosphereCb = dag::buffers::create_persistent_cb(dag::buffers::cb_struct_reg_count<AtmosphereParameters>(), "atmosphere_cb");
  // FIXME: sort out create_persistent_cb underlying meaning and upload methods
  // for vulkan we use writeonly here because discard can't be happening in parallel to d3d::update_screen
  const int lockFlags = d3d::get_driver_code().is(d3d::vulkan) ? VBLOCK_WRITEONLY : VBLOCK_DISCARD;
  gpuAtmosphereCb->updateDataWithLock(0, sizeof(AtmosphereParameters), &atmosphere, lockFlags);
}
