// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/captureCursor.h>

#include <osApiWrappers/dag_progGlobals.h>
#include <debug/dag_debug.h>
#include <EditorCore/ec_input.h>
#include <EditorCore/ec_rect.h>
#include <math/integer/dag_IPoint2.h>

#include <imgui/imgui.h>

#if _TARGET_PC_WIN

#include <windows.h>

void capture_cursor(void *handle) { SetCapture((HWND)handle); }


void *get_capture() { return GetCapture(); }


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

#include <osApiWrappers/dag_linuxGUI.h>

static void *ec_capture_cursor_window = nullptr;

static IPoint2 area_cursor_wrap_internal(const EcRect &rc)
{
  const int left = rc.l + 5;
  const int right = rc.r - 5;
  const int top = rc.t + 5;
  const int bottom = rc.b - 5;
  if (right <= left || bottom <= top)
    return IPoint2(0, 0);

  const IPoint2 oldPos = ec_get_cursor_pos();
  IPoint2 pos = oldPos;

  if (pos.x < left)
    pos.x = right;
  else if (pos.x > right)
    pos.x = left;

  if (pos.y < top)
    pos.y = bottom;
  else if (pos.y > bottom)
    pos.y = top;

  const IPoint2 delta = pos - oldPos;
  if (delta.x != 0 || delta.y != 0)
    ec_set_cursor_pos(pos);

  return delta;
}

void capture_cursor(void *handle)
{
  // TODO: tools Linux porting: multi viewport: replace win32_get_main_wnd()
  G_ASSERT((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) == 0);

  linux_GUI::clip_cursor();

  ec_capture_cursor_window = handle;
}

void *get_capture() { return ec_capture_cursor_window; }

void release_cursor()
{
  linux_GUI::unclip_cursor();

  ec_capture_cursor_window = nullptr;
}

void cursor_wrap(int &p1, int &p2, void *handle)
{
  // TODO: tools Linux porting: multi viewport: get the correct ImGui viewport
  G_ASSERT((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) == 0);

  const ImGuiViewport *viewport = ImGui::GetMainViewport();
  if (!viewport)
    return;

  const EcRect rc{.l = (int)(viewport->Pos.x),
    .t = (int)(viewport->Pos.y),
    .r = (int)(viewport->Pos.x + viewport->Size.x),
    .b = (int)(viewport->Pos.y + viewport->Size.y)};
  const IPoint2 delta = area_cursor_wrap_internal(rc);
  p1 += delta.x;
  p2 += delta.y;
}

void area_cursor_wrap(const EcRect &rc, int &p1, int &p2, int &wrapedX, int &wrapedY)
{
  const IPoint2 delta = area_cursor_wrap_internal(rc);
  p1 += delta.x;
  p2 += delta.y;
  wrapedX += delta.x;
  wrapedY += delta.y;
}

#endif