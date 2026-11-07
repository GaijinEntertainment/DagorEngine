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


// every form: dead_zone in parts of deflection
inline void apply_stick_deadzone(float &inout_x, float &inout_y, float dead_zone)
{
  dead_zone = max(dead_zone, 0.f);
  float d = sqrtf(inout_x * inout_x + inout_y * inout_y);
  if (d <= dead_zone || dead_zone >= 1.f)
  {
    inout_x = inout_y = 0.f;
    return;
  }
  float s = min((d - dead_zone) / (1.f - dead_zone), 1.f) / d;
  inout_x *= s;
  inout_y *= s;
}


inline void apply_stick_deadzone(int &inout_x, int &inout_y, float dead_zone, int full_deflection)
{
  float x = float(inout_x) / full_deflection, y = float(inout_y) / full_deflection;
  apply_stick_deadzone(x, y, dead_zone);
  inout_x = (int)::roundf(x * full_deflection);
  inout_y = (int)::roundf(y * full_deflection);
}


inline void apply_stick_deadzone(uint8_t &inout_x, uint8_t &inout_y, float dead_zone)
{
  int x = inout_x - 0x80, y = inout_y - 0x80;
  apply_stick_deadzone(x, y, dead_zone, STICK_MAX_VAL);
  inout_x = x + 0x80;
  inout_y = y + 0x80;
}


struct LogicalAxis
{
  int *val = nullptr;
  float kMul = 1.f, kAdd = 0.f;
};

} // namespace HumanInput
