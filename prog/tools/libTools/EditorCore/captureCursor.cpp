// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/captureCursor.h>

#include <osApiWrappers/dag_progGlobals.h>
#include <debug/dag_debug.h>
#include <startup/dag_inpDevClsDrv.h>
#include <drv/hid/dag_hiPointing.h>

#include <EditorCore/ec_rect.h>

#if _TARGET_PC_WIN

#include <windows.h>

void capture_cursor(void *handle) { SetCapture((HWND)handle); }


void release_cursor(void) { ReleaseCapture(); }


void cursor_wrap(int &p1, int &p2, void *handle)
{
  HWND window = (HWND)handle;

  if (!window)
    window = GetForegroundWindow();

  RECT rc;

  GetClientRect(window, &rc);

  POINT pt;
  pt.x = 0;
  pt.y = 0;
  ClientToScreen(window, &pt);

  rc.top += pt.y;
  rc.left += pt.x;
  rc.right += pt.x;
  rc.bottom += pt.y;

  int x, y, oldX, oldY;
  GetCursorPos(&pt);
  x = pt.x;
  y = pt.y;

  const int dx = x - p1;
  const int dy = y - p2;

  const int left = rc.left + 5;
  const int right = rc.right - 5;
  const int top = rc.top + 5;
  const int bottom = rc.bottom - 5;

  oldX = x;
  oldY = y;

  if (x < left)
    x = right;
  else if (x > right)
    x = left;

  if (y < top)
    y = bottom;
  else if (y > bottom)
    y = top;

  if (x != oldX)
    p1 = x - dx;

  if (y != oldY)
    p2 = y - dy;

  if ((oldX != x) || (oldY != y))
    SetCursorPos(x, y);
}


void area_cursor_wrap(const EcRect &rc, int &p1, int &p2, int &wrapedX, int &wrapedY)
{
  const int left = 5;
  const int top = 5;
  const int right = rc.width() - left;
  const int bottom = rc.height() - top;

  if (p1 < left || p1 > right || p2 < top || p2 > bottom)
  {
    int op1 = p1;
    int op2 = p2;

    if (p1 < left)
      p1 = right;
    else if (p1 > right)
      p1 = left;

    if (p2 < top)
      p2 = bottom;
    else if (p2 > bottom)
      p2 = top;

    const int delta1 = op1 - p1;
    const int delta2 = op2 - p2;

    wrapedX += delta1;
    wrapedY += delta2;

    int x;
    int y;

    POINT pt;
    GetCursorPos(&pt);
    SetCursorPos(pt.x - delta1, pt.y - delta2);
  }
}

#else

// TODO: tools Linux porting: !!! captureCursor.cpp !!!

void capture_cursor(void *handle) {}

void release_cursor(void) {}

void cursor_wrap(int &p1, int &p2, void *handle) {}

void area_cursor_wrap(const EcRect &rc, int &p1, int &p2, int &wrapedX, int &wrapedY) {}

#endif