// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <sepGui/wndMenuInterface.h>
#include "../wndBase.h"
#include "../wndClientArea.h"
#include <util/dag_simpleString.h>


class VirtualWindow;
class MovableWindow;
class LayoutWindowHeader;
class Menu;
class IWndEmbeddedWindow;


//=============================================================================
class ClientWindow : public CascadeWindow, public BaseWindow, public IMenuEventHandler
{
public:
  ClientWindow(VirtualWindow *parent, WinManager *owner, const char *caption, int x, int y, unsigned w, unsigned h);
  ~ClientWindow();

  void notifyWmDestroyWindow();

  virtual void getPos(IPoint2 &left_top, IPoint2 &right_bottom) const;
  virtual void resize(const IPoint2 &left_top, const IPoint2 &right_bottom);

  virtual bool getMinSize(hdpi::Px &min_w, hdpi::Px &min_h) const;
  virtual WindowSizeLock getSizeLock() const;

  virtual intptr_t winProc(void *h_wnd, unsigned msg, TSgWParam w_param, TSgLParam l_param);

  virtual ClientWindow *getWindowOnPos(const IPoint2 &cur_point);
  virtual VirtualWindow *getVirtualWindow() const;
  virtual ClientWindow *getClientWindow() const;

  void setMinSize(hdpi::Px min_w, hdpi::Px min_h);
  void setMenuArea(IPoint2 pos, IPoint2 size);

  void setType(int type);
  int getType() const;

  void setFixed(bool is_fixed);
  bool getFixed() const;

  void setCaption(const char *caption);
  const char *getCaption() const;

  void setMovable(MovableWindow *movable);
  MovableWindow *getMovable();
  bool onMakingMovable(int &width, int &height);

  // window header
  void setHeader(HeaderPos header_pos);
  void headerUpdate();
  void *getClientAreaHandle() { return mPClientArea->getHandle(); }

  // IMenuEventHandler
  virtual int onMenuItemClick(unsigned id);

  Menu *getMenu() { return mPopupMenuUser; }

private:
  int calcMousePos(int left, int right, int min_old, int size_new, bool to_left, bool to_root);

  bool mIsFixed;

  IPoint2 mMouseClickPos;

  int mType;
  IPoint2 mMenuAreaPos;
  IPoint2 mMenuAreaSize;

  Menu *mPopupMenuActions, *mPopupMenuUser;

  hdpi::Px mMinWidth;
  hdpi::Px mMinHeight;

  LayoutWindowHeader *mPHeader;
  ClientWindowArea *mPClientArea;
  HeaderPos mHeaderPos;

  MovableWindow *mMovableWindow;

  SimpleString mCaption;

  IWndEmbeddedWindow *mEmbeddedWindow;
};
