// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "wndClientArea.h"

#include <windows.h>

#include <debug/dag_debug.h>


extern const char *WNDCLASS_CLIENT_AREA;

ClientWindowArea::ClientWindowArea(void *hparent, int x, int y, int w, int h) :

  mPHandle(hparent)
{
  mHWnd = CreateWindowEx(0, WNDCLASS_CLIENT_AREA, "", WS_CHILD | WS_VISIBLE, x, y, w, h, (HWND)mPHandle, NULL, NULL,
    static_cast<BaseWindow *>(this));
}


ClientWindowArea::~ClientWindowArea() { DestroyWindow((HWND)mHWnd); }


intptr_t ClientWindowArea::winProc(void *h_wnd, unsigned msg, TSgWParam w_param, TSgLParam l_param)
{
  switch (msg)
  {
    case WM_RBUTTONUP:
    {
      if (mPHandle)
        SendMessage((HWND)mPHandle, msg, (WPARAM)w_param, (LPARAM)l_param);
    }
      return 0;

    case WM_ERASEBKGND: return 1;

    case WM_DESTROY: break;
  }

  return BaseWindow::winProc(h_wnd, msg, w_param, l_param);
}


void ClientWindowArea::resize(int x, int y, int width, int height) { setWindowPos(x, y, width, height); }
