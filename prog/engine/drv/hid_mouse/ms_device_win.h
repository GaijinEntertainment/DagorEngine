// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/hid/dag_hiPointing.h>
#include <osApiWrappers/dag_wndProcComponent.h>
#include "ms_device_common.h"
#include "api_wrappers.h"

namespace HumanInput
{

class WinMouseDevice : public GenericMouseDevice, public IWndProcComponent
{
public:
  WinMouseDevice();
  ~WinMouseDevice();

  // IGenPointing interface implementation
  virtual const char *getName() const { return "Mouse@RawInput"; }

  virtual void setRelativeMovementMode(bool enable);
  virtual bool getRelativeMovementMode() { return relMode; }

  virtual void setPosition(int x, int y);
  virtual bool isPointerOverWindow();
  virtual void setClipRect(int l, int t, int r, int b);
  virtual void setAlwaysClipToWindow(bool on);

  // IWndProcComponent interface implementation
  virtual RetCode process(void *hwnd, unsigned msg, uintptr_t wParam, intptr_t lParam, intptr_t &result);

  void update();

  virtual void fetchLastState();

protected:
  bool relMode = false, overWnd = false;
  bool needToKeepClipToWindow = true;
  bool cursorClipPerformed = false;
  bool shouldAlwaysClipTopWindows = false;

  void deviceSetPosition(int x, int y);

#if _TARGET_PC_WIN
  bool relativeModeNextTime = false;
  bool canClipCursor();

  void updateState(int wParam, int lParam);
  void onButtonDownWin(int btn_id, int wParam, int lParam);
  void onButtonUpWin(int btn_id, int wParam, int lParam);
  void updateCursorClipping();
  void releaseCursorClipping();
#endif

private:
  bool getCursorPos(POINT &pt, RECT *wr = NULL);

  int lbmPressedT0, lbmReleasedT0, lbmMotion, lbmMotionDx, lbmMotionDy;
#if _TARGET_PC_MACOSX
  bool ignoreNextDelta = false;
#endif
};
} // namespace HumanInput
