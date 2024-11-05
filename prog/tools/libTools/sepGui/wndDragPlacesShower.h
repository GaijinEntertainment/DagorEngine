// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "constants.h"
#include <math/integer/dag_IPoint2.h>


class WinManager;
class CascadeWindow;


class DragPlacesShower
{
public:
  DragPlacesShower(WinManager *owner);
  CascadeWindow *getWindowAlign(const IPoint2 &mouse_pos, WindowAlign &align);
  void Update(const IPoint2 &mouse_pos, void *moving_handle);
  void Stop();

private:
  void drawAlignWindows(CascadeWindow *window, WindowAlign align, void *moving_handle);
  WindowAlign getRootAlign(const IPoint2 &mouse_pos);
  void ReDrawRootPlaces(bool show);
  void setDefaultAlpha();
  bool isInside(void *h_wnd, const IPoint2 &mouse_pos);

private:
  void *mLeft, *mRight, *mTop, *mBottom;
  WinManager *mOwner;
  void *mCurAlignWindow;
  void *mDragPlaceWindowHwnd;

  CascadeWindow *mCurWindow;
  WindowAlign mAlign;
  bool mMoving;
};
