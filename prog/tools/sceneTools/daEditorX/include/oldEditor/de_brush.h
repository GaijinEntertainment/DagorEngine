//
// DaEditorX
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EditorCore/ec_brush.h>


class DeBrush : public Brush
{
public:
  DeBrush(IBrushClient *bc) : Brush(bc) {}

  bool calcCenterFromIface(IGenViewportWnd *wnd);
  bool traceDownFromIface(const Point3 &pos, Point3 &clip_pos, IGenViewportWnd *wnd);

protected:
  virtual bool calcCenter(IGenViewportWnd *wnd);
  virtual bool traceDown(const Point3 &pos, Point3 &clip_pos, IGenViewportWnd *wnd);
};
