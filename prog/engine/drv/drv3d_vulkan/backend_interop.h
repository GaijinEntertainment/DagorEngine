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
  // sync tracker capture request and status
  std::atomic<size_t> syncCaptureRequest{0};
  // toggle multi queue submit support in realtime
  std::atomic<size_t> toggleMultiQueueSubmit{0};
};


} // namespace drv3d_vulkan
