// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/lights/lightsCounter.h>
#include <render/lights/lightsPartition.h>
#include <perfMon/dag_statDrv.h>
#include <debug/dag_assert.h>
#include <util/dag_globDef.h>
#include <math/dag_hlsl_floatx.h>
#include <render/lights/renderLights.hlsli>

LightsCounter::LightsCounter(OmniLightsManager *omni_lights_manager, SpotLightsManager *spot_lights_manager) :
  omniLightsManager(omni_lights_manager), spotLightsManager(spot_lights_manager)
{}

LightsCounter::Counts LightsCounter::countLights(LightType light_type, int max_index, uint32_t max_clustered_count,
  const LightsVisibilityChecker &visibility_checker)
{
  Counts counts;
  for (int i = 0; i <= max_index; ++i)
  {
    switch (visibility_checker.test(light_type, i))
    {
      case LightsVisibilityChecker::TestResult::Clustered: ++counts.clustered; break;
      case LightsVisibilityChecker::TestResult::Far: ++counts.far; break;
      case LightsVisibilityChecker::TestResult::Invisible: break;
      default:
        G_ASSERT_FAIL("LightsCounter::countLights - unknown test result for light of type '%d' and id '%d'",
          static_cast<uint32_t>(light_type), i);
        break;
    }
  }

  if (counts.clustered > max_clustered_count)
  {
    counts.far += counts.clustered - max_clustered_count;
    counts.clustered = max_clustered_count;
  }
  counts.far = min<uint32_t>(counts.far, MAX_VISIBLE_FAR_LIGHTS);

  return counts;
}

void LightsCounter::update(const LightsVisibilityChecker &visibility_checker)
{
  TIME_D3D_PROFILE(lightsCounter_update);

  omniCounts = countLights(LightType::Omni, omniLightsManager->maxIndex(), MAX_CLUSTERED_OMNI_LIGHTS, visibility_checker);
  spotCounts = countLights(LightType::Spot, spotLightsManager->maxIndex(), MAX_CLUSTERED_SPOT_LIGHTS, visibility_checker);
}

void LightsCounter::reset()
{
  omniCounts = Counts{};
  spotCounts = Counts{};
}

const LightsCounter::Counts &LightsCounter::getCounts(LightType light_type) const
{
  switch (light_type)
  {
    case LightType::Omni: return omniCounts;
    case LightType::Spot: return spotCounts;
    default: G_ASSERT_FAIL("LightsCounter::getCounts - not supported light type"); return omniCounts;
  }
}

uint32_t LightsCounter::getClusteredLightsCount(LightType light_type) const { return getCounts(light_type).clustered; }

uint32_t LightsCounter::getFarLightsCount(LightType light_type) const { return getCounts(light_type).far; }

DynLightsOptimizationMode LightsCounter::getLightsCountInterval() const
{
  const uint32_t omniCount = getClusteredLightsCount(LightType::Omni);
  const uint32_t spotsCount = getClusteredLightsCount(LightType::Spot);

  if (spotsCount == 0 && omniCount == 0)
    return DynLightsOptimizationMode::NO_LIGHTS;

  if (spotsCount == 0 && omniCount <= LIGHTS_OPTIMIZATION_THRESHOLD)
    return DynLightsOptimizationMode::NO_SPOTS_FEW_OMNI;
  else if (spotsCount <= LIGHTS_OPTIMIZATION_THRESHOLD && omniCount == 0)
    return DynLightsOptimizationMode::FEW_SPOTS_NO_OMNI;
  else if (spotsCount <= LIGHTS_OPTIMIZATION_THRESHOLD && omniCount <= LIGHTS_OPTIMIZATION_THRESHOLD)
    return DynLightsOptimizationMode::FEW_SPOTS_FEW_OMNI;
  return DynLightsOptimizationMode::FULL_CLUSTERED;
}
