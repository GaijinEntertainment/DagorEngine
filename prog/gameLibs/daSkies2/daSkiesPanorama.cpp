// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daSkies2/daSkies.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVar.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_renderPass.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_async_pipeline.h>
#include <drv/3d/dag_lock.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_adjpow2.h>
#include <dag_noise/dag_uint_noise.h>
#include <perfMon/dag_statDrv.h>
#include <render/scopeRenderTarget.h>
#include <shaders/dag_shaderBlock.h>
#include "daStars.h"
#include "skiesData.h"
#include "shaders/clouds2/panorama_samples.hlsli"
#include "render/downsampleDepth.h"
#include <render/viewVecs.h>
#include <drv/3d/dag_info.h>
#include <math/dag_hlsl_floatx.h>

#include <daSkies2/panorama_util.hlsli>

static const int panorma_temporal_upsample = PANORAMA_TEMPORAL_SAMPLES; // more than 32 can decrease quality, but due to lack of bits
                                                                        // in TEXFMT_R11G11B10F will cause banding on blending (1/32 =
                                                                        // 0.032)
static const int panorma_quads_frames = 8;
static const int panorma_temporal_frames = panorma_temporal_upsample * panorma_quads_frames;
const int frames_to_exp_validate = 12;
static uint32_t panorama_async_pipe_feedback = 0;

static uint32_t panoramaFmt = TEXFMT_R11G11B10F; // TEXCF_SRGBREAD|TEXCF_SRGBWRITE;// TEXFMT_A16B16G16R16F;//TEXFMT_R11G11B10F;

#define GLOBAL_VARS_LIST                             \
  VAR(render_sun, false)                             \
  VAR(real_skies_sun_light_dir, false)               \
  VAR(real_skies_sun_color, false)                   \
  VAR(skies_moon_dir, true)                          \
  VAR(skies_moon_color, true)                        \
  VAR(skies_primary_sun_light_dir, false)            \
  VAR(skies_secondary_sun_light_dir, false)          \
  VAR(skies_primary_sun_color, false)                \
  VAR(skies_secondary_sun_color, false)              \
  VAR(skies_panorama_light_dir, false)               \
  VAR(panoramaTC, false)                             \
  VAR(clouds_panorama_tex_res, false)                \
  VAR(clouds_panorama_use_biquadratic, true)         \
  VAR(cloudsAlphaPanoramaWorldPos, false)            \
  VAR(currentPanoramaWorldOffset, false)             \
  VAR(skies_world_view_pos, false)                   \
  VAR(strata_pos_x, false)                           \
  VAR(strata_pos_z, false)                           \
  VAR(skies_render_panorama_scattering, false)       \
  VAR(skies_panorama_mu_horizon, false)              \
  VAR(skies_panorama_sun_opposite_tc_x, true)        \
  VAR(clouds_panorama_frame, false)                  \
  VAR(clouds_panorama_blend, false)                  \
  VAR(clouds_panorama_blend_from, false)             \
  VAR(clouds_panorama_tex, false)                    \
  VAR(clouds_panorama_tex_samplerstate, false)       \
  VAR(clouds_panorama_patch_tex, false)              \
  VAR(clouds_panorama_patch_tex_samplerstate, false) \
  VAR(rgbm_panorama_scale_factor, false)             \
  VAR(clouds_panorama_depth_out, true)               \
  VAR(clouds_panorama_split, true)                   \
  VAR(clouds_alpha_panorama_tex_samplerstate, true)

#define VAR(a, opt) static ShaderVariableInfo a##VarId;
GLOBAL_VARS_LIST
#undef VAR

