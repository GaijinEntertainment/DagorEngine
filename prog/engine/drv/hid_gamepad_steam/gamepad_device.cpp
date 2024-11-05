// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gamepad_device.h"
#include <drv/hid/dag_hiGlobals.h>
#include <supp/_platform.h>
#include <startup/dag_demoMode.h>
#include <debug/dag_debug.h>
#include <perfMon/dag_cpuFreq.h>
#include <steam/steam.h>

static constexpr int EXTBTN_LTHUMB_RIGHT = 0x00040000;

const char *HumanInput::SteamGamepadDevice::buttonName[HumanInput::SteamGamepadDevice::BUTTONS_NUM] = {"D.Up", "D.Down", "D.Left",
  "D.Right", "Start", "Select", "L3", "R3", "L1", "R1", "0x0400", "0x0800", "CROSS", "CIRCLE", "SQUARE", "TRIANGLE", "L2", "R2",
  "LS.Right", "LS.Left", "LS.Up", "LS.Down", "RS.Right", "RS.Left", "RS.Up", "RS.Down", "L3.Centered", "R3.Centered"};

const char *HumanInput::SteamGamepadDevice::axisName[HumanInput::SteamGamepadDevice::AXES_NUM] = {
  "L.Thumb.h", "L.Thumb.v", "R.Thumb.h", "R.Thumb.v", "L.Trigger", "R.Trigger"};

#define Steam_GAMEPAD_RUMBLE_MAX_SPEED 65535

HumanInput::SteamGamepadDevice::SteamGamepadDevice(int gamepad_no, const char *_name, int ord_id)
{
  unsigned port;

  name = NULL;
  client = NULL;
  ordId = ord_id;
  userId = gamepad_no;
  xStPktId = 0xFFFFFFFF;

  DEBUG_CTX("inited gamepad %s", _name);
  name = _name;
  memset(&state, 0, sizeof(state));
  for (int i = 0; i < JoystickRawState::MAX_POV_HATS; i++)
    state.povValues[i] = state.povValuesPrev[i] = -1;

  axis[0].val = &state.x;
  axis[1].val = &state.y;
  axis[2].val = &state.rx;
  axis[3].val = &state.ry;
  axis[4].val = &state.slider[0];
  axis[5].val = &state.slider[1];
  for (int i = 0; i < AXES_NUM; i++)
  {
    axis[i].kMul = 1.0f;
    axis[i].kAdd = 0.0f;
  }

  doRumble(0, 0);
  deviceType = -1;
}

HumanInput::SteamGamepadDevice::~SteamGamepadDevice()
{
  setClient(NULL);
  name = NULL;
}

bool HumanInput::SteamGamepadDevice::updateState(int dt_msec, bool def, bool has_def)
{
  static int lastKeyPressedTime = 0;
  int ctime = ::get_time_msec();
  ButtonBits buttonsPrev = state.buttons;
  bool stateRepeat = false;
  unsigned lastXStPktId = xStPktId;

  if (!rumblePassed)
  {
    if (rumbleLeftMotor > 0)
      steamapi::trigger_haptic_pulse(userId, steamapi::STEAM_CONTROLLER_PAD_LEFT, rumbleLeftMotor);
    if (rumbleRightMotor > 0)
      steamapi::trigger_haptic_pulse(userId, steamapi::STEAM_CONTROLLER_PAD_RIGHT, rumbleRightMotor);
  }

  if (state.buttons.hasAny() && ctime > lastKeyPressedTime + stg_joy.keyRepeatDelay)
  {
    lastKeyPressedTime = ctime;
    stateRepeat = true;
  }
  else
  {
    stateRepeat = false;
  }

  if (!steamapi::get_controller_state(userId, &state, &xStPktId))
  {
    deviceType = -1;
    return false;
  }

  state.buttonsPrev = buttonsPrev;
  state.repeat = stateRepeat;
  if (def)
  {
    raw_state_joy.buttonsPrev = buttonsPrev;
    raw_state_joy.repeat = state.repeat;
  }

  if (lastXStPktId == xStPktId)
  {
    for (int i = 0, inc = 0; i < state.MAX_BUTTONS; i += inc)
      if (state.buttonsPrev.getIter(i, inc))
        state.bDownTms[i] += dt_msec;

    if (def)
      memcpy(raw_state_joy.bDownTms, state.bDownTms, sizeof(raw_state_joy.bDownTms));
    return false;
  }

  if (state.getKeysPressed().hasAny()) // Little trick to avoid new static bool isFirstTimePressed
    lastKeyPressedTime = ctime + (stg_joy.keyFirstRepeatDelay - stg_joy.keyRepeatDelay);

  for (int i = 0, inc = 0; i < state.MAX_BUTTONS; i += inc)
    if (state.buttons.getIter(i, inc))
      if (!state.buttonsPrev.get(i))
        state.bDownTms[i] = 0;

  for (int i = 0, inc = 0; i < state.MAX_BUTTONS; i += inc)
    if (state.buttonsPrev.getIter(i, inc))
      state.bDownTms[i] += dt_msec;

  if (def)
    raw_state_joy = state;

  if (client)
    client->stateChanged(this, ordId);

  if (def)
    dagor_demo_reset_idle_timer();
  else if (has_def && ((state.getKeysPressed().getWord0() | state.getKeysReleased().getWord0()) & (EXTBTN_LTHUMB_RIGHT - 1)))
    dagor_demo_reset_idle_timer();

  return !state.buttons.cmpEq(state.buttonsPrev);
}

void HumanInput::SteamGamepadDevice::doRumble(float lowFreq, float highFreq)
{
  rumbleLeftMotor = Steam_GAMEPAD_RUMBLE_MAX_SPEED * lowFreq;
  rumbleRightMotor = Steam_GAMEPAD_RUMBLE_MAX_SPEED * highFreq;
  rumblePassed = false;
}

bool HumanInput::SteamGamepadDevice::isConnected() { return steamapi::get_controller_state(userId, NULL, 0); }
