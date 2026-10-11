//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>

struct PixelShaderOutputInfo
{
  uint32_t colorTargetMask = 0;
  bool writesDepth = false;
  bool valid = false;
  uint16_t pad_ = 0;
};

static_assert(sizeof(PixelShaderOutputInfo) == 8);
