// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/integer/dag_IPoint2.h>

class BaseTexture;

// the shader lives only in the debug shaderdump.
namespace render::pbr_validation
{
struct Settings
{
  int mode = 2; // 0 = albedo, 1 = smoothness, 2 = all
  float nonMetalMin = 0.04f;
  float nonMetalMax = 0.8f;
  float metalMin = 0.5f;
  float metalMax = 0.98f;
  float smoothnessMin = 0.05f;
  float smoothnessMax = 0.95f;
  float overlayOpacity = 0.7f;
  bool neutralBase = false;
  float neutralBaseLevel = 0.12f;
};

Settings &settings();
bool is_enabled();
void set_enabled(bool on);
void apply_settings();
bool bind_probe();
void after_render(BaseTexture *albedo_tex, BaseTexture *depth_tex, const IPoint2 &display_res);
void close();
} // namespace render::pbr_validation
