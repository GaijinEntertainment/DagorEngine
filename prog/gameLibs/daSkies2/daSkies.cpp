// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_adjpow2.h>
#include <math/random/dag_random.h>
#include <math/dag_rayIntersectSphere.h>
#include <generic/dag_staticTab.h>
#include <daSkies2/daSkies.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVar.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_commands.h>
#include <util/dag_simpleString.h>
#include <math/dag_Point3.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_TMatrix4.h>
#include <perfMon/dag_statDrv.h>
#include <render/scopeRenderTarget.h>
#include <startup/dag_globalSettings.h>
#include <render/viewVecs.h>
#include <ioSys/dag_dataBlock.h>
#include <shaders/dag_shaderBlock.h>
#include <render/noiseTex.h>
#include "daStars.h"
#include <render/fx/auroraBorealis.h>
#include "star_rendering_consts.hlsli"

#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <math/dag_mathUtils.h>
#include <math/dag_frustum.h>
#include <3d/dag_textureIDHolder.h>

#define USE_CONSOLE_BOOL_VAL         CONSOLE_BOOL_VAL
#define USE_CONSOLE_INT_VAL          CONSOLE_INT_VAL
#define USE_CONSOLE_FLOAT_VAL_MINMAX CONSOLE_FLOAT_VAL_MINMAX
#define USE_CONSOLE_FLOAT_VAL        CONSOLE_FLOAT_VAL

#include "skiesData.h"
#include "clouds2.h"
static uint32_t noAlphaHdrFmt = TEXFMT_R11G11B10F;
static uint32_t noAlphaSkyHdrFmt = TEXFMT_R11G11B10F;

#define GLOBAL_VARS_LIST                    \
  VAR(render_sun, false)                    \
  VAR(skies_have_two_suns, true)            \
  VAR(skies_moon_info, true)                \
  VAR(skies_moon_dir, true)                 \
  VAR(skies_moon_color, true)               \
  VAR(real_skies_sun_light_dir, false)      \
  VAR(real_skies_sun_color, false)          \
  VAR(light_shadow_dir, true)               \
  VAR(skies_primary_sun_light_dir, false)   \
  VAR(skies_secondary_sun_light_dir, false) \
  VAR(skies_primary_sun_color, false)       \
  VAR(skies_secondary_sun_color, false)     \
  VAR(skies_world_view_pos, false)          \
  VAR(strata_clouds_altitude, false)        \
  VAR(strata_clouds_effect, false)          \
  VAR(strata_tc_scale, false)               \
  VAR(strata_pos_x, false)                  \
  VAR(strata_pos_z, false)                  \
  VAR(skies_use_2d_shadows, true)           \
  VAR(lowres_sky, true)                     \
  VAR(skies_has_clouds, false)
// VAR(infinite_skies)

#define VAR(a, opt) static ShaderVariableInfo a##VarId;
GLOBAL_VARS_LIST
#undef VAR

static bool clouds_frustum_check(const Point3 &pos, const TMatrix &view_tm, const TMatrix4 &proj_tm, float earthRadius,
  float bottom_border, float top_border)
{
  const float earthRad = earthRadius * 1000.f;
  const float cloudsRad0 = earthRad + bottom_border * 1000.f;
  const float cloudsRad1 = earthRad + top_border * 1000.f;
  const Point3 earthCenter = Point3(pos.x, -earthRad, pos.z);
  const float viewRad = earthRad + pos.y;
  if (viewRad >= cloudsRad0 && viewRad <= cloudsRad1) // we are within clouds layer
    return true;

  const TMatrix4 viewProjTm = TMatrix4(view_tm) * proj_tm;
  Frustum frustum(viewProjTm);
  Point3_vec4 points[8];
  frustum.generateAllPointFrustm((vec3f *)points);
  if (viewRad > cloudsRad1 + 100000.f) // if we are very high, just check sphere visibility
    return frustum.testSphereB(earthCenter, cloudsRad1);

  // otherwise check if any frustum ray intersects clouds earlier, than it intersects earth
  const float checkRad = viewRad < cloudsRad0 ? cloudsRad0 : cloudsRad1;
  for (int i = 0; i < 4; ++i)
  {
    const Point3 dir = normalize(points[i * 2 + 1] - pos);
    const Point3 p(pos.x, max(pos.y, 10.f), pos.z);

    const float cloudsDist = rayIntersectSphereDist(p, dir, earthCenter, checkRad);
    if (cloudsDist >= 0.f) // we have hit to clouds
    {
      const float earthDist = rayIntersectSphereDist(p, dir, earthCenter, earthRad);
      if (earthDist < 0.f || cloudsDist < earthDist)
        return true;
    }
  }
  return false;
}

void DaSkies::updateCloudsOrigin()
{
  if (clouds)
  {
    // todo: replace with
    // clouds->setCloudsOfs(DPoint2(clouds2.getCloudsOfs().x - windX, clouds2.getCloudsOfs().y - windZ));
    clouds->setCloudsOfs(DPoint2(cloudsOrigin.x - clouds->getCloudsOfs().x, cloudsOrigin.z - clouds->getCloudsOfs().y)); // float
                                                                                                                         // overflow!!!
                                                                                                                         // fixme
    // clouds->setWindAltGradient(-windDirX, -windDirZ.0);
  }
  ShaderGlobal::set_real(strata_pos_xVarId, -cloudsOrigin.strataX);
  ShaderGlobal::set_real(strata_pos_zVarId, -cloudsOrigin.strataZ);
}

void DaSkies::setSkyParams(const SkyAtmosphereParams &params)
{
  if (skyParams != params)
  {
    if (clouds && skyParams.planet_scale != params.planet_scale)
      clouds->invalidateAll();
    skyParams = params;
    skies.setParams(skyParams);
    moon_check_ht = 15000 * skyParams.planet_scale * skyParams.atmosphere_scale;
    nextScatteringGeneration++;
  }
}

float DaSkies::getHazeStrength() const
{
  float timeOffsetSeconds = -skyParams.haze_peak_hour_offset * 60 * 60;
  Point3 earlierSunDir, earlierMoonDir;
  calcSunMoonDir(timeOffsetSeconds, earlierSunDir, earlierMoonDir);

  const float minAngleRad = clamp(skyParams.haze_min_angle * DEG_TO_RAD, 0.0f, float(M_PI / 2));
  const float minSunCos = sinf(minAngleRad);
  const float currentCos = earlierSunDir.y;
  if (currentCos < minSunCos)
    return 0;

  const float sunAngleStrength = max(0.f, currentCos - minSunCos);
  return skyParams.haze_strength * sunAngleStrength;
}

