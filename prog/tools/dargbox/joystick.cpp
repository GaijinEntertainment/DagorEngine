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
#include <drv/hid/dag_hiComposite.h>
#include <ioSys/dag_dataBlock.h>
#include "main.h"


class JoyRestartProc : public SRestartProc
{
public:
  const char *procname() override { return "dargbox_joystick"; }
  JoyRestartProc() : SRestartProc(RESTART_INPUT) {}

  void startup() override
  {
    ::global_cls_composite_drv_joy = HumanInput::CompositeJoystickClassDriver::create();
    bool remapAsX360 = ::dgs_get_settings()->getBool("joystickRemap360", false);
    HumanInput::IGenJoystickClassDrv *drivers[] = {
#if _TARGET_PC_WIN
      HumanInput::createXinputJoystickClassDriver(), // XInput first, avoid misinterpretation of btns/axes
      HumanInput::createJoystickClassDriver(true, remapAsX360),
#else
      HumanInput::createJoystickClassDriver(false, remapAsX360),
#endif
    };

    if (auto composite = ::global_cls_composite_drv_joy)
    {
      ::global_cls_drv_joy = composite;
      for (int i = 0, totalDrivers = countof(drivers); i < totalDrivers; ++i)
        composite->addClassDrv(drivers[i], i == 0);
    }
    else // fallback to chaining the drivers if could not create composite driver
    {
      ::global_cls_drv_joy = drivers[0];
      for (int i = 1; i < countof(drivers); ++i)
        drivers[i - 1]->setSecondaryClassDrv(drivers[i]);
    }

    ::global_cls_drv_joy->enable(true); // Some drivers take a long time to populate device list
  }


  void shutdown() override
  {
    if (::global_cls_drv_joy)
    {
      ::global_cls_drv_joy->destroy();
      ::global_cls_drv_joy = nullptr;
    }
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
