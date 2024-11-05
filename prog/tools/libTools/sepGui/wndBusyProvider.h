// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class WinBusyProvider
{
public:
  WinBusyProvider();
  ~WinBusyProvider();

  int setBusy(bool value);

private:
  bool mBusyState;
  void *kHookHandle, *mHookHandle;
  void *hmCursor;
};
