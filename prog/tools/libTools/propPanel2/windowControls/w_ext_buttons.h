// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#pragma once

#include "w_simple_controls.h"


class WExtButtons : public WindowControlEventHandler, public WContainer
{
public:
  WExtButtons(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h);

  ~WExtButtons();

  int getValue() const;
  void setItemFlags(int flags);

  virtual void setEnabled(bool enabled);
  virtual void resizeWindow(int w, int h);
  virtual void moveWindow(int x, int y);

  virtual void onWcClick(WindowBase *source);

private:
  int getItemFlag(int item_id);

  WButton mMenuButton;
  mutable int mButtonStatus;
  WindowControlEventHandler *mExternalHandler;
  int mFlags;
};


class WPlusButton : public WButton
{
public:
  WPlusButton(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h);

  virtual intptr_t onButtonStaticDraw(void *hdc);
  virtual intptr_t onButtonDraw(void *hdc);

private:
  WindowControlBase mStatic;
};
