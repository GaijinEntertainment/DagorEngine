// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// direct interop with backend logic via low-level primitives
#include <atomic>
#include <osApiWrappers/dag_critSec.h>
#include "fence_manager.h"
#include <util/dag_string.h>

class WinCritSec;

namespace drv3d_vulkan
{

class ThreadedFence;

struct BackendInterop
{
  // id of replay work that was completed on GPU last time, biased by 1 to avoid first frame issues
  struct LastGPUCompleted
  {
    static constexpr size_t BIAS = 1;
    std::atomic<size_t> id{0};
    void store(size_t v) { id.store(v + BIAS, std::memory_order_release); }
  } lastGPUCompleted{};
  // sync tracker capture request and status
  std::atomic<size_t> syncCaptureRequest{0};
  // toggle multi queue submit support in realtime
  std::atomic<size_t> toggleMultiQueueSubmit{0};
  // change barrier merge logic in realtime
  std::atomic<size_t> barrierMergeMode{0};
  // sync tracker capture resource name filter
  String syncCaptureNameFilter;
  // signalizes device lost from backend
  std::atomic<bool> deviceLost{false};
  // shows that backend is busy with pipeline compilation
  std::atomic<bool> blockingPipelineCompilation{false};
  // out-of-arch passing of primary swapchain vsync status for streamline workaround
  std::atomic<bool> isVsyncOnPrimarySwapchain{false};


  struct PendingGPUWork
  {
    WinCritSec cs;
    size_t replayId;
    ThreadedFence *gpuFence;
  };

  PendingGPUWork pendingGpuWork{};
};


} // namespace drv3d_vulkan
