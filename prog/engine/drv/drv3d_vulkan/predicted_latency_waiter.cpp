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
  // track average for external wait time to treat it as dynamic threshold if needed
  externalWaitHistoryIndex = (externalWaitHistoryIndex + 1) % AVG_ELEMS;
  externalWaitUsSum -= externalWaitUsHistory[externalWaitHistoryIndex];
  externalWaitUsHistory[externalWaitHistoryIndex] = external_waited_us;
  externalWaitUsSum += external_waited_us;

  // we can reach max wait time either because frame rate is too low
  // and we don't want to correct latency in this case, as prediction time correction step is considered small
  // or if we trying to move non-movable time into predicted wait
  // in both cases just consume external wait time as dynamic threshold
  // reducing inertion on frame rate on low frame rates and fixing up non-movable time block
  if (timeToWaitUs == Globals::cfg.latencyWaitMaxUs)
    dynamicThresholdUs = (externalWaitUsSum / AVG_ELEMS);
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
