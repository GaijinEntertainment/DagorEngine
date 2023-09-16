//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once
#include <stdint.h>

enum ObstacleFlags : uint32_t
{
  NONE = 0U,
  CROSS_WITH_JL = (1U << 0),
  DISABLE_JL_AROUND = (1U << 1)
};