// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stdint.h>

class LightfxShadowParams;

union LightShadowParams
{
  struct
  {
    uint32_t isEnabled : 1;
    uint32_t isDynamic : 1;
    uint32_t supportsDynamicObjects : 1;
    uint32_t approximateStatic : 1;
    uint32_t supportsGpuObjects : 1;
    uint32_t quality : 10;
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
  LightShadowParams(bool is_enabled,
    DynamicLight is_dynamic,
    DynamicCasters supports_dynamic_objects,
    GpuObjects supports_gpu_objects,
    ApproximateStatic approx,
    uint16_t shadow_quality,
    uint8_t shadow_shrink,
    uint8_t light_priority) :
    isEnabled(is_enabled),
    isDynamic(bool(is_dynamic)),
    supportsDynamicObjects(bool(supports_dynamic_objects)),
    supportsGpuObjects(bool(supports_gpu_objects)),
    approximateStatic(bool(approx)),
    quality(shadow_quality),
    shadowShrink(shadow_shrink),
    priority(light_priority)
  {}
  LightShadowParams() = default;

  LightShadowParams(const LightfxShadowParams &shadowParams);
};