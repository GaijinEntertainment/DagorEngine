// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "joy_drv.h"

#include <drv/hid/dag_hiCreate.h>
#include <drv/hid/dag_hiDInput.h>
#include <drv/hid/dag_hiJoystick.h>
#include <startup/dag_restart.h>
#include <startup/dag_inpDevClsDrv.h>
#include <ioSys/dag_genIo.h>
#include <generic/dag_initOnDemand.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <memory/dag_mem.h>
#include <debug/dag_debug.h>


static void dir_buttons_from_pov_angle(int angle, int &x, int &y)
{
  x = 0;
  y = 0;

  if (angle < 0)
    return;

  if (angle < 6000 || angle > 30000)
    y = 1;
  else if (angle > 12000 && angle < 24000)
    y = -1;

  if (angle > 9000 - 6000 && angle < 9000 + 6000)
    x = 1;
  else if (angle > 27000 - 6000 && angle < 27000 + 6000)
    x = -1;
}

//-----------------------------------------------------------------------------


typedef int UserInputDeviceClassId;
class UserInputDeviceState;

class UserInputDevice
{
public:
  DAG_DECLARE_NEW(inimem)

  virtual ~UserInputDevice() {}

  virtual UserInputDeviceState *getCurrentState() = 0;
  virtual void saveState(IGenSave &) = 0;
  virtual UserInputDeviceState *loadState(IGenLoad &) = 0;
  virtual const char *getButtonName(int btn_id) = 0;
};


class UserInputDeviceState
{
public:
  DAG_DECLARE_NEW(tmpmem)

  virtual ~UserInputDeviceState() {}

  virtual UserInputDevice *getDevice() = 0;
};


class DeviceWithButtonsState : public UserInputDeviceState
{
public:
  virtual unsigned getButtonBits() = 0;
  virtual bool isButtonPressed(int button_id) = 0;
  virtual bool isToggleButton(int button_id) = 0;
};


class GuiJoyState : public DeviceWithButtonsState
{
public:
  HumanInput::JoystickRawState rs;
  UserInputDevice *dev;

public:
  unsigned getButtonBits() override { return rs.buttons.getWord0(); }

  bool isButtonPressed(int button_id) override { return ((DeviceWithButtonsState *)dev)->isButtonPressed(button_id); }

  UserInputDevice *getDevice() override { return dev; }
  bool isToggleButton(int button_id) override { return false; }
};


//-----------------------------------------------------------------------------

// generic joystick class device for GUI
class GuiJoyClassInpDev : public UserInputDevice, public DeviceWithButtonsState
{
public:
  DAG_DECLARE_NEW(inimem)

  enum
  {
    POV_OFFS_IN_BUTTONS = 32,
    NUM_VIRT_POV_BTNS = 4
  };

  GuiJoyClassInpDev() {}

  UserInputDeviceState *getCurrentState() override { return this; }

  void saveState(IGenSave &) override {}

  UserInputDeviceState *loadState(IGenLoad &crd) override
  {
    GuiJoyState *js = new (tmpmem) GuiJoyState;
    crd.read(&js->rs, sizeof(js->rs));
    js->dev = this;
    return js;
  }

  const char *getButtonName(int n) override
  {
    static const char *prettyNames[] = {"^x:DU^", "^x:DD^", "^x:DL^", "^x:DR^",
      "Start", //"Start",
      "^x:BACK^", "^x:LS^", "^x:RS^", "^x:LB^", "^x:RB^", "", "", "^x:A^", "^x:B^", "^x:X^", "^x:Y^", "^x:LT^", "^x:RT^", "LS.Right",
      "LS.Left", "LS.Up", "LS.Down", "RS.Right", "RS.Left", "RS.Up", "RS.Down"};
    if ((n >= 0) && (n < countof(prettyNames)))
      return prettyNames[n];

    return "";
  }

  // DeviceWithButtonsState interface implementation
  unsigned getButtonBits() override
  {
    HumanInput::IGenJoystick *j = global_cls_drv_joy->getDefaultJoystick();
    return j ? j->getRawState().buttons.getWord0() : 0;
  }

