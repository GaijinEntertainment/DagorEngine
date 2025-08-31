// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// timings of execution process

#include <drv/3d/dag_commands.h>
#include <util/dag_stdint.h>
#include "timeline_latency.h"

namespace drv3d_vulkan
{

struct BackExecutionTimings
{
  int64_t gpuWaitDuration = 0;
  int64_t workWaitDuration = 0;
  int64_t presentWaitDuration = 0;
  int64_t lastMemoryStatTime = 0;
};

struct FrontExecutionTimings
{
  Drv3dTimings history[FRAME_TIMING_HISTORY_LENGTH]{};
  uint32_t completedFrameIndex = 0;
  int64_t lastPresentTimeStamp = 0;
  int64_t frontendBackendWaitDuration = 0;
  int64_t acquireBackBufferDuration = 0;

  const Drv3dTimings &get(uintptr_t offset) const { return history[(completedFrameIndex - offset) % FRAME_TIMING_HISTORY_LENGTH]; }

  Drv3dTimings &next(uint32_t frame_index, uint64_t present_time_stamp)
  {
    auto &ret = history[frame_index % FRAME_TIMING_HISTORY_LENGTH];

    ret.frontendUpdateScreenInterval = present_time_stamp - lastPresentTimeStamp;
    ret.frontendBackendWaitDuration = frontendBackendWaitDuration;
    lastPresentTimeStamp = present_time_stamp;

    // advance to surely finished frame index
    // A queued, B in thread, C on GPU, D is done
    completedFrameIndex = (uint32_t)TimelineLatency::getLastCompletedReplay(frame_index);
    return ret;
  }

  void init(uint64_t init_timestamp) { lastPresentTimeStamp = init_timestamp; }
};

} // namespace drv3d_vulkan
