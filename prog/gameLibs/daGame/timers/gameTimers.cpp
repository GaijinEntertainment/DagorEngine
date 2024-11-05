// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "daGame/timers.h"
#include <math.h>

namespace game
{

InitOnDemand<TimerManager> g_timers_mgr;

enum TimerStatus
{
  Invalid,
  Active,
  Paused,
  Executing
};

struct TimerRec
{
  TimerStatus status;
  timer_handle_t handle;
  timer_cb_t cb;

  double expireTime; // if timer is paused, then it's delta, not absolute value
  float rate;
  bool loop;

  bool operator<(const TimerRec &rhs) const { return expireTime < rhs.expireTime; } // for vector_multiset<>

  TimerRec(timer_cb_t cb_, timer_handle_t h, float rate_, bool loop_, double et) :
    status(Active), handle(h), cb(eastl::move(cb_)), expireTime(et), rate(rate_), loop(loop_)
  {
    G_ASSERT(cb);
  }

  void execute() { cb(); }
};

// These are required because of undefined in header TimerRec struct
TimerManager::TimerManager() {}
TimerManager::~TimerManager() {}

void TimerManager::act(float dt)
{
  internalTime += dt;

  while (!activeTimers.empty())
  {
    if (internalTime > activeTimers.begin()->expireTime)
    {
      TimerRec cur = eastl::move(*activeTimers.begin());
      activeTimers.erase(activeTimers.begin());
      currentlyExecutingTimer = &cur;
      cur.status = Executing;

      G_ASSERT(!cur.loop || cur.rate > 0.f); // guaranteed by setTimer implementation
#if 1
      int callCount = 1; // assume that game code either shouldn't care or should be using getTimerElapsed()
#else
      int callCount = cur.loop ? int(floor((internalTime - cur.expireTime) / cur.rate)) + 1 : 1;
#endif
      for (int i = 0; i < callCount && cur.status == Executing; ++i)
        cur.execute();

      currentlyExecutingTimer = NULL;

      if (cur.loop && cur.status == Executing)
      {
        cur.status = Active;
        cur.expireTime += callCount * cur.rate;
        activeTimers.insert(eastl::move(cur));
        debugValidateTimers();
      }
    }
    else
      break;
  }
}

void TimerManager::debugValidateTimers() const
{
#if DAGOR_DBGLEVEL > 0
  for (int i = 1; i < activeTimers.size(); ++i)
  {
    G_ASSERT(activeTimers[i].status == Active);
    G_ASSERT(activeTimers[i - 1].status == Active);
    G_ASSERTF(activeTimers[i].expireTime >= activeTimers[i - 1].expireTime, "Broken sort timers invariant!");
  }
  for (int i = 0; i < pausedTimers.size(); ++i)
    G_ASSERT(pausedTimers[i].status == Paused);
  G_ASSERT(!currentlyExecutingTimer || currentlyExecutingTimer->status == Executing);
#endif
}

void TimerManager::setTimer(Timer &handle, timer_cb_t &&cb, float rate, bool loop, float first_delay)
{
  G_ASSERT_RETURN(cb, );

  if (handle)
    handle.reset();

  if (rate > 0.f || !loop)
  {
    handle.reset(++lastAssignedHandle);
    TimerRec newTimer(eastl::move(cb), handle.get(), rate, loop, internalTime + (first_delay >= 0.f ? first_delay : rate));
    activeTimers.insert(eastl::move(newTimer));
    debugValidateTimers();
  }
}

void TimerManager::clearTimer(timer_handle_t handle)
{
  if (TimerRec *timer = findTimer(handle))
  {
    switch (timer->status)
    {
      case Active: activeTimers.erase(timer); break;
      case Paused: pausedTimers.erase(timer); break;
      case Executing:
        currentlyExecutingTimer->handle = INVALID_TIMER_HANDLE;
        currentlyExecutingTimer->status = Invalid;
        break;
      default: G_ASSERT(0); break;
    };
  }
}

void TimerManager::clearAllTimers()
{
  activeTimers.clear();
  pausedTimers.clear();
  G_ASSERT(!currentlyExecutingTimer);
}

void TimerManager::pauseTimer(const Timer &handle)
{
  TimerRec *timer = findTimer(handle);
  TimerStatus status = timer ? timer->status : Active;
  if (timer && status != Paused)
  {
    if (status != Executing || timer->loop) // timer that currently executing can't be paused unless it's going to loop
    {
      pausedTimers.emplace_back(eastl::move(*timer));
      pausedTimers.rbegin()->status = Paused;
      pausedTimers.rbegin()->expireTime -= internalTime; // store the rest of expireTime (delta) as expireTime itself
    }
    switch (status)
    {
      case Active: activeTimers.erase(timer); break;
      case Executing:
        currentlyExecutingTimer->handle = timer_handle_t();
        currentlyExecutingTimer->status = Invalid;
        break;
      default: G_ASSERT(0); break;
    };
    debugValidateTimers();
  }
}

void TimerManager::unPauseTimer(const Timer &handle)
{
  for (int i = 0; i < pausedTimers.size(); ++i)
    if (pausedTimers[i].handle == handle.get())
    {
      TimerRec &timer = pausedTimers[i];
      G_ASSERT(timer.status == Paused);
      timer.status = Active;
      timer.expireTime += internalTime;
      activeTimers.insert(eastl::move(timer));
      pausedTimers.erase(pausedTimers.begin() + i);
      debugValidateTimers();
      break;
    }
}

const TimerRec *TimerManager::findTimer(timer_handle_t handle) const
{
  if (handle == INVALID_TIMER_HANDLE)
    return NULL;
  if (currentlyExecutingTimer && currentlyExecutingTimer->handle == handle)
    return currentlyExecutingTimer;
  for (int i = 0; i < activeTimers.size(); ++i)
    if (activeTimers[i].handle == handle)
      return &activeTimers[i];
  for (int i = 0; i < pausedTimers.size(); ++i)
    if (pausedTimers[i].handle == handle)
      return &pausedTimers[i];
  return NULL;
}

float TimerManager::getTimerRate(const Timer &handle) const
{
  const TimerRec *td = findTimer(handle);
  return td ? td->rate : -1.f;
}

bool TimerManager::isTimerExists(const Timer &handle) const { return findTimer(handle) != NULL; }

bool TimerManager::isTimerActive(const Timer &handle) const
{
  const TimerRec *td = findTimer(handle);
  return td && td->status != Paused;
}

bool TimerManager::isTimerPaused(const Timer &handle) const
{
  const TimerRec *td = findTimer(handle);
  return td && td->status == Paused;
}

float TimerManager::getTimerElapsed(const Timer &handle) const
{
  const TimerRec *td = findTimer(handle);
  if (td)
    return td->rate - (td->status == Paused ? td->expireTime : (td->expireTime - internalTime));
  else
    return -1.f;
}

float TimerManager::getTimerRemaining(const Timer &handle) const
{
  const TimerRec *td = findTimer(handle);
  if (td)
    return td->status == Paused ? td->expireTime : (td->expireTime - internalTime);
  else
    return -1.f;
}

} // namespace game
