// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "predicted_latency_waiter.h"
#include <perfMon/dag_statDrv.h>
#include "globals.h"
#include "driver_config.h"
#include "util/scoped_timer.h"
#include <osApiWrappers/dag_atomic.h>

using namespace drv3d_vulkan;

#if DAGOR_DBGLEVEL > 0
int64_t PredictedLatencyWaiter::getDbgLastExternalWaitTimeUs() { return interlocked_acquire_load(dbgLastExternalWaitUs); }
int64_t PredictedLatencyWaiter::getDbgLastWaitTimeUs() { return interlocked_acquire_load(dbgLastInternalWaitUs); }
#endif

int64_t PredictedLatencyWaiter::getLastWaitTimeUs() { return profile_usec_from_ticks_delta(lastWaitTicks); }
int64_t PredictedLatencyWaiter::getLastWaitTimeTicks() { return lastWaitTicks; }

void PredictedLatencyWaiter::updateDynamicThreshold(uint64_t external_waited_us)
{
  // track average for external wait time to treat it as dynamic threshold if needed
  externalWaitHistoryIndex = (externalWaitHistoryIndex + 1) % AVG_ELEMS;
  externalWaitUsSum -= externalWaitUsHistory[externalWaitHistoryIndex];
  externalWaitUsHistory[externalWaitHistoryIndex] = external_waited_us;
  externalWaitUsSum += external_waited_us;

#if DAGOR_DBGLEVEL > 0
  interlocked_release_store(dbgLastExternalWaitUs, external_waited_us);
#endif

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
    TIME_PROFILE(vulkan_latency_wait);
    ScopedTimerTicks watch(lastWaitTicks);
    sleep_precise_usec(time_to_wait_us, preciseSleepContext);
  }
  else
    lastWaitTicks = 0;
#if DAGOR_DBGLEVEL > 0
  interlocked_release_store(dbgLastInternalWaitUs, profile_usec_from_ticks_delta(lastWaitTicks));
#endif
}

void PredictedLatencyWaiter::reset() { timeToWaitUs = 0; }

void PredictedLatencyWaiter::wait()
{
  // usual predicted wait
  wait(timeToWaitUs);
}

void PredictedLatencyWaiter::markAsyncWait()
{
  WinAutoLock lock(asyncWaitMutex);

  // if async wait is skipped by application, process it here
  // mixing skipped and non skipped frames is not ok, but that's considered application issue
  if (asyncWaitStartTicks != 0)
    wait();
  asyncTimeToWaitUs = timeToWaitUs;
  asyncWaitStartTicks = profile_ref_ticks();
}

void PredictedLatencyWaiter::asyncWait()
{
  int64_t correctedWaitTimeUs;
  {
    WinAutoLock lock(asyncWaitMutex);
    correctedWaitTimeUs = asyncTimeToWaitUs - profile_time_usec(asyncWaitStartTicks);
    asyncWaitStartTicks = 0;
  }
  wait(correctedWaitTimeUs);
}
