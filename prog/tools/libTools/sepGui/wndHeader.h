// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "wndBase.h"


class WinManager;
class ClientWindow;


enum
{
  CLOSE_BUTTON_SIZE = 16,
  CLOSE_BUTTON_FONT_HEIGHT = 12,
  CLOSE_CAPTION_X = 10,
  CLOSE_CAPTION_Y = 2,

  TOP_HEADER_HEIGHT = 21,
  LEFT_HEADER_WIDTH = 8,
};


union HeaderWindows
{
  struct TopHeader
  {
    void *mCloseButtonHandle;
  } mTopHeader;

  struct LeftHeader
  {
    void *mVerticalLineHandle1;
    void *mVerticalLineHandle2;
  } mLeftHeader;
};


class LayoutWindowHeader : public BaseWindow
{
public:
  LayoutWindowHeader(ClientWindow *parent, WinManager *manager, bool is_top = true, int w = 10, int h = 10);
  ~LayoutWindowHeader();

  virtual intptr_t winProc(void *h_wnd, unsigned msg, TSgWParam w_param, TSgLParam l_param);

  void resize(int x, int y, int width, int height);

  void show();
  void hide();

protected:
  void onResize(int width, int height);
  void onCloseClick();

  const char *getCaption();
  void *getParentHandle();

  bool mTopHeader;
  HeaderWindows mInternalHandles;

  ClientWindow *mParent;
  WinManager *mManager;
};
