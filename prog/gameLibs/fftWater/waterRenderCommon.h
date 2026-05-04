// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/unique_ptr.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <drv/3d/dag_driver.h>
#include <generic/dag_smallTab.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_overrideStateId.h>

struct WaterRenderCommon
{
  struct SavedStates
  {
    TEXTUREID shoreDistanceFieldTexId = BAD_TEXTUREID;
  };

  void init();
  bool isInited() const { return inited; };

  void setGlobalShaderConsts(bool is_chop_water) const;

  WaterRenderCommon::SavedStates setCachedStates(TEXTUREID distanceTex) const;
  void resetCachedStates(const WaterRenderCommon::SavedStates &states) const;

  void setWakeHtTex(TEXTUREID wake_ht_tex) { wakeHtTexId = wake_ht_tex; }
  void setWaterLevel(float water_level) { waterLevel = water_level; }
  void setWind(float wind_dir_x, float wind_dir_y, float wind_speed);
  void setMaxWave(float max_wave_height);

  float getShoreWaveThreshold() const { return shoreWaveThreshold; }
  void setShoreWaveThreshold(float value);
  bool isShoreEnabled() const { return shoreEnabled; }
  void shoreEnable(bool enable);

  void setChopEnabled(bool chop_enabled) { chopEnabled = chop_enabled; }
  bool getChopEnabled() const { return chopEnabled; }

private:
  bool inited = false;
  bool chopEnabled = false;

  TEXTUREID wakeHtTexId = BAD_TEXTUREID;

  float waterLevel = 0.0f;
  float windDirX = 1.0f;
  float windDirY = 0.0f;
  float windSpeed = 0.0f;

  float maxWaveHeight = 0.0f;

  bool shoreEnabled = false;
  float shoreWaveThreshold = 0.4f;
};
