// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "clouds2.h"
#include "cloudsShaderVars.h"

#include <math/dag_mathBase.h>
#include "shaders/clouds2/clouds_droplet_phase.hlsli"

#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_lock.h>
#include <3d/dag_lockTexture.h>
#include <render/noiseTex.h>

#include <osApiWrappers/dag_miscApi.h>

bool Clouds2::hasVisibleClouds() const // GPU readback!
{
  if (weatherParams.layers[0].coverage > 0 || weatherParams.layers[1].coverage > 0 || weatherParams.cumulonimbusCoverage > 0 ||
      weatherParams.epicness > 0)
  {
    float altStart, altTop;
    return field.getReadbackData(altStart, altTop) ? altStart <= altTop : true;
  }
  return false;
}

void Clouds2::updateErosionNoiseWind(float dt, const Point2 &wind_dir)
{
  erosionWindChange = wind_dir * (renderingParams.erosionWindSpeed * dt);
  clouds_erosion_noise_wind_ofs += erosionWindChange;
  float erosionNoiseSize = renderingParams.erosion_noise_size * 32.f;
  clouds_erosion_noise_wind_ofs =
    clouds_erosion_noise_wind_ofs - floor(clouds_erosion_noise_wind_ofs / erosionNoiseSize) * erosionNoiseSize;
}

/*static*/ Point2 Clouds2::alt_to_diap(float bottom, float thickness, float bigger_bottom, float bigger_thickness)
{
  const float zero = (bottom - bigger_bottom) / bigger_thickness;
  const float one = (bottom + thickness - bigger_bottom) / bigger_thickness;
  return Point2(1 / (one - zero), -zero / (one - zero));
}

void Clouds2::calcCloudsAlt()
{
  cloudsStartAt = min(formParams.layers[0].startAt, formParams.layers[1].startAt);
  cloudsEndAt =
    max(formParams.layers[0].thickness + formParams.layers[0].startAt, formParams.layers[1].thickness + formParams.layers[1].startAt);
  cloudsEndAt = max(cloudsStartAt + 0.001f, cloudsEndAt);
}

void Clouds2::setCloudsShadowCoverage()
{
  calcCloudsAlt();
  const float cloudsThickness = cloudsEndAt - cloudsStartAt;
  // we can calculate exact coverage with compute shaders. But not as easy on pixel shaders.
  // so, instead we use CPU approximation
  // and for better quality we need godrays anyway, so it is just plan B
  // magic numbers obtained with empirical data
  auto safePow = [](float v, float p) { return v > 0.f ? powf(v, p) : 0.f; };
  float averageCoverage =
    safePow(weatherParams.layers[0].coverage * 2 - 1, 8) * 0.25 * lerp(0.17, 0.31, formParams.layers[0].clouds_type) *
      max(0.f, formParams.layers[0].thickness - (2.f / cloudsThickness / 64)) / cloudsThickness // 2/64 is two texels
    + safePow(weatherParams.layers[1].coverage * 2 - 1, 8) * 0.25 * lerp(0.17, 0.31, formParams.layers[1].clouds_type) *
        max(0.f, formParams.layers[1].thickness - (2.f / cloudsThickness / 64)) / cloudsThickness; // 2/64 is two texels
  float averageCumulonimbusCoverage = safePow(weatherParams.cumulonimbusCoverage * 2 - 1, 4) * 0.5;
  cloudsShadowCoverage = lerp(averageCoverage, averageCumulonimbusCoverage, weatherParams.cumulonimbusCoverage);
  ShaderGlobal::set_float(clouds_shadow_coverageVarId, cloudsShadowCoverage);
}

