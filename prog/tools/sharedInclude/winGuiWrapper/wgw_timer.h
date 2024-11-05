//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>

class ITimerCallBack
{
public:
  virtual void update() = 0;
  virtual ~ITimerCallBack() {}
};


class WinTimer // create over new()!
{
public:
  WinTimer(ITimerCallBack *callback, unsigned interval_ms, bool need_start);
  virtual ~WinTimer();

  void start();
  void stop();
  bool isActive() { return mTimerHandle != NULL; }

  unsigned getHandle() { return mTimerHandle; }
  static WinTimer *getTimerByHandle(unsigned timer_handle);

  void update();

private:
  ITimerCallBack *mCallBack;
  unsigned mIntervalMs;
  unsigned mTimerHandle;
  static Tab<WinTimer *> mTimers;
};
