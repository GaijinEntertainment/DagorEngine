// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define _WIN32_WINNT 0x500

#include "wndBusyProvider.h"

#include <windows.h>
#include <stdio.h>

//-----------------------------------------------------------------------------

static HCURSOR hc_timer_cursor = LoadCursor(NULL, IDC_WAIT);


LRESULT CALLBACK hook_proc(int nCode, WPARAM wParam, LPARAM lParam)
{
  if (GetCursor() != hc_timer_cursor)
    SetCursor(hc_timer_cursor);

  return 1;
}

//-----------------------------------------------------------------------------

WinBusyProvider::WinBusyProvider() : mBusyState(false) {}


WinBusyProvider::~WinBusyProvider()
{
  if (mBusyState)
    setBusy(false);
}


int WinBusyProvider::setBusy(bool value)
{
  if (value != mBusyState)
  {
    mBusyState = value;

    if (mBusyState)
    {
      hmCursor = SetCursor(hc_timer_cursor);

      kHookHandle = SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)::hook_proc, NULL, GetCurrentThreadId());
      mHookHandle = SetWindowsHookEx(WH_MOUSE, (HOOKPROC)::hook_proc, NULL, GetCurrentThreadId());
    }
    else
    {
      UnhookWindowsHookEx((HHOOK)kHookHandle);
      UnhookWindowsHookEx((HHOOK)mHookHandle);

      SetCursor((HCURSOR)hmCursor);
    }
  }

  return 0;
}