void Clouds2::setCloudLightVars()
{
  // todo: if layer is invisible, hide one inside other

  ShaderGlobal::set_int(clouds_shape_scaleVarId,
    max(int(1), (int)floorf((formParams.shapeNoiseScale * weatherParams.worldSize) / 65536.0 + 0.5)));

  ShaderGlobal::set_int(clouds_cumulonimbus_shape_scaleVarId,
    max(int(1), (int)floorf((formParams.cumulonimbusShapeScale * weatherParams.worldSize) / 65536.0 + 0.5)));
  ShaderGlobal::set_int(clouds_turbulence_freqVarId, formParams.turbulenceFreq);
  ShaderGlobal::set_float(clouds_turbulence_scaleVarId, formParams.turbulenceStrength);
  calcCloudsAlt();
  const float cloudsThickness = cloudsEndAt - cloudsStartAt;
  ShaderGlobal::set_float4(clouds_layers_typesVarId, formParams.layers[0].clouds_type, formParams.layers[0].clouds_type_variance,
    formParams.layers[1].clouds_type, formParams.layers[1].clouds_type_variance);
  float global_clouds_sigma = clouds_sigma_from_extinction(formParams.extinction);
  ShaderGlobal::set_float(global_clouds_sigmaVarId, global_clouds_sigma);
  ShaderGlobal::set_float(clouds_first_layer_densityVarId, formParams.layers[0].density);
  ShaderGlobal::set_float(clouds_second_layer_densityVarId, formParams.layers[1].density);
  ShaderGlobal::set_float(clouds_thickness2VarId, cloudsThickness);
  ShaderGlobal::set_float(clouds_start_altitude2VarId, cloudsStartAt);
  ShaderGlobal::set_float4(clouds_height_fractionsVarId,
    P2D(alt_to_diap(formParams.layers[0].startAt, formParams.layers[0].thickness, cloudsStartAt, cloudsThickness)),
    P2D(alt_to_diap(formParams.layers[1].startAt, formParams.layers[1].thickness, cloudsStartAt, cloudsThickness)));
  ShaderGlobal::set_float(clouds_rain_map_max_heightVarId, (formParams.layers[0].thickness + formParams.layers[0].startAt) * 1000);

  // TODO: until node based shaders have preshader support, we calculate the necessary shadervars by hand here:
  float clouds_start_altitude2 = cloudsStartAt;
  float clouds_thickness2 = cloudsThickness;
  float clouds_weather_size = weatherParams.worldSize;
  ShaderGlobal::set_float4(nbs_world_pos_to_clouds_alt__inv_clouds_weather_size__neg_clouds_thickness_mVarId,
    0.001f / clouds_thickness2, -clouds_start_altitude2 / clouds_thickness2, 1.0f / clouds_weather_size, -1000 * clouds_thickness2);
  ShaderGlobal::set_float(nbs_clouds_start_altitude2_metersVarId, cloudsStartAt * 1000);

  ShaderGlobal::set_float(clouds_perlin_worley_dilationVarId, formParams.perlin_worley_dilation);
  ShaderGlobal::set_float(clouds_worley_erosionVarId, formParams.worley_erosion);
  ShaderGlobal::set_float(clouds_shape_gammaVarId, formParams.shape_gamma);
  setCloudsShadowCoverage();
}

void Clouds2::setCloudRenderingVars()
{
  ShaderGlobal::set_float(clouds_forward_eccentricityVarId, renderingParams.forward_eccentricity);
  ShaderGlobal::set_float(clouds_back_eccentricityVarId, -renderingParams.back_eccentricity);
  ShaderGlobal::set_float(clouds_forward_eccentricity_weightVarId, renderingParams.forward_eccentricity_weight);
  float erosionNoiseSize = renderingParams.erosion_noise_size * 32.f;
  ShaderGlobal::set_float(clouds_erosion_noise_tile_sizeVarId, erosionNoiseSize);
  ShaderGlobal::set_float(clouds_erosion_strengthVarId, renderingParams.erosion_strength);
  ShaderGlobal::set_float(clouds_erosion_height_biasVarId, renderingParams.erosion_height_bias);
  ShaderGlobal::set_float(clouds_erosion_edge_mulVarId, renderingParams.erosion_edge_mul);
  ShaderGlobal::set_float(clouds_erosion_edge_addVarId, renderingParams.erosion_edge_add);
  ShaderGlobal::set_float(clouds_ambient_desaturationVarId, renderingParams.ambient_desaturation);
  ShaderGlobal::set_float(clouds_ms_contributionVarId, renderingParams.ms_contribution);
  ShaderGlobal::set_float(clouds_ms_attenuationVarId, renderingParams.ms_attenuation);
  ShaderGlobal::set_float(clouds_ms_eccentricity_attenuationVarId, renderingParams.ms_ecc_attenuation);
  ShaderGlobal::set_float(clouds_droplet_diameter_umVarId, renderingParams.droplet_diameter_um);
  {
    // fold the droplet HG-fit constants once (see clouds_droplet_phase.hlsli); the
    // in-shader spike needs only ((1-w)(1-g^2)/4, (1-g)^2, 2g)
    float dUm = clamp(renderingParams.droplet_diameter_um, 5.f, 50.f);
    float g = clouds_droplet_hg_g(dUm), w = clouds_droplet_hg_weight(dUm);
    ShaderGlobal::set_float4(clouds_droplet_phase_paramsVarId, (1.f - w) * (1.f - g * g) * 0.25f, (1.f - g) * (1.f - g), 2.f * g, 0.f);
  }
  ShaderGlobal::set_float(clouds_edge_albedoVarId, renderingParams.edge_albedo);
  ShaderGlobal::set_float(clouds_edge_albedo_sharpnessVarId, renderingParams.edge_albedo_sharpness);
  // negative exposure would make the TAA tonemap rcp(luma*e+1) singular; 0 = off
  ShaderGlobal::set_float(clouds_taa_exposureVarId, max(0.f, renderingParams.taa_exposure_scale));
  ShaderGlobal::set_float(clouds_bsm_ms_attn_sqVarId, renderingParams.ms_attenuation * renderingParams.ms_attenuation);
  // consumer-side decode only: the bake keeps its units, so no map invalidation
  ShaderGlobal::set_float(clouds_bsm_scattering_physicalityVarId, renderingParams.bsm_scattering_physicality);
  cloudShadows.setBSMLog2AmortizeFrames(renderingParams.bsm_log2_amortize_frames);
}

