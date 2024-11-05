// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/lightShadowParams.h>

#include <generic/dag_smallTab.h>
#include <math/dag_Point3.h>
#include <dafxCompound_decl.h>

LightShadowParams::LightShadowParams(const LightfxShadowParams &shadowParams) :
  isEnabled(shadowParams.enabled),
  isDynamic(shadowParams.is_dynamic_light),
  supportsDynamicObjects(shadowParams.shadows_for_dynamic_objects),
  supportsGpuObjects(shadowParams.shadows_for_gpu_objects),
  quality(shadowParams.quality),
  shadowShrink(shadowParams.shrink),
  priority(shadowParams.priority)
{}