// Copyright (C) Gaijin Games KFT.  All rights reserved.

#import <Cocoa/Cocoa.h>
#include <osApiWrappers/setProgGlobals.h>
#include "api_wrappers.h"
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <debug/dag_debug.h>
#include <math/dag_Point2.h>

bool mac_video_fullscreen = false;
int  mac_video_width = -1;
int  mac_video_height = -1;

Point2 getBackingScale()
{
  NSRect r = [[NSScreen mainScreen] frame];
  if (mac_video_fullscreen)
    return Point2(r.size.width > 0 ? mac_video_width / (float)r.size.width : 1.0f,
                  r.size.height > 0 ? mac_video_height / (float)r.size.height : 1.0f);
  float s = d3d::get_driver_code().is(d3d::metal) ? [NSScreen mainScreen].backingScaleFactor : 1.0f;
  return Point2(s, s);
}

void mouse_api_SetFullscreenMode(int width, int hight)
{
  mac_video_fullscreen = true;
  mac_video_width = width;
  mac_video_height = hight;
}

bool mouse_api_GetClientRect(void *w, RECT *rect)
{
  if (!w || !rect)
    return false;

  Point2 scale = getBackingScale();

  NSRect wr = [(NSWindow*)w frame];
  NSRect cr = [(NSWindow*)w contentRectForFrameRect:wr];
  rect->left = (cr.origin.x - wr.origin.x) * scale.x;
  rect->top  = (cr.origin.y - wr.origin.y) * scale.y;
  rect->right  = (cr.origin.x + cr.size.width  - wr.origin.x) * scale.x;
  rect->bottom = (cr.origin.y + cr.size.height - wr.origin.y) * scale.y;
  return true;
}
bool mouse_api_GetWindowRect(void *w, RECT *rect)
{
  if (!w || !rect)
    return false;
  NSRect r = [(NSWindow*)w frame];

  Point2 scale = getBackingScale();

  rect->left = r.origin.x * scale.x;
  rect->top  = r.origin.y * scale.y;
  rect->right  = (r.origin.x + r.size.width) * scale.x;
  rect->bottom = (r.origin.y + r.size.height) * scale.y;
  return true;
}
bool mouse_api_GetCursorPosRel(POINT *pt, void *w)
{
  NSPoint p = [(NSWindow*)w mouseLocationOutsideOfEventStream];
  pt->x = p.x;
  pt->y = [[(NSWindow*)w contentView] frame].size.height - p.y;

  Point2 scale = getBackingScale();
  pt->x *= scale.x;
  pt->y *= scale.y;

  return true;
}
void mouse_api_SetCursorPosRel(void *w, POINT *pt)
{
  NSRect r = [(NSWindow*)w convertRectToScreen:NSMakeRect(pt->x, pt->y, 0, 0)];
  CGPoint p;

  Point2 scale = getBackingScale();

  p.x = r.origin.x / scale.x;
  p.y = r.origin.y / scale.y;
  CGWarpMouseCursorPosition(p);
}
void mouse_api_ClipCursorToAppWindow()
{
  CGAssociateMouseAndMouseCursorPosition(false);
}
void mouse_api_UnclipCursor()
{
  CGAssociateMouseAndMouseCursorPosition(true);
}
void mouse_api_SetCapture(void *w)
{
}
void mouse_api_ReleaseCapture()
{
}
void mouse_api_get_deltas(int &dx, int &dy)
{
  CGGetLastMouseDelta(&dx, &dy);

  Point2 scale = getBackingScale();

  dx = scale.x * dx;
  dy = scale.y * dy;
}
void mouse_api_hide_cursor(bool hide)
{
  if (hide)
    [NSCursor hide];
  else
    [NSCursor unhide];
}
