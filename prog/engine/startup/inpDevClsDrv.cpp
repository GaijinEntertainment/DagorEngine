#include <startup/dag_inpDevClsDrv.h>
#include <humanInput/dag_hiKeyboard.h>
#include <humanInput/dag_hiKeybIds.h>
#include <humanInput/dag_hiPointing.h>
#include <humanInput/dag_hiJoystick.h>
#include <humanInput/dag_hiGlobals.h>
#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_critSec.h>
#include <string.h>

HumanInput::IGenKeyboardClassDrv *global_cls_drv_kbd = NULL;
HumanInput::IGenPointingClassDrv *global_cls_drv_pnt = NULL;
HumanInput::IGenJoystickClassDrv *global_cls_drv_joy = NULL;
WinCritSec global_cls_drv_update_cs;
bool dgs_inpdev_allow_rawinput = true;
bool dgs_inpdev_rawinput_kbd_inited = false, dgs_inpdev_rawinput_mouse_inited = false;
bool dgs_inpdev_disable_acessibility_shortcuts = false;

void acquire_all_inpdev()
{
  WinAutoLock joyGuard(global_cls_drv_update_cs);
  if (global_cls_drv_kbd)
    global_cls_drv_kbd->acquireDevices();

  if (global_cls_drv_pnt)
    global_cls_drv_pnt->acquireDevices();

  if (global_cls_drv_joy)
    global_cls_drv_joy->acquireDevices();
}

void reset_all_inpdev(bool report_btn_release)
{
#if _TARGET_PC
  if (global_cls_drv_kbd)
    for (int i = global_cls_drv_kbd->getDeviceCount() - 1; i >= 0; i--)
    {
      HumanInput::IGenKeyboard *dev = global_cls_drv_kbd->getDevice(i);
      HumanInput::IGenKeyboardClient *cli = report_btn_release ? dev->getClient() : NULL;
      HumanInput::KeyboardRawState &s = (HumanInput::KeyboardRawState &)dev->getRawState();
      if (cli)
        for (int j = 0; j < 256; j++)
          if (s.isKeyDown(j))
          {
            s.clrKeyDown(j);
            switch (j)
            {
              case HumanInput::DKEY_LALT:
              case HumanInput::DKEY_RALT:
              case HumanInput::DKEY_LWIN:
              case HumanInput::DKEY_RWIN:
                if (dgs_app_active)
                {
                  // report special keys release in 'inactive' app state
                  dgs_app_active = false;
                  cli->gkcButtonUp(dev, j);
                  dgs_app_active = true;
                  continue;
                }

              default: cli->gkcButtonUp(dev, j);
            }
          }
      memset(&s, 0, sizeof(s));
      dev->syncRawState();
    }
  memset(&HumanInput::raw_state_kbd, 0, sizeof(HumanInput::raw_state_kbd));

  if (global_cls_drv_pnt)
    for (int i = global_cls_drv_pnt->getDeviceCount() - 1; i >= 0; i--)
    {
      HumanInput::IGenPointing *dev = global_cls_drv_pnt->getDevice(i);
      HumanInput::IGenPointingClient *cli = report_btn_release ? dev->getClient() : NULL;
      HumanInput::PointingRawState &s = (HumanInput::PointingRawState &)dev->getRawState();
      if (cli)
        for (int j = 0; j < 32; j++)
          if (s.mouse.isBtnDown(j))
          {
            s.mouse.buttons &= ~(1 << j);
            cli->gmcMouseButtonUp(dev, j);
          }
      s.mouse.buttons = 0;
      s.resetDelta();
    }
  HumanInput::raw_state_pnt.mouse.buttons = 0;
  HumanInput::raw_state_pnt.resetDelta();

  {
    WinAutoLock joyGuard(global_cls_drv_update_cs);
    if (global_cls_drv_joy)
      for (int i = global_cls_drv_joy->getDeviceCount() - 1; i >= 0; i--)
      {
        HumanInput::IGenJoystick *dev = global_cls_drv_joy->getDevice(i);
        HumanInput::IGenJoystickClient *cli = report_btn_release ? dev->getClient() : NULL;
        HumanInput::JoystickRawState &s = (HumanInput::JoystickRawState &)dev->getRawState();
        if (cli)
        {
          s.buttons.reset();
          cli->stateChanged(dev, i);
        }
        memset(&s, 0, sizeof(s));
      }
    // memset(&HumanInput::raw_state_joy, 0, sizeof(HumanInput::raw_state_joy));
  }
#endif
}

void destroy_all_inpdev()
{
  if (global_cls_drv_kbd)
    global_cls_drv_kbd->destroy();

  if (global_cls_drv_pnt)
    global_cls_drv_pnt->destroy();

  if (global_cls_drv_joy)
    global_cls_drv_joy->destroy();

  global_cls_drv_kbd = NULL;
  global_cls_drv_pnt = NULL;
  global_cls_drv_joy = NULL;
}
