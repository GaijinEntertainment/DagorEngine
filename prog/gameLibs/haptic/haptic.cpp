// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "haptic.h"

#include <perfMon/dag_statDrv.h>
#include <EASTL/string.h>
#include <EASTL/vector_map.h>
#include <math/dag_mathBase.h>

#if _TARGET_ANDROID
#include "android/haptic_vibration.h"
#elif _TARGET_IOS
#include "ios/haptic_vibration.h"
#endif

namespace haptic
{
eastl::vector_map<eastl::string, int> g_registeredPatterns;
float g_globalIntensityMultiplier = 1.0f;
} // namespace haptic

int haptic::registerHapticPattern(const char *patternName, const event_t events[], const int eventsCount)
{
  const auto it = g_registeredPatterns.find(patternName);
  if (it != g_registeredPatterns.end())
  {
    return it->second;
  }

  if (!eventsCount)
    return -1;

#if _TARGET_IOS
  const int patternId = haptic::registerHapticPattern_iOS(events, eventsCount);
#elif _TARGET_ANDROID
  const int patternId = haptic::registerHapticPattern_android(events, eventsCount);
#else  // _TARGET_IOS|_TARGET_ANDROID
  (void)events;
  (void)eventsCount;
  const int patternId = -1;
#endif // _TARGET_IOS|_TARGET_ANDROID

  g_registeredPatterns[patternName] = patternId;

  return patternId;
}

void haptic::playHapticPattern(const int patternId, float intensityMultiplier)
{
  TIME_PROFILE(playHapticPattern);
  intensityMultiplier *= g_globalIntensityMultiplier;
  if (!float_nonzero(intensityMultiplier))
    return;
#if _TARGET_IOS
  playHapticPattern_iOS(patternId, intensityMultiplier);
#elif _TARGET_ANDROID
  playHapticPattern_android(patternId, intensityMultiplier);
#else
  (void)patternId;
#endif
}

void haptic::setGlobalIntensity(const float multiplier) { g_globalIntensityMultiplier = multiplier; }

float haptic::getGlobalIntensity() { return g_globalIntensityMultiplier; }