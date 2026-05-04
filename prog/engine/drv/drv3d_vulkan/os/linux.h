// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#ifndef _TARGET_PC_LINUX
#error using linux specific implementation with wrong platform
#endif

#include <drv/3d/dag_driver.h>
#include "../drv3d_commonCode/drv_utils.h"

namespace drv3d_vulkan
{

struct WindowState
{
  inline static main_wnd_f *mainCallback = nullptr;
  const char *windowTitle = nullptr;
  bool ownsWindow = false;
  int currentResolutionX = 0;
  int currentResolutionY = 0;
  int windowMode = -1;

  struct Settings
  {
    int resolutionX;
    int resolutionY;
    float aspect;
  } settings = {};

  int refreshRate = 0;
  void updateRefreshRateFromCurrentDisplayMode();

  void set(void *hinst, const char *name, int show, void *mainw, void *icon, const char *title, void *wnd_proc);

  inline void getRenderWindowSettings(Driver3dInitCallback *) { getRenderWindowSettings(); }

  void getRenderWindowSettings();

  bool setRenderWindowParams();

  void *getMainWindow();

  void closeWindow();
};

} // namespace drv3d_vulkan
