//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>
#include <drv/hid/dag_hiJoyTriggerEffect.h>

namespace force_feedback
{
namespace rumble
{
struct EventParams
{
  enum Band : uint8_t
  {
    LO,
    HI
  } band = LO;
  float freq = 0;
  int durationMsec = 0;
  int startTimeMsec = 0;
  EventParams() = default;
  EventParams(Band _band, float _freq, int duration_msec) : band(_band), freq(_freq), durationMsec(duration_msec) {}
};

void init();
void shutdown();
void reset();

void add_event(const EventParams &evtp);
void set_scale(float scale);
} // namespace rumble

namespace trigger_effect
{
// programs the triggers of the default gamepad; a pad without adaptive triggers ignores it
void set(const HumanInput::TriggerEffectParam &param);
} // namespace trigger_effect
}; // namespace force_feedback
