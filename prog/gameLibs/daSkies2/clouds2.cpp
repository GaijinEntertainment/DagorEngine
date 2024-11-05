// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "clouds2.h"
#include "cloudsShaderVars.h"

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

void Clouds2::updateErosionNoiseWind(float dt)
{
  erosionWindChange = windDir * (renderingParams.erosionWindSpeed * dt);
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
  ShaderGlobal::set_real(clouds_shadow_coverageVarId, cloudsShadowCoverage);
}

void Clouds2::setCloudLightVars()
{
  // todo: if layer is invisible, hide one inside other

  ShaderGlobal::set_int(clouds_shape_scaleVarId,
    max(int(1), (int)floorf((formParams.shapeNoiseScale * weatherParams.worldSize) / 65536.0 + 0.5)));

  ShaderGlobal::set_int(clouds_cumulonimbus_shape_scaleVarId,
    max(int(1), (int)floorf((formParams.cumulonimbusShapeScale * weatherParams.worldSize) / 65536.0 + 0.5)));
  ShaderGlobal::set_int(clouds_turbulence_freqVarId, formParams.turbulenceFreq);
  ShaderGlobal::set_real(clouds_turbulence_scaleVarId, formParams.turbulenceStrength);
  calcCloudsAlt();
  const float cloudsThickness = cloudsEndAt - cloudsStartAt;
  ShaderGlobal::set_color4(clouds_layers_typesVarId, formParams.layers[0].clouds_type, formParams.layers[0].clouds_type_variance,
    formParams.layers[1].clouds_type, formParams.layers[1].clouds_type_variance);
  float global_clouds_sigma = clouds_sigma_from_extinction(formParams.extinction);
  ShaderGlobal::set_real(global_clouds_sigmaVarId, global_clouds_sigma);
  ShaderGlobal::set_real(clouds_first_layer_densityVarId, formParams.layers[0].density);
  ShaderGlobal::set_real(clouds_second_layer_densityVarId, formParams.layers[1].density);
  ShaderGlobal::set_real(clouds_thickness2VarId, cloudsThickness);
  ShaderGlobal::set_real(clouds_start_altitude2VarId, cloudsStartAt);
  ShaderGlobal::set_color4(clouds_height_fractionsVarId,
    P2D(alt_to_diap(formParams.layers[0].startAt, formParams.layers[0].thickness, cloudsStartAt, cloudsThickness)),
    P2D(alt_to_diap(formParams.layers[1].startAt, formParams.layers[1].thickness, cloudsStartAt, cloudsThickness)));

  // TODO: until node based shaders have preshader support, we calculate the necessary shadervars by hand here:
  float clouds_start_altitude2 = cloudsStartAt;
  float clouds_thickness2 = cloudsThickness;
  float clouds_weather_size = weatherParams.worldSize;
  ShaderGlobal::set_color4(nbs_world_pos_to_clouds_alt__inv_clouds_weather_size__neg_clouds_thickness_mVarId,
    0.001f / clouds_thickness2, -clouds_start_altitude2 / clouds_thickness2, 1.0f / clouds_weather_size, -1000 * clouds_thickness2);
  ShaderGlobal::set_real(nbs_clouds_start_altitude2_metersVarId, cloudsStartAt * 1000);

  setCloudsShadowCoverage();
}

void Clouds2::setCloudRenderingVars()
{
  ShaderGlobal::set_real(clouds_forward_eccentricityVarId, renderingParams.forward_eccentricity);
  ShaderGlobal::set_real(clouds_back_eccentricityVarId, -renderingParams.back_eccentricity);
  ShaderGlobal::set_real(clouds_forward_eccentricity_weightVarId, renderingParams.forward_eccentricity_weight);
  float erosionNoiseSize = renderingParams.erosion_noise_size * 32.f;
  ShaderGlobal::set_real(clouds_erosion_noise_tile_sizeVarId, erosionNoiseSize);
  ShaderGlobal::set_real(clouds_ambient_desaturationVarId, renderingParams.ambient_desaturation);
  ShaderGlobal::set_real(clouds_ms_contributionVarId, renderingParams.ms_contribution);
  ShaderGlobal::set_real(clouds_ms_attenuationVarId, renderingParams.ms_attenuation);
  ShaderGlobal::set_real(clouds_ms_eccentricity_attenuationVarId, renderingParams.ms_ecc_attenuation);
}

