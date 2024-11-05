// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/computeShaderFallback/voltexRenderer.h>
#include "shaders/clouds2/cloud_settings.hlsli"
#include "clouds2Common.h"

class CloudsShadows
{
public:
  void init();
  void invalidate() { resetGen = 0; }

  CloudsChangeFlags render(const Point3 &main_light_dir); // todo: use main_light_dir for next temporality

  void closeShadows2D() { cloudsShadows2d.close(); }
  void initShadows2D();

  UniqueTexHolder cloudsShadowsVol;

private:
  void validate();

  void addRecalcAll();
  void forceFullUpdate(const Point3 &main_light_dir, bool update_ambient);
  void renderShadows2D();

  void initTemporal();
  bool updateTemporal();
  void updateTemporalStep(int x, int y, int z);

  UniqueTexHolder cloudsShadowsTemp;
  UniqueTexHolder cloudsShadows2d;

  VoltexRenderer gen_cloud_shadows_volume_partial, copy_cloud_shadows_volume;
  VoltexRenderer genCloudShadowsVolume;
  PostFxRenderer build_shadows_2d_ps; // todo: implement compute version

  bool rerenderShadows2D = false;
  Point3 lastLightDir = {0, 0, 0};
  uint32_t resetGen = 0;
  bool updateAmbient = false;

  enum
  {
    CLOUDS_MAX_SHADOW_TEMPORAL_XZ = 8,
    CLOUDS_MAX_SHADOW_TEMPORAL_Y = 8,
    CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD = CLOUDS_MAX_SHADOW_TEMPORAL_XZ * CLOUD_SHADOWS_WARP_SIZE_XZ,
    CLOUDS_MAX_SHADOW_TEMPORAL_Y_WD = CLOUDS_MAX_SHADOW_TEMPORAL_Y * CLOUD_SHADOWS_WARP_SIZE_Y,
    CLOUDS_MAX_SHADOW_TEMPORAL_XZ_STEPS =
      (CLOUD_SHADOWS_VOLUME_RES_XZ + CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD - 1) / CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD,
    CLOUDS_MAX_SHADOW_TEMPORAL_Y_STEPS =
      (CLOUD_SHADOWS_VOLUME_RES_Y + CLOUDS_MAX_SHADOW_TEMPORAL_Y_WD - 1) / CLOUDS_MAX_SHADOW_TEMPORAL_Y_WD,
  };

  enum
  {
    TEMPORAL_STEPS = CLOUDS_MAX_SHADOW_TEMPORAL_XZ_STEPS * CLOUDS_MAX_SHADOW_TEMPORAL_XZ_STEPS * CLOUDS_MAX_SHADOW_TEMPORAL_Y_STEPS,
    TEMPORAL_STEPS_LERP = 1, // can be used for smoother update light
    ALL_TEMPORAL_STEPS_LERP = TEMPORAL_STEPS * TEMPORAL_STEPS_LERP,
    TEMPORAL_STEP_FORCED = ~0u,
  };
  uint32_t temporalStep = 0, temporalStepFinal = 0;
};