void Clouds2::setWeatherGenVars()
{
  ShaderGlobal::set_float(clouds_weather_sizeVarId, weatherParams.worldSize);
  ShaderGlobal::set_float(clouds_layer1_coverageVarId, weatherParams.layers[0].coverage);
  ShaderGlobal::set_float(clouds_layer1_freqVarId, weatherParams.layers[0].freq);
  ShaderGlobal::set_float(clouds_layer1_seedVarId, weatherParams.layers[0].seed);
  ShaderGlobal::set_float(clouds_layer2_coverageVarId, weatherParams.layers[1].coverage);
  ShaderGlobal::set_float(clouds_layer2_freqVarId, weatherParams.layers[1].freq);
  ShaderGlobal::set_float(clouds_layer2_seedVarId, weatherParams.layers[1].seed);
  ShaderGlobal::set_float(clouds_epicnessVarId, weatherParams.epicness);
  ShaderGlobal::set_float(clouds_rain_clouds_amountVarId, weatherParams.cumulonimbusCoverage);
  ShaderGlobal::set_float(clouds_rain_clouds_seedVarId, weatherParams.cumulonimbusSeed);
}

void Clouds2::setGameParams(const DaSkies::CloudsSettingsParams &game_params)
{
  if (gameParams == game_params)
    return;
  gameParams = game_params;
  field.setParams(gameParams);
  field.invalidate();
  cloudShadows.invalidate(); // todo: instead force temporal recalc!
  holeFound = HoleFindStatus::REQUIRE_FIND;
}

void Clouds2::setWeatherGen(const DaSkies::CloudsWeatherGen &weather_params)
{
  if (weatherParams == weather_params)
    return;
  weatherParams = weather_params;
  invalidateWeather();
}

void Clouds2::invalidateWeather()
{
  setWeatherGenVars();
  weather.invalidate();
  invalidateShadows();
}

void Clouds2::invalidateShadows()
{
  setCloudLightVars();

  cloudsForm.invalidate(); // actually we need to invalidate only if something except extinction. Extinction shouldn't invalidate
                           // lut.
  // but it is so fast to compute

  field.invalidate();
  cloudShadows.invalidate(); // todo: instead force temporal recalc!
  holeFound = HoleFindStatus::REQUIRE_FIND;
  // light.invalidate();
}

void Clouds2::setCloudsForm(const DaSkies::CloudsForm &form_params)
{
  if (formParams == form_params)
    return;
  formParams = form_params;
  calcCloudsAlt();
  invalidateShadows();
}

