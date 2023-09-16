//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_stdint.h>
#include <math/dag_mathBase.h>
#include <debug/dag_assert.h>

/// simple half float implementation with custom mantissa/exponent/bias
/// without mathematics, only for storing/serialization

const int min_exponent_size = 3;
const int max_exponent_size = 8;

float half_float_ex_decode(uint16_t v, bool positive, int exponent_size, int bias);
uint16_t half_float_ex_encode(float v, bool positive, int exponent_size, int bias);

template <int EXPONENT_SIZE, int EXPONENT_BIAS>
struct HalfFloatSimpleEx
{
  uint16_t val;

  HalfFloatSimpleEx() = default;

  HalfFloatSimpleEx(const HalfFloatSimpleEx &other) : val(other.val) {}

  HalfFloatSimpleEx(float v)
  {
    G_STATIC_ASSERT(EXPONENT_SIZE >= min_exponent_size && EXPONENT_SIZE <= max_exponent_size);
    G_STATIC_ASSERT(EXPONENT_BIAS >= 1 && EXPONENT_BIAS < (1 << EXPONENT_SIZE));
    val = half_float_ex_encode(v, false, EXPONENT_SIZE, EXPONENT_BIAS);
  }

  HalfFloatSimpleEx &operator=(const HalfFloatSimpleEx &other) = default;

  HalfFloatSimpleEx &operator=(float v)
  {
    val = half_float_ex_encode(v, false, EXPONENT_SIZE, EXPONENT_BIAS);
    return *this;
  }

  operator float() const { return half_float_ex_decode(val, false, EXPONENT_SIZE, EXPONENT_BIAS); }

  bool operator==(const HalfFloatSimpleEx &other) const { return val == other.val; }
};

template <int EXPONENT_SIZE, int EXPONENT_BIAS>
struct HalfFloatSimplePositiveEx
{
  uint16_t val;

  HalfFloatSimplePositiveEx() = default;

  HalfFloatSimplePositiveEx(const HalfFloatSimplePositiveEx &other) : val(other.val) {}

  HalfFloatSimplePositiveEx(float v)
  {
    G_STATIC_ASSERT(EXPONENT_SIZE >= min_exponent_size && EXPONENT_SIZE <= max_exponent_size);
    G_STATIC_ASSERT(EXPONENT_BIAS >= 0 && EXPONENT_BIAS < (1 << EXPONENT_SIZE));
    val = half_float_ex_encode(v, true, EXPONENT_SIZE, EXPONENT_BIAS);
  }

  HalfFloatSimplePositiveEx &operator=(const HalfFloatSimplePositiveEx &other) = default;

  HalfFloatSimplePositiveEx &operator=(float v)
  {
    val = half_float_ex_encode(v, true, EXPONENT_SIZE, EXPONENT_BIAS);
    return *this;
  }

  operator float() const { return half_float_ex_decode(val, true, EXPONENT_SIZE, EXPONENT_BIAS); }

  bool operator==(const HalfFloatSimplePositiveEx &other) const { return val == other.val; }
};

using HalfFloatSimplePositive = HalfFloatSimplePositiveEx<5, 15>;
