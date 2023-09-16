//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include "commonLUT.h"

// ColorGradingLUT implements white balance and color grading with
// gameLibs/render/shaders/tonemapHelpers/makeGradedColorLUT.sh
// in addition it (should) convert for required space/gamut (HDR/SDR output, etc)
class ColorGradingLUT : public CommonLUT
{
public:
  ColorGradingLUT(bool need_hdr);
};
