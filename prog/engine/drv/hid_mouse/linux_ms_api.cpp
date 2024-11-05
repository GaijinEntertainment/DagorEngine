// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/setProgGlobals.h>
#include "api_wrappers.h"
#include <debug/dag_debug.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <workCycle/dag_gameSettings.h>
#include "../workCycle/workCyclePriv.h"

#include <X11/X.h>
#include <X11/Xlib.h>

static POINT last_mouse_pos = {-1, -1};

bool get_window_screen_rect(void *w, RECT *rect, RECT *rect_unclipped)
{
  if (!w || !rect)
    return false;

  const XWindowAttributes &attrib = workcycle_internal::get_window_attrib(*(Window *)w, true);
  const XWindowAttributes &rootAttrib = workcycle_internal::get_window_attrib(0, false);

  RECT winRect;
  RECT rootRect;

  winRect.left = attrib.x;
  winRect.right = attrib.x + attrib.width;
  winRect.top = attrib.y;
  winRect.bottom = attrib.y + attrib.height;

  rootRect.left = rootAttrib.x;
  rootRect.right = rootAttrib.x + rootAttrib.width;
  rootRect.top = rootAttrib.y;
  rootRect.bottom = rootAttrib.y + rootAttrib.height;

  if (rect_unclipped)
    *rect_unclipped = winRect;

  // clip visible area
  if (winRect.left < rootRect.left)
    winRect.left = rootRect.left;

  if (winRect.right > rootRect.right)
    winRect.right = rootRect.right;

  if (winRect.top < rootRect.top)
    winRect.top = rootRect.top;

  if (winRect.bottom > rootRect.bottom)
    winRect.bottom = rootRect.bottom;

  *rect = winRect;
  rect->left += attrib.border_width;
  rect->right -= attrib.border_width;
  rect->top += attrib.border_width;
  rect->bottom -= attrib.border_width;

  return true;
}

bool mouse_api_GetWindowRect(void *w, RECT *rect) { return get_window_screen_rect(w, rect, NULL); }

bool mouse_api_GetClientRect(void *w, RECT *rect)
{
  if (!w || !rect)
    return false;

  RECT client, unclipped;
  get_window_screen_rect(w, &client, &unclipped);
  rect->right = client.right - unclipped.left;
  rect->left = client.left - unclipped.left;
  rect->top = client.top - unclipped.top;
  rect->bottom = client.bottom - unclipped.top;
  return true;
}

bool mouse_api_GetCursorPosRel(POINT *pt, void *w)
{
  // use cached value from last processed motion window event
  return workcycle_internal::get_last_cursor_pos(&pt->x, &pt->y, *(Window *)w);
}

void mouse_api_SetCursorPosRel(void *w, POINT *pt)
{
  last_mouse_pos = *pt;
  XWarpPointer(workcycle_internal::get_root_display(), None, *(Window *)w, 0, 0, 0, 0, pt->x, pt->y);
  // update cached cursor position directly, avoiding extra api call
  workcycle_internal::update_cursor_position(pt->x, pt->y, *(Window *)w);
}

void mouse_api_ClipCursorToAppWindow()
{
  Display *display = workcycle_internal::get_root_display();
  Window *window = (Window *)win32_get_main_wnd();

  if (window)
  {
    XSetInputFocus(display, *window, RevertToParent, CurrentTime);
    XGrabPointer(display, *window, 0, ButtonPressMask | ButtonReleaseMask | PointerMotionMask | ButtonMotionMask, GrabModeAsync,
      GrabModeAsync, *window, None, CurrentTime);
  }
}

void mouse_api_UnclipCursor()
{
  Display *display = workcycle_internal::get_root_display();
  XUngrabPointer(display, CurrentTime);
}

void mouse_api_SetCapture(void *wnd) { G_UNREFERENCED(wnd); }


void mouse_api_ReleaseCapture() {}

void mouse_api_get_deltas(int &dx, int &dy)
{
  POINT curPos;
  mouse_api_GetCursorPosRel(&curPos, win32_get_main_wnd());

  if (last_mouse_pos.x == -1 && last_mouse_pos.y == -1)
    last_mouse_pos = curPos;

  dx = curPos.x - last_mouse_pos.x;
  dy = curPos.y - last_mouse_pos.y;

  last_mouse_pos = curPos;
}

void mouse_api_hide_cursor(bool hide)
{
  Display *display = workcycle_internal::get_root_display();
  Window window = *(Window *)win32_get_main_wnd();

  if (hide)
  {
    Pixmap bm_no;
    Colormap cmap;
    Cursor no_ptr;
    XColor black, dummy;
    static char bm_no_data[] = {0, 0, 0, 0, 0, 0, 0, 0};

    cmap = DefaultColormap(display, DefaultScreen(display));
    XAllocNamedColor(display, cmap, "black", &black, &dummy);
    bm_no = XCreateBitmapFromData(display, window, bm_no_data, 8, 8);
    no_ptr = XCreatePixmapCursor(display, bm_no, bm_no, &black, &black, 0, 0);

    XDefineCursor(display, window, no_ptr);
    XFreeCursor(display, no_ptr);
    if (bm_no != None)
      XFreePixmap(display, bm_no);
    XFreeColors(display, cmap, &black.pixel, 1, 0);
  }
  else
  {
    XUndefineCursor(display, window);
  }
}
