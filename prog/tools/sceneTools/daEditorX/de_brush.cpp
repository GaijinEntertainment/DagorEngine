// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <oldEditor/de_brush.h>
#include <oldEditor/de_clipping.h>

#include <debug/dag_debug.h>


bool DeBrush::calcCenter(IGenViewportWnd *wnd) { return calcCenterFromIface(wnd); }


bool DeBrush::calcCenterFromIface(IGenViewportWnd *wnd)
{
  const bool prevUseVisible = DagorPhys::use_only_visible_colliders;
  DagorPhys::use_only_visible_colliders = true;

  const bool ret = Brush::calcCenter(wnd);

  DagorPhys::use_only_visible_colliders = prevUseVisible;

  return ret;
}


bool DeBrush::traceDown(const Point3 &pos, Point3 &clip_pos, IGenViewportWnd *wnd) { return traceDownFromIface(pos, clip_pos, wnd); }


bool DeBrush::traceDownFromIface(const Point3 &pos, Point3 &clip_pos, IGenViewportWnd *wnd)
{
  const bool prevUseVisible = DagorPhys::use_only_visible_colliders;
  DagorPhys::use_only_visible_colliders = true;

  const bool ret = Brush::traceDown(pos, clip_pos, wnd);

  DagorPhys::use_only_visible_colliders = prevUseVisible;

  return ret;
}
