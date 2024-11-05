// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiJoystick.h>
#include <drv/hid/dag_hiJoyData.h>
#include <drv/hid/dag_hiKeyboard.h>
#include <drv/hid/dag_hiKeybData.h>
#include <drv/hid/dag_hiKeybIds.h>

#if _TARGET_TVOS
#include <drv/hid/dag_hiXInputMappings.h>
#include <drv/hid/dag_hiTvosMap.h>
#endif

extern bool fullScreenMovieSkipped;

namespace movieinput
{
static HumanInput::IGenJoystickClient *oldJoyClient = NULL;
static HumanInput::IGenJoystickClassDrv *jdrv = NULL;
static HumanInput::IGenJoystick *joy = NULL;

#if _TARGET_PC | _TARGET_C1 | _TARGET_C2 | _TARGET_XBOX
static HumanInput::IGenKeyboardClient *oldKbdClient = NULL;
static HumanInput::IGenKeyboard *kbd = NULL;
static HumanInput::KeyboardRawState kbdPrevState;
#endif
#if _TARGET_TVOS
static HumanInput::ButtonBits tvos_button_mask;
#endif

static void on_movie_start()
{
  jdrv = ::global_cls_drv_joy;
  if (jdrv)
    for (int i = 0; i < jdrv->getDeviceCount(); i++)
      if (jdrv->getDevice(i))
        jdrv->getDevice(i)->doRumble(0.0, 0.0);

  joy = jdrv ? jdrv->getDefaultJoystick() : NULL;
  oldJoyClient = joy ? joy->getClient() : NULL;
  if (joy)
  {
    joy->setClient(NULL);
  }
  if (jdrv)
    jdrv->updateDevices();

#if _TARGET_PC | _TARGET_C1 | _TARGET_C2 | _TARGET_XBOX
  kbd = ::global_cls_drv_kbd ? ::global_cls_drv_kbd->getDevice(0) : NULL;
  oldKbdClient = kbd ? kbd->getClient() : NULL;
  if (kbd)
    kbd->setClient(NULL);
  if (::global_cls_drv_kbd)
    ::global_cls_drv_kbd->updateDevices();
  if (kbd)
    kbdPrevState = kbd->getRawState();
#endif
#if _TARGET_TVOS
  tvos_button_mask.setRange(HumanInput::TVOS_REMOTE_DPAD_CLICK_TOP_LEFT, HumanInput::TVOS_REMOTE_BUTTON_MENU);
  tvos_button_mask.setRange(HumanInput::JOY_XINPUT_REAL_BTN_A, HumanInput::JOY_XINPUT_REAL_BTN_Y);
#endif
}

static bool is_movie_interrupted(bool allDevices)
{
#if _TARGET_PC | _TARGET_C1 | _TARGET_C2 | _TARGET_XBOX
  if (kbd)
  {
    ::global_cls_drv_kbd->updateDevices();
    if (kbd->getRawState().isKeyDown(HumanInput::DKEY_ESCAPE) && !kbdPrevState.isKeyDown(HumanInput::DKEY_ESCAPE))
    {
      fullScreenMovieSkipped = true;
      return true;
    }
    kbdPrevState = kbd->getRawState();
  }
#endif

  if (jdrv)
  {
    if (jdrv->isDeviceConfigChanged())
    {
      joy = NULL;
      jdrv->refreshDeviceList();
      jdrv->enableAutoDefaultJoystick(true);
      jdrv->setDefaultJoystick(joy);
    }

    jdrv->updateDevices();
    int cnt = jdrv->getDeviceCount();
#if _TARGET_TVOS
    for (int i = 0; i < cnt; i++)
    {
      auto device = jdrv->getDevice(i);
      if (device)
      {
        if (device->getRawState().buttons.bitTest(tvos_button_mask))
        {
          fullScreenMovieSkipped = true;
          return true;
        }
      }
    }
#else

    // check Start, Back, A, B, X, Y
    if (allDevices || !jdrv->getDefaultJoystick())
    {
      for (int i = 0; i < cnt; i++)
        if (jdrv->getDevice(i) && jdrv->getDevice(i)->getRawState().getKeysReleased().getWord0() & 0xF030)
        {
          fullScreenMovieSkipped = true;
          return !(jdrv->getDevice(i)->getRawState().getKeysDown().getWord0() & 0xF030);
        }
    }
    else
    {
      if (jdrv->getDefaultJoystick() && jdrv->getDefaultJoystick()->getRawState().getKeysPressed().getWord0() & 0xF030)
      {
        fullScreenMovieSkipped = true;
        return true;
      }
    }
#endif
  }
  return false;
}

static void on_movie_end()
{
  jdrv = ::global_cls_drv_joy;
  if (jdrv)
    for (int i = 0; i < jdrv->getDeviceCount(); i++)
      if (jdrv->getDevice(i))
        jdrv->getDevice(i)->doRumble(0.0, 0.0);
  if (joy)
  {
    joy->setClient(oldJoyClient);
  }
#if _TARGET_PC | _TARGET_C1 | _TARGET_C2 | _TARGET_XBOX
  if (kbd)
  {
    while (kbd->getRawState().isKeyDown(HumanInput::DKEY_ESCAPE))
    {
      dagor_idle_cycle();
      ::global_cls_drv_kbd->updateDevices();
      sleep_msec(10);
    }
    kbd->setClient(oldKbdClient);
  }
#endif
}
} // namespace movieinput
