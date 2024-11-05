// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/genericLUT/fullColorGradingLUT.h>

FullColorGradingTonemapLUT::FullColorGradingTonemapLUT(bool need_hdr, int size) : _hasHDR(need_hdr)
{
  lutBuilder.init("full_tonemap_lut", "render_full_tonemap", "compute_full_tonemap",
    need_hdr ? GenericTonemapLUT::HDROutput::HDR : GenericTonemapLUT::HDROutput::LDR, size);
}