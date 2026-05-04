// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>

class BaseTexture;

namespace drv3d_dx11
{

// global driver state
struct DriverState
{
  BaseTexture *backBufferColorTex;
  uint16_t width;
  uint16_t height;

  int msaaMode;
  int msaaSamples;
  int msaaQuality;
  bool msaaEnabled;

  bool createSurfaces(uint32_t screenWidth, uint32_t screenHeight);
};

} // namespace drv3d_dx11
