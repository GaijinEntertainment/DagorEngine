// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/adaptationSettingsEcs.h>
#include <render/exposureCompute.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/baseComponentTypes/objectType.h>
#include <math/dag_Point2.h>

void overrideAdaptationSetting(AdaptationSettings &settings, const ecs::Object &config)
{
#define READ_REAL(group, name) settings.name = config[ECS_HASH(group "__" #name)].getOr<float>(settings.name)
#define READ_BOOL(group, name) settings.name = config[ECS_HASH(group "__" #name)].getOr<bool>(settings.name)

  if (config.getMemberOr(ECS_HASH("adaptation__on"), settings.fixedExposure < 0))
  {
    settings.fixedExposure = -1;
    READ_BOOL("adaptation", useLuminanceTrimming);
    READ_BOOL("adaptation", includesHeroCockpit);
    READ_BOOL("adaptation", focusOnAim);
    settings.focusAimScale = config[ECS_HASH("adaptation__focusAimScale")].getOr<Point2>(settings.focusAimScale);
    READ_REAL("adaptation", minSamples);
    READ_REAL("adaptation", autoExposureScale);
    READ_REAL("adaptation", luminanceLowRange);
    READ_REAL("adaptation", luminanceHighRange);
    READ_REAL("adaptation", minExposure);
    READ_REAL("adaptation", maxExposure);
    READ_REAL("adaptation", adaptDownSpeed);
    READ_REAL("adaptation", adaptUpSpeed);
    READ_REAL("adaptation", brightnessPerceptionLinear);
    READ_REAL("adaptation", brightnessPerceptionPower);
    READ_REAL("adaptation", fadeMul);
    READ_REAL("adaptation", centerWeight);
    READ_REAL("adaptation", fnUniformSamplesPercentage);
    READ_REAL("adaptation", fnUniformDistributionRadius);
    READ_REAL("adaptation", fnNonLinearDistributionRadius);
    READ_REAL("adaptation", fnNonLinearDistributionCurvinesPow);
  }
  else
  {
    READ_REAL("adaptation", fixedExposure);
    if (settings.fixedExposure < 0)
      settings.fixedExposure = 1.0;
  }
#undef READ_REAL
#undef READ_BOOL
}