bool DaSkies::currentSunSkyColor(float ht, float &sun_cos, float &moon_cos, Color3 &sun_color, Color3 &sky_color, Color3 &moon_color,
  Color3 &moon_sky_color) const
{
  Color3 sunColor = moonIsPrimary ? secondarySunColor : primarySunColor;
  Color3 moonColor = moonIsPrimary ? primarySunColor : secondarySunColor;
  sun_cos = real_skies_sun_light_dir.y;
  moon_cos = moonDir.y;
  sun_color = getTransmittance(ht, max(0.f, sun_cos)) * sunColor;
  sky_color = getIrradiance(ht, sun_cos) * sunColor / PI;

  moon_color = getTransmittance(ht, max(0.f, moon_cos)) * moonColor;
  moon_sky_color = getIrradiance(ht, moon_cos) * moonColor / PI;
  return true;
}

bool DaSkies::currentGroundSunSkyColor(float &sun_cos, float &moon_cos, Color3 &sun_color, Color3 &sky_color, Color3 &moon_color,
  Color3 &moon_sky_color) const
{
  return currentSunSkyColor(10.0f, sun_cos, moon_cos, sun_color, sky_color, moon_color, moon_sky_color);
}

static inline void set_resolution_var(int, int) {}

bool DaSkies::isPrepareRequired() const { return clouds && clouds->isPrepareRequired(); }

bool DaSkies::isCloudsReady() const { return clouds && clouds->isReady(); }

void DaSkies::prepare(const Point3 &dir_to_sun, bool force_update, float dt)
{
  prepareFrame++;
  real_skies_sun_light_dir = normalize(dpoint3(dir_to_sun));

  if (clouds)
  {
    clouds->setGameParams(cloudsSettings);
    clouds->setWeatherGen(cloudsWeatherGen);
    clouds->setCloudsForm(cloudsForm);
    clouds->setCloudsRendering(cloudsRendering);
    clouds->update(dt);
    if (skies.setStatisticalCloudsInfo(cloudsRendering.ms_contribution, cloudsRendering.ms_attenuation, clouds->minimalStartAlt(),
          clouds->maximumTopAlt(), clouds->getCloudsShadowCoverage(), clouds_sigma_from_extinction(cloudsForm.extinction)))
      nextScatteringGeneration++;
  }

  shrinkRequests();
  prepareSkies(force_update);
  if (skies.updateColors())
    invalidateRequests();

  const float startDisappearSun = -0.07f, endDisappearSun = -0.15f;
  sunEffect = 1 - clamp((startDisappearSun - real_skies_sun_light_dir.y) / (startDisappearSun - endDisappearSun), 0.f, 1.f);
  moonEffect = 1 - clamp((startDisappearSun - moonDir.y) / (startDisappearSun - endDisappearSun), 0.f, 1.f);

  primarySunDir = real_skies_sun_light_dir;
  primarySunEffect = sunEffect;
  primarySunColor = sunEffect * skies.getBaseSunColor();

  secondarySunDir = moonDir;
  secondarySunEffect = moonEffect;
  const float moonAgeEffect = 1.f - fabsf(moonAge * 2.f / 29.530588853f - 1.f); // moon period
  secondarySunColor = moonEffect * (moonAgeEffect * 2.0f / 3.0f + 1.0f / 3.0f) * skies.getBaseMoonColor();

  // should we render moon on sky (as celestial object)
  // const bool additionalMoonOnSky = moonDir.y >= -.1f && secondarySunEffect > 0. &&
  //   brightness(getCpuSunSky(Point3(0,moon_check_ht,0), real_skies_sun_light_dir)) < 0.001f;

  moonIsPrimary = brightness(primarySunColor * getTransmittance(moon_check_ht, primarySunDir.y)) <
                  brightness(secondarySunColor * getTransmittance(moon_check_ht, secondarySunDir.y));
  if (moonIsPrimary)
  {
    eastl::swap(primarySunDir, secondarySunDir);
    eastl::swap(primarySunColor, secondarySunColor);
    eastl::swap(primarySunEffect, secondarySunEffect);
  }
  ShaderGlobal::set_color4(skies_primary_sun_light_dirVarId, primarySunDir.x, primarySunDir.z, primarySunDir.y, 0);
  ShaderGlobal::set_color4(skies_secondary_sun_light_dirVarId, secondarySunDir.x, secondarySunDir.z, secondarySunDir.y, 0);
  ShaderGlobal::set_color4(skies_primary_sun_colorVarId, color4(primarySunColor, primarySunEffect));
  ShaderGlobal::set_color4(skies_secondary_sun_colorVarId, color4(secondarySunColor, secondarySunEffect));

  ShaderGlobal::set_int(skies_have_two_sunsVarId, brightness(secondarySunColor) > 0);

  ShaderGlobal::set_color4(real_skies_sun_light_dirVarId, real_skies_sun_light_dir.x, real_skies_sun_light_dir.z,
    real_skies_sun_light_dir.y, 0);
  ShaderGlobal::set_color4(skies_moon_dirVarId, moonDir.x, moonDir.z, moonDir.y, 0);
  ShaderGlobal::set_color4(real_skies_sun_colorVarId, color4(skies.getBaseSunColor(), 0));

  ShaderGlobal::set_color4(skies_moon_infoVarId, moonEffect, moonAgeEffect, (moonAgeEffect * 2.0f / 3.0f + 1.0f / 3.0f), 0);

  if (clouds)
  {
    if (uint32_t(clouds->prepareLighting(getPrimarySunDir(), getSecondarySunDir())) & CLOUDS_INVALIDATED)
    {
      cloudsGeneration++;
      invalidatePanorama(true);
    }
    ShaderGlobal::set_color4(light_shadow_dirVarId, P3D(getPrimarySunDir()), 0); // we set it always, even if prepareLighting hasn't
                                                                                 // done anything
  }
  ShaderGlobal::set_int(skies_has_cloudsVarId, !!clouds); // todo: only if there are enough clouds in the skies. todo: // gpu readback
                                                          // of filled layers
}

