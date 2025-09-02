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
  WinCritSec samplerResourcesMutex;
  typedef eastl::pair<SamplerState, SamplerResource *> SamplerResourcesDescPtrPair;
  Tab<SamplerResourcesDescPtrPair> samplerResources;

  SamplerCache(const SamplerCache &);
  SamplerCache &operator=(const SamplerCache &);
  SamplerResource *defaultSampler = nullptr;
  SamplerState defaultSamplerState = {};

public:
  SamplerCache() {}

  SamplerInfo create(SamplerState state);
  SamplerResource *getResource(SamplerState state);
  SamplerResource *getDefaultSampler() { return defaultSampler; }

  void init();
  void shutdown();
};

} // namespace drv3d_vulkan