void Clouds2::setWeatherGenVars()
{
  ShaderGlobal::set_real(clouds_weather_sizeVarId, weatherParams.worldSize);
  ShaderGlobal::set_real(clouds_layer1_coverageVarId, weatherParams.layers[0].coverage);
  ShaderGlobal::set_real(clouds_layer1_freqVarId, weatherParams.layers[0].freq);
  ShaderGlobal::set_real(clouds_layer1_seedVarId, weatherParams.layers[0].seed);
  ShaderGlobal::set_real(clouds_layer2_coverageVarId, weatherParams.layers[1].coverage);
  ShaderGlobal::set_real(clouds_layer2_freqVarId, weatherParams.layers[1].freq);
  ShaderGlobal::set_real(clouds_layer2_seedVarId, weatherParams.layers[1].seed);
  ShaderGlobal::set_real(clouds_epicnessVarId, weatherParams.epicness);
  ShaderGlobal::set_real(clouds_rain_clouds_amountVarId, weatherParams.cumulonimbusCoverage);
  ShaderGlobal::set_real(clouds_rain_clouds_seedVarId, weatherParams.cumulonimbusSeed);
}

void Clouds2::setGameParams(const DaSkies::CloudsSettingsParams &game_params)
{
  if (gameParams == game_params)
    return;
  gameParams = game_params;
  field.setParams(gameParams);
  field.invalidate();
  cloudShadows.invalidate(); // todo: instead force temporal recalc!
  holeFound = false;
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
  holeFound = false;
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
  if (renderingParams == rendering_params)
    return;
  renderingParams = rendering_params;
  // fixme: only ambient desturation should cause this
  invalidateLight();
  setCloudRenderingVars();

  // fixme: invalidate panorama, if we change everything besides erosion speed
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

void Clouds2::update(float dt)
{
  updateErosionNoiseWind(dt);
  light.update(dt);

  ShaderGlobal::set_color4(clouds_erosion_noise_wind_ofsVarId, clouds_erosion_noise_wind_ofs.x, 0, clouds_erosion_noise_wind_ofs.y, 0);
}

void Clouds2::renderClouds(CloudsRendererData &data, const TextureIDPair &depth, const TextureIDPair &prev_depth,
  const TMatrix &view_tm, const TMatrix4 &proj_tm)
{
  if (!renderingEnabled)
  {
    return;
  }
  noises.setVars();
  clouds.render(data, depth, prev_depth, erosionWindChange, weatherParams.worldSize, view_tm, proj_tm);
}

void Clouds2::renderCloudsScreen(CloudsRendererData &data, const TextureIDPair &downsampled_depth, TEXTUREID target_depth,
  const Point4 &target_depth_transform, const TMatrix &view_tm, const TMatrix4 &proj_tm)
{
  if (!renderingEnabled)
  {
    return;
  }
  clouds.renderFull(data, downsampled_depth, target_depth, target_depth_transform, view_tm, proj_tm);
}

void Clouds2::processHole()
{
  if (!useHole)
    return;

  if (needHoleCPUUpdate == HOLE_UPDATED)
    return;

  if (needHoleCPUUpdate == HOLE_CREATED)
  {
    int frame1;
    Texture *currentTarget = ringTextures.getNewTarget(frame1);
    if (currentTarget)
    {
      d3d::stretch_rect(holePosTex.getTex2D(), currentTarget, NULL, NULL);
      ringTextures.startCPUCopy();
      needHoleCPUUpdate = HOLE_PROCESSED;
    }
  }

  if (needHoleCPUUpdate == HOLE_PROCESSED)
  {
    uint32_t frame2;
    int stride;
    uint8_t *data = ringTextures.lock(stride, frame2);
    if (!data)
      return;
    Point4 destData;
    memcpy(&destData, data, sizeof(Point4));

    ringTextures.unlock();
    ShaderGlobal::set_color4(clouds_hole_posVarId, destData.x, destData.y, destData.z, destData.w);
    needHoleCPUUpdate = HOLE_UPDATED;
  }
}

CloudsChangeFlags Clouds2::prepareLighting(const Point3 &main_light_dir, const Point3 &second_light_dir)
{
  uint32_t changes = CLOUDS_NO_CHANGE;
  if (noises.render())
  {
    invalidateWeather();
    invalidateLight();
    changes = CLOUDS_INVALIDATED;
  }
  changes |= uint32_t(weather.render());
  changes |= uint32_t(cloudsForm.render());
  changes |= uint32_t(field.render());

  if (noises.isReady())
  {
    const uint32_t cloudsShadowsUpdateFlags = cloudShadows.render(main_light_dir);
    changes |= uint32_t(cloudsShadowsUpdateFlags);
    changes |= uint32_t(light.render(main_light_dir, second_light_dir)); // not afr ready

    if (cloudsShadowsUpdateFlags == CLOUDS_INVALIDATED)
      holeFound = false;
  }

  changes |= validateHole(main_light_dir) ? uint32_t(CLOUDS_INVALIDATED) : uint32_t(0);

  processHole();

  return CloudsChangeFlags(changes);
}

void Clouds2::renderCloudVolume(VolTexture *cloud_volume, float max_dist, const TMatrix &view_tm, const TMatrix4 &proj_tm)
{
  if (!renderingEnabled)
  {
    return;
  }
  field.renderCloudVolume(cloud_volume, max_dist, view_tm, proj_tm);
}

void Clouds2::setUseShadows2D(bool on)
{
  if (on)
    cloudShadows.initShadows2D();
  else
    cloudShadows.closeShadows2D();
}

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
    holeBuf = dag::buffers::create_ua_sr_structured(4, 1, "clouds_hole_buf");

    clouds_hole_pos_cs.reset(new_compute_shader("clouds_hole_pos_cs", true));
    posTexFlags |= TEXCF_UNORDERED;
  }
  else
  {
    clouds_hole_ps.init("clouds_hole_ps");
    clouds_hole_pos_ps.init("clouds_hole_pos_ps");

    holeTex = dag::create_tex(nullptr, 1, 1, TEXCF_RTARGET | TEXFMT_R32F, 1, "clouds_hole_tex");
    posTexFlags |= TEXCF_RTARGET;
  }

  holePosTex = dag::create_tex(nullptr, 1, 1, posTexFlags | TEXCF_CLEAR_ON_CREATE, 1, "clouds_hole_pos_tex");
  ringTextures.close();
  ringTextures.init(1, 1, 1, "CPU_clouds_hole_tex",
    TEXFMT_A32B32G32R32F | TEXCF_RTARGET | TEXCF_LINEAR_LAYOUT | TEXCF_CPU_CACHED_MEMORY);
