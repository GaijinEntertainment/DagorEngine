#pragma once

#include "../wndBase.h"

class WinManager;
class Menu;


class MainWindow : public BaseWindow
{
public:
  MainWindow(WinManager *cur_owner);
  ~MainWindow();

  virtual intptr_t winProc(void *h_wnd, unsigned msg, TSgWParam w_param, TSgLParam l_param);

  Menu *getMenu() { return mMenu; }

private:
  void resizeRoot();

  Menu *mMenu;
  WinManager *mOwner;

  bool mWasMaximized;
  IPoint2 mLeftTop, mRightBottom;
};