void Clouds2::setCloudsRendering(const DaSkies::CloudsRendering &rendering_params)
{
  // the aerosol params feed only the atmosphere medium (derived and edge-detected in
  // DaSkies::prepare): neutralize them here so tuning haze does not rebuild cloud
  // light or re-render a converged panorama
  DaSkies::CloudsRendering visibleNew = rendering_params;
  visibleNew.copyAerosolFrom(renderingParams);
  if (renderingParams == visibleNew)
  {
    renderingParams = rendering_params; // still record the aerosol values
    return;
  }
  if (renderingParams.ms_attenuation != visibleNew.ms_attenuation)
    cloudShadows.invalidateBSM(); // the map stores optical depths scaled by ms_attenuation^2
  if (renderingParams.droplet_diameter_um != visibleNew.droplet_diameter_um)
    cloudsForm.invalidate(); // the phase LUT is baked from the droplet size
  if (renderingParams.erosion_strength != visibleNew.erosion_strength ||
      renderingParams.erosion_height_bias != visibleNew.erosion_height_bias ||
      renderingParams.erosion_edge_mul != visibleNew.erosion_edge_mul ||
      renderingParams.erosion_edge_add != visibleNew.erosion_edge_add)
    cloudShadows.invalidate(); // the erosion curve is live everywhere except the shadow volume, which marches with it
  DaSkies::CloudsRendering visibleOld = renderingParams;
  visibleOld.erosionWindSpeed = visibleNew.erosionWindSpeed; // only moves the erosion offset over time
  renderingParamsDirty |= visibleOld != visibleNew;
  renderingParams = rendering_params;
  // fixme: only ambient desturation should cause this
  invalidateLight();
  setCloudRenderingVars();
}

void Clouds2::invalidateAll()
{
  invalidateWeather();
  invalidateLight();
  invalidateShadows();
  setCloudLightVars();
  setCloudRenderingVars();
}

void Clouds2::init(bool use_hole)
{
#define VAR(a, opt) a##VarId = ::get_shader_variable_id(#a, opt);
  CLOUDS_VARS_LIST
#undef VAR

  noises.init();
  weather.init();
  clouds.init();
  field.init();
  cloudShadows.init();
  light.init();
  cloudsForm.init();
  calcCloudsAlt();
  useHole = use_hole;
  if (use_hole)
    initHole();

  invalidateWeather();
  invalidateLight();
  invalidateShadows();
  setCloudLightVars();
  setCloudRenderingVars();
}

void Clouds2::update(float dt, const Point2 &wind_dir)
{
  updateErosionNoiseWind(dt, wind_dir);

  ShaderGlobal::set_float4(clouds_erosion_noise_wind_ofsVarId, clouds_erosion_noise_wind_ofs.x, 0, clouds_erosion_noise_wind_ofs.y, 0);
  const float windLen = length(wind_dir);
  const Point2 windDirNorm = windLen > 1e-5f ? wind_dir / windLen : Point2(0, 0);
  ShaderGlobal::set_float4(clouds_erosion_wind_dirVarId, windDirNorm.x, 0, windDirNorm.y, 0);
}

void Clouds2::renderCloudsPrepare(CloudsRendererData &data, BaseTexture *depth, BaseTexture *prev_depth, const TMatrix &view_tm,
  const TMatrix4 &proj_tm, const CloudsRenderFlags flags)
{
  if (!renderingEnabled)
  {
    return;
  }
  noises.setVars();
  clouds.renderCloudsPrepare(data, depth, prev_depth, erosionWindChange, weatherParams.worldSize, view_tm, proj_tm, nullptr, flags);
}

void Clouds2::renderCloudsApply(CloudsRendererData &data, BaseTexture *downsampled_depth, BaseTexture *target_depth,
  const Point4 &target_depth_transform, const TMatrix &view_tm, const TMatrix4 &proj_tm, const CloudsRenderFlags flags)
{
  if (!renderingEnabled)
  {
    return;
  }
  clouds.renderCloudsApply(data, downsampled_depth, target_depth, target_depth_transform, view_tm, proj_tm, flags);
}