#if !_TARGET_PC_WIN && !_TARGET_XBOX
  d3d::GpuAutoLock gpuLock;
  if (VoltexRenderer::is_compute_supported())
  {
    Point4 zeroPos(0, 0, 0, 0);
    d3d::clear_rwtexf(holePosTex.getTex2D(), reinterpret_cast<float *>(&zeroPos), 0, 0);
  }
  else
  {
    SCOPE_RENDER_TARGET;
    d3d::set_render_target(holePosTex.getTex2D(), 0);
    d3d::clearview(CLEAR_TARGET, E3DCOLOR(), 0, 0);
  }
#endif
  d3d::resource_barrier({holePosTex.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_ALL_GRAPHICS, 0, 0});
  holePosTexStaging = dag::create_tex(nullptr, 1, 1, TEXFMT_A32B32G32R32F, 1, "clouds_hole_pos_tex_1");
}

bool Clouds2::validateHole(const Point3 &main_light_dir)
{
  if (holeFound)
    return false;

  holeFound = true;
  if (!findHole(main_light_dir))
  {
    Point2 hole(0, 0);
    setHole(hole);
  }
  needHoleCPUUpdate = HOLE_CREATED;
  ringTextures.reset();
  return true;
}

bool Clouds2::findHole(const Point3 &main_light_dir)
{
  if (!useHole || (!((holeBuf || holeTex) && cloudShadows.cloudsShadowsVol)))
    return false;

  ShaderGlobal::set_real(clouds_hole_densityVarId, holeDensity);
  TIME_D3D_PROFILE(look_for_hole);
  const float alt = (minimalStartAlt() + (maximumTopAlt() - minimalStartAlt()) * 0.5 / 32) * 1000.f - holeTarget.y;
  if (clouds_hole_cs && clouds_hole_pos_cs)
  {
    {
      uint32_t v[4] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
      d3d::clear_rwbufi(holeBuf.getBuf(), v);
      d3d::resource_barrier({holeBuf.getBuf(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});
      STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), holeBuf.getBuf());
      clouds_hole_cs->dispatchThreads(CLOUDS_HOLE_GEN_RES, CLOUDS_HOLE_GEN_RES, 1);
      d3d::resource_barrier({holeBuf.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});
    }
    {
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), holePosTex.getTex2D());
      ShaderGlobal::set_color4(clouds_hole_target_altVarId, P3D(holeTarget), alt);
      ShaderGlobal::set_color4(clouds_hole_light_dirVarId, P3D(main_light_dir), 0);
      clouds_hole_pos_cs->dispatch();
      d3d::resource_barrier({holePosTex.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_ALL_GRAPHICS, 0, 0});
    }
  }
  else if (clouds_hole_ps.getElem() && clouds_hole_pos_ps.getElem())
  {
    SCOPE_RENDER_TARGET;
    d3d::set_render_target(holeTex.getTex2D(), 0);
    d3d::clearview(CLEAR_TARGET, E3DCOLOR(), 0, 0);
    shaders::overrides::set(blendMaxState.get());
    clouds_hole_ps.getElem()->setStates();
    d3d::draw_instanced(PRIM_TRILIST, 0, 1, CLOUDS_HOLE_GEN_RES * CLOUDS_HOLE_GEN_RES);
    shaders::overrides::reset();

    ShaderGlobal::set_color4(clouds_hole_target_altVarId, P3D(holeTarget), alt);
    ShaderGlobal::set_color4(clouds_hole_light_dirVarId, P3D(main_light_dir), 0);
    d3d::set_render_target(holePosTex.getTex2D(), 0);
    d3d::clearview(CLEAR_TARGET, E3DCOLOR(), 0, 0);
    clouds_hole_pos_ps.getElem()->setStates();
    d3d::draw_instanced(PRIM_TRILIST, 0, 1, 1);
    d3d::resource_barrier({holePosTex.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_ALL_GRAPHICS, 0, 0});
  }
  return true;
}

