// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>

void add_static_flare(const TMatrix &tm,
  E3DCOLOR color,
  float size,
  float fade_distance_from,
  float fade_distance_to,
  float horz_dir_deg,
  float horz_delta_deg,
  float vert_dir_deg,
  float vert_delta_deg);