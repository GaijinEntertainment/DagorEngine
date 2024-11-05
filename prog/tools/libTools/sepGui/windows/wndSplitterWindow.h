// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <sepGui/wndMenuInterface.h>
#include "../wndBase.h"


class ClientWindow;
class VirtualWindow;
class Menu;


class SplitterWindow : public CascadeWindow, public BaseWindow, public IMenuEventHandler
{
public:
  SplitterWindow(VirtualWindow *parent, WinManager *owner, bool is_vertical, int x, int y, unsigned w, unsigned h);
  ~SplitterWindow();

  virtual void getPos(IPoint2 &left_top, IPoint2 &right_bottom) const;
  virtual void resize(const IPoint2 &left_top, const IPoint2 &right_bottom);

  virtual bool getMinSize(hdpi::Px &min_w, hdpi::Px &min_h) const;
  virtual WindowSizeLock getSizeLock() const;

  virtual intptr_t winProc(void *h_wnd, unsigned msg, TSgWParam w_param, TSgLParam l_param);

  virtual ClientWindow *getWindowOnPos(const IPoint2 &cur_point);
  virtual VirtualWindow *getVirtualWindow() const;
  virtual ClientWindow *getClientWindow() const;

  void moveTo(IPoint2 mouse_pos);
  void drawMovingLine(IPoint2 mouse_pos);
  bool getIsVertical() const;
  void click(IPoint2 mouse_pos);

  // IMenuEventHandler
  virtual int onMenuItemClick(unsigned id);

private:
  bool checkFixed();
  void setNormalCursor();
  bool canMoveSplitter();

  bool mIsVertical;
  IPoint2 mMousePosOnClick;
  bool mMoving;
  bool mIsHold;
  void *mHdc;
  int mOldDistance;

  Menu *mPopupMenuActions;
};
