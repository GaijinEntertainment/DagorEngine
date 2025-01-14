// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_resPtr.h>
#include <3d/dag_ringCPUTextureLock.h>
#include <shaders/dag_overrideStates.h>

#include "clouds2Common.h"
#include "cloudsGenWeather.h"
#include "cloudsFormLUT.h"
#include "cloudsRendererData.h"
#include "cloudsGenNoise.h"
#include "cloudsRenderer.h"
#include "cloudsShadows.h"
#include "cloudsField.h"
#include "cloudsLightRenderer.h"

inline float clouds_sigma_from_extinction(float ext) { return -0.09f * ext; }

struct Clouds2
{
  void setCloudsOfs(const DPoint2 &xz) { clouds.setCloudsOfs(xz); }
  DPoint2 getCloudsOfs() const { return clouds.getCloudsOfs(); }
  float minimalStartAlt() const { return cloudsStartAt; }
  float maximumTopAlt() const { return cloudsEndAt; }
  float getCloudsShadowCoverage() const { return cloudsShadowCoverage; }
  float effectiveStartAlt() const
  {
    float altStart, altTop;
    return field.getReadbackData(altStart, altTop) ? altStart : cloudsStartAt;
  }
  float effectiveTopAlt() const
  {
    float altStart, altTop;
    return field.getReadbackData(altStart, altTop) ? altTop : cloudsEndAt;
  }
  bool hasVisibleClouds() const; // GPU readback!

  void setErosionNoiseWindDir(const Point2 &wind_dir) { windDir = wind_dir; } // normalized

  void setCloudsShadowCoverage();

  void setGameParams(const DaSkies::CloudsSettingsParams &game_params);

  void setWeatherGen(const DaSkies::CloudsWeatherGen &weather_params);
  void setCloudsForm(const DaSkies::CloudsForm &form_params);
  void setCloudsRendering(const DaSkies::CloudsRendering &rendering_params);

  // temporary function for a workaround on A8 iPads (gpu hang)
  void setRenderingEnabled(bool enabled) { renderingEnabled = enabled; }

  // causes clouds to change lighting
  void invalidateAll();

  void init(bool use_hole = true);
  void update(float dt);
  void setCloudsOffsetVars(CloudsRendererData &data) { clouds.setCloudsOffsetVars(data, weatherParams.worldSize); }
  void setCloudsOffsetVars(float current_clouds_offset) { clouds.setCloudsOffsetVars(current_clouds_offset, weatherParams.worldSize); }
  IPoint2 getResolution(CloudsRendererData &data) { return IPoint2(data.w, data.h); }
  void renderClouds(CloudsRendererData &data, const TextureIDPair &depth, const TextureIDPair &prev_depth, const TMatrix &view_tm,
    const TMatrix4 &proj_tm);
  void renderCloudsScreen(CloudsRendererData &data, const TextureIDPair &downsampled_depth, TEXTUREID target_depth,
    const Point4 &target_depth_transform, const TMatrix &view_tm, const TMatrix4 &proj_tm);

  bool isPrepareRequired() const { return noises.isPrepareRequired(); }
  bool isReady() const { return noises.isReady(); }

  CloudsChangeFlags prepareLighting(const Point3 &main_light_dir, const Point3 &second_light_dir);
  void renderCloudVolume(VolTexture *cloud_volume, float max_dist, const TMatrix &view_tm, const TMatrix4 &proj_tm);
  void setUseShadows2D(bool on);
  void reset() { noises.reset(); }

  void layersHeightsBarrier() { field.layersHeightsBarrier(); }

  void setUseHole(bool set);
  bool getUseHole() const { return useHole; }
  void resetHole(const Point3 &hole_target, const float &hole_density);
  Point2 getCloudsHole() const;
  void setHole(const Point2 &p);

  void resetHole() { setHole({0, 0}); }
  void getTextureResourceDependencies(Tab<TEXTUREID> &dependencies) const;

  void setExternalWeatherTexture(TEXTUREID tid)
  {
    if (weather.setExternalWeatherTexture(tid))
      field.invalidate();
  }

private:
  void processHole();

  void initHole();
  bool findHole(const Point3 &main_light_dir);
  bool validateHole(const Point3 &main_light_dir);

  void invalidateWeather();
  void invalidateLight() { light.invalidate(); }
  void invalidateShadows();
  void setCloudLightVars();
  void setCloudRenderingVars();
  void setWeatherGenVars();

  static Point2 alt_to_diap(float bottom, float thickness, float bigger_bottom, float bigger_thickness);
  void calcCloudsAlt();

  void updateErosionNoiseWind(float dt);

protected:
  GenWeather weather;
  GenNoise noises;
  CloudsFormLUT cloudsForm;
  CloudsRenderer clouds;
  CloudsShadows cloudShadows;
  CloudsField field;
  CloudsLightRenderer light;

  float cloudsStartAt = 0.7f, cloudsEndAt = 8.0f;
  float cloudsShadowCoverage = 0;

  DaSkies::CloudsSettingsParams gameParams;
  DaSkies::CloudsWeatherGen weatherParams;
  DaSkies::CloudsForm formParams;
  DaSkies::CloudsRendering renderingParams;

  Point2 clouds_erosion_noise_wind_ofs = {0, 0};
  Point2 windDir = {0, 0}, erosionWindChange = {0, 0};
  Point3 holeTarget = {0, 0, 0};
  float holeDensity = 0;
  bool holeFound = true;
  bool useHole = true;

  RingCPUTextureLock ringTextures;
  UniqueBufHolder holeBuf;
  UniqueTexHolder holeTex;
  UniqueTexHolder holePosTex;
  UniqueTex holePosTexStaging;

  eastl::unique_ptr<ComputeShaderElement> clouds_hole_cs;
  eastl::unique_ptr<ComputeShaderElement> clouds_hole_pos_cs;
  PostFxRenderer clouds_hole_ps;
  PostFxRenderer clouds_hole_pos_ps;
  shaders::UniqueOverrideStateId blendMaxState;

  bool renderingEnabled = true;

  enum
  {
    HOLE_UPDATED = 0,
    HOLE_PROCESSED = 1,
    HOLE_CREATED = 2
  };
  int needHoleCPUUpdate = HOLE_UPDATED;
};