void Clouds2::setUseHole(bool set)
{
  if (useHole == set)
    return;
  useHole = set;
  if (useHole)
  {
    ShaderGlobal::set_real(clouds_hole_densityVarId, holeDensity);
  }
  else
  {
    holeFound = false;
    ShaderGlobal::set_real(clouds_hole_densityVarId, 1);
  }
}

void Clouds2::resetHole(const Point3 &hole_target, const float &hole_density)
{
  if (!useHole)
    return;

  holeTarget = hole_target;
  holeDensity = hole_density;
  holeFound = false;
}

Point2 Clouds2::getCloudsHole() const
{
  Point2 point(0, 0);
  if (holePosTex.getTex2D() && holeFound)
  {
    if (auto lockedTex = lock_texture<const Point4>(holePosTex.getTex2D(), 0, TEXLOCK_READ))
    {
      Point4 src = lockedTex.at(0, 0);
      point = Point2(src.x, src.y);
      d3d::resource_barrier({holePosTex.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_ALL_GRAPHICS, 0, 0});
    }
  }
  return point;
}

void Clouds2::setHole(const Point2 &p)
{
  Point4 bufferData(p.x, p.y, p.x / weatherParams.worldSize, p.y / weatherParams.worldSize);
  Point4 *holePos;
  int stride;
  if (holePosTexStaging && holePosTexStaging->lockimgEx(&holePos, stride, 0, TEXLOCK_WRITE) && holePos)
  {
    *holePos = bufferData;
    holePosTexStaging->unlockimg();
    holePosTex->update(holePosTexStaging.getTex2D());
    d3d::resource_barrier({holePosTex.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_ALL_GRAPHICS, 0, 0});
    holeFound = true;
  }
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