void DaSkies::initPanorama(SkiesData * /*panorama_data*/, bool blend_two, int resolutionW, int resolutionH, bool compress_panorama,
  bool sky_panorama_patch_enabled, bool allow_async_pipelines_feedback)
{
  if (cpuOnly)
    return;
  if (panoramaTCVarId < 0)
  {
#define VAR(a, b) a##VarId = get_shader_variable_id(#a, b);
    GLOBAL_VARS_LIST
#undef VAR
  }

  if (!applyCloudsPanorama.getElem())
  {
    applyCloudsPanorama.init("applyCloudsPanorama");
    cloudsPanorama.init("clouds_panorama");
    cloudsAlphaPanorama.init("clouds_alpha_panorama");
    cloudsPanoramaMip.init("clouds_panorama_mip");
    skyPanorama.init("skyPanorama");
    cloudsPanoramaBlend.init("clouds_panorama_blend");
  }

  skyPanoramaPatchEnabled = sky_panorama_patch_enabled;
  panoramaAllowAsyncPipelineFeedback = allow_async_pipelines_feedback;

  TextureInfo tinfo;
  tinfo.w = tinfo.h = 0;
  if (cloudsPanoramaTex[0])
    cloudsPanoramaTex[0]->getinfo(tinfo);
  if (resolutionH <= 0)
    resolutionH = resolutionW / 2;
  if (!blend_two && cloudsPanoramaTex[1])
  {
    for (int i = 1; i < cloudsPanoramaTex.size(); ++i)
    {
      cloudsPanoramaTex[i].close();
      cloudsPanoramaPatchTex[i].close();
    }
  }

  if (tinfo.w != resolutionW || tinfo.h != resolutionH || (blend_two && !cloudsPanoramaTex[1]))
  {
    int start = 0;
    if (tinfo.w != resolutionW || tinfo.h != resolutionH)
    {
      panoramaFrame = 0;
      currentPanorama = 0;
      closePanorama();
      int panoramaQuality = clamp(tinfo.w / 1024, 1, 8);
      panoramaData = create_prepared_skies("panorama",
        PreparedSkiesParams{0,   // skies
          (panoramaQuality + 1), // scattering
          panoramaQuality * 16,  // slices
          panoramaQuality,       // transmittance
          -1.0f,                 //
          0.f, PreparedSkiesParams::Panoramic::ON, PreparedSkiesParams::Reprojection::OFF});
      panoramaData->scatteringVolume[1].close();
      G_ASSERT(!panoramaData->skiesLutTex[0]);
    }
    else
      start = 1;
    uint32_t alphaPanoramaFmt = TEXFMT_L8;
    if ((d3d::get_texformat_usage(TEXFMT_L8) & (d3d::USAGE_RTARGET | d3d::USAGE_FILTER)) == (d3d::USAGE_RTARGET | d3d::USAGE_FILTER))
      alphaPanoramaFmt = TEXFMT_L8;
    else
      alphaPanoramaFmt = TEXFMT_DEFAULT;

    debug("alphaPanoramaFmt=0x%08X, noAlphaSkyHdrFmt=0x%08X", alphaPanoramaFmt, panoramaFmt);
    invalidatePanorama(true);

    cloudsAlphaPanoramaTex = dag::create_tex(NULL, 256, 128, TEXCF_RTARGET | alphaPanoramaFmt, 1, "clouds_alpha_panorama_tex");
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = d3d::AddressMode::Wrap;
      smpInfo.address_mode_v = d3d::AddressMode::Clamp;
      ShaderGlobal::set_sampler(clouds_alpha_panorama_tex_samplerstateVarId, d3d::request_sampler(smpInfo));
    }
    cloudsAlphaPanoramaTex->disableSampler();

    if (VariableMap::isGlobVariablePresent(get_shader_variable_id("clouds_panorama_mip", true)))
    {
      cloudsPanoramaMipTex =
        dag::create_tex(NULL, resolutionW / 4, resolutionH / 4, TEXCF_RTARGET | panoramaFmt, 4, "clouds_panorama_mip");
      mipRenderer.init("bloom_filter_mipchain");
      cloudsPanoramaMipTex->texaddru(TEXADDR_WRAP);
      cloudsPanoramaMipTex->texaddrv(TEXADDR_CLAMP);
    }

    // todo: sky panorama patch
    // todo: skyPanoramaTex can and should be just LUT in panoramaData
    skyPanoramaTex = dag::create_tex(NULL, 256, 128, TEXCF_RTARGET | panoramaFmt, 1, "sky_panorama_tex");
    skyPanoramaTex->disableSampler();
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = d3d::AddressMode::Wrap;
      smpInfo.address_mode_v = d3d::AddressMode::Clamp;
      ShaderGlobal::set_sampler(::get_shader_variable_id("sky_panorama_tex_samplerstate"), d3d::request_sampler(smpInfo));
    }

    createCloudsPanoramaTex(blend_two, resolutionW, resolutionH);

    if (compress_panorama)
      initPanoramaCompression(resolutionW, resolutionH);
    ShaderGlobal::set_real(rgbm_panorama_scale_factorVarId, 1);

    TextureInfo tinfo;
    tinfo.w = tinfo.h = 0;
    if (cloudsPanoramaMipTex)
      cloudsPanoramaMipTex->getinfo(tinfo);
    ShaderGlobal::set_color4(clouds_panorama_tex_resVarId, resolutionW, resolutionH, tinfo.w, tinfo.h);
  }

  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = d3d::AddressMode::Wrap;
    smpInfo.address_mode_v = d3d::AddressMode::Clamp;
    ShaderGlobal::set_sampler(clouds_panorama_tex_samplerstateVarId, d3d::request_sampler(smpInfo));
  }
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = d3d::AddressMode::Clamp;
    smpInfo.address_mode_v = d3d::AddressMode::Clamp;
    smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    ShaderGlobal::set_sampler(clouds_panorama_patch_tex_samplerstateVarId, d3d::request_sampler(smpInfo));
  }
}

void DaSkies::destroyCloudsPanoramaSplitResources()
{
  cloudsPanoramaSplitTex.close();
  if (cloudsPanoramaSplitRP)
  {
    d3d::delete_render_pass(cloudsPanoramaSplitRP);
    cloudsPanoramaSplitRP = nullptr;
  }
}

void DaSkies::createCloudsPanoramaSplitTex(int resolution_width, int resolution_height)
{
  cloudsPanoramaSplitTex = dag::create_tex(NULL, resolution_width, resolution_height,
    TEXCF_RTARGET | TEXFMT_A32B32G32R32F | TEXCF_TRANSIENT, 1, "clouds_panorama_split_tex");
}

