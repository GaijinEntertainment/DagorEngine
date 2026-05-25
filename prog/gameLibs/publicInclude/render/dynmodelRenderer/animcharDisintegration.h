//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point4.h>

namespace animchar_disintegration
{

struct DisintegrationParameters
{
  float duration = 0;
  float scale = 1;
};

inline Point4 get_additional_data(float animation_time, const DisintegrationParameters &params)
{
  float progress = params.duration > 0 ? clamp(0.f, 1.f, animation_time / params.duration) : 0.f;
  return Point4(progress, params.scale, 0, 0);
}

} // namespace animchar_disintegration