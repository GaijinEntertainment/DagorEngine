// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <workCycle/dag_startupModules.h>
#include <startup/dag_restart.h>
#include <drv/hid/dag_hiCreate.h>
#include <drv/hid/dag_hiPointing.h>
#include <startup/dag_inpDevClsDrv.h>
#include <generic/dag_initOnDemand.h>
#include "workCyclePriv.h"
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_driver.h>

class MouseRestartProcWnd : public SRestartProc
{
public:
  virtual const char *procname() { return "mouse@Windows"; }
  MouseRestartProcWnd() : SRestartProc(RESTART_INPUT | RESTART_VIDEO) {}
  void startup()
  {
    global_cls_drv_pnt = HumanInput::createWinMouseClassDriver();
    int w, h;
    d3d::get_screen_size(w, h);
    global_cls_drv_pnt->getDevice(0)->setClipRect(0, 0, w, h);
  }
  void shutdown()
  {
    if (global_cls_drv_pnt)
      global_cls_drv_pnt->destroy();
    global_cls_drv_pnt = NULL;
  }
};

class MouseRestartModeProcWnd : public SRestartProc
{
public:
  virtual const char *procname() { return "mouseMode@Windows"; }
  MouseRestartModeProcWnd() : SRestartProc(RESTART_VIDEODRV | RESTART_DRIVER_VIDEO_MODE) {}
  void startup()
  {
    if (global_cls_drv_pnt)
    {
      int w, h;
      d3d::get_screen_size(w, h);
      global_cls_drv_pnt->getDevice(0)->setClipRect(0, 0, w, h);
    }
  }
  void shutdown() {}
};

static InitOnDemand<MouseRestartProcWnd> mouse_wnd_rproc;
static InitOnDemand<MouseRestartModeProcWnd> mouse_mode_wnd_rproc;

void dagor_init_mouse_win()
{
  mouse_wnd_rproc.demandInit();
  mouse_mode_wnd_rproc.demandInit();
  add_restart_proc(mouse_wnd_rproc);
  add_restart_proc(mouse_mode_wnd_rproc);
}
