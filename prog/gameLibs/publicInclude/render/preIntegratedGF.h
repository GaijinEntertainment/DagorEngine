//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_resPtr.h>

enum
{
  PREINTEGRATE_QUALITY_DEFAULT = 0,
  PREINTEGRATE_QUALITY_MAX = 2
};
enum
{
  PREINTEGRATE_SPECULAR_ONLY = 0,
  PREINTEGRATE_SPECULAR_DIFFUSE = 1,
  PREINTEGRATE_SPECULAR_DIFFUSE_QUALTY_MAX = PREINTEGRATE_SPECULAR_DIFFUSE | PREINTEGRATE_QUALITY_MAX,
};

TexPtr render_preintegrated_fresnel_GGX(const char *name = "preIntegratedGF", uint32_t preintegrate_type = PREINTEGRATE_SPECULAR_ONLY);