void DaSkies::createCloudsPanoramaSplitResources(int resolution_width, int resolution_height)
{
  cloudsPanoramaSplitTex.close();
  // do not keep split tex allocated on TBDR devices without lazy memory (budget costly!)
  cleanupPanoramaSplitTex = !d3d::get_driver_desc().caps.hasLazyMemory && d3d::get_driver_desc().caps.hasTileBasedArchitecture;
  if (!cleanupPanoramaSplitTex)
    createCloudsPanoramaSplitTex(resolution_width, resolution_height);

  if (!cloudsPanoramaSplitRP)
  {
    RenderPassTargetDesc targets[] = {
      {nullptr, TEXCF_RTARGET | TEXFMT_A32B32G32R32F | TEXCF_TRANSIENT, false}, {nullptr, TEXCF_RTARGET | panoramaFmt, false}};
    RenderPassBind binds[] = {{0, 0, 0, RP_TA_LOAD_CLEAR | RP_TA_SUBPASS_WRITE, RB_STAGE_PIXEL | RB_RW_RENDER_TARGET},
      {0, 1, 0, RP_TA_SUBPASS_READ, RB_STAGE_PIXEL | RB_RO_SRV},
      {1, 1, 0, RP_TA_LOAD_READ | RP_TA_SUBPASS_WRITE, RB_STAGE_PIXEL | RB_RW_RENDER_TARGET},
      {0, RenderPassExtraIndexes::RP_SUBPASS_EXTERNAL_END, 0, RP_TA_STORE_NONE, RB_STAGE_PIXEL | RB_RO_SRV},
      {1, RenderPassExtraIndexes::RP_SUBPASS_EXTERNAL_END, 0, RP_TA_STORE_WRITE, RB_STAGE_PIXEL | RB_RO_SRV}};

    cloudsPanoramaSplitRP = d3d::create_render_pass(
      {"clouds_panorama_multipass", sizeof(targets) / sizeof(targets[0]), sizeof(binds) / sizeof(binds[0]), targets, binds, 11});
  }
}

