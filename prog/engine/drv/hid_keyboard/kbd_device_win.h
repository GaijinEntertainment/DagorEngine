// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/hid/dag_hiKeyboard.h>
#include <osApiWrappers/dag_wndProcComponent.h>
#include "kbd_device_common.h"

#if _TARGET_PC_LINUX
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBrules.h>
#endif

namespace HumanInput
{

class WinKeyboardDevice : public GenericKeyboardDevice, public IWndProcComponent
{
public:
  WinKeyboardDevice();
  ~WinKeyboardDevice();

  // IWndProcComponent interface implementation
  virtual RetCode process(void *hwnd, unsigned msg, uintptr_t wParam, intptr_t lParam, intptr_t &result);

  // IGenKeyboard interface implementation
  virtual const char *getName() const { return "Keyboard@RawInput"; }

  int pendingKeyDownCode;

  virtual void enableLayoutChangeTracking(bool en);
  virtual void enableLocksChangeTracking(bool en);

  bool layoutChangeTracked;
  bool locksChangeTracked;

#if _TARGET_PC_WIN
  virtual void enableIme(bool on);
  virtual bool isImeActive() const;

#elif _TARGET_PC_LINUX
  Display *display;
  void *xkbfileLibrary;
  typedef Bool (*XkbRF_GetNamesPropDecl)(Display *, char **, XkbRF_VarDefsPtr);
  XkbRF_GetNamesPropDecl xkbRF_GetNamesProp;
#endif

private:
#if _TARGET_PC_WIN | _TARGET_XBOX
  void OnChar(uintptr_t wParam);
  void OnKeyUpKeyDown(unsigned msg, uintptr_t wParam, intptr_t lParam);
#endif
  void updateLayout(bool notify_when_not_changed = false);
  void updateLocks(bool notify_when_not_changed = false);
};
} // namespace HumanInput
