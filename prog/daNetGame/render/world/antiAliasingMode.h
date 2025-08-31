// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

enum class AntiAliasingMode
{
  OFF = 0,
  FXAA = 1,
  TAA = 2, // for backwards compatibility, dont use
  TSR = 3,
  DLSS = 4,
  MSAA = 5,
  XESS = 6,
  FSR2 = 7,
  SSAA = 8,
  PSSR = 9
};

// TODO rename these values, and refactor the dof handling
enum DofAntiAliasingType
{
  AA_TYPE_DLSS = 0,
  AA_TYPE_TAA = 1,
  AA_TYPE_TSR = 2
};