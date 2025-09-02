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
  bool isActive() const { return mTimerHandle != nullptr; }

  void *getHandle() const { return mTimerHandle; }
  int getStartId() const { return mStartId; }

  void update();

private:
  static constexpr int START_ID_STOPPED = -1;

  ITimerCallBack *mCallBack;
  void *mTimerHandle;
  unsigned mIntervalMs;
  int mStartId = START_ID_STOPPED;
};
