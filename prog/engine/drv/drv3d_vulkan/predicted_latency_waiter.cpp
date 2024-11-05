// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "predicted_latency_waiter.h"
#include <perfMon/dag_sleepPrecise.h>
#include <perfMon/dag_statDrv.h>
#include "globals.h"
#include "driver_config.h"
#include "util/scoped_timer.h"

using namespace drv3d_vulkan;

int64_t PredictedLatencyWaiter::getLastWaitTimeUs() { return profile_usec_from_ticks_delta(lastWaitTicks); }

void PredictedLatencyWaiter::updateDynamicThreshold(uint64_t external_waited_us)
{
  // track average for external wait time and treat it as dynamic threshold
  externalWaitHistoryIndex = (externalWaitHistoryIndex + 1) % AVG_ELEMS;
  externalWaitUsSum -= externalWaitUsHistory[externalWaitHistoryIndex];
  externalWaitUsHistory[externalWaitHistoryIndex] = external_waited_us;
  externalWaitUsSum += external_waited_us;
  dynamicThresholdUs = (externalWaitUsSum / AVG_ELEMS);
  // reduce threshold from pure average
  // this allows feedback to loop around stable point in order to avoid latching on pure average
  if (dynamicThresholdUs > Globals::cfg.latencyWaitDeltaUs)
    dynamicThresholdUs -= Globals::cfg.latencyWaitDeltaUs;
}

void PredictedLatencyWaiter::update(int64_t external_waited_ticks)
{
  int64_t waitDurUs = profile_usec_from_ticks_delta(external_waited_ticks);
  updateDynamicThreshold(waitDurUs);

  int thresholdUs =
    dynamicThresholdUs > Globals::cfg.latencyWaitThresholdUs ? dynamicThresholdUs : Globals::cfg.latencyWaitThresholdUs;
  timeToWaitUs += (waitDurUs > thresholdUs ? 1 : -1) * Globals::cfg.latencyWaitDeltaUs;
  if (timeToWaitUs < 0)
    timeToWaitUs = 0;
  else if (timeToWaitUs > Globals::cfg.latencyWaitMaxUs)
    timeToWaitUs = Globals::cfg.latencyWaitMaxUs;
}

void PredictedLatencyWaiter::wait(int64_t time_to_wait_us)
{
  if (time_to_wait_us > 0)
  {
    // wait for previous frame completion on GPU to reduce latency in GPU/worker bound scenarions
    DA_PROFILE_AUTO_WAIT();
    ScopedTimerTicks watch(lastWaitTicks);
    sleep_precise_usec(time_to_wait_us);
  }
}

void PredictedLatencyWaiter::wait()
{
  // usuall predicted wait
  wait(timeToWaitUs);
}

void PredictedLatencyWaiter::markAsyncWait()
{
  if (asyncWaitStartTicks != 0)
    wait();
  asyncWaitStartTicks = profile_ref_ticks();
}

void PredictedLatencyWaiter::asyncWait()
{
  int64_t correctedWaitTimeUs = timeToWaitUs - profile_time_usec(asyncWaitStartTicks);
  asyncWaitStartTicks = 0;
  wait(correctedWaitTimeUs);
}
