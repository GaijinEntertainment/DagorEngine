// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <windows.h>
#include <winGuiWrapper/wgw_timer.h>

#pragma comment(lib, "user32.lib")


Tab<WinTimer *> WinTimer::mTimers(midmem);

WinTimer *WinTimer::getTimerByHandle(unsigned timer_handle)
{
  for (int i = 0; i < mTimers.size(); ++i)
    if (mTimers[i]->getHandle() == timer_handle)
      return mTimers[i];
  return NULL;
}


void CALLBACK timerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
  WinTimer *timer = WinTimer::getTimerByHandle(idEvent);
  if (timer)
    timer->update();
}


WinTimer::WinTimer(ITimerCallBack *callback, unsigned interval_ms, bool need_start) :
  mCallBack(callback), mIntervalMs(interval_ms), mTimerHandle(NULL)
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
  mTimerHandle = SetTimer(NULL, NULL, mIntervalMs, &timerProc);
  mTimers.push_back(this);
}


void WinTimer::stop()
{
  KillTimer(NULL, mTimerHandle);
  mTimerHandle = NULL;
  for (int i = 0; i < mTimers.size(); ++i)
    if (mTimers[i] == this)
    {
      erase_items(mTimers, i, 1);
      break;
    }
}


void WinTimer::update()
{
  if (mCallBack)
    mCallBack->update();
}
