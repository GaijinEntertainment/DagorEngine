//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_initOnDemand.h>
#include <stdint.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/utility.h>
#include <EASTL/vector_multiset.h>
#include <EASTL/functional.h>

namespace game
{

typedef uint64_t timer_handle_t;
constexpr timer_handle_t INVALID_TIMER_HANDLE{};

struct TimerDeleter
{
  typedef timer_handle_t pointer;
  void operator()(pointer handle);
};

typedef eastl::function<void()> timer_cb_t;
typedef eastl::unique_ptr<void, TimerDeleter> Timer;

struct TimerRec;

class TimerManager
{
public:
  TimerManager();
  ~TimerManager();

  void act(float dt);

  // If timer already exist - it's remaining time will be reset. Rate should be > 0 if loop is true
  void setTimer(Timer &handle, timer_cb_t &&cb, float rate, bool loop = false, float first_delay = -1.f);
  void clearTimer(timer_handle_t handle);
  void clearTimer(Timer &handle);
  void clearAllTimers();

  void pauseTimer(const Timer &handle);
  void unPauseTimer(const Timer &handle);

  float getTimerRate(const Timer &handle) const;
  bool isTimerExists(const Timer &handle) const;
  bool isTimerActive(const Timer &handle) const; // i.e. exist and not being paused
  bool isTimerPaused(const Timer &handle) const;
  float getTimerElapsed(const Timer &handle) const;
  float getTimerRemaining(const Timer &handle) const;

private:
  void debugValidateTimers() const;
  const TimerRec *findTimer(timer_handle_t handle) const;
  TimerRec *findTimer(timer_handle_t handle);
  const TimerRec *findTimer(const Timer &handle) const { return findTimer(handle.get()); }
  TimerRec *findTimer(const Timer &handle) { return findTimer(handle.get()); }

private:
  eastl::vector_multiset<TimerRec> activeTimers;
  eastl::vector<TimerRec> pausedTimers;
  TimerRec *currentlyExecutingTimer = NULL;

  double internalTime = 0;
  uint64_t lastAssignedHandle = 0;
};

inline void TimerManager::clearTimer(Timer &handle)
{
  clearTimer(handle.get());
  handle.release();
}
inline TimerRec *TimerManager::findTimer(timer_handle_t handle)
{
  return const_cast<TimerRec *>(const_cast<const TimerManager *>(this)->findTimer(handle));
}

extern InitOnDemand<TimerManager> g_timers_mgr;

inline void TimerDeleter::operator()(timer_handle_t handle)
{
  if (handle != INVALID_TIMER_HANDLE && g_timers_mgr)
    g_timers_mgr->clearTimer(handle);
}

} // namespace game
