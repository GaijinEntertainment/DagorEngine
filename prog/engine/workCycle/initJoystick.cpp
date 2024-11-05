// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <workCycle/dag_startupModules.h>
#include <startup/dag_restart.h>
#include <drv/hid/dag_hiCreate.h>
#include <drv/hid/dag_hiJoystick.h>
#include <drv/hid/dag_hiDInput.h>
#include <startup/dag_inpDevClsDrv.h>
#include <generic/dag_initOnDemand.h>
#include "workCyclePriv.h"

class JoystickRestartProcDInput : public SRestartProc
{
public:
  virtual const char *procname() { return "joystick@dinput"; }
  JoystickRestartProcDInput() : SRestartProc(RESTART_INPUT | RESTART_VIDEO) {}

  void startup() { global_cls_drv_joy = HumanInput::createJoystickClassDriver(); }

  void shutdown()
  {
    if (global_cls_drv_joy)
      global_cls_drv_joy->destroy();
    global_cls_drv_joy = NULL;
  }
};

static InitOnDemand<JoystickRestartProcDInput> joy_rproc;

void dagor_init_joystick()
{
  joy_rproc.demandInit();

  HumanInput::startupDInput();
  add_restart_proc(joy_rproc);
}