CloudsChangeFlags Clouds2::prepareLighting(const Point3 &main_light_dir, const Point3 &second_light_dir, bool scattering_ready,
  const Point2 &camera_xz)
{
  uint32_t changes = CLOUDS_NO_CHANGE;
  if (renderingParamsDirty)
  {
    renderingParamsDirty = false;
    changes = CLOUDS_INVALIDATED;
  }
  if (noises.render())
  {
    invalidateWeather();
    invalidateLight();
    changes = CLOUDS_INVALIDATED;
  }
  changes |= uint32_t(weather.render());
  changes |= uint32_t(cloudsForm.render());
  if (noises.isReady())
    changes |= uint32_t(field.render());
  if (changes & CLOUDS_INVALIDATED)
  {
    cloudShadows.invalidate();
    light.invalidate();
  }

  if (noises.isReady() && scattering_ready)
  {
    const uint32_t cloudsShadowsUpdateFlags = cloudShadows.render(main_light_dir, camera_xz);
    changes |= uint32_t(cloudsShadowsUpdateFlags);
    changes |= uint32_t(light.render(main_light_dir, second_light_dir)); // not afr ready

    if (cloudsShadowsUpdateFlags == CLOUDS_INVALIDATED)
      holeFound = HoleFindStatus::REQUIRE_FIND;

    changes |= validateHole(main_light_dir) ? uint32_t(CLOUDS_INVALIDATED) : uint32_t(0);
  }

  return CloudsChangeFlags(changes);
}

void Clouds2::renderCloudVolume(VolTexture *cloud_volume, TEXTUREID prev_cloud_volume, float max_dist, const TMatrix &view_tm,
  const TMatrix4 &proj_tm, const TMatrix4 &prev_glob_tm)
{
  if (!renderingEnabled)
  {
    return;
  }
  field.renderCloudVolume(cloud_volume, prev_cloud_volume, max_dist, view_tm, proj_tm, prev_glob_tm);
}

void Clouds2::resetCloudsShadows() { cloudShadows.invalidateBSM(); }

void Clouds2::initHole()
{
  shaders::OverrideState state;
  state.set(shaders::OverrideState::BLEND_OP);
  state.blendOp = BLENDOP_MAX;
  blendMaxState.reset(shaders::overrides::create(state));

  int posTexFlags = TEXFMT_A32B32G32R32F;
  if (VoltexRenderer::is_compute_supported())
  {
    clouds_hole_cs.reset(new_compute_shader("clouds_hole_cs", true));
    holeBuf = dag::buffers::create_ua_sr_structured(4, 1, "clouds_hole_buf", d3d::buffers::Init::No, RESTAG_DASKIES2);

    clouds_hole_pos_cs.reset(new_compute_shader("clouds_hole_pos_cs", true));
    posTexFlags |= TEXCF_UNORDERED;
  }
  else
  {
    clouds_hole_ps.init("clouds_hole_ps");
    clouds_hole_pos_ps.init("clouds_hole_pos_ps");

    holeTex = dag::create_tex(nullptr, 1, 1, TEXCF_RTARGET | TEXFMT_R32F, 1, "clouds_hole_tex", RESTAG_DASKIES2);
    posTexFlags |= TEXCF_RTARGET;
  }

  holePosTex = dag::create_tex(nullptr, 1, 1, posTexFlags, 1, "clouds_hole_pos_tex", RESTAG_DASKIES2);
  holeReadbackQuery.reset(d3d::create_event_query());
}

bool Clouds2::validateHole(const Point3 &main_light_dir)
{
  if (!d3d::get_event_query_status(holeReadbackQuery.get(), false))
    return false;

  if (holeFound == HoleFindStatus::REQUIRE_FIND)
  {
    holeFound = HoleFindStatus::REQUIRE_READBACK;
    if (!findHole(main_light_dir))
    {
      ShaderGlobal::set_float4(clouds_hole_posVarId, Point4::ZERO);
      holeFound = HoleFindStatus::DONE;
      return true;
    }
    d3d::issue_event_query(holeReadbackQuery.get());
  }
  else if (holeFound == HoleFindStatus::REQUIRE_READBACK)
  {
    G_ASSERT(useHole);
    if (auto locked = lock_texture<const Point4>(holePosTex.getTex2D(), 0, TEXLOCK_READ | TEXLOCK_NOSYSLOCK))
    {
      Point4 destData = locked.at(0, 0);
      ShaderGlobal::set_float4(clouds_hole_posVarId, destData);
      holeFound = HoleFindStatus::DONE;
      return true;
    }
  }

  return false;
}

