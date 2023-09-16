//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_texMgr.h>

namespace fps_profile
{
struct PerfRating
{
  int flags;
  int frames;
};

enum PerfRatingDims
{
  PERF_RATING_PERIODS = 7,
  PERF_RATING_GROUPS = 6,
  IMAGE_W = PERF_RATING_GROUPS,
  IMAGE_H = PERF_RATING_PERIODS
};

TEXTUREID getTextureID();
void updatePerformanceRating(int frameLimitationFlags);
PerfRating getPerformanceRating(int period, int group);
void shutdown();
void initPerfProfile();
}; // namespace fps_profile
