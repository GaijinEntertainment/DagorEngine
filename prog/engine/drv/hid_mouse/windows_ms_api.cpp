// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "api_wrappers.h"

static bool system_cursor_pos_change_allowed = true;

bool mouse_api_GetClientRect(void *w, RECT *r) { return ::GetClientRect((HWND)w, r); }
bool mouse_api_GetWindowRect(void *w, RECT *r) { return ::GetWindowRect((HWND)w, r); }
bool mouse_api_GetCursorPosRel(POINT *pt, void *w) { return ::GetCursorPos(pt) && ::ScreenToClient((HWND)w, pt); }
void mouse_api_SetCursorPosRel(void *w, POINT *pt)
{
  if (system_cursor_pos_change_allowed && ::ClientToScreen((HWND)w, pt))
    ::SetCursorPos(pt->x, pt->y);
}
void mouse_api_ClipCursorToRect(const RECT &r)
{
  if (system_cursor_pos_change_allowed)
    ::ClipCursor(&r);
}
void mouse_api_ClipCursorToAppWindow()
{
  if (system_cursor_pos_change_allowed)
  {
    RECT r;
    mouse_api_GetWindowRect(win32_get_main_wnd(), &r);
    ::ClipCursor(&r);
  }
}
void mouse_api_UnclipCursor()
{
  if (system_cursor_pos_change_allowed)
    ::ClipCursor(NULL);
}
void mouse_api_SetCapture(void *w) { ::SetCapture((HWND)w); }
void mouse_api_ReleaseCapture() { ::ReleaseCapture(); }
void mouse_api_hide_cursor(bool) {}
void mouse_api_SetSystemCursorPosChangeAllowed(bool allowed) { system_cursor_pos_change_allowed = allowed; }

void *mouse_api_create_mouse_cursor(int cursorWidth, int cursorHeight, uint32_t *rgba)
{
  // default cursor size, 32x32 on windows assuming no scaling
  // but we render our cursors ourselves so we use our own resolution
  // https://devblogs.microsoft.com/oldnewthing/20210819-00/?p=105572
  // const int cursorWidth = GetSystemMetrics(SM_CXCURSOR);
  // const int cursorHeight = GetSystemMetrics(SM_CYCURSOR);

  HCURSOR resultCursor = nullptr;

  uint8_t maskBits[64 * 64 / 8];
  memset(maskBits, 0xFF, sizeof(maskBits));
  HBITMAP hMask = CreateBitmap(cursorWidth, cursorHeight, 1, 1, maskBits);
  if (hMask)
  {
    HBITMAP hBitmap = CreateBitmap(cursorWidth, cursorHeight, 1, 32, rgba);
    if (hBitmap)
    {
      ICONINFO iconInfo = {
        .fIcon = false,
        .xHotspot = 0,
        .yHotspot = 0,
        .hbmMask = hMask,
        .hbmColor = hBitmap,
      };

      resultCursor = CreateIconIndirect(&iconInfo);

      DeleteObject(hMask);
    }

    DeleteObject(hBitmap);
  }

  return resultCursor;
}
