// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "constants.h"
#include <math/integer/dag_IPoint2.h>
#include <libTools/util/hdpiUtil.h>


class VirtualWindow;
class ClientWindow;
class WinManager;


class BaseWindow
{
public:
  BaseWindow() : mHWnd(0){};
  virtual ~BaseWindow();

  virtual intptr_t winProc(void *h_wnd, unsigned msg, TSgWParam w_param, TSgLParam l_param);

  void *getHandle() const { return mHWnd; }
  void setHandle(void *hwnd) { mHWnd = hwnd; }

  void updateWindow();

protected:
  void setWindowPos(int x, int y, int width, int height);

  void *mHWnd;
};


class CascadeWindow
{
public:
  CascadeWindow(WinManager *owner, VirtualWindow *parent, int x, int y, unsigned w, unsigned h);

  virtual ~CascadeWindow(){};

  virtual void getPos(IPoint2 &left_top, IPoint2 &right_bottom) const = 0;
  virtual void resize(const IPoint2 &left_top, const IPoint2 &right_bottom) = 0;

  virtual bool getMinSize(hdpi::Px &min_w, hdpi::Px &min_h) const = 0;
  inline bool getMinSizeIP2(IPoint2 &min_px) const;
  virtual WindowSizeLock getSizeLock() const = 0;

  virtual ClientWindow *getWindowOnPos(const IPoint2 &cur_point) = 0;
  virtual VirtualWindow *getVirtualWindow() const = 0;
  virtual ClientWindow *getClientWindow() const = 0;

  VirtualWindow *getParent() const { return mParent; }
  void setParent(VirtualWindow *new_parent) { mParent = new_parent; }

  WinManager *getOwner() const { return mOwner; }

protected:
  WinManager *mOwner;
  VirtualWindow *mParent;

  IPoint2 mLeftTop, mRightBottom;
};


void client_to_screen(void *hWnd, IPoint2 &pos);
void screen_to_client(void *hWnd, IPoint2 &pos);
inline bool CascadeWindow::getMinSizeIP2(IPoint2 &min_px) const
{
  hdpi::Px min_w, min_h;
  bool res = getMinSize(min_w, min_h);
  min_px.x = _px(min_w);
  min_px.y = _px(min_h);
  return res;
}
