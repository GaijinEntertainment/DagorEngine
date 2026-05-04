// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class IModelessWindowController
{
public:
  // Must consist of identifier characters: a-z, A-Z, 0-9, '_', '-', '.' and '~'. See dblk::is_ident_char().
  virtual const char *getWindowId() const = 0;

  // It must release the window. Caller when resetting the layout and when exiting the application.
  virtual void releaseWindow() = 0;

  // It must either show or hide the window.
  // When showing is requested for the first time then the window must be created. Hiding should not release the window.
  virtual void showWindow(bool show = true) = 0;

  // Must return true if the window has been created and visible.
  virtual bool isWindowShown() const = 0;
};