bool Clouds2::findHole(const Point3 &main_light_dir)
{
  if (!useHole || (!((holeBuf || holeTex) && cloudShadows.cloudsShadowsVol)))
    return false;

  ShaderGlobal::set_float(clouds_hole_densityVarId, holeDensity);
  TIME_D3D_PROFILE(look_for_hole);
  const float alt = (minimalStartAlt() + (maximumTopAlt() - minimalStartAlt()) * 0.5 / 32) * 1000.f - holeTarget.y;
  if (clouds_hole_cs && clouds_hole_pos_cs)
  {
    {
      const uint32_t v = 0xFFFFFFFF;
      holeBuf->updateData(0, sizeof(v), &v, VBLOCK_WRITEONLY);
      d3d::resource_barrier({holeBuf.getBuf(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});
      STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), holeBuf.getBuf());
      clouds_hole_cs->dispatchThreads(CLOUDS_HOLE_GEN_RES, CLOUDS_HOLE_GEN_RES, 1);
      d3d::resource_barrier({holeBuf.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});
    }
    {
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), holePosTex.getTex2D());
      ShaderGlobal::set_float4(clouds_hole_target_altVarId, P3D(holeTarget), alt);
      ShaderGlobal::set_float4(clouds_hole_light_dirVarId, P3D(main_light_dir), 0);
      clouds_hole_pos_cs->dispatch();
    }
  }
  else if (clouds_hole_ps.getElem() && clouds_hole_pos_ps.getElem())
  {
    SCOPE_RENDER_TARGET;
    d3d::set_render_target({}, DepthAccess::RW, {{holeTex.getTex2D(), 0, 0}});
    d3d::clearview(CLEAR_TARGET, E3DCOLOR(), 0, 0);
    shaders::overrides::set(blendMaxState.get());
    clouds_hole_ps.getElem()->setStates();
    d3d::draw_instanced(PRIM_TRILIST, 0, 1, CLOUDS_HOLE_GEN_RES * CLOUDS_HOLE_GEN_RES);
    shaders::overrides::reset();

    ShaderGlobal::set_float4(clouds_hole_target_altVarId, P3D(holeTarget), alt);
    ShaderGlobal::set_float4(clouds_hole_light_dirVarId, P3D(main_light_dir), 0);
    d3d::set_render_target({}, DepthAccess::RW, {{holePosTex.getTex2D(), 0, 0}});
    d3d::clearview(CLEAR_TARGET, E3DCOLOR(), 0, 0);
    clouds_hole_pos_ps.getElem()->setStates();
    d3d::draw_instanced(PRIM_TRILIST, 0, 1, 1);
  }
  request_readback_nosyslock(holePosTex.getTex2D(), 0);
  return true;
}

void Clouds2::setUseHole(bool set)
{
  if (useHole == set)
    return;
  useHole = set;
  if (useHole)
  {
    ShaderGlobal::set_float(clouds_hole_densityVarId, holeDensity);
  }
  else
  {
    holeFound = HoleFindStatus::REQUIRE_FIND;
    ShaderGlobal::set_float(clouds_hole_densityVarId, 1);
  }
}

void Clouds2::setHole(const Point3 &hole_target, float hole_density)
{
  holeTarget = hole_target;
  holeDensity = hole_density;
  holeFound = HoleFindStatus::REQUIRE_FIND;
}

Point2 Clouds2::getCloudsHole() const
{
  Color4 point = ShaderGlobal::get_float4(clouds_hole_posVarId);
  return {point.r, point.g};
}

void Clouds2::getTextureResourceDependencies(Tab<TEXTUREID> &dependencies) const
{
  if (noises.resCloud1.getTexId() != BAD_TEXTUREID)
  {
    dependencies.push_back(noises.resCloud1.getTexId());
  }
  if (noises.resCloud2.getTexId() != BAD_TEXTUREID)
  {
    dependencies.push_back(noises.resCloud2.getTexId());
  }
}

bool Clouds2::isLightRendered() const { return light.isRendered(); }

bool Clouds2::isLightingConverged() const
{
  if (!isReady() || !light.isRendered() || !cloudShadows.isConverged() || renderingParamsDirty)
    return false;
  return !useHole || holeFound == HoleFindStatus::DONE;
}