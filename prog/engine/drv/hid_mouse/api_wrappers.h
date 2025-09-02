// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#if _TARGET_PC_WIN
#include <supp/_platform.h>
#include <osApiWrappers/dag_progGlobals.h>

extern void mouse_api_ClipCursorToRect(const RECT &r);

#else
#include <osApiWrappers/dag_wndProcCompMsg.h>

#if !_TARGET_XBOX
struct POINT
{
  int x, y;
};
struct RECT
{
  int left, top, right, bottom;
};
#endif

extern void mouse_api_get_deltas(int &dx, int &dy);
#endif

extern bool mouse_api_GetClientRect(void *w, RECT *r);
extern bool mouse_api_GetWindowRect(void *w, RECT *r);
extern bool mouse_api_GetCursorPosRel(POINT *pt, void *w);
extern void mouse_api_SetCursorPosRel(void *w, POINT *pt);
extern void mouse_api_ClipCursorToAppWindow();
extern void mouse_api_UnclipCursor();
extern void mouse_api_SetCapture(void *w);
extern void mouse_api_ReleaseCapture();
extern void mouse_api_hide_cursor(bool hide);
