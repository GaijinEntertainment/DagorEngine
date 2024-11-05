// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// direct interop with backend logic via low-level primitives
#include <atomic>

namespace drv3d_vulkan
{

struct BackendInterop
{
  // id of replay work that was completed on GPU last time
  std::atomic<size_t> lastGPUCompletedReplayWorkId{0};
};


} // namespace drv3d_vulkan
