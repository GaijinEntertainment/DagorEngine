// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#ifndef _TARGET_ANDROID
#error using android specific implementation with wrong platform
#endif

#include <drv/3d/dag_driver.h>
#include <osApiWrappers/dag_progGlobals.h>

namespace drv3d_vulkan
{

struct WindowState
{
  inline static main_wnd_f *mainCallback = nullptr;
  const char *windowTitle = nullptr;
  void *window = nullptr;
  struct Settings
  {
    int resolutionX;
    int resolutionY;
    float aspect;
  } settings = {};

  int refreshRate = 0;
  void updateRefreshRateFromCurrentDisplayMode(){};

  void set(void *, const char *, int, void *, void *, void *, const char *title, void *wnd_proc)
  {
    windowTitle = title;
    mainCallback = (main_wnd_f *)wnd_proc;
    window = win32_get_main_wnd();
  }

  void getRenderWindowSettings(Driver3dInitCallback *) { getRenderWindowSettings(); }

  void getRenderWindowSettings();

  bool setRenderWindowParams()
  {
    settings.aspect = (float)settings.resolutionX / settings.resolutionY;
    window = win32_get_main_wnd();
    return true;
  }

  void *getMainWindow() { return window; }

  void closeWindow() { window = nullptr; }
};

} // namespace drv3d_vulkan