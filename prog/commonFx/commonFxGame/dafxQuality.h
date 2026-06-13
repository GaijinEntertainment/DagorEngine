// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "dafx_quality_flags.hlsli"

template <typename T>
uint32_t fx_apply_quality_bits(const T &par, uint32_t src_flags)
{
  uint32_t dst = 0;
  if (par.low_quality)
    dst |= DAFX_LOW_FX_QUALITY;
  if (par.medium_quality)
    dst |= DAFX_MEDIUM_FX_QUALITY;
  if (par.high_quality)
    dst |= DAFX_HIGH_FX_QUALITY;

  return dst & src_flags;
}