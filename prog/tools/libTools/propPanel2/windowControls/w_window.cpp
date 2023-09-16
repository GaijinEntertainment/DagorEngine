// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "w_window.h"
#include "../c_constants.h"

#include <windows.h>

// -------------- Window --------------


WWindow::WWindow(WindowControlEventHandler *event_handler, void *hwnd, int x, int y, unsigned w, unsigned h, const char caption[],
  bool user_color) :
  WindowControlBase(event_handler, NULL, user_color ? USER_PROPERTY_WINDOW_CLASS_NAME : PROPERTY_WINDOW_CLASS_NAME, 0,
    (hwnd) ? WS_CHILD : 0, caption, x, y, w, h),
  flagHasVScroll(false),
  mVScrollPosition(0),
  mVPageSize(0),
  mVSize(0),
  flagHasHScroll(false),
  mHScrollPosition(0),
  mHPageSize(0),
  mHSize(0)
{
  mUserGuiColor = user_color;

  if (hwnd)
  {
    SetParent((HWND)this->getHandle(), (HWND)hwnd);
  }

  // this->show();
}

// vertical scroll

void WWindow::addVScroll()
{
  flagHasVScroll = true;
  ShowScrollBar((HWND)this->getHandle(), SB_VERT, flagHasVScroll);
}


void WWindow::vScrollUpdate(int size, int pagesize)
{
  HWND hw = (HWND)this->getHandle();
  mVPageSize = pagesize;
  mVSize = size;
  int deltaY = (mVPageSize - mVSize) - mVScrollPosition;
  SCROLLINFO si;
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  si.nMin = 0;
  si.nMax = mVSize;
  si.nPage = mVPageSize;
  SetScrollInfo(hw, SB_VERT, &si, true);

  if (deltaY > 0)
  {
    ScrollWindowEx(hw, 0, deltaY, NULL, NULL, NULL, NULL, SW_SCROLLCHILDREN | SW_ERASE | SW_INVALIDATE);
    mVScrollPosition += deltaY;
    SetScrollPos(hw, SB_VERT, -mVScrollPosition, true);
  }
}


int WWindow::getVScrollPos() { return mVScrollPosition; }


void WWindow::setVScrollPos(int pos) { this->onVScroll(pos - mVScrollPosition, false); }


bool WWindow::hasVScroll() { return flagHasVScroll; }


void WWindow::removeVScroll()
{
  HWND hw = (HWND)this->getHandle();

  ScrollWindowEx(hw, 0, -mVScrollPosition, NULL, NULL, NULL, NULL, SW_SCROLLCHILDREN | SW_ERASE | SW_INVALIDATE);

  mVScrollPosition = 0;
  flagHasVScroll = false;
  ShowScrollBar(hw, SB_VERT, flagHasVScroll);
}


intptr_t WWindow::onVScroll(int dy, bool is_wheel)
{
  G_UNUSED(is_wheel);
  if (!flagHasVScroll)
    return 0;


  if (mVScrollPosition + dy > 0)
    dy = -mVScrollPosition;
  else if (mVScrollPosition + dy < (mVPageSize - mVSize))
    dy = mVPageSize - mVSize - mVScrollPosition;

  if (dy)
  {
    HWND hw = (HWND)this->getHandle();
    ScrollWindowEx(hw, 0, dy, NULL, NULL, NULL, NULL, SW_SCROLLCHILDREN | SW_ERASE | SW_INVALIDATE);

    mVScrollPosition += dy;
    SetScrollPos(hw, SB_VERT, -mVScrollPosition, true);
  }

  return 1;
}

//------------------------------------------

// move scroll with TAB

void WWindow::onTabFocusChange()
{
  HWND _hwindow = GetFocus();
  POINT _pos = {0, 0};
  int _dy = 0;
  int _dymax_correction = 0;
  RECT _rect;

  GetWindowRect(_hwindow, &_rect);
  ClientToScreen(_hwindow, &_pos);
  ScreenToClient((HWND)this->getHandle(), &_pos);

  // vertical scroll

  _dymax_correction = DEFAULT_CONTROLS_INTERVAL + (_rect.bottom - _rect.top);

  if (_pos.y < 0)
  {
    _dy = _pos.y - DEFAULT_CONTROLS_INTERVAL;
  }

  if (_pos.y > mVPageSize - _dymax_correction)
  {
    _dy = _pos.y - mVPageSize + _dymax_correction;
  }

  if (_dy)
  {
    this->onVScroll(-_dy, false);
  }
}


bool WWindow::onClose() { return mEventHandler->onWcClose(this); }


void WWindow::onPostEvent(unsigned id) { mEventHandler->onWcPostEvent(id); }


//------------------------------------------

// horizontal scroll

void WWindow::addHScroll() { flagHasHScroll = true; }


void WWindow::hScrollUpdate(int size, int pagesize)
{
  HWND hw = (HWND)this->getHandle();
  mHPageSize = pagesize;
  mHSize = size;
  int deltaX = (mHPageSize - mHSize) - mHScrollPosition;

  if (deltaX > 0)
  {
    ScrollWindowEx(hw, deltaX, 0, NULL, NULL, NULL, NULL, SW_SCROLLCHILDREN | SW_ERASE | SW_INVALIDATE);
    mHScrollPosition += deltaX;
  }
}


int WWindow::getHScrollPos() { return mHScrollPosition; }


bool WWindow::hasHScroll() { return flagHasHScroll; }


void WWindow::removeHScroll()
{
  HWND hw = (HWND)this->getHandle();

  int deltaX = -getHScrollPos();
  ScrollWindowEx(hw, deltaX, 0, NULL, NULL, NULL, NULL, SW_SCROLLCHILDREN | SW_ERASE | SW_INVALIDATE);

  mHScrollPosition = 0;
  flagHasHScroll = false;
}


intptr_t WWindow::onHScroll(int dx)
{
  if (!flagHasHScroll)
    return 0;

  if (mHScrollPosition + dx > 0)
    dx = -mHScrollPosition;
  else if (mHScrollPosition + dx < (mHPageSize - mHSize))
    dx = mHPageSize - mHSize - mHScrollPosition;

  if (dx)
  {
    ScrollWindowEx((HWND)this->getHandle(), dx, 0, NULL, NULL, NULL, NULL, SW_SCROLLCHILDREN | SW_ERASE | SW_INVALIDATE);

    mHScrollPosition += dx;
  }

  return 1;
}
