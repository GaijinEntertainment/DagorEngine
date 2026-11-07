// Copyright (C) Gaijin Games KFT.  All rights reserved.

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x500
#endif

#include <windows.h>
#include <stdio.h>
#include <libTools/util/daKernel.h>


namespace wingw
{
static HCURSOR get_wait_cursor()
{
  static HCURSOR cursor = LoadCursor(NULL, IDC_WAIT);
  return cursor;
}

LRESULT CALLBACK hook_proc(int nCode, WPARAM wParam, LPARAM lParam)
{
  if (GetCursor() != get_wait_cursor())
    SetCursor(get_wait_cursor());

  return 1;
}


//---------------------------------------------------------------------------


class WinBusyProvider
{
public:
  WinBusyProvider() : mBusyState(false) {}

  ~WinBusyProvider()
  {
    if (mBusyState)
      setBusy(false);
  }

  bool setBusy(bool value)
  {
    bool busyOld = mBusyState;
    if (value != mBusyState)
    {
      mBusyState = value;

      if (mBusyState)
      {
        hmCursor = SetCursor(get_wait_cursor());

        kHookHandle = SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)wingw::hook_proc, NULL, GetCurrentThreadId());
        mHookHandle = SetWindowsHookEx(WH_MOUSE, (HOOKPROC)wingw::hook_proc, NULL, GetCurrentThreadId());
      }
      else
      {
        UnhookWindowsHookEx((HHOOK)kHookHandle);
        UnhookWindowsHookEx((HHOOK)mHookHandle);

        SetCursor((HCURSOR)hmCursor);
      }
    }

    return busyOld;
  }

  bool getBusy() { return mBusyState; }

private:
  bool mBusyState;
  void *kHookHandle, *mHookHandle;
  void *hmCursor;
};


//---------------------------------------------------------------------------


const char *BUSY_VAR_NAME = "wingw::busy";

bool set_busy(bool value)
{
  WinBusyProvider *busy = (WinBusyProvider *)dakernel::get_named_pointer(BUSY_VAR_NAME);
  if (!busy)
  {
    busy = new WinBusyProvider();
    dakernel::set_named_pointer(BUSY_VAR_NAME, busy);
  }

  return busy->setBusy(value);
}


bool get_busy()
{
  WinBusyProvider *busy = (WinBusyProvider *)dakernel::get_named_pointer(BUSY_VAR_NAME);
  if (!busy)
    return false;

  return busy->getBusy();
}
} // namespace wingw
