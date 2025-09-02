// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/componentTypes.h>

class IPoint2;
class Point3;

void apply_lens_glow_settings(bool enabled, const IPoint2 &display_resolution, float lens_glow_config__bloom_offset,
  float lens_glow_config__bloom_multiplier, float lens_glow_config__lens_flare_offset, float lens_glow_config__lens_flare_multiplier,
  const ecs::string &texture_name, float lens_glow_config__texture_intensity_multiplier, const Point3 &lens_glow_config__texture_tint);

ecs::string pick_best_texture(const ecs::Array &texture_variants, const IPoint2 &display_resolution);