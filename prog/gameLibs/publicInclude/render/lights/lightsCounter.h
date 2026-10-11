//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/lights/lightsBase.h>

class LightsVisibilityChecker;
class OmniLightsManager;
class SpotLightsManager;

class LightsCounter final
{
public:
  LightsCounter(OmniLightsManager *omni_lights_manager, SpotLightsManager *spot_lights_manager);

  void update(const LightsVisibilityChecker &visibility_checker);
  void reset();

  uint32_t getClusteredLightsCount(LightType light_type) const;
  uint32_t getFarLightsCount(LightType light_type) const;

  DynLightsOptimizationMode getLightsCountInterval() const;

private:
  struct Counts
  {
    uint32_t clustered = 0;
    uint32_t far = 0;
  };

  static Counts countLights(LightType light_type, int max_index, uint32_t max_clustered_count,
    const LightsVisibilityChecker &visibility_checker);

  const Counts &getCounts(LightType light_type) const;

  OmniLightsManager *omniLightsManager;
  SpotLightsManager *spotLightsManager;

  Counts omniCounts;
  Counts spotCounts;
};
