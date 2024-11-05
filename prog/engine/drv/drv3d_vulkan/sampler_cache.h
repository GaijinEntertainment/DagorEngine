// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// sampler info-handle cache storage

#include <osApiWrappers/dag_critSec.h>
#include <generic/dag_tab.h>

#include "globals.h"
#include "driver.h"
#include "sampler_resource.h"

namespace drv3d_vulkan
{

class SamplerCache
{
private:
  // it is cached in texture, no need to double waste memory on lower bound here
  Tab<SamplerInfo *> samplers;

  WinCritSec samplerResourcesMutex;
  typedef eastl::pair<SamplerState, SamplerResource *> SamplerResourcesDescPtrPair;
  Tab<SamplerResourcesDescPtrPair> samplerResources;

private:
  SamplerCache(const SamplerCache &);
  SamplerCache &operator=(const SamplerCache &);

public:
  SamplerCache() {}
  SamplerInfo *get(SamplerState state);
  SamplerInfo create(SamplerState state);

  SamplerResource *getResource(SamplerState state);

  void shutdown();
};

} // namespace drv3d_vulkan
