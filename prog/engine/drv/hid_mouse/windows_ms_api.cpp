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