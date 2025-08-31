// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// direct interop with backend logic via low-level primitives
#include <atomic>
#include <osApiWrappers/dag_critSec.h>
#include "fence_manager.h"

class WinCritSec;

namespace drv3d_vulkan
{

class ThreadedFence;

struct BackendInterop
{
  // id of replay work that was completed on GPU last time
  std::atomic<size_t> lastGPUCompletedReplayWorkId{0};
  // sync tracker capture request and status
  std::atomic<size_t> syncCaptureRequest{0};
  // toggle multi queue submit support in realtime
  std::atomic<size_t> toggleMultiQueueSubmit{0};
  // change barrier merge logic in realtime
  std::atomic<size_t> barrierMergeMode{0};

  struct PendingGPUWork
  {
    WinCritSec cs;
    size_t replayId;
    ThreadedFence *gpuFence;
  };

  PendingGPUWork pendingGpuWork{};
};


} // namespace drv3d_vulkan
