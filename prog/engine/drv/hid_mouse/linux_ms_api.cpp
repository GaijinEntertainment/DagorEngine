// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/setProgGlobals.h>
#include "api_wrappers.h"
#include <debug/dag_debug.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/dag_linuxGUI.h>

namespace
{
static POINT last_mouse_pos = {-1, -1};
} // namespace

bool mouse_api_GetWindowRect(void *w, RECT *out_rect)
{
  if (!out_rect)
    return false;

  linux_GUI::RECT rect;
  bool ret = linux_GUI::get_window_screen_rect(w, &rect, nullptr);
  out_rect->left = rect.left;
  out_rect->top = rect.top;
  out_rect->right = rect.right;
  out_rect->bottom = rect.bottom;
  return ret;
}

bool mouse_api_GetClientRect(void *w, RECT *rect)
{
  if (!w || !rect)
    return false;

  linux_GUI::RECT client, unclipped;
  bool ret = linux_GUI::get_window_screen_rect(w, &client, &unclipped);
  rect->right = client.right - unclipped.left;
  rect->left = client.left - unclipped.left;
  rect->top = client.top - unclipped.top;
  rect->bottom = client.bottom - unclipped.top;
  return ret;
}

bool mouse_api_GetCursorPosRel(POINT *pt, void *w)
{
  // use cached value from last processed motion window event
  return linux_GUI::get_last_cursor_pos(&pt->x, &pt->y, w);
}

void mouse_api_SetCursorPosRel(void *w, POINT *pt)
{
  last_mouse_pos = *pt;
  linux_GUI::set_cursor_position(pt->x, pt->y, w);
}

void mouse_api_ClipCursorToAppWindow() { linux_GUI::clip_cursor(); }

void mouse_api_UnclipCursor() { linux_GUI::unclip_cursor(); }

void mouse_api_SetCapture(void *wnd) { G_UNREFERENCED(wnd); }


void mouse_api_ReleaseCapture() {}

void mouse_api_get_deltas(int &dx, int &dy)
{
  dx = 0;
  dy = 0;
  if (linux_GUI::get_cursor_delta(dx, dy, win32_get_main_wnd()))
    return;

  POINT curPos;
  if (!mouse_api_GetCursorPosRel(&curPos, win32_get_main_wnd()))
    return;

  if (last_mouse_pos.x == -1 && last_mouse_pos.y == -1)
    last_mouse_pos = curPos;

  dx = curPos.x - last_mouse_pos.x;
  dy = curPos.y - last_mouse_pos.y;

  last_mouse_pos = curPos;
}

void mouse_api_hide_cursor(bool hide) { linux_GUI::hide_cursor(hide); }
