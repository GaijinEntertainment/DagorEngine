//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_enumBitMask.h>

enum class ShadowCastersFlag
{
  None = 0,
  Dynamic = 1 << 0,
  ApproximateStatic = 1 << 1,
  TwoSided = 1 << 2, // only for dynamic lights (static lights should be placed carefully to not rely on it)
};
DAGOR_ENABLE_ENUM_BITMASK(ShadowCastersFlag);
