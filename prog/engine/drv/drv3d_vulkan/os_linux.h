// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#ifndef _TARGET_PC_LINUX
#error using linux specific implementation with wrong platform
#endif

#include <linux/x11.h>
#include <drv/3d/dag_driver.h>
#include "../drv3d_commonCode/drv_utils.h"

namespace drv3d_vulkan
{

struct WindowState
{
  inline static main_wnd_f *mainCallback = nullptr;
  const char *windowTitle = nullptr;
  bool ownsWindow = false;

  struct Settings
  {
    int resolutionX;
    int resolutionY;
    float aspect;
  } settings = {};

  int refreshRate = 0;
  void updateRefreshRateFromCurrentDisplayMode();

  void set(void *, const char *, int, void *, void *, void *, const char *title, void *wnd_proc)
  {
    x11.init();
    windowTitle = title;
    mainCallback = (main_wnd_f *)wnd_proc;
  }

  inline void getRenderWindowSettings(Driver3dInitCallback *) { getRenderWindowSettings(); }

  inline void getRenderWindowSettings()
  {
    int w, h;
    bool isAuto, isRetina;
    x11.getDisplaySize(w, h, dgs_get_window_mode() == WindowMode::FULLSCREEN_EXCLUSIVE);
    get_settings_resolution(settings.resolutionX, settings.resolutionY, isRetina, w, h, isAuto);
  }

  inline bool setRenderWindowParams()
  {
    closeWindow();

    XVisualInfo vInfoTemplate = {};
    vInfoTemplate.screen = x11.rootScreenIndex;
    long visualMask = VisualScreenMask | VisualDepthMask;

    // this bugs out vulkan render on some compositors/WMs (namely xfce, cinammon)
    // vInfoTemplate.c_class = DirectColor;
    // visualMask |= VisualClassMask;

    int numberOfVisuals;
    XVisualInfo *vi = nullptr;
    for (int i = 0; i < 2 && !vi; ++i)
    {
      vInfoTemplate.depth = i ? 32 : 24;
      vi = XGetVisualInfo(x11.rootDisplay, visualMask, &vInfoTemplate, &numberOfVisuals);
    }
    if (!vi)
      DAG_FATAL("x11: can't get usefull VisualInfo from xlib");

    debug("x11: zero visual info id: %u class: %i depth: %u", vi->visualid, vi->c_class, vi->depth);

    if (x11.initWindow(vi, windowTitle, settings.resolutionX, settings.resolutionY))
    {
      settings.aspect = (float)settings.resolutionX / settings.resolutionY;
      XFree(vi);
      ownsWindow = true;
      return true;
    }

    XFree(vi);
    return false;
  }

  inline void *getMainWindow()
  {
    if (!ownsWindow)
      return nullptr;
    return &x11.mainWindow;
  }

  void closeWindow()
  {
    if (ownsWindow)
    {
      x11.destroyMainWindow();
      ownsWindow = false;
    }
  }
};

} // namespace drv3d_vulkan
