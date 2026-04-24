//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>

class LightfxShadowParams;

union LightShadowParams
{
  static constexpr bool FX_LIGHTS_IS_TWO_SIDED = false; // FX lights are never two-sided

  struct
  {
    uint32_t isEnabled : 1;
    uint32_t isDynamic : 1;
    uint32_t supportsDynamicObjects : 1;
    uint32_t approximateStatic : 1;
    uint32_t supportsGpuObjects : 1;
    uint32_t isTwoSided : 1;
    uint32_t quality : 9;
    uint32_t shadowShrink : 8;
    uint32_t priority : 8;
  };
  uint32_t u = 0;
  enum class DynamicLight
  {
    Off = 0,
    On = 1
  };
  enum class DynamicCasters
  {
    Off = 0,
    On = 1
  };
  enum class GpuObjects
  {
    Off = 0,
    On = 1
  };
  enum class ApproximateStatic
  {
    Off = 0,
    On = 1
  };
  enum class TwoSided
  {
    Off = 0,
    On = 1
  };
  LightShadowParams(bool is_enabled, DynamicLight is_dynamic, DynamicCasters supports_dynamic_objects, GpuObjects supports_gpu_objects,
    ApproximateStatic approx, TwoSided two_sided, uint16_t shadow_quality, uint8_t shadow_shrink, uint8_t light_priority) :
    isEnabled(is_enabled),
    isDynamic(bool(is_dynamic)),
    supportsDynamicObjects(bool(supports_dynamic_objects)),
    supportsGpuObjects(bool(supports_gpu_objects)),
    approximateStatic(bool(approx)),
    isTwoSided(bool(two_sided)),
    quality(shadow_quality),
    shadowShrink(shadow_shrink),
    priority(light_priority)
  {}
  LightShadowParams() = default;

  LightShadowParams(const LightfxShadowParams &shadowParams);
};