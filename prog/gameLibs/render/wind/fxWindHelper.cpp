// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_dataBlock.h>

namespace fxwindhelper
{
// See https://graphtoy.com/?f1(x,t)=6-log(0.5+exp(-x+6))&v1=true to understand the curve params
static float curve_falloff_start = 6.f;
static float curve_falloff_strength = 0.5f;

void load_fx_wind_curve_params(const DataBlock *game_params_blk)
{
  const DataBlock *fx_wind_curve_blk = game_params_blk->getBlockByNameEx("particleWindInfluenceCurve");
  curve_falloff_start = fx_wind_curve_blk->getReal("curveFalloffStart", 6.f);
  curve_falloff_strength = fx_wind_curve_blk->getReal("curveFalloffStrength", 0.5f);
}

// VFX artists want wind influence on fx to reach a plateau after a certain threshold. So that, for example, the particles behave very
// differently at beaufort 1 and beaufort 4, but after beaufort ~6 the change in wind speed shouldn't make much difference Actual
// numbers are defined by curve_... params
float apply_wind_strength_curve(float beaufort)
{
  return fmax(0.f, curve_falloff_start - logf(curve_falloff_strength + expf(-beaufort + curve_falloff_start)));
}
} // namespace fxwindhelper