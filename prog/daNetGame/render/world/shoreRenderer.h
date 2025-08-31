// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_resPtr.h>

class FFTWater;

class ShoreRenderer
{
public:
  ShoreRenderer();

  ShoreRenderer(const ShoreRenderer &) = delete;
  ShoreRenderer(ShoreRenderer &&) = delete;
  ShoreRenderer &operator=(const ShoreRenderer &) = delete;
  ShoreRenderer &operator=(ShoreRenderer &&) = delete;

  void buildUI();
  void setupShore(bool enabled, int texture_size, float hmap_size, float rivers_width, float significant_wave_threshold);
  void setupShoreSurf(float wave_height_to_amplitude,
    float amplitude_to_length,
    float parallelism_to_wind,
    float width_k,
    const Point4 &waves_dist,
    float depth_min,
    float depth_fade_interval,
    float gerstner_speed,
    float rivers_wave_multiplier);
  bool getNeedShore(const FFTWater *water) const;
  void updateShore(FFTWater *water, int32_t water_quality, const DPoint3 &cameraWorldPos);
  void closeShoreAndWaterFlowmap(FFTWater *water);

  TEXTUREID getDistanceFieldTexId() const { return shoreDistanceField.getTexId(); }

  bool isForcingWaterWaves = false;

private:
  void buildShore(const FFTWater *water);

  bool shoreEnabled = false;
  int shoreTextureSize = 1024;
  float shoreHmapSize = 1024.0f;
  float shoreRiversWidth = 200.0f;
  float shoreSignificantWaveThreshold = 0.1f;
  UniqueTexHolder shoreHeightmapTex;
  UniqueTexHolder shoreDistanceField;

  struct
  {
    bool generate_shore = false;
  } land_panel;
};
