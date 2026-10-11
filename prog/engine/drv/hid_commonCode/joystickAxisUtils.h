// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include <math/dag_mathUtils.h>

namespace HumanInput
{

static constexpr int MIN_AXIS_VAL = -32000;
static constexpr int MAX_AXIS_VAL = 32000;
static constexpr int STICK_MAX_VAL = 0x7f;

inline uint32_t sel_vbtn(int v, uint32_t lo, uint32_t hi)
{
  const int threshold = MAX_AXIS_VAL / 2;
  return (v > threshold) ? hi : ((v < -threshold) ? lo : 0);
}


inline int remap_axis(uint16_t value)
{
  // 0000..ffff -> -32000..+32000
  return (value - 0x8000) * HumanInput::MAX_AXIS_VAL / 32767;
}


inline int remap_axis(uint8_t value)
{
  // 01..ff -> -32000..+32000, 00 overshoots to -32251
  return (value - 0x80) * HumanInput::MAX_AXIS_VAL / STICK_MAX_VAL;
}


inline int remap_trigger(uint8_t value)
{
  // 00..ff -> 0..+32000
  return value * HumanInput::MAX_AXIS_VAL / 255;
}


inline int remap_sensor(float value)
{
  //-1.0..1.0 -> -32000..+32000
  return clamp(int(value * HumanInput::MAX_AXIS_VAL), -HumanInput::MAX_AXIS_VAL, HumanInput::MAX_AXIS_VAL);
}


inline void apply_stick_deadzone(uint8_t &inout_x, uint8_t &inout_y, float deadzone)
{
  deadzone = clamp(deadzone, 0.f, STICK_MAX_VAL - 1.f);
  int x = inout_x - 0x80;
  int y = inout_y - 0x80;
  if (deadzone > 0)
  {
    int d2 = x * x + y * y;
    if (d2 < deadzone * deadzone)
      x = y = 0;
    else
    {
      float d = sqrtf(d2), s = float(STICK_MAX_VAL) / (STICK_MAX_VAL - deadzone);
      x = (int)::roundf((x - deadzone / d * x) * s);
      y = (int)::roundf((y - deadzone / d * y) * s);
    }
  }

  int d2 = x * x + y * y;
  if (d2 > STICK_MAX_VAL * STICK_MAX_VAL)
  {
    float d = sqrtf(d2);
    x = (int)::roundf(x * STICK_MAX_VAL / d);
    y = (int)::roundf(y * STICK_MAX_VAL / d);
  }
  inout_x = x + 0x80;
  inout_y = y + 0x80;
}


struct LogicalAxis
{
  int *val = nullptr;
  float kMul = 1.f, kAdd = 0.f;
};

} // namespace HumanInput
