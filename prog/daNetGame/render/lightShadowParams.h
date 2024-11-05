// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stdint.h>

class LightfxShadowParams;

struct LightShadowParams
{
  uint32_t isEnabled : 1;
  uint32_t isDynamic : 1;
  uint32_t supportsDynamicObjects : 1;
  uint32_t supportsGpuObjects : 1;
  uint32_t quality : 10;
  uint32_t shadowShrink : 10;
  uint32_t priority : 8;

  LightShadowParams(bool is_enabled,
    bool is_dynamic,
    bool supports_dynamic_objects,
    bool supports_gpu_objects,
    unsigned int shadow_quality,
    unsigned int shadow_shrink,
    unsigned int light_priority) :
    isEnabled(is_enabled),
    isDynamic(is_dynamic),
    supportsDynamicObjects(supports_dynamic_objects),
    supportsGpuObjects(supports_gpu_objects),
    quality(shadow_quality),
    shadowShrink(shadow_shrink),
    priority(light_priority)
  {}

  LightShadowParams() : LightShadowParams(false, false, false, false, 0, 0, 0) {}

  LightShadowParams(const LightfxShadowParams &shadowParams);
};