// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include <atomic>
#include "backend.h"
#include "backend_interop.h"

namespace drv3d_vulkan
{

class AsyncCompletionState
{
  size_t pendingForReplayId = 0;
  bool requested = false;

public:
  AsyncCompletionState() = default;
  ~AsyncCompletionState() = default;

  AsyncCompletionState(const AsyncCompletionState &) = delete;
  AsyncCompletionState &operator=(const AsyncCompletionState &) = delete;


  bool isRequested() const { return requested; }

  bool isCompletedOrNotRequested() const
  {
    if (!requested)
      return true;
    return pendingForReplayId <= Backend::interop.lastGPUCompletedReplayWorkId.load(std::memory_order_acquire);
  }

  bool isCompleted() const
  {
    G_ASSERTF(requested, "vulkan: AsyncCompletionState %p queried without request!", this);
    return pendingForReplayId <= Backend::interop.lastGPUCompletedReplayWorkId.load(std::memory_order_acquire);
  }

  void reset() { requested = false; }
  void request(size_t cur_replay_id)
  {
    G_ASSERTF(!requested, "vulkan: reused AsyncCompletionState %p without reset!", this);
    pendingForReplayId = cur_replay_id;
    requested = true;
  }
};

} // namespace drv3d_vulkan