// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <types/halfFloatSimple.h>

#include <debug/dag_assert.h>

const uint32_t signMask = 0x80000000U;
const uint32_t exponentMask = 0x7F800000U;
const uint32_t fractionMask = 0x7FFFFFU;
const int float_fraction_size = 23;
const int float_bias = 127;

inline uint16_t calc_fraction_size(bool positive, int exponent_size) { return 16 - exponent_size - (positive ? 0 : 1); }

inline int calc_max_exp(int exponent_size) { return (1 << exponent_size) - 1; }

float half_float_ex_decode(uint16_t v, bool positive, int exponent_size, int bias)
{
  int fractionSize = calc_fraction_size(positive, exponent_size);
  int maxExp = calc_max_exp(exponent_size);

  int exponent = int((positive ? v : (v & 0x7FFFU)) >> fractionSize);
  bool isNan = exponent >= maxExp;
  uint32_t exponentPart = exponent <= 0 ? 0U : (isNan ? exponentMask : ((exponent - bias + float_bias) << float_fraction_size));
  uint32_t signPart = !positive && (v & 0x8000U) ? signMask : 0;
  uint32_t fractionPart = isNan ? fractionMask : (uint32_t(v) << (float_fraction_size - fractionSize)) & fractionMask;

  union
  {
    float f;
    uint32_t i;
  } flt;
  flt.i = signPart + exponentPart + fractionPart;

  return flt.f;
}

uint16_t half_float_ex_encode(float v, bool positive, int exponent_size, int bias)
{
  G_ASSERT_AND_DO(!positive || v >= 0.f, v = 0.f);

  int fractionSize = calc_fraction_size(positive, exponent_size);
  int maxExp = calc_max_exp(exponent_size);

  union
  {
    float f;
    uint32_t i;
  } flt;
  flt.f = v;

  int exponent = ((flt.i & exponentMask) >> float_fraction_size) - float_bias + bias;
  uint32_t exponentPart = clamp(exponent, 0, maxExp) << fractionSize;
  uint32_t signPart = positive ? 0 : ((flt.i & signMask) >> 16);
  uint32_t fractionPart = exponent >= 0 ? ((flt.i & fractionMask) >> (float_fraction_size - fractionSize)) : 0U;

  return signPart + exponentPart + fractionPart;
}
