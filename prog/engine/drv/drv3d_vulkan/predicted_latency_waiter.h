// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// class to track amount of waits we have on end of operations
// to execute this waits in predicted manner before we block on them
// thus reducing input latency
#include <util/dag_stdint.h>

namespace drv3d_vulkan
{

struct PredictedLatencyWaiter
{
  int64_t timeToWaitUs = 0;
  int64_t lastWaitTicks = 0;
  uint64_t asyncWaitStartTicks = 0;
  int64_t dynamicThresholdUs = 0;
  int64_t externalWaitUsSum = 0;
  static constexpr int AVG_ELEMS = 32;
  int64_t externalWaitUsHistory[AVG_ELEMS] = {};
  int externalWaitHistoryIndex = 0;

  int64_t getLastWaitTimeUs();

private:
  // executes wait for needed time with internal tracking
  void wait(int64_t time_to_wait_us);
  void updateDynamicThreshold(uint64_t external_waited_us);

public:
  // update prediction by external signal i.e. wait time
  void update(int64_t external_waited_ticks);
  // executes predicted wait
  void wait();
  // set timestamp at place where we should have waited
  // to properly wait at another place/thread, by subtracting passed time
  void markAsyncWait();
  // wait from another place/thread (from APP)
  // includes async wait mark timestamp correction
  void asyncWait();
};

} // namespace drv3d_vulkan
