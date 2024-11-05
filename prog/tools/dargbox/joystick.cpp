// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "joystick.h"
#include <debug/dag_debug.h>


#if _TARGET_PC
#include <startup/dag_restart.h>
#include <startup/dag_inpDevClsDrv.h>
#include <startup/dag_globalSettings.h>
#include <generic/dag_initOnDemand.h>
#include <drv/hid/dag_hiJoystick.h>
#include <drv/hid/dag_hiDInput.h>
#include <drv/hid/dag_hiCreate.h>
#include <ioSys/dag_dataBlock.h>
#include "main.h"


HumanInput::IGenJoystickClassDrv *xinput_cls_drv = NULL;

class JoyRestartProc : public SRestartProc
{
public:
  const char *procname() { return "dargbox_joystick"; }
  JoyRestartProc() : SRestartProc(RESTART_INPUT) {}

  void startup()
  {
    global_cls_drv_joy = HumanInput::createJoystickClassDriver(true, ::dgs_get_settings()->getBool("joystickRemap360", false));

    if (!global_cls_drv_joy)
      return;

#if _TARGET_PC_WIN
    xinput_cls_drv = HumanInput::createXinputJoystickClassDriver();
    global_cls_drv_joy->setSecondaryClassDrv(xinput_cls_drv);
#endif

    if (global_cls_drv_joy->getDeviceCount() > 0)
      global_cls_drv_joy->enable(true);
  }

  void shutdown()
  {
    if (global_cls_drv_joy)
      global_cls_drv_joy->setSecondaryClassDrv(NULL);
    destroy_it(global_cls_drv_joy);
    destroy_it(xinput_cls_drv);
  }
};

static InitOnDemand<JoyRestartProc> joy_rproc;
#else
#include <workCycle/dag_startupModules.h>
#endif


void dargbox_init_joystick()
{
#if _TARGET_PC
  joy_rproc.demandInit();

#if _TARGET_PC_WIN
  HumanInput::startupDInput();
#endif
  add_restart_proc(joy_rproc);
#else
  ::dagor_init_joystick();
#endif
}
