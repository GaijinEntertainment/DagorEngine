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

// inspired by https://fgiesen.wordpress.com/2012/03/28/half-to-float-done-quic/
static uint32_t half_to_float_uint32_t(uint16_t h)
{
  union FP32
  {
    uint32_t u;
    float f;
  } o;
  static const FP32 magic = {(254 - 15) << 23};
  static const FP32 was_infnan = {(127 + 16) << 23};

  o.u = (h & 0x7fff) << 13; // exponent/mantissa bits
  o.f *= magic.f;           // exponent adjust
  if (o.f >= was_infnan.f)  // make sure Inf/NaN survive
    o.u |= 255 << 23;
  o.u |= (h & 0x8000) << 16; // sign bit
  return o.u;
}

static inline float half_to_float(uint16_t h)
{
  union
  {
    uint32_t i;
    float f;
  } u;
  u.i = half_to_float_uint32_t(h);
  return u.f;
}

static inline uint16_t float_to_half(float f)
{
  union
  {
    uint32_t i;
    float f;
  } u;
  u.f = f;
  return half_from_float(u.i);
}

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
  union
  {
    uint32_t i;
    float f;
  } u;
  u.f = f;
  return uint32_t_float_to_half_unsafe(u.i);
};

// do not handle inf/nan
static uint32_t half_to_float_uint32_t_unsafe(uint16_t h) // fast only on x64
{
  union FP32
  {
    uint32_t u;
    float f;
  } o;
  static const FP32 magic = {(254 - 15) << 23};

  o.u = (h & 0x7fff) << 13;  // exponent/mantissa bits
  o.f *= magic.f;            // exponent adjust
  o.u |= (h & 0x8000) << 16; // sign bit
  return o.u;
}

static inline float half_to_float_unsafe(uint16_t h) // not checking corner cases, like NANs, infs, etc
{
  union
  {
    uint32_t i;
    float f;
  } u;
  u.i = half_to_float_uint32_t_unsafe(h);
  return u.f;
}
