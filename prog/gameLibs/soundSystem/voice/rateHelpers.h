// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math.h>

namespace voicechat
{

inline unsigned re_rate(unsigned value, unsigned rate_in, unsigned rate_out)
{
  unsigned quot = value / rate_in;
  unsigned rem = value % rate_in;
  return rem * rate_out / rate_in + quot * rate_out;
}

inline int samples_to_ms(int samples, unsigned rate) { return ceilf(samples * 1000.0f / rate); }
inline int ms_to_samples(int ms, unsigned rate) { return rate * ms / 1000; }

} // namespace voicechat
