// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include <debug/dag_assert.h>
#include <atomic>
#include "backend.h"
#include "backend_interop.h"

namespace drv3d_vulkan
{

class AsyncCompletionState
{
  size_t pendingForBiasedReplayId = 0;
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
    return pendingForBiasedReplayId <= Backend::interop.lastGPUCompleted.id.load(std::memory_order_acquire);
  }

  bool isCompleted() const
  {
    G_ASSERTF(requested, "vulkan: AsyncCompletionState %p queried without request!", this);
    return pendingForBiasedReplayId <= Backend::interop.lastGPUCompleted.id.load(std::memory_order_acquire);
  }

  // saying other way - difference between provided replay id and replay id that will finish request
  // yet if it is not waited replay may not be completed immediately
  size_t getCompletionDelay(size_t cur_replay_id)
  {
    size_t biasedReplayId = cur_replay_id + Backend::interop.lastGPUCompleted.BIAS;
    return biasedReplayId < pendingForBiasedReplayId ? (pendingForBiasedReplayId - biasedReplayId) : 0;
  }

  bool isRequestedThisFrame(size_t cur_replay_id)
  {
    return pendingForBiasedReplayId == (cur_replay_id + Backend::interop.lastGPUCompleted.BIAS);
  }

  void reset() { requested = false; }
  void request(size_t cur_replay_id)
  {
    G_ASSERTF(!requested, "vulkan: reused AsyncCompletionState %p without reset!", this);
    pendingForBiasedReplayId = cur_replay_id + Backend::interop.lastGPUCompleted.BIAS;
    requested = true;
  }
};

} // namespace drv3d_vulkan