//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace haptic
{
struct event_t
{
  float time_s;
  float intensity;
  float sharpness;
  float duration;
  float attack;
  float release;
};

//! find by name or create new haptic pattern from sequence of events
int registerHapticPattern(const char *patternName, const event_t *events, int eventsCount);
//! play pattern registered earlier
void playHapticPattern(int patternId, float intensityMultiplier = 1.0f);
//! set global intensity multiplier in [0, 1] range
void setGlobalIntensity(float multiplier);
float getGlobalIntensity();
} // namespace haptic
