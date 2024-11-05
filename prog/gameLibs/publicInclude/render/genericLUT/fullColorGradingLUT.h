//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "commonLUT.h"

// FullColorGradingTonemapLUT implements full color grading tonemap with
// gameLibs/render/shaders/tonemapHelpers/makeFullTonemapLUT.sh
// in addition it (should) convert for required space/gamut (HDR/SDR output, etc)
// it should be used as one and only source of tonemap, as almost everything can be implemented with one LUT lookup
// if you need some specific tonemap in your shader - noproblem, use per_project shader overload.
class FullColorGradingTonemapLUT : public CommonLUT
{
public:
  FullColorGradingTonemapLUT(bool need_hdr, int size = 32);
  bool hasHDR() const { return _hasHDR; }

private:
  bool _hasHDR;
};
