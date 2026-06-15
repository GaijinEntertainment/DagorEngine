//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/exposureCompute.h>

void draw_adaptation_imgui(const uint32_t *hist256, const ExposureBuffer &exposure_buffer, const AdaptationSettings &settings,
  bool *draw_samples_distribution = nullptr);
