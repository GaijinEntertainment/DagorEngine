//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace render::special_vision
{
enum
{
  OFF = 0,
  RENDER_NIGHT_VISION = 1,
  RENDER_THERMAL_VISION = 2,
  RENDER_TACVIEW = 3,
  RENDER_SONAR = 4,
  NV_NUM
};

constexpr float exposureThermal = 2.5f;
constexpr float exposureTacview = 1.0f;

void set_special_vision_shader_mode(bool on);

} // namespace render::special_vision
