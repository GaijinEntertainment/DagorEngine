#pragma once

#include <sepGui/wndMenuInterface.h>
#include "../wndBase.h"

class ClientWindow;
class VirtualWindow;
class WinManager;
class Menu;


struct MovableOriginalPos
{
  VirtualWindow *mGrandParent;
  bool mIsParentFirst;
  WindowAlign mWindowAlign;
  float mSize; // size or ratio
};


//=============================================================================
class MovableWindow : public BaseWindow, public IMenuEventHandler
{
public:
  MovableWindow(WinManager *win_manager, ClientWindow *client_window);
  ~MovableWindow();

  intptr_t winProc(void *h_wnd, unsigned msg, TSgWParam w_param, TSgLParam l_param);

  void setClientWindow(ClientWindow *client_window);
  ClientWindow *getClientWindow();

  void lockSize(WindowSizeLock size_lock, int w, int h);
  WindowSizeLock getSizeLock() const;

  void setOriginalPlace(VirtualWindow *grand_parent, bool is_parent_first, WindowAlign align, float size);
  void onVirtualWindowDeleted(VirtualWindow *window);

  // IMenuEventHandler
  virtual int onMenuItemClick(unsigned id);

private:
  WinManager *mOwner;
  ClientWindow *mClientWindow;

  bool mDragMovable;

  WindowSizeLock mSizeLock;
  int mLockW, mLockH;

  Menu *mMenuActions;

  MovableOriginalPos *mOriginalPosInfo;

  bool mWasMaximized;
  IPoint2 mLeftTop, mRightBottom;
};