void DaSkies::createCloudsPanoramaTex(bool blend_two, int resolution_width, int resolution_height)
{
  auto create = [&](int i, int w, int h, int pw, int ph) {
    String name;
    name.printf(128, "clouds_panorama_tex_%d", i);
    const uint32_t flg = (TEXCF_RTARGET | panoramaFmt);
    cloudsPanoramaTex[i].close();
    cloudsPanoramaTex[i] = dag::create_tex(NULL, w, h, flg, 1, name);
    cloudsPanoramaTex[i]->disableSampler();

    if (skyPanoramaPatchEnabled)
    {
      name.printf(128, "clouds_panorama_patch_tex_%d", i);
      cloudsPanoramaPatchTex[i].close();
      cloudsPanoramaPatchTex[i] = dag::create_tex(NULL, pw, ph, flg, 1, name);
      cloudsPanoramaPatchTex[i]->disableSampler();
    }

    d3d::GpuAutoLock gpuLock;
    SCOPE_RENDER_TARGET;
    d3d::set_render_target(cloudsPanoramaTex[i].getTex2D(), 0);
    d3d::clearview(CLEAR_TARGET, 0, 0.f, 0);
    d3d::resource_barrier({cloudsPanoramaTex[i].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

    if (skyPanoramaPatchEnabled)
    {
      d3d::set_render_target(cloudsPanoramaPatchTex[i].getTex2D(), 0);
      d3d::clearview(CLEAR_TARGET, 0, 0.f, 0);
      d3d::resource_barrier({cloudsPanoramaPatchTex[i].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    }
  };

  allowCloudsPanoramaSplit = d3d::get_driver_desc().caps.hasNativeRenderPassSubPasses &&
                             d3d::get_driver_desc().caps.hasTileBasedArchitecture && d3d::get_driver_code().is(d3d::vulkan);
  if (allowCloudsPanoramaSplit)
    createCloudsPanoramaSplitResources(resolution_width, resolution_height);

  create(0, resolution_width, resolution_height, max(8, resolution_height / 16), max(8, resolution_height / 16));
  // create(2, (resolutionW+panorama_temporal_scale_x-1)/panorama_temporal_scale_x,
  //           (resolutionH+panorama_temporal_scale_y-1)/panorama_temporal_scale_y,
  //           (max(8, resolutionH/16)+panorama_temporal_scale_x-1)/panorama_temporal_scale_x,
  //           (max(8, resolutionH/16)+panorama_temporal_scale_y-1)/panorama_temporal_scale_y
  //           );

  if (blend_two)
  {
    create(1, resolution_width, resolution_height, max(8, resolution_height / 16), max(8, resolution_height / 16));
    create(2, resolution_width, resolution_height, max(8, resolution_height / 16), max(8, resolution_height / 16));
  }
  ShaderGlobal::set_color4(clouds_panorama_tex_resVarId, resolution_width, resolution_height, 0, 0);
}

void DaSkies::initPanoramaCompression(int width, int height)
{
  panoramaRGBMScaleFactor = 1.f;
  isPanoramaCompressed = false;
  clearUncompressedPanorama = false;

  auto etc2Type = get_texture_compression_type(TEXFMT_ETC2_RGBA);
  auto dxtType = get_texture_compression_type(TEXFMT_DXT5);
  bool etc2 = BcCompressor::isAvailable(etc2Type);
  bool dxt = BcCompressor::isAvailable(dxtType);

  if (etc2 || dxt)
  {
    auto compressFormat = etc2 ? TEXFMT_ETC2_RGBA : TEXFMT_DXT5;
    compressedCloudsPanoramaTex = dag::create_tex(NULL, width, height,
      compressFormat | TEXCF_CLEAR_ON_CREATE | TEXCF_UPDATE_DESTINATION, 1, "compressed_clouds_panorama_tex");
    compressedCloudsPanoramaTex->disableSampler();

    panoramaCompressor = eastl::make_unique<PanoramaCompressor>(cloudsPanoramaTex[0], compressFormat);
  }
}

void DaSkies::closePanorama()
{
  for (int i = 0; i < cloudsPanoramaTex.size(); ++i)
  {
    cloudsPanoramaTex[i].close();
    cloudsPanoramaPatchTex[i].close();
  }
  cloudsAlphaPanoramaTex.close();
  cloudsDepthPanoramaTex.close();
  cloudsDepthPanoramaPatchTex.close();
  cloudsDepthDownsampledPanoramaTex.close();
  cloudsDepthDownsampledPanoramaPatchTex.close();
  cloudsPanoramaMipTex.close();
  skyPanoramaTex.close();
  compressedCloudsPanoramaTex.close();
  destroy_prepared_skies(panoramaData);
  panoramaTemporarilyDisabled = false;
  destroyCloudsPanoramaSplitResources();
}

static inline void set_horizon_shadervar(double earth_radius, double height)
{
  double RG = earth_radius;
  double rRG = RG / (RG + max(0.0, height * 0.001));
  float mu_horiz = -safe_sqrt(1.0 - rRG * rRG);
  ShaderGlobal::set_real(skies_panorama_mu_horizonVarId, mu_horiz);
}

void DaSkies::clearUncompressedPanoramaTex()
{
  clearUncompressedPanorama = false;
  panoramaCompressor->clearIntemediateData();

  cloudsPanoramaTex[0].close();
}

void DaSkies::renderPanorama(const Point3 &origin, const TMatrix &view_tm, const TMatrix4 &proj_tm, const Driver3dPerspective &persp)
{
  if (!cloudsPanoramaTex[0] && !compressedCloudsPanoramaTex)
    return;

  set_viewvecs_to_shader(view_tm, proj_tm);

  ShaderGlobal::set_color4(currentPanoramaWorldOffsetVarId, P3D(panorama_render_origin), panoramaReprojectionWeight);

  TextureInfo tinfo;
  if (isPanoramaCompressed)
    compressedCloudsPanoramaTex->getinfo(tinfo);
  else
    cloudsPanoramaTex[0]->getinfo(tinfo);
  bool useBiQ = true;
  if (d3d::validatepersp(persp))
  {
    int w, h;
    d3d::get_target_size(w, h);
    const float pixelRatio = (0.5f * persp.hk * max(h, 1)) / tinfo.h;
    useBiQ = pixelRatio > (3.0f * tinfo.h) / 1024 ? 1 : 0;
  }
  ShaderGlobal::set_int(clouds_panorama_use_biquadraticVarId, useBiQ ? 1 : 0);

  set_horizon_shadervar(skies.getEarthRadius(), origin.y);
  ShaderGlobal::set_color4(skies_world_view_posVarId, origin.x, origin.y, origin.z, 0);
  ShaderGlobal::set_color4(skies_panorama_light_dirVarId, panoramaVariables.real_skies_sun_light_dir);
  float sunOppositeTcX =
    safe_atan2(-panoramaVariables.real_skies_sun_light_dir.r, -panoramaVariables.real_skies_sun_light_dir.g) * (0.5f / PI) + 0.5f;
  ShaderGlobal::set_real(skies_panorama_sun_opposite_tc_xVarId, sunOppositeTcX);
  ShaderGlobal::set_texture(clouds_panorama_texVarId,
    isPanoramaCompressed ? compressedCloudsPanoramaTex : cloudsPanoramaTex[currentPanorama]);

  if (skyPanoramaPatchEnabled)
    ShaderGlobal::set_texture(clouds_panorama_patch_texVarId, cloudsPanoramaPatchTex[currentPanorama]);

  {
    TIME_D3D_PROFILE(applyPanorama);
    // There is some renderpass split when rendering envi probe.
    // Since it happens on events (i. e. not every frame) it's ok to just allow RP interruption.
    d3d::allow_render_pass_target_load();
    applyCloudsPanorama.render();
  }

  if (clearUncompressedPanorama)
  {
    clearUncompressedPanoramaTex();
  }
}

void DaSkies::get_panorama_settings(PanoramaSavedSettings &a)
{
#define COLOR4_PARAM(v) a.v = ShaderGlobal::get_color4(v##VarId);
#define FLOAT_PARAM(v)  a.v = ShaderGlobal::get_real(v##VarId);
  PANORAMA_PARAMETERS
#undef COLOR4_PARAM
#undef FLOAT_PARAM
  a.cloudsOrigin = cloudsOrigin;
}

void DaSkies::set_panorama_settings(const PanoramaSavedSettings &a)
{
#define COLOR4_PARAM(v) ShaderGlobal::set_color4(v##VarId, a.v);
#define FLOAT_PARAM(v)  ShaderGlobal::set_real(v##VarId, a.v);
  PANORAMA_PARAMETERS
#undef COLOR4_PARAM
#undef FLOAT_PARAM
  if (cloudsOrigin != a.cloudsOrigin)
  {
    cloudsOrigin = a.cloudsOrigin;
    updateCloudsOrigin();
  }
}

void DaSkies::startUseOfCompressedTex()
{
  // 12 comes from daScatteringCPU (hardcoded), 2 is hardcoded in renderSkiesInc.sh
  float maxSunBrightness = 12.f * skyParams.sun_brightness * 2.f;
  panoramaRGBMScaleFactor = maxSunBrightness > maxRGBMScaleFactor ? maxRGBMScaleFactor : maxSunBrightness;
  panoramaCompressor->updateCompressedTexture(compressedCloudsPanoramaTex.getTex2D(), panoramaRGBMScaleFactor);
  ShaderGlobal::set_real(rgbm_panorama_scale_factorVarId, panoramaRGBMScaleFactor);

  isPanoramaCompressed = true;
  clearUncompressedPanorama = true;
}

bool DaSkies::updatePanorama(const Point3 &origin_, const TMatrix &view_tm, const TMatrix4 &proj_tm)
{
  if (!panoramaEnabled())
    return false;
  const uint32_t totalCycle = targetPanorama == currentPanorama ? panorma_temporal_upsample : panorma_temporal_frames;
  const float panorama_light_dir_change_threshold = cosf(5 * PI / 180.f);
  const float sunBrightness = brightness(getCpuSun(Point3(0, getCloudsStartAltSettings(), 0), primarySunDir));
  bool forcedInvalidate = false;
  if (panoramaValid != PANORAMA_INVALID && panoramaValid != PANORAMA_WAIT_RESOURCES &&
      (dot(primarySunDir, panorama_sun_light_dir) < panorama_light_dir_change_threshold ||
        !is_relative_equal_float(sunBrightness, panorama_sun_light_color_brightness, 0.05f, 0.2f) ||
        computedScatteringGeneration != panoramaScatteringGeneration || cloudsGeneration != panoramaCloudsGeneration))
  {
    if (panoramaValid == PANORAMA_VALID)
    {
      panoramaValidFrame =
        panoramaFrame + (cloudsPanoramaTex[1] ? panorma_temporal_frames * 2 : panorma_temporal_upsample * frames_to_exp_validate);
      panoramaValid = PANORAMA_OLD;
    }
    else
      panoramaValidFrame += totalCycle;
    forcedInvalidate = true;
    logwarn("One shouldn't use dynamic skies with panorama rendering. If you want to trigger update, call invalidatePanorama()."
            "sun dir %@ %@ dot %f < %f == %d br = %f %f %d generation = %d %d clouds %d %d",
      primarySunDir, panorama_sun_light_dir, dot(primarySunDir, panorama_sun_light_dir), panorama_light_dir_change_threshold,
      dot(primarySunDir, panorama_sun_light_dir) < panorama_light_dir_change_threshold, sunBrightness,
      panorama_sun_light_color_brightness, !is_relative_equal_float(sunBrightness, panorama_sun_light_color_brightness, 0.05f, 0.2f),
      computedScatteringGeneration, panoramaScatteringGeneration, cloudsGeneration, panoramaCloudsGeneration);
  }
  d3d::AutoPipelineAsyncCompileFeedback<false /*allow_compute_pipelines*/> asyncFeedback(
    panoramaAllowAsyncPipelineFeedback ? &panorama_async_pipe_feedback : nullptr);
  bool isRetryNeeded = asyncFeedback.isRetryNeeded();
  forcedInvalidate |= isRetryNeeded;
  if (panoramaValid == PANORAMA_VALID && !forcedInvalidate)
  {
    if (panoramaCompressor && !isPanoramaCompressed)
    {
      startUseOfCompressedTex();
    }
    return false;
  }
  else if (panoramaShouldWaitResourcesLoaded)
  {
    Tab<TEXTUREID> waitTex{};
    getCloudsTextureResourceDependencies(waitTex);
    if (!::prefetch_and_check_managed_textures_loaded(waitTex, true))
    {
      if (panoramaValid == PANORAMA_INVALID)
      {
        panoramaValid = PANORAMA_WAIT_RESOURCES;
      }
      return false;
    }
    else if (panoramaValid == PANORAMA_WAIT_RESOURCES)
    {
      panoramaValid = PANORAMA_INVALID;
    }
  }

  if (isPanoramaCompressed)
  {
    TextureInfo tinfo;
    tinfo.w = tinfo.h = 0;
    compressedCloudsPanoramaTex->getinfo(tinfo);
    createCloudsPanoramaTex(false, tinfo.w, tinfo.h);

    compressedCloudsPanoramaTex.close();
    initPanoramaCompression(tinfo.w, tinfo.h);
    ShaderGlobal::set_real(rgbm_panorama_scale_factorVarId, panoramaRGBMScaleFactor);
  }

  PanoramaSavedSettings vars;
  get_panorama_settings(vars);

  if (panoramaValid == PANORAMA_INVALID || forcedInvalidate)
  {
    isPanoramaDepthTexReady = false;
    panorama_sun_light_dir = primarySunDir; // todo: move me
    panorama_render_origin = origin_;       // todo: we should gradually move origin on apply
    get_panorama_settings(panoramaVariables);
  }
  set_panorama_settings(panoramaVariables);
  if (clouds && clouds->hasVisibleClouds())
    clouds->setCloudsOffsetVars(0);

  // todo: use panorama_sun_light_dir
  panoramaScatteringGeneration = computedScatteringGeneration;
  panorama_sun_light_color_brightness = sunBrightness;
  panoramaCloudsGeneration = cloudsGeneration;

  const bool strataTextureLoaded = !strataClouds || prefetch_and_check_managed_texture_loaded(strataClouds.getTexId(), true);
  if (!cloudsPanoramaTex[1])
    targetPanorama = 0;
  currentPanorama = 0;
  if (panoramaValid == PANORAMA_INVALID || isRetryNeeded)
  {
    targetPanorama = currentPanorama;
    panoramaFrame = 0;
    panoramaValidFrame = panoramaFrame + panorma_temporal_upsample;
    panoramaInvalidFrames = panoramaFrame + panorma_temporal_upsample;
  }
  else if (!strataTextureLoaded)
    panoramaValidFrame = panoramaFrame + totalCycle;
  const Point3 origin = panorama_render_origin;
  G_ASSERT(panoramaData);
  if (panoramaData)
  {
    if (panoramaValid == PANORAMA_INVALID || forcedInvalidate)
      panoramaData->resetGen = panoramaData->frame = panoramaData->lastSkiesPrepareFrame = 0;
    ShaderGlobal::set_int(skies_render_panorama_scatteringVarId, 1);
    skies.prepareSkyForAltitude(origin, 1.0f, computedScatteringGeneration, panoramaData, view_tm, proj_tm,
      panoramaData->lastSkiesPrepareFrame + 1, nullptr, getPrimarySunDir(), getSecondarySunDir());
    use_prepared_skies(panoramaData, nullptr);
    ShaderGlobal::set_int(skies_render_panorama_scatteringVarId, 0);
  }
  const uint32_t panoramaFrameFraction = panoramaFrame % totalCycle;
  TextureInfo tinfo;
  cloudsPanoramaTex[0]->getinfo(tinfo);
  {
    const uint32_t panoramaFrameQuadAndPatch = targetPanorama == currentPanorama ? 0 : panoramaFrameFraction % panorma_quads_frames;
    ShaderGlobal::set_color4(skies_world_view_posVarId, origin.x, origin.y, origin.z, 0);
    ShaderGlobal::set_color4(currentPanoramaWorldOffsetVarId, P3D(origin), panoramaReprojectionWeight);
    set_horizon_shadervar(skies.getEarthRadius(), origin.y);
    // set_panorama_settings(panoramaSaved[nextPanorama]);
    TIME_D3D_PROFILE(updatePanorama)
    // setCloudsVars();

    ShaderGlobal::set_real(render_sunVarId, 1);
    const bool updatePatch = panoramaFrameQuadAndPatch == 0;
    const int panoramaFrameQuad = panoramaFrameQuadAndPatch;
    skyStars->setMoonVars(origin, moonDir, moonAge, brightness(getCpuSunSky(Point3(0, moon_check_ht, 0), real_skies_sun_light_dir)));
    // todo stop animation
    {
      // todo: update in quads, and add patch as well.
      // for quads update with +1 pixel both sides
      TIME_D3D_PROFILE(skyPanorama)
      SCOPE_RENDER_TARGET;
      d3d::set_render_target(skyPanoramaTex.getTex2D(), 0);
      d3d::clearview(CLEAR_TARGET, 0, 0, 0);
      ShaderGlobal::set_color4(panoramaTCVarId, 0, 0, 1, 1);
      skyPanorama.render();
      d3d::resource_barrier({skyPanoramaTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    }
    renderStrataClouds(origin, view_tm, proj_tm, true);
    SCOPE_RENDER_TARGET;
    const uint32_t panoramaSubPixel = targetPanorama == currentPanorama ? panoramaFrameFraction % panorma_temporal_upsample
                                                                        : panoramaFrameFraction / panorma_quads_frames;
    ShaderGlobal::set_int(clouds_panorama_frameVarId, panoramaSubPixel);
    ShaderGlobal::set_real(clouds_panorama_blendVarId,
      (cloudsPanoramaTex[1] || panoramaFrame < panoramaInvalidFrames) ? 1.f / (1.f + panoramaSubPixel) : 0);

    auto updateSubpixel = [&](ManagedTex &to, int x, int y, int w, int h) {
      TIME_D3D_PROFILE(panoramaSubframe);

      if (allowCloudsPanoramaSplit)
      {
        RenderPassArea rpArea = {(uint32_t)x, (uint32_t)y, (uint32_t)w, (uint32_t)h, 0, 1};
        if (w == 0)
        {
          TextureInfo ti;
          to.getTex2D()->getinfo(ti, 0);
          rpArea.width = ti.w;
          rpArea.height = ti.h;
        }

        if (cleanupPanoramaSplitTex)
          createCloudsPanoramaSplitTex(rpArea.width, rpArea.height);

        RenderPassTarget targets[] = {{{cloudsPanoramaSplitTex.getTex2D(), 0, 0}, make_clear_value(0, 0, 0, 0)},
          {{to.getTex2D(), 0, 0}, make_clear_value(0, 0, 0, 0)}};

        d3d::begin_render_pass(cloudsPanoramaSplitRP, rpArea, targets);

        // clouds trace part
        ShaderGlobal::set_int(clouds_panorama_splitVarId, 1);
        cloudsPanorama.render();

        d3d::next_subpass();

        // followup blending part
        ShaderGlobal::set_int(clouds_panorama_splitVarId, 2);
        cloudsPanorama.render();

        d3d::end_render_pass();

        if (cleanupPanoramaSplitTex)
          cloudsPanoramaSplitTex.close();

        ShaderGlobal::set_int(clouds_panorama_splitVarId, 0);
      }
      else
      {
        d3d::set_render_target(to.getTex2D(), 0);
        if (w != 0)
          d3d::setview(x, y, w, h, 0, 1);
        cloudsPanorama.render();
      }
      d3d::resource_barrier({to.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    };

    auto updateSubpixelWithDepth = [&](ManagedTex &to, UniqueTex &depth, UniqueTexHolder &downsampled_depth, int x, int y, int w,
                                     int h) {
      TIME_D3D_PROFILE(panoramaSubframe);
      d3d::set_render_target(to.getTex2D(), 0);
      d3d::set_render_target(1, depth.getTex2D(), 0);
      if (w != 0)
        d3d::setview(x, y, w, h, 0, 1);
      cloudsPanorama.render();
      d3d::resource_barrier({to.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
      d3d::resource_barrier({depth.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
      if (panoramaFrame == panoramaValidFrame - 1)
        downsamplePanoramaDepth(depth, downsampled_depth);
    };
    auto panoramaTC = [&](int &x, int &y, int &w, int &h, ManagedTex &tex) {
      TextureInfo ti;
      tex->getinfo(ti);
      if (targetPanorama != currentPanorama)
      {
        y = 0;
        h = ti.h;
        const int stripW = ti.w / panorma_quads_frames;
        x = panoramaFrameQuad * stripW;
        w = panoramaFrameQuad == panorma_quads_frames - 1 ? ti.w - x : stripW;
        ShaderGlobal::set_color4(panoramaTCVarId, float(x) / ti.w, float(y) / ti.h, float(w) / ti.w, float(h) / ti.h);
      }
      else
      {
        x = y = w = h = 0;
        ShaderGlobal::set_color4(panoramaTCVarId, 0, 0, 1, 1);
      }
    };

    cloudsLayersHeightsBarrier();
    bool needDepthOut = renderDownsampledDepth && !isPanoramaDepthTexReady;
    ShaderGlobal::set_int(clouds_panorama_depth_outVarId, needDepthOut ? 1 : 0);
    {
      TIME_D3D_PROFILE(panoramaBig);
      int x, y, w, h;
      panoramaTC(x, y, w, h, cloudsPanoramaTex[targetPanorama]);
      if (needDepthOut)
      {
        if (!cloudsDepthPanoramaTex)
          createPanoramaDepthTex();
        updateSubpixelWithDepth(cloudsPanoramaTex[targetPanorama], cloudsDepthPanoramaTex, cloudsDepthDownsampledPanoramaTex, x, y, w,
          h);
      }
      else
        updateSubpixel(cloudsPanoramaTex[targetPanorama], x, y, w, h);
    }
    if (updatePatch && skyPanoramaPatchEnabled)
    {
      TIME_D3D_PROFILE(panoramaPatch);
      ShaderGlobal::set_color4(panoramaTCVarId, 0, 0, 0, 0);
      if (needDepthOut)
      {
        if (!cloudsDepthPanoramaPatchTex)
          createDepthPanoramaPatchTex();
        updateSubpixelWithDepth(cloudsPanoramaPatchTex[targetPanorama], cloudsDepthPanoramaPatchTex,
          cloudsDepthDownsampledPanoramaPatchTex, 0, 0, 0, 0);
      }
      else
        updateSubpixel(cloudsPanoramaPatchTex[targetPanorama], 0, 0, 0, 0);
    }
    // if (updatePatch)
    {
      TIME_D3D_PROFILE(update_clouds_alpha)
      ShaderGlobal::set_color4(cloudsAlphaPanoramaWorldPosVarId, origin.x, origin.y, origin.z, 0);
      ShaderGlobal::set_real(clouds_panorama_blendVarId, (panoramaFrame < panoramaInvalidFrames) ? 1.f / (1.f + panoramaSubPixel) : 0);
      int x, y, w, h;
      panoramaTC(x, y, w, h, cloudsAlphaPanoramaTex);
      d3d::set_render_target(cloudsAlphaPanoramaTex.getTex2D(), 0);
      if (panoramaFrame == 0)
        d3d::clearview(CLEAR_TARGET, 0, 0, 0);

      if (w != 0)
        d3d::setview(x, y, w, h, 0, 1);
      cloudsAlphaPanorama.render();
      d3d::resource_barrier({cloudsAlphaPanoramaTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    }
  }
  if (targetPanorama != currentPanorama)
  {
    SCOPE_RENDER_TARGET;
    TIME_D3D_PROFILE(panoramaBlend);
    G_ASSERT(targetPanorama >= 1 && currentPanorama == 0);
    auto blendTo = [&](ManagedTex &to, ManagedTex &from) {
      d3d::set_render_target(to.getTex2D(), 0);
      ShaderGlobal::set_texture(clouds_panorama_blend_fromVarId, from);
      cloudsPanoramaBlend.render();
      d3d::resource_barrier({to.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    };
    const uint32_t panoramaBlendFraction = panoramaFrameFraction;
    ShaderGlobal::set_real(clouds_panorama_blendVarId, 1. / (panorma_temporal_frames - panoramaBlendFraction));
    {
      TIME_D3D_PROFILE(panoramaBlend);
      blendTo(cloudsPanoramaTex[currentPanorama], cloudsPanoramaTex[2 - (targetPanorama - 1)]);
    }

    if (skyPanoramaPatchEnabled)
    {
      TIME_D3D_PROFILE(patchBlend);
      blendTo(cloudsPanoramaPatchTex[currentPanorama], cloudsPanoramaPatchTex[2 - (targetPanorama - 1)]);
    }
  }
  panoramaFrame++;
  if (panoramaFrame % totalCycle == 0)
  {
    uint32_t nextTarget = cloudsPanoramaTex[1] ? clamp(2 - (targetPanorama - 1), 1, 2) : 0;
    if (targetPanorama == currentPanorama && cloudsPanoramaTex[1])
    {
      d3d::stretch_rect(cloudsPanoramaTex[currentPanorama].getTex2D(), cloudsPanoramaTex[2 - (nextTarget - 1)].getTex2D());
      d3d::resource_barrier({cloudsPanoramaTex[currentPanorama].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

      if (skyPanoramaPatchEnabled)
      {
        d3d::stretch_rect(cloudsPanoramaPatchTex[currentPanorama].getTex2D(), cloudsPanoramaPatchTex[2 - (nextTarget - 1)].getTex2D());
        d3d::resource_barrier({cloudsPanoramaPatchTex[currentPanorama].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
      }
    }
    targetPanorama = nextTarget;
  }
  panoramaValid = (panoramaValidFrame == panoramaFrame) ? PANORAMA_VALID : PANORAMA_OLD;
  ShaderGlobal::set_texture(clouds_panorama_texVarId,
    isPanoramaCompressed ? compressedCloudsPanoramaTex : cloudsPanoramaTex[currentPanorama]);

  if (skyPanoramaPatchEnabled)
    ShaderGlobal::set_texture(clouds_panorama_patch_texVarId, cloudsPanoramaPatchTex[currentPanorama]);

  if (cloudsPanoramaMipTex && (panoramaFrame % totalCycle == 0 || panoramaFrame < panoramaInvalidFrames))
  {
    TIME_D3D_PROFILE(panorama_lowres_mip);
    SCOPE_RENDER_TARGET;
    d3d::set_render_target(cloudsPanoramaMipTex.getTex2D(), 0);
    ShaderGlobal::set_texture(clouds_panorama_texVarId,
      isPanoramaCompressed ? compressedCloudsPanoramaTex : cloudsPanoramaTex[currentPanorama]);
    cloudsPanoramaMip.render();
    mipRenderer.render(cloudsPanoramaMipTex.getTex2D());
    d3d::resource_barrier({cloudsPanoramaMipTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }

  set_panorama_settings(vars);
  return true;
}

const Point3 DaSkies::calcPanoramaSunDir(const Point3 &camera_pos) const
{
  if (!panoramaEnabled() || !isPanoramaValid() || clouds == nullptr)
    return primarySunDir;
  float skies_planet_radius = skies.getEarthRadius();
  float clouds_start_altitude2 = clouds->minimalStartAlt();
  float clouds_thickness2 = clouds->maximumTopAlt() - clouds->minimalStartAlt();
  float clouds_layer = skies_planet_radius + min(1000.0f, clouds_start_altitude2 + clouds_thickness2 * 0.5f);
  return get_panorama_inv_reprojection(camera_pos, panorama_sun_light_dir, panorama_render_origin, panoramaReprojectionWeight,
    clouds_layer, clouds_layer * clouds_layer, skies_planet_radius);
}

void DaSkies::createPanoramaDepthTexHelper(UniqueTex &depth, const char *depth_name, UniqueTexHolder &downsampled_depth,
  const char *downsampled_depth_name, int w, int h, int addru, int addrv)
{
  depth.close();
  downsampled_depth.close();

  depth = dag::create_tex(NULL, w, h, TEXCF_RTARGET | TEXFMT_R32F | TEXCF_UNORDERED | TEXCF_GENERATEMIPS,
    panoramaDepthTexMipNumber + 1, depth_name);
  depth->texfilter(TEXFILTER_LINEAR);
  depth->texmipmap(TEXMIPMAP_LINEAR);
  depth->texaddru(addru);
  depth->texaddrv(addrv);

  uint16_t downsampledW = w >> panoramaDepthTexMipNumber;
  uint16_t downsampledH = h >> panoramaDepthTexMipNumber;
  downsampled_depth =
    dag::create_tex(NULL, downsampledW, downsampledH, TEXCF_RTARGET | TEXFMT_R32F | TEXCF_UNORDERED, 1, downsampled_depth_name);
  downsampled_depth->texfilter(TEXFILTER_LINEAR);
  downsampled_depth->texaddru(addru);
  downsampled_depth->texaddrv(addrv);
}

void DaSkies::createPanoramaDepthTex()
{
  TextureInfo info;
  cloudsPanoramaTex[0]->getinfo(info);
  int resolutionW = info.w;
  int resolutionH = info.h;
  createPanoramaDepthTexHelper(cloudsDepthPanoramaTex, "clouds_depth_panorama_tex", cloudsDepthDownsampledPanoramaTex,
    "clouds_depth_downsampled_panorama_tex", resolutionW, resolutionH, TEXADDR_WRAP, TEXADDR_CLAMP);
}

void DaSkies::createDepthPanoramaPatchTex()
{
  TextureInfo info;
  cloudsPanoramaPatchTex[0]->getinfo(info);
  int resolutionW = info.w;
  int resolutionH = info.h;
  createPanoramaDepthTexHelper(cloudsDepthPanoramaPatchTex, "clouds_depth_panorama_patch_tex", cloudsDepthDownsampledPanoramaPatchTex,
    "clouds_depth_downsampled_panorama_patch_tex", resolutionW, resolutionH, TEXADDR_CLAMP, TEXADDR_CLAMP);
}

void DaSkies::downsamplePanoramaDepth(UniqueTex &depth, UniqueTexHolder &downsampled_depth)
{
  depth.getTex2D()->generateMips();
  TextureInfo depthTexInfo;
  depth->getinfo(depthTexInfo);
  uint16_t panoramaDownsampledDepthTexWidth = depthTexInfo.w >> panoramaDepthTexMipNumber;
  uint16_t panoramaDownsampledDepthTexHeight = depthTexInfo.h >> panoramaDepthTexMipNumber;
  downsampled_depth->updateSubRegion(depth.getTex2D(), 2, 0, 0, 0, panoramaDownsampledDepthTexWidth, panoramaDownsampledDepthTexHeight,
    1, 0, 0, 0, 0);
  depth.close();
  isPanoramaDepthTexReady = true;
}

void DaSkies::invalidatePanorama(bool force, bool waitResourcesLoaded)
{
  panoramaShouldWaitResourcesLoaded = waitResourcesLoaded;
  if (panoramaShouldWaitResourcesLoaded)
  {
    Tab<TEXTUREID> waitTex{};
    getCloudsTextureResourceDependencies(waitTex);
    prefetch_managed_textures(waitTex);
  }
  // Cannot be more invalid than it already is.
  if (panoramaValid == PANORAMA_INVALID) //-V1051
    return;

  if (force)
    panoramaValid = PANORAMA_INVALID;
  else
  {
    panoramaValid = PANORAMA_OLD;
    panoramaValidFrame =
      panoramaFrame + (cloudsPanoramaTex[1] ? panorma_temporal_frames * 3 : panorma_temporal_upsample * frames_to_exp_validate);
  }
}