  bool isButtonPressed(int button_id) override
  {
    HumanInput::IGenJoystick *j = global_cls_drv_joy->getDefaultJoystick();

    if (!j)
      return false;

    if (button_id < POV_OFFS_IN_BUTTONS)
      return j->getButtonState(button_id);
    else
    {
      int povInd = (button_id - POV_OFFS_IN_BUTTONS) / NUM_VIRT_POV_BTNS;
      int butInd = (button_id - POV_OFFS_IN_BUTTONS) % NUM_VIRT_POV_BTNS;

      int povVal = j->getPovHatAngle(povInd);
      if (povVal < 0)
        return false;

      int x, y;
      dir_buttons_from_pov_angle(povVal, x, y);

      return (butInd == 0 && y > 0) || (butInd == 1 && x > 0) || (butInd == 2 && y < 0) || (butInd == 3 && x < 0);
    }
  }

  UserInputDevice *getDevice() override { return this; }
  bool isToggleButton(int button_id) override { return false; }
};

//-----------------------------------------------------------------------------

// generic joystick client to save events for GUI
class GuiJoyClient : public HumanInput::IGenJoystickClient
{
public:
  void attached(HumanInput::IGenJoystick *joy) override {}
  void detached(HumanInput::IGenJoystick *joy) override {}

  void stateChanged(HumanInput::IGenJoystick * /*joy*/, int /*joy_ord_id*/) override {}
};


static InitOnDemand<GuiJoyClient> gui_joy_client;

//-----------------------------------------------------------------------------

HumanInput::IGenJoystickClassDrv *joy_cls_drv = NULL;
IJoyCallback *joy_cb = 0;


class AdrenalinJoyRestartProc : public SRestartProc
{
public:
  const char *procname() override { return "AdrenalinJoy"; }
  AdrenalinJoyRestartProc() : SRestartProc(RESTART_INPUT | RESTART_VIDEO) {}

  void startup() override
  {
#if _TARGET_PC_WIN // TODO: tools Linux porting: AdrenalinJoyRestartProc::startup
    joy_cls_drv = HumanInput::createXinputJoystickClassDriver();
    if (joy_cls_drv)
    {
      gui_joy_client.demandInit();
      joy_cls_drv->useDefClient(gui_joy_client);

      HumanInput::IGenJoystickClassDrv *sec_joy = HumanInput::createJoystickClassDriver(true, true);
      if (sec_joy)
      {
        joy_cls_drv->setSecondaryClassDrv(sec_joy);
        sec_joy->useDefClient(gui_joy_client);
        joy_cls_drv->setDefaultJoystick(joy_cls_drv->getDevice(0));
        joy_cls_drv->enableAutoDefaultJoystick(false);
        joy_cls_drv->enable(true);
      }
    }
#else
    LOGERR_CTX("TODO: tools Linux porting: AdrenalinJoyRestartProc::startup");
#endif
  }

  void shutdown() override
  {
    if (joy_cls_drv)
      joy_cls_drv->destroy();
    joy_cls_drv = NULL;
  }
};

static InitOnDemand<AdrenalinJoyRestartProc> joy_rproc;
static InitOnDemand<GuiJoyClassInpDev> gui_joy_class_inp_dev;


HumanInput::IGenJoystick *get_joystick()
{
  if (joy_cls_drv && joy_cls_drv->getDeviceCount())
    return joy_cls_drv->getDevice(0); // joy_cls_drv->getDefaultJoystick();
  else
    return NULL;
}


void update_joysticks()
{
  if (!joy_cls_drv)
    return;

  joy_cls_drv->updateDevices();
  if (!joy_cls_drv->isDeviceConfigChanged())
    return;

  joy_cls_drv->refreshDeviceList();
  joy_cls_drv->setDefaultJoystick(joy_cls_drv->getDevice(0));
  joy_cls_drv->enable(joy_cls_drv->getDeviceCount() > 0);

  if (joy_cb)
    joy_cb->updateJoyState();
}

void startup_joystick(IJoyCallback *_joy_cb)
{
  joy_cb = _joy_cb;

#if _TARGET_PC_WIN // TODO: tools Linux porting: startup_joystick: HumanInput::startupDInput
  HumanInput::startupDInput();
#else
  LOGERR_CTX("TODO: tools Linux porting: startup_joystick: HumanInput::startupDInput");
#endif

  joy_rproc.demandInit();
  add_restart_proc(joy_rproc);

  gui_joy_class_inp_dev.demandInit();

  startup_game(RESTART_ALL);
}

void shutdown_joystick()
{
  joy_cb = NULL;
  shutdown_game(RESTART_ALL);
}
