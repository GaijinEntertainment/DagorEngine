// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <winGuiWrapper/wgw_timer.h>

#include <debug/dag_debug.h>
#include <util/dag_delayedAction.h>

#if _TARGET_PC_WIN
#include <windows.h>

#pragma comment(lib, "user32.lib")
#else
#include <signal.h>
#include <time.h>
#endif

static Tab<WinTimer *> timers(midmem);
static int timer_start_id = 0;

#if _TARGET_PC_WIN
static WinTimer *getTimerByHandle(void *timer_handle)
{
  for (int i = 0; i < timers.size(); ++i)
    if (timers[i]->getHandle() == timer_handle)
      return timers[i];
  return NULL;
}


static void CALLBACK timerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
  WinTimer *timer = getTimerByHandle((void *)((uintptr_t)idEvent));
  if (timer)
    timer->update();
}
#else
static WinTimer *getTimerByStartId(int start_id)
{
  for (WinTimer *timer : timers)
    if (timer->getStartId() == start_id)
      return timer;
  return nullptr;
}


static void timerProc(sigval_t param)
{
  // timerProc is called from a thread. Do the callback on the main thread.
  int startId = param.sival_int;
  delayed_call([startId] {
    WinTimer *timer = getTimerByStartId(startId);
    if (timer)
      timer->update();
  });
}
#endif


WinTimer::WinTimer(ITimerCallBack *callback, unsigned interval_ms, bool need_start) :
  mCallBack(callback), mIntervalMs(interval_ms), mTimerHandle(nullptr)
{
  if (need_start)
    start();
}


WinTimer::~WinTimer()
{
  if (isActive())
    stop();
}


void WinTimer::start()
{
  if (isActive())
    return;

  // Each start gets a globally unique ID to ensure that stopped or deleted (and recreated with the same address) timers
  // do not trigger incorrectly on Linux.
  G_ASSERT(is_main_thread());
  mStartId = ++timer_start_id;
  if (mStartId == START_ID_STOPPED)
    mStartId = ++timer_start_id;

#if _TARGET_PC_WIN
  mTimerHandle = (void *)(uintptr_t)SetTimer(NULL, NULL, mIntervalMs, &timerProc);
#else
  sigevent event = {0};
  event.sigev_notify = SIGEV_THREAD;
  event.sigev_notify_function = timerProc;
  event.sigev_value.sival_int = mStartId;

  G_STATIC_ASSERT(sizeof(timer_t) == sizeof(void *));
  if (timer_create(CLOCK_MONOTONIC, &event, &mTimerHandle) == 0)
  {
    itimerspec tm;
    tm.it_value.tv_sec = tm.it_interval.tv_sec = mIntervalMs / 1000;
    tm.it_value.tv_nsec = tm.it_interval.tv_nsec = (mIntervalMs % 1000) * 1000000;
    if (timer_settime(mTimerHandle, 0, &tm, nullptr) != 0)
      logerr("Failed to set WinTimer interval parameters.");
  }
  else
  {
    logerr("Failed to start WinTimer.");
  }
#endif

  timers.push_back(this);
}


void WinTimer::stop()
{
  mStartId = START_ID_STOPPED;

#if _TARGET_PC_WIN
  KillTimer(NULL, (uintptr_t)mTimerHandle);
#else
  timer_delete(mTimerHandle);
#endif

  mTimerHandle = nullptr;
  for (int i = 0; i < timers.size(); ++i)
    if (timers[i] == this)
    {
      erase_items(timers, i, 1);
      break;
    }
}


void WinTimer::update()
{
  if (mCallBack)
    mCallBack->update();
}
