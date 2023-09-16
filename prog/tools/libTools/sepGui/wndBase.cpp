// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "wndBase.h"
#include <debug/dag_debug.h>

#include <windows.h>


//=============================================================================
void client_to_screen(void *hWnd, IPoint2 &pos)
{
  POINT position = {pos.x, pos.y};
  ClientToScreen((HWND)hWnd, &position);
  pos = IPoint2(position.x, position.y);
}


void screen_to_client(void *hWnd, IPoint2 &pos)
{
  POINT position = {pos.x, pos.y};
  ScreenToClient((HWND)hWnd, &position);
  pos = IPoint2(position.x, position.y);
}


//=============================================================================
BaseWindow::~BaseWindow()
{
  if (mHWnd)
  {
    DestroyWindow((HWND)mHWnd);
    mHWnd = 0;
  }
}


intptr_t BaseWindow::winProc(void *h_wnd, unsigned msg, TSgWParam w_param, TSgLParam l_param)
{
  return DefWindowProc((HWND)h_wnd, msg, (WPARAM)w_param, (LPARAM)l_param);
}


void BaseWindow::setWindowPos(int x, int y, int width, int height) { SetWindowPos((HWND)mHWnd, 0, x, y, width, height, SWP_NOZORDER); }


void BaseWindow::updateWindow() { InvalidateRect((HWND)mHWnd, NULL, true); }

//=============================================================================


CascadeWindow::CascadeWindow(WinManager *owner, VirtualWindow *parent, int x, int y, unsigned w, unsigned h)

  :
  mOwner(owner), mParent(parent), mLeftTop(x, y), mRightBottom(x + w, y + h)
{}