void DaSkies::projectUses2DShadows(bool on)
{
  if (cpuOnly)
    return;
  ShaderGlobal::set_int(skies_use_2d_shadowsVarId, 1);
  if (clouds)
    clouds->setUseShadows2D(on);
}

void DaSkies::setStrataClouds(const StrataClouds &a)
{
  if (strataCloudsParams == a)
    return;
  strataCloudsParams = a;
  cloudsGeneration++;
}

void DaSkies::setStrataCloudsTexture(const char *tex_name)
{
  if (!tex_name)
    return;

  strataClouds.close();
  strataClouds = dag::get_tex_gameres(tex_name, "strata_clouds");
  if (strataClouds)
    strataClouds->disableSampler();
  prefetch_managed_texture(strataClouds.getTexId());
  strataClouds.setVar();
  ShaderGlobal::set_sampler(::get_shader_variable_id("strata_clouds_samplerstate"), d3d::request_sampler({}));
}

void DaSkies::renderStrataClouds(const Point3 &origin, const TMatrix &view_tm, const TMatrix4 &proj_tm, bool panorama)
{
  if (!strataClouds || !shouldRenderStrata)
    return;
  ShaderGlobal::set_real(strata_clouds_effectVarId, strataCloudsParams.amount);
  if (strataCloudsParams.amount < 0.0001f || origin.y >= strataCloudsParams.altitude * 1000)
    return;
  ShaderGlobal::set_real(strata_clouds_altitudeVarId, strataCloudsParams.altitude);
  strataClouds.setVar();
  if (panorama)
    return;

  TIME_D3D_PROFILE(strata_clouds);
  if (clouds_frustum_check(origin, view_tm, proj_tm, skies.getEarthRadius(), strataCloudsParams.altitude - 0.01f,
        strataCloudsParams.altitude + 0.001f))
    strata.render();
}

void DaSkies::destroy_skies_data(SkiesData *&data) { del_it(data); }

IPoint2 DaSkies::getCloudsResolution(SkiesData *data)
{
  if (clouds)
    return clouds->getResolution(data->clouds);
  else
    return IPoint2(0, 0);
}

