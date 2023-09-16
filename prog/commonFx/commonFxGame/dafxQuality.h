#pragma once

template <typename T>
uint32_t fx_apply_quality_bits(const T &par, uint32_t src_flags)
{
  uint32_t dst = 0;
  if (par.low_quality)
    dst |= 1 << 0;
  if (par.medium_quality)
    dst |= 1 << 1;
  if (par.high_quality)
    dst |= 1 << 2;

  return dst & src_flags;
}