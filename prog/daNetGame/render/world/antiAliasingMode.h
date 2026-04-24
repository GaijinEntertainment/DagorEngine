// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

enum class AntiAliasingMode
{
  OFF,
  FXAA,
  TSR,
  DLSS,
  XESS,
  FSR,
  SSAA,
#if _TARGET_C2

#endif
};

enum AntiAliasingType
{
  TEMPORAL = 0,
  TSR = 1,
  NON_TEMPORAL = 2
};