void DaSkies::skiesDataScatteringVolumeBarriers(SkiesData *data)
{
  d3d::resource_barrier({data->baseSkies->scatteringVolume[0].getVolTex(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
  d3d::resource_barrier({data->baseSkies->scatteringVolume[1].getVolTex(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
}

void DaSkies::changeSkiesData(int sky_detail_level, int clouds_detail_level, bool fly_through_clouds, int targetW, int targetH,
  SkiesData *data, CloudsResolution clouds_resolution, bool use_blurred_clouds)
{
  if (cpuOnly)
    return;
  G_ASSERT(data);
  if (!data)
    return;
  data->autoDetect = false;
  if (panoramaEnabled())
  {
    data->lowresSkies.close();
    data->clouds.close();
    data->skyDetailLevel = 0;
    data->cloudsDetailLevel = 0;
    return;
  }

  if (clouds_resolution == CloudsResolution::ForceFullresClouds)
    clouds_detail_level = 0; // just force fullres no matter what

  sky_detail_level = clamp(sky_detail_level, 0, 3);
  clouds_detail_level = clamp((int)clouds_detail_level, 0, 4);
  if (data->tw == targetW && data->th == targetH && data->flyThrough == fly_through_clouds &&
      data->skyDetailLevel == sky_detail_level && data->cloudsDetailLevel == clouds_detail_level)
    return;

  data->tw = targetW;
  data->th = targetH;
  data->flyThrough = fly_through_clouds;
  data->skyDetailLevel = sky_detail_level;

  if (data->lowresSkies)
  {
    TextureInfo tinfo;
    data->lowresSkies->getinfo(tinfo);
    if ((targetW >> data->skyDetailLevel) != tinfo.w || (targetH >> data->skyDetailLevel) != tinfo.h)
      data->lowresSkies.close();
  }

  if (data->skyDetailLevel > 0 && targetW > 0 && !data->lowresSkies)
  {
    int skyW = (targetW >> data->skyDetailLevel), skyH = (targetH >> data->skyDetailLevel);
    String name(128, "%s_lowres_sky", data->base_name);
    if (d3d::get_driver_code().is(d3d::dx11))
      d3d::driver_command(Drv3dCommand::D3D_FLUSH);
    data->lowresSkies = dag::create_tex(NULL, skyW, skyH, TEXCF_RTARGET | noAlphaSkyHdrFmt, 1, name);
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      ShaderGlobal::set_sampler(::get_shader_variable_id("lowres_sky_samplerstate"), d3d::request_sampler(smpInfo));
    }
    if (data->lowresSkies)
      data->lowresSkies->disableSampler();
  }

  data->cloudsDetailLevel = clouds_detail_level;
  // clouds_detail_level: 3, 4 -- mobile low quality
  const int divider = clouds_detail_level >= 3 ? clouds_detail_level : (1 << clouds_detail_level);
  // have to keep legacy behav because of config files
  if (clouds_detail_level > 0 || clouds_resolution == CloudsResolution::ForceFullresClouds)
    data->clouds.init(targetW / divider, targetH / divider, data->base_name, fly_through_clouds, clouds_resolution,
      use_blurred_clouds);
  else
    data->clouds.close();
}

SkiesData *DaSkies::createSkiesData(const char *base_name, const PreparedSkiesParams &params, bool render_stars)
{
  if (params.panoramic == PreparedSkiesParams::Panoramic::OFF)
  {
    // thickness can affect volumetric clouds worse than panoramic, hiding wrong params, so we check validity here:
    float minThickness = getMinCloudThickness();
    const CloudsForm &form = getCloudsForm();
    for (int layerId = 0; layerId < 2; ++layerId)
      if (form.layers[layerId].thickness < minThickness)
        logerr("Cloud layer thickness must be at least %f. It was set to %f on layer %d!", minThickness,
          form.layers[layerId].thickness, layerId);
  }

  if (cpuOnly)
    return NULL;

  SkiesData *data = new SkiesData(base_name, params);
  data->shouldRenderStars &= render_stars;
  return data;
}

// cloudsDepth is supposed to be /2 of targetDepth
// fixme: verify that!
void DaSkies::prepareRenderClouds(bool infinite, const Point3 &origin, const TextureIDPair &cloudsDepth,
  const TextureIDPair &prevCloudsDepth, SkiesData *data, const TMatrix &view_tm, const TMatrix4 &proj_tm)
{
  if (cpuOnly)
    return;
  if (!clouds)
    return;
  data->cloudsVisible = clouds->hasVisibleClouds() && clouds_frustum_check(origin, view_tm, proj_tm, skies.getEarthRadius(),
                                                        clouds->effectiveStartAlt(), clouds->effectiveTopAlt());
  if (data->cloudsVisible)
    clouds->renderClouds(data->clouds, infinite ? TextureIDPair() : cloudsDepth, infinite ? TextureIDPair() : prevCloudsDepth, view_tm,
      proj_tm);
}

void DaSkies::renderCloudsToTarget(bool infinite, const Point3 &origin, const TextureIDPair &downsampledDepth, TEXTUREID targetDepth,
  const Point4 &targetDepthTransform, SkiesData *data, const TMatrix &view_tm, const TMatrix4 &proj_tm)
{
  if (cpuOnly)
    return;
  if (!clouds)
    return;

  if (data->cloudsVisible)
  {
    ShaderGlobal::set_color4(skies_world_view_posVarId, origin.x, origin.y, origin.z, 0);
    clouds->renderCloudsScreen(data->clouds, infinite ? TextureIDPair() : downsampledDepth, infinite ? BAD_TEXTUREID : targetDepth,
      targetDepthTransform, view_tm, proj_tm);
  }
}

void DaSkies::prepareSkies(bool invalidate_cpu)
{
  if (cpuOnly)
    return;

  if (computedScatteringGeneration != nextScatteringGeneration)
  {
    if (clouds)
      clouds->setCloudsShadowCoverage();
    skies.setParams(skyParams);
    skies.precompute(invalidate_cpu); // todo: invalidate_cpu should be always true if we have invalidated skies
    computedScatteringGeneration = nextScatteringGeneration;
    invalidateRequests();
  }
}

void DaSkies::initCloudsRender(bool useHole)
{
  if (cpuOnly)
    return;
  if (!clouds)
    clouds = new Clouds2;
  clouds->init(useHole);
  strataClouds.close();
  strataClouds = dag::get_tex_gameres("strata_clouds_simple", "strata_clouds");
  if (strataClouds)
    strataClouds->disableSampler();
  prefetch_managed_texture(strataClouds.getTexId());
  strataClouds.setVar();
  ShaderGlobal::set_sampler(::get_shader_variable_id("strata_clouds_samplerstate"), d3d::request_sampler({}));

  //==ddsx::tex_pack2_perform_delayed_data_loading();

  skiesApply.init("applySkies");
  strata.init("strata_clouds");
}

void DaSkies::setNearCloudsRenderingEnabled(bool enabled)
{
  if (clouds)
  {
    clouds->setRenderingEnabled(enabled);
  }
}

void DaSkies::renderCloudVolume(VolTexture *cloudVolume, float max_dist, const TMatrix &view_tm, const TMatrix4 &proj_tm)
{
  if (cpuOnly || !clouds || !cloudVolume)
    return;
  clouds->renderCloudVolume(cloudVolume, max_dist, view_tm, proj_tm);
}

DaSkies::DaSkies(bool cpu_only) : cpuOnly(cpu_only), panoramaData(NULL), panoramaValid(PANORAMA_INVALID), shouldRenderStrata(true)
{
  primarySunColor = skies.getBaseSunColor();
  starsJulianDay = 2457388.5; // 2016-01-01:00
  starsLatitude = 55;
  starsLongtitude = 37;
  skyStars = NULL;
  moonDir = normalize(Point3(1, 1, 1));
  moonAge = 10;
  sunEffect = moonEffect = 0;
  cloudsOrigin = CloudsOrigin();
  nextScatteringGeneration = 1;
  real_skies_sun_light_dir = primarySunDir = normalize(Point3(0.1, 0.9, 0));

  initialAzimuthAngle = 0.0f;
}

DaSkies::~DaSkies() { closeSky(); }

void DaSkies::closeSky()
{
  closePanorama();
  skies.close();

  del_it(skyStars);
  del_it(clouds);

  strataClouds.close();
}

void DaSkies::initSky(int, uint32_t skyfmt, bool useHole)
{
#define VAR(a, opt) a##VarId = get_shader_variable_id(#a, cpuOnly || opt);
  GLOBAL_VARS_LIST
#undef VAR

  // const GpuUserConfig &gpuCfg = d3d_get_gpu_cfg();
  panoramaData = NULL;
  sunEffect = moonEffect = 0;
  cloudsOrigin = CloudsOrigin();
  nextScatteringGeneration = 1;
  panoramaFrame = 0;
  if (cpuOnly)
  {
    skies.setCPUConsts();
    return;
  }

  noAlphaHdrFmt = TEXFMT_R11G11B10F;

  noAlphaSkyHdrFmt = skyfmt == 0xFFFFFFFF ? noAlphaHdrFmt : skyfmt;
  if ((d3d::get_texformat_usage(noAlphaSkyHdrFmt) & (d3d::USAGE_RTARGET | d3d::USAGE_FILTER)) !=
      (d3d::USAGE_RTARGET | d3d::USAGE_FILTER))
    noAlphaSkyHdrFmt = TEXCF_SRGBREAD | TEXCF_SRGBWRITE;

  if (!(d3d::get_texformat_usage(noAlphaHdrFmt) & d3d::USAGE_RTARGET))
    noAlphaHdrFmt = TEXFMT_A16B16G16R16F;
  skies.init();
  initCloudsRender(useHole);
  skyStars = new DaStars;
  skyStars->init("stars", "moon");
  initCloudsTracer();
  initRainQuery();
}


void DaSkies::useFog(const Point3 &origin, SkiesData *data, const TMatrix &view_tm, const TMatrix4 &proj_tm, bool update_sky,
  float altitude_tolerance)
{
  G_ASSERT(data);
  if (!data)
    return;

  if (update_sky)
  {
    skies.prepareSkyForAltitude(origin, altitude_tolerance, computedScatteringGeneration, data->baseSkies, view_tm, proj_tm,
      prepareFrame, panoramaEnabled() ? panoramaData : nullptr, getPrimarySunDir(), getSecondarySunDir());
    data->preparedOrigin = origin;
  }
  use_prepared_skies(data->baseSkies, panoramaEnabled() ? panoramaData : nullptr);
}

Color3 DaSkies::calculateFogLoss(const Point3 &cameraPosition, const Point3 &worldPos) const
{
  Point3 view = worldPos - cameraPosition;
  float len = length(view);
  view /= max(1e-6f, len);
  view.y = clamp(view.y, -1.f, 1.f); // It is used as a mu and is sensitive to floating point inaccuracy.
  return skies.getCpuFogLoss(cameraPosition, view, len);
}

Color3 DaSkies::getMieColor(const Point3 &cameraPosition, const Point3 &viewdir, float d) const
{
  return skies.getCpuFogSingleMieInscatter(cameraPosition, viewdir, d, getPrimarySunDir(), 16) * primarySunColor;
}

void DaSkies::getCpuFogSingle(const Point3 &cameraPosition, const Point3 &viewdir, float d, Color3 &insc, Color3 &loss) const
{
  insc = skies.getCpuFogSingleInscatter(cameraPosition, viewdir, d, getPrimarySunDir(), 16) * primarySunColor;
  loss = skies.getCpuFogLoss(cameraPosition, viewdir, d);
}

Color3 DaSkies::getCpuSun(const Point3 &origin, const Point3 &lightDir) const
{
  return getTransmittance(origin.y, max(lightDir.y, 0.f)) * skies.getBaseSunColor(); //*sunEffect;
}

Color3 DaSkies::getCpuMoon(const Point3 &origin, const Point3 &lightDir) const
{
  return getTransmittance(origin.y, max(lightDir.y, 0.f)) * skies.getBaseMoonColor(); //*moonEffect;
}

Color3 DaSkies::getCpuSunSky(const Point3 &origin, const Point3 &sunDir, int, int) const
{
  return getIrradiance(origin.y, sunDir.y) * skies.getBaseSunColor() / PI;
}
Color3 DaSkies::getCpuMoonSky(const Point3 &origin, const Point3 &moonLight, int, int) const
{
  return getIrradiance(origin.y, moonLight.y) * skies.getBaseMoonColor() / PI;
}
Color3 DaSkies::getCpuIrradiance(const Point3 &origin, const Point3 &lightDir, int, int) const
{
  return getIrradiance(origin.y, lightDir.y) / PI;
}

bool DaSkies::isCloudsVisible(SkiesData *data) { return data->cloudsVisible; }

void DaSkies::cloudsLayersHeightsBarrier() { clouds->layersHeightsBarrier(); }

void DaSkies::prepareSkyAndClouds(bool infinite, const DPoint3 &origin, const DPoint3 &dir, uint32_t render_sun_moon,
  const ManagedTex &cloudsDepth, const ManagedTex &prevCloudsDepth, SkiesData *data, const TMatrix &view_tm, const TMatrix4 &proj_tm,
  bool update_sky, bool fixed_offset, float altitude_tolerance, AuroraBorealis *aurora)
{
  prepareSkyAndClouds(infinite, origin, dir, render_sun_moon, TextureIDPair(cloudsDepth.getTex2D(), cloudsDepth.getTexId()),
    TextureIDPair(prevCloudsDepth.getTex2D(), prevCloudsDepth.getTexId()), data, view_tm, proj_tm, update_sky, fixed_offset,
    altitude_tolerance, aurora);
}

void DaSkies::prepareSkyAndClouds(bool infinite, const DPoint3 &origin, const DPoint3 &dir, uint32_t render_sun_moon,
  const TextureIDPair &cloudsDepth, const TextureIDPair &prevCloudsDepth, SkiesData *data, const TMatrix &view_tm,
  const TMatrix4 &proj_tm, bool update_sky, bool fixed_offset, float altitude_tolerance, AuroraBorealis *aurora)
{
  if (!data || cpuOnly)
    return;

  /*frustum_check(
    origin,
    getCloudsTopBorder(),
    getCloudsBottomBorder(),
    strataCloudsParams.altitude,
    data->shouldRenderClouds,
    shouldRenderStrata );*/
  // if (data->autoDetect && data->level )
  // void DaSkies::changeSkiesData(int sky_level, uint32_t level, bool fly_through_clouds, int targetW, int targetH, SkiesData *data)

  TIME_D3D_PROFILE(prepareSky)
  ShaderGlobal::set_color4(skies_world_view_posVarId, origin.x, origin.y, origin.z, 0);
  useFog(origin, data, view_tm, proj_tm, update_sky, altitude_tolerance);
  set_viewvecs_to_shader(view_tm, proj_tm);
  if (panoramaEnabled())
  {
    if ((update_sky || panoramaValid == PANORAMA_INVALID) && updatePanorama(origin, view_tm, proj_tm))
      useFog(origin, data, view_tm, proj_tm, false);
    clouds->setCloudsOffsetVars(data->clouds);
    return;
  }
  if (update_sky)
  {
    data->renderSunMoon = render_sun_moon;
    if (data->lowresSkies)
    {
      SCOPE_RENDER_TARGET;
      ShaderGlobal::set_real(render_sunVarId, (data->renderSunMoon & 1) ? 1 : 0);

      // #if 0
      {
        TIME_D3D_PROFILE(render_sky);
        d3d::set_render_target(data->lowresSkies.getTex2D(), 0);
        d3d::clearview(CLEAR_DISCARD_TARGET, 0, 0, 0);
        skies.renderSky();

        if (aurora)
        {
          aurora->beforeRender();
          aurora->render();
        }
      }

      renderStrataClouds(origin, view_tm, proj_tm, false);

      d3d::resource_barrier({data->lowresSkies.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    }
    if (!fixed_offset)
      data->clouds.update(dir, origin);
    prepareRenderClouds(infinite, origin, cloudsDepth, prevCloudsDepth, data, view_tm, proj_tm);
  }
}

// if panorama is enabled we use just it for all types of render
//  for alpha of clouds there is alpha_panorama
//  it used to work differently (i.e. rendering water or cube always used explicit data)
//  however, it is slower and less consistent with what you see (reflections can differ from environment).
//  so, we don't do that any more
//  if ever needed to be restored, add && panoramaData == data to expression

void DaSkies::renderSky(SkiesData *data, const TMatrix &view_tm, const TMatrix4 &proj_tm, const Driver3dPerspective &persp,
  bool render_prepared_lowres, AuroraBorealis *aurora)
{
  if (cpuOnly)
    return;
  TIME_D3D_PROFILE(applySky)
  G_ASSERT(data);
  if (!data)
    return;

  const Point3 &origin = data->preparedOrigin;
  ShaderGlobal::set_color4(skies_world_view_posVarId, origin.x, origin.y, origin.z, 0);
  useFog(origin, data, view_tm, proj_tm, false);
  set_viewvecs_to_shader(view_tm, proj_tm);

  bool strataCloudsOverMoon = false;
  ShaderGlobal::set_real(render_sunVarId, (data->renderSunMoon & 1) ? 1 : 0);
  if (panoramaEnabled())
  {
    G_ASSERT(panoramaValid != PANORAMA_INVALID);
    renderPanorama(origin, view_tm, proj_tm, persp);
  }
  else if (data->lowresSkies && render_prepared_lowres)
  {
    ShaderGlobal::set_texture(lowres_skyVarId, data->lowresSkies);
    skiesApply.render();
  }
  else
  {
    skies.renderSky(); //??
    strataCloudsOverMoon = true;
  }

  useFog(origin, data, view_tm, proj_tm, false);
  if (canRenderStars())
  {
    const float sunHighBrightness = brightness(getCpuSunSky(Point3(0, moon_check_ht, 0), real_skies_sun_light_dir));
    if (data->shouldRenderStars)
    {
      float minInscatter;
      {
        TIME_PROFILE(star_rendering_check);
        static constexpr int STEP_CNT = 8; // 16 is safe, but 8 seems to be enough (and 2 times faster)
        Point3 cameraPos = inverse(view_tm).getcol(3);
        minInscatter =
          skies.getCpuFogSingleInscatter(cameraPos, Point3(0, 1, 0), 100000, getPrimarySunDir(), STEP_CNT).b * primarySunColor.b;
      }
      if (minInscatter < 1.0f / STAR_RENDERING_INSCATTER_THRESHOLD_INV)
        skyStars->renderStars(persp, sunHighBrightness, starsLatitude, starsLongtitude, starsJulianDay, initialAzimuthAngle);
    }
    if (data->renderSunMoon & 2)
      skyStars->renderMoon(origin, moonDir, moonAge, sunHighBrightness);
    if (aurora)
    {
      aurora->beforeRender();
      aurora->render();
    }
  }

  if (strataCloudsOverMoon)
    renderStrataClouds(origin, view_tm, proj_tm, false);
}

void DaSkies::renderStars(const Driver3dPerspective &persp, const Point3 &origin, float stars_intensity_mul)
{
  G_UNREFERENCED(origin);
  if (canRenderStars())
    skyStars->renderStars(persp, 0, starsLatitude, starsLongtitude, starsJulianDay, initialAzimuthAngle, stars_intensity_mul);
}

void DaSkies::renderClouds(bool infinite, const ManagedTex &cloudsDepth, TEXTUREID targetDepth, SkiesData *data,
  const TMatrix &view_tm, const TMatrix4 &proj_tm, const Point4 &targetDepthTransform)
{
  renderClouds(infinite, TextureIDPair{cloudsDepth.getTex2D(), cloudsDepth.getTexId()}, targetDepth, data, view_tm, proj_tm,
    targetDepthTransform);
}

void DaSkies::renderClouds(bool infinite, const TextureIDPair &downsampledDepth, TEXTUREID targetDepth, SkiesData *data,
  const TMatrix &view_tm, const TMatrix4 &proj_tm, const Point4 &targetDepthTransform)
{
  if (cpuOnly || panoramaEnabled() || !data || !clouds)
    return;
  if (!data->cloudsVisible)
    return;
  TIME_D3D_PROFILE(applyClouds)
  set_viewvecs_to_shader(view_tm, proj_tm);

  const Point3 origin = data->preparedOrigin;
  ShaderGlobal::set_color4(skies_world_view_posVarId, origin.x, origin.y, origin.z, 0);
  useFog(origin, data, view_tm, proj_tm, false);
  renderCloudsToTarget(infinite, origin, downsampledDepth, targetDepth, targetDepthTransform, data, view_tm, proj_tm);
}

void DaSkies::renderEnvi(bool infinite, const DPoint3 &origin, const DPoint3 &dir, uint32_t render_sun_moon,
  const ManagedTex &cloudsDepth, const ManagedTex &prevCloudsDepth, TEXTUREID targetDepth, SkiesData *data, const TMatrix &view_tm,
  const TMatrix4 &proj_tm, const Driver3dPerspective &persp, bool update_sky, bool fixed_offset, float altitude_tolerance,
  AuroraBorealis *aurora)
{
  if (cpuOnly)
    return;
  G_ASSERT(data);
  TIME_D3D_PROFILE(envi)
  prepareSkyAndClouds(infinite, origin, dir, render_sun_moon, TextureIDPair(cloudsDepth.getTex2D(), cloudsDepth.getTexId()),
    TextureIDPair(prevCloudsDepth.getTex2D(), prevCloudsDepth.getTexId()), data, view_tm, proj_tm, update_sky, fixed_offset,
    altitude_tolerance, aurora);
  renderSky(data, view_tm, proj_tm, persp, true, aurora);
  renderClouds(infinite, TextureIDPair(cloudsDepth.getTex2D(), cloudsDepth.getTexId()), targetDepth, data, view_tm, proj_tm);
}

void DaSkies::reset()
{
  invalidate();
  if (clouds)
    clouds->reset();
  if (skyStars)
    skyStars->afterReset();
  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
  prepare(getSunDir(), false, 0.f);
}

void DaSkies::invalidate()
{
  nextScatteringGeneration++;
  invalidatePanorama(true);
}

/*void DaSkies::setSkiesRenderData(const Point3 &world_view_pos, const Point3 &sun_light_dir, const Color3 &sun_light_color)
{
  ShaderGlobal::set_color4(skies_world_view_posVarId, Color4::xyz1(world_view_pos));
  ShaderGlobal::set_color4(skies_sun_light_dirVarId, Color4::xyz0(sun_light_dir));
  ShaderGlobal::set_color4(skies_sun_colorVarId, color4(sun_light_color, 1.0f));
}*/

void DaSkies::renderCelestialObject(TEXTUREID tid, const Point3 &dir, float phase, float intensity, float size)
{
  if (skyStars)
    skyStars->renderCelestialObject(tid, dir, phase, intensity, size);
}

void DaSkies::renderCelestialObject(const Point3 &dir, float phase, float intensity, float size)
{
  if (skyStars)
    renderCelestialObject(skyStars->getMoonTexId(), dir, phase, intensity, size);
}

void DaSkies::shrinkRequests()
{
  // instead of making any LRU logic, we just re-populate cache if too much requests were made. the cost of query isn't very big
  // anyway, the main purpose is to hide in-frame calls
  if (transmittanceRequests.size() > 1024)
    transmittanceRequests.clear();
  if (irradianceRequests.size() > 1024)
    irradianceRequests.clear();
}
void DaSkies::invalidateRequests()
{
  transmittanceRequests.clear();
  irradianceRequests.clear();
}
Color3 DaSkies::getTransmittance(float ht, float mu) const
{
  Request rq = {mu, ht};
  auto it = transmittanceRequests.find(rq);
  if (it != transmittanceRequests.end())
    return it->second;
  return (transmittanceRequests[rq] = skies.getCpuTransmittance(ht, mu));
}

Color3 DaSkies::getIrradiance(float ht, float mu) const
{
  Request rq = {mu, ht};
  auto it = irradianceRequests.find(rq);
  if (it != irradianceRequests.end())
    return it->second;
  return (irradianceRequests[rq] = skies.getCpuIrradiance(ht, mu));
}
void DaSkies::setWindDirection(const Point2 &windDir)
{
  if (clouds)
    clouds->setErosionNoiseWindDir(windDir);
}

float DaSkies::getCloudsStartAlt() { return clouds ? clouds->effectiveStartAlt() * 1000.f : -1.f; }

float DaSkies::getCloudsTopAlt() { return clouds ? clouds->effectiveTopAlt() * 1000.f : -1.f; }

float DaSkies::getCloudsStartAltSettings() const
{
  return clouds ? clouds->minimalStartAlt() * 1000.f : min(cloudsForm.layers[0].startAt, cloudsForm.layers[1].startAt) * 1000.f;
}

float DaSkies::getCloudsTopAltSettings() const { return clouds ? clouds->maximumTopAlt() * 1000.f : -1.f; }

void DaSkies::resetCloudsHole(const Point3 &hole_target, const float &hole_density)
{
  if (clouds)
    return clouds->resetHole(hole_target, hole_density);
}

void DaSkies::resetCloudsHole()
{
  if (clouds)
    return clouds->resetHole();
}

void DaSkies::setUseCloudsHole(bool set)
{
  if (clouds)
    clouds->setUseHole(set);
}

bool DaSkies::getUseCloudsHole() const { return clouds ? clouds->getUseHole() : false; }

Point2 DaSkies::getCloudsHolePosition() const
{
  if (!clouds)
    return Point2(0, 0);
  return clouds->getCloudsHole();
}

void DaSkies::setCloudsHolePosition(const Point2 &p)
{
  if (!clouds)
    return;
  return clouds->setHole(p);
}

void DaSkies::setExternalWeatherTexture(TEXTUREID tid)
{
  if (clouds)
    clouds->setExternalWeatherTexture(tid);
}

// todo: implement on client
bool DaSkies::traceRayClouds(const Point3 &, const Point3 &, float &, const int) const { return false; }
// todo: remove me
float DaSkies::getCloudsDensityFiltered(const Point3 &) const { return 0; }
float DaSkies::getCloudsDensityFilteredPrecise(const Point3 &) const { return 0; }


void DaSkies::initCloudsTracer()
{
  if (traceCloudsCs || traceCloudsPs.getElem())
    return;

  traceRaysCountVarId = get_shader_variable_id("trace_rays_count");
  if (d3d::get_driver_desc().shaderModel >= 5.0_sm && !d3d::get_driver_desc().issues.hasComputeTimeLimited)
  {
    traceCloudsCs.reset(new_compute_shader("trace_clouds_cs", true));
    cloudsTraceResultRingBuffer.init(sizeof(uint32_t), numCloudsTracesPerFrame, 3, "clouds_trace_result", SBCF_UA_STRUCTURED_READBACK,
      0, false);
  }
  else
  {
    traceCloudsPs.init("trace_clouds_ps");
    cloudsTraceResultRingBuffer.init(numCloudsTracesPerFramePs, 1, 3, "clouds_trace_result", 0,
      TEXCF_RTARGET | TEXFMT_G32R32F | TEXCF_LINEAR_LAYOUT, true);
  }
}

int DaSkies::addCloudsTrace(const Point3 &pos, const Point3 &dir, float dist, float threshold, float scattering_threshold)
{
  int traceId = currentCloudsTraceId % cloudsTraceHistorySize;
  CloudsTrace &data = cloudsTraces[traceId];
  if (data.state != CloudsTrace::State::FREE && data.state != CloudsTrace::State::FINISHED)
  {
    logwarn("CloudsTrace overflow");
    return -1;
  }

  data.state = CloudsTrace::State::ADDED;
  data.inPos = pos;
  data.inDir = dir;
  data.inDist = dist;
  data.inThreshold = threshold;
  data.inScatteringThreshold = scattering_threshold;

  currentCloudsTraceId++;

  return traceId;
}

void DaSkies::dispatchCloudsTraces()
{
  if (currentCloudsTraceDispatchId < currentCloudsTraceId)
  {
    int frame;
    D3dResource *resultBuffer = cloudsTraceResultRingBuffer.getNewTarget(frame);

    if (resultBuffer)
    {
      uint64_t startTraceId = currentCloudsTraceDispatchId;
      Point4 dispatchData[numCloudsTracesPerFrame * 2];
      int dispatchSize = 0;
      int maxDispatchSize = traceCloudsCs ? numCloudsTracesPerFrame : numCloudsTracesPerFramePs;
      for (; currentCloudsTraceDispatchId < currentCloudsTraceId && dispatchSize < maxDispatchSize;
           currentCloudsTraceDispatchId++, dispatchSize++)
      {
        CloudsTrace &data = cloudsTraces[currentCloudsTraceDispatchId % cloudsTraceHistorySize];
        dispatchData[dispatchSize * 2] = Point4(data.inPos.x, data.inPos.y, data.inPos.z, data.inThreshold);
        dispatchData[dispatchSize * 2 + 1] = Point4(data.inDir.x, data.inDir.y, data.inDir.z, data.inDist);
        data.state = CloudsTrace::State::DISPATCHED;
      }

      ShaderGlobal::set_int(traceRaysCountVarId, dispatchSize);
      if (traceCloudsCs)
      {
        int numThreadGroups = (dispatchSize - 1) / numCloudsTracesPerGroup + 1;
        if (numThreadGroups > 1)
          d3d::set_cs_constbuffer_size(8 + numThreadGroups * numCloudsTracesPerGroup * 2);
        d3d::set_cs_const(8, &dispatchData[0], dispatchSize * 2);
        STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), (Sbuffer *)resultBuffer);
        traceCloudsCs->dispatch(numThreadGroups, 1, 1);
        if (numThreadGroups > 1)
          d3d::set_cs_constbuffer_size(0);
      }
      else
      {
        SCOPE_RENDER_TARGET;
        SCOPE_RESET_SHADER_BLOCKS;
        ScopeResetShaderBlocks savedBlocks;
        d3d::set_render_target((Texture *)resultBuffer, 0);
        d3d::set_ps_const(15, &dispatchData[0], dispatchSize * 2);
        traceCloudsPs.render();
      }
      cloudsTraceResultRingBuffer.startCPUCopy();
      cloudsTracesInFlight.push_back({frame, startTraceId, dispatchSize, ::dagor_frame_no()});
    }
  }

  int stride;
  uint32_t frame;
  while (uint32_t *bufData = reinterpret_cast<uint32_t *>(cloudsTraceResultRingBuffer.lock(stride, frame, false)))
  {
    for (const CloudsTracesInFlight &dispatch : cloudsTracesInFlight)
    {
      if (dispatch.frameNo == frame)
      {
        for (int resultNo = 0; resultNo < dispatch.dispatchSize; resultNo++)
        {
          CloudsTrace &data = cloudsTraces[(dispatch.startTraceId + resultNo) % cloudsTraceHistorySize];
          data.state = CloudsTrace::State::FINISHED;
          if (traceCloudsCs)
          {
            data.outT = (bufData[resultNo] & 0xFFFF) == 0xFFFF ? -1.f : (bufData[resultNo] & 0xFFFF) / 65534.f;
            data.outTransmittance = (bufData[resultNo] >> 16 & 0xFFFF) / 65535.f;
          }
          else
          {
            data.outT = bitwise_cast<float>(bufData[resultNo * 2]);
            data.outTransmittance = bitwise_cast<float>(bufData[resultNo * 2 + 1]);
          }
        }
        cloudsTracesInFlight.erase(cloudsTracesInFlight.begin(), &dispatch + 1);
        break;
      }
    }
    cloudsTraceResultRingBuffer.unlock();
  }

  const int maxWaitFrames = 10;
  for (const auto &dispatch : cloudsTracesInFlight)
  {
    if (dispatch.dispatchedAtFrame + maxWaitFrames < ::dagor_frame_no())
    {
      for (int resultNo = 0; resultNo < dispatch.dispatchSize; resultNo++)
      {
        CloudsTrace &data = cloudsTraces[(dispatch.startTraceId + resultNo) % cloudsTraceHistorySize];
        data.state = CloudsTrace::State::FINISHED;
        data.outT = -1.f;
        data.outTransmittance = 0.f;
      }
      cloudsTracesInFlight.erase(cloudsTracesInFlight.begin(), &dispatch + 1);
      break;
    }
  }
}

bool DaSkies::getCloudsTraceResult(int trace_id, float &out_t, float &out_transmittance)
{
  if (trace_id == -1)
    return false;

  CloudsTrace &data = cloudsTraces[trace_id % cloudsTraceHistorySize];
  if (data.state == CloudsTrace::State::FINISHED)
  {
    out_t = data.outT;
    out_transmittance = data.outTransmittance;
    if (data.inScatteringThreshold > 0.f)
    {
      float scatteringT = data.outT > 0.f ? data.outT : 1.f;
      float transmittance = brightness(calculateFogLoss(data.inPos, data.inPos + scatteringT * data.inDist * data.inDir));
      if (transmittance < data.inScatteringThreshold)
        out_t = transmittance > 0.f ? scatteringT * log(data.inScatteringThreshold) / log(transmittance) : 0.f; // Simple exponential
                                                                                                                // approximation, good
                                                                                                                // enough for the fact
                                                                                                                // of collision.
      out_transmittance *= transmittance;
    }
    return true;
  }

  return false;
}

void DaSkies::initRainQuery()
{
  if (rainQueryCs || d3d::get_driver_desc().shaderModel < 5.0_sm)
    return;

  rainQueryCs.reset(new_compute_shader("query_rainmap_cs", true));
  traceRaysCountVarId = get_shader_variable_id("trace_rays_count");

  rainQueryResultRingBuffer.init(sizeof(float), 1, 3, "rain_query_result", SBCF_UA_STRUCTURED_READBACK, 0, false);
}

float DaSkies::getRainAtPos(const Point3 &pos)
{
  if (!rainQueryCs)
    return 0.f;

  int frame;
  Sbuffer *resultBuffer = (Sbuffer *)rainQueryResultRingBuffer.getNewTarget(frame);
  if (resultBuffer)
  {
    ShaderGlobal::set_int(traceRaysCountVarId, 1);
    Point4 pos4(pos.x, pos.y, pos.z, 0.f);
    d3d::set_cs_const(8, &pos4, 1);
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), resultBuffer);
    rainQueryCs->dispatch();
    rainQueryResultRingBuffer.startCPUCopy();
  }

  int stride;
  uint32_t uframe;
  float *data = reinterpret_cast<float *>(rainQueryResultRingBuffer.lock(stride, uframe, true));
  if (data)
  {
    lastRainQueryResult = data[0];
    rainQueryResultRingBuffer.unlock();
  }

  return lastRainQueryResult;
}

bool DaSkies::setMoonResourceName(const char *res_name) { return skyStars ? skyStars->setMoonResourceName(res_name) : false; }

void DaSkies::setMoonSize(float size)
{
  if (skyStars)
    skyStars->setMoonSize(size);
}

void DaSkies::getCloudsTextureResourceDependencies(Tab<TEXTUREID> &dependencies) const
{
  if (strataClouds.getTexId() != BAD_TEXTUREID)
  {
    dependencies.push_back(strataClouds.getTexId());
  }
  if (clouds)
  {
    clouds->getTextureResourceDependencies(dependencies);
  }
}
