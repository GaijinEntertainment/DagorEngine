//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once
#include <stdint.h>

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
} // namespace rumble
}; // namespace force_feedback
