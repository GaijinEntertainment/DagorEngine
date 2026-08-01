// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <workCycle/dag_startupModules.h>
#include <startup/dag_restart.h>
#include <drv/hid/dag_hiCreate.h>
#include <drv/hid/dag_hiKeyboard.h>
#include <startup/dag_inpDevClsDrv.h>
#include <generic/dag_initOnDemand.h>
#include "workCyclePriv.h"

class KeybRestartProcWnd : public SRestartProc
{
public:
  virtual const char *procname() { return "keyboard@WindMsg"; }
  KeybRestartProcWnd() : SRestartProc(RESTART_INPUT | RESTART_VIDEO) {}

  void startup() { global_cls_drv_kbd = HumanInput::createWinKeyboardClassDriver(); }

  void shutdown()
  {
    if (global_cls_drv_kbd)
      global_cls_drv_kbd->destroy();
    global_cls_drv_kbd = NULL;
  }
};
static InitOnDemand<KeybRestartProcWnd> keyb_wnd_rproc;

void dagor_init_keyboard_win()
{
  keyb_wnd_rproc.demandInit();
  add_restart_proc(keyb_wnd_rproc);
}
