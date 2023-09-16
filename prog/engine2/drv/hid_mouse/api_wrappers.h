#pragma once

#if _TARGET_PC_WIN
#include <supp/_platform.h>
#include <osApiWrappers/dag_progGlobals.h>

static inline bool mouse_api_GetClientRect(void *w, RECT *r) { return ::GetClientRect((HWND)w, r); }
static inline bool mouse_api_GetWindowRect(void *w, RECT *r) { return ::GetWindowRect((HWND)w, r); }
static inline bool mouse_api_GetCursorPosRel(POINT *pt, void *w) { return ::GetCursorPos(pt) && ::ScreenToClient((HWND)w, pt); }
static inline void mouse_api_SetCursorPosRel(void *w, POINT *pt)
{
  if (::ClientToScreen((HWND)w, pt))
    ::SetCursorPos(pt->x, pt->y);
}
static inline void mouse_api_ClipCursorToRect(const RECT &r) { ::ClipCursor(&r); }
static inline void mouse_api_ClipCursorToAppWindow()
{
  RECT r;
  mouse_api_GetWindowRect(win32_get_main_wnd(), &r);
  ::ClipCursor(&r);
}
static inline void mouse_api_UnclipCursor() { ::ClipCursor(NULL); }
static inline void mouse_api_SetCapture(void *w) { ::SetCapture((HWND)w); }
static inline void mouse_api_ReleaseCapture() { ::ReleaseCapture(); }
static inline void mouse_api_hide_cursor(bool) {}

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

extern bool mouse_api_GetClientRect(void *w, RECT *r);
extern bool mouse_api_GetWindowRect(void *w, RECT *r);
extern bool mouse_api_GetCursorPosRel(POINT *pt, void *p);
extern void mouse_api_SetCursorPosRel(void *w, POINT *pt);
extern void mouse_api_ClipCursorToAppWindow();
extern void mouse_api_UnclipCursor();
extern void mouse_api_SetCapture(void *w);
extern void mouse_api_ReleaseCapture();
extern void mouse_api_get_deltas(int &dx, int &dy);
extern void mouse_api_hide_cursor(bool hide);

#endif
