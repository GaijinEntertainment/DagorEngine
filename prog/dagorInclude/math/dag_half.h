//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#ifdef __cplusplus
extern "C"
{
#endif

  uint32_t half_to_float_uint32_t_ref(uint16_t h);
  uint16_t half_from_float(uint32_t f);

  uint16_t half_add(uint16_t arg0, uint16_t arg1);
  uint16_t half_mul(uint16_t arg0, uint16_t arg1);

#ifdef __cplusplus
}
#endif
#include <util/dag_bitwise_cast.h>

// inspired by https://fgiesen.wordpress.com/2012/03/28/half-to-float-done-quic/
static inline uint32_t half_to_float_uint32_t(uint16_t h)
{
  float f = bitwise_cast<float, uint32_t>(((h & 0x7fff) << 13)) * bitwise_cast<float, uint32_t>((254 - 15) << 23);
  uint32_t u = bitwise_cast<uint32_t, float>(f) | ((h & 0x8000) << 16);
  if (f >= bitwise_cast<float, uint32_t>((127 + 16) << 23)) // make sure Inf/NaN survive
    u |= 255 << 23;
  return u;
}

static inline float half_to_float(uint16_t h) { return bitwise_cast<float, uint32_t>(half_to_float_uint32_t(h)); }

static inline uint16_t float_to_half(float f) { return half_from_float(bitwise_cast<uint32_t, float>(f)); }

static inline uint16_t half_sub(uint16_t ha, uint16_t hb)
{
  // (a-b) is the same as (a+(-b))
  return half_add(ha, hb ^ 0x8000);
}


static inline uint16_t uint32_t_float_to_half_unsafe(uint32_t fltInt32) // not checking corner cases, like NANs, infs, etc
{
  uint16_t fltInt16 = (fltInt32 >> 31) << 5;
  uint16_t tmp = (fltInt32 >> 23) & 0xff;
  tmp = (tmp - 0x70) & (uint32_t((int)(0x70 - tmp) >> 4) >> 27);
  fltInt16 = (fltInt16 | tmp) << 10;
  fltInt16 |= (fltInt32 >> 13) & 0x3ff;

  return (uint16_t)fltInt16;
};

static inline uint16_t float_to_half_unsafe(float f) // not checking corner cases, like NANs, infs, etc
{
  return uint32_t_float_to_half_unsafe(bitwise_cast<uint32_t, float>(f));
};

// do not handle inf/nan

static inline float half_to_float_unsafe(uint16_t h) // not checking corner cases, like NANs, infs, etc
{
  return bitwise_cast<float, uint32_t>(((h & 0x7fff) << 13) | ((h & 0x8000) << 16)) * bitwise_cast<float, uint32_t>((254 - 15) << 23);
}

static inline uint32_t half_to_float_uint32_t_unsafe(uint16_t h) // fast only on x64
{
  return bitwise_cast<uint32_t, float>(half_to_float_unsafe(h));
}

static inline uint16_t float_to_half_denorm(float f)
{
  uint32_t fi = bitwise_cast<uint32_t, float>(f);

  uint32_t exp8 = (fi >> 23) & 0xFF;

  // Convert exp8  expN: subtract bias diff
  static constexpr uint32_t HALF_EXP_BITS = 5, HALF_MANTISSA_BITS = 10, HALF_EXP_BIAS = (1 << (HALF_EXP_BITS - 1)) - 1;

  static constexpr int IEEE_EXP_BIAS = 127, HALF_EXP_MAX = (1 << HALF_EXP_BITS) - 1;
  int expN = (int)exp8 - (IEEE_EXP_BIAS - HALF_EXP_BIAS);

  const uint32_t mant23 = (fi & 0x7FFFFF);
  const uint16_t sign16 = (fi & 0x80000000) >> 16;
  if (expN < 0)
  {
    int shift = 24 - HALF_MANTISSA_BITS - expN;
    return sign16 | ((mant23 | 0x800000UL) >> (shift < 31 ? shift : 31)); // if we would add 0x1000 to mantissa that would make it
                                                                          // round
  }
  else if (expN > HALF_EXP_MAX) // clamps
    return 0x7FFF | sign16;

  return sign16 | ((uint16_t)((expN << HALF_MANTISSA_BITS) | (mant23 >> (23 - HALF_MANTISSA_BITS))));
}

static inline float half_to_float_denorm(uint16_t h)
{
  static constexpr uint32_t HALF_EXP_BITS = 5, HALF_MANTISSA_BITS = 10, HALF_EXP_BIAS = (1 << (HALF_EXP_BITS - 1)) - 1;
  const uint16_t uh = h & 0x7FFF;
  const uint32_t hs = ((h & 0x8000) << 16);
  if (uh >> HALF_MANTISSA_BITS) // normal float
    return bitwise_cast<float, uint32_t>((uint32_t(uh) << (23 - HALF_MANTISSA_BITS)) | hs) *
           bitwise_cast<float, uint32_t>(((254 - HALF_EXP_BIAS) << 23));
  if (uh == 0)
    return 0.0f;
  static constexpr int IEEE_EXP_BIAS = 127;
  return bitwise_cast<float, uint32_t>(((IEEE_EXP_BIAS - HALF_EXP_BIAS) << 23) | (uh << (23 - HALF_MANTISSA_BITS)) | hs);
}
