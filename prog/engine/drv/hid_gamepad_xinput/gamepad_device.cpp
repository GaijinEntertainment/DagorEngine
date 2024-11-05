// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gamepad_device.h"
#include <math/dag_mathUtils.h>
#include <drv/hid/dag_hiGlobals.h>
#include <supp/_platform.h>
#include "emu_hooks.h"
#if _TARGET_PC_WIN
#include <xinput.h>
#elif _TARGET_XBOX
#include "xboxOneGamepad.h"
#endif
#include <startup/dag_demoMode.h>
#include <debug/dag_debug.h>
#include <perfMon/dag_cpuFreq.h>

#define VIRTUAL_THUMBKEY_PRESS_THRESHOLD 0.5f

const int HumanInput::axis_dead_thres[2][3] = {
  {XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE, XINPUT_GAMEPAD_TRIGGER_THRESHOLD},
  {XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE, XINPUT_GAMEPAD_TRIGGER_THRESHOLD},
};

#define XBOX360_GAMEPAD_RUMBLE_MAX_SPEED 65535

HumanInput::Xbox360GamepadDevice::Xbox360GamepadDevice(int gamepad_no, const char *_name, int ord_id, bool is_virtual)
{
  DWORD port;

  name = NULL;
  client = NULL;
  ordId = ord_id;
  userId = gamepad_no;
  isVirtual = is_virtual;
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
  axis[6].val = &state.z;
  for (int i = 0; i < AXES_NUM; i++)
  {
    axis[i].kMul = 1.0f;
    axis[i].kAdd = 0.0f;
  }

  doRumble(0, 0);
  deviceType = -1;
}
HumanInput::Xbox360GamepadDevice::~Xbox360GamepadDevice()
{
  setClient(NULL);
  name = NULL;
}


bool HumanInput::Xbox360GamepadDevice::updateState(int dt_msec, bool def, bool has_def)
{
  if (isVirtual)
    return false;

  static int lastKeyPressedTime = 0;
  int ctime = ::get_time_msec();
  XINPUT_STATE st;

  if (state.buttons.hasAny() && ctime > lastKeyPressedTime + stg_joy.keyRepeatDelay)
  {
    lastKeyPressedTime = ctime;
    state.repeat = true;
    if (def)
      raw_state_joy.repeat = true;
  }
  else
  {
    state.repeat = false;
    if (def)
      raw_state_joy.repeat = false;
  }

  if (XInputGetState(userId, &st) != ERROR_SUCCESS)
  {
    deviceType = -1;
    return false;
  }

  if (deviceType == -1)
  {
    deviceType = 0;
  }

  if (st.dwPacketNumber == xStPktId)
  {
    state.buttonsPrev = state.buttons;
    for (int i = 0, inc = 0; i < state.MAX_BUTTONS; i += inc)
      if (state.buttonsPrev.getIter(i, inc))
        state.bDownTms[i] += dt_msec;

    if (def)
    {
      raw_state_joy.buttonsPrev = raw_state_joy.buttons;
      memcpy(raw_state_joy.bDownTms, state.bDownTms, sizeof(raw_state_joy.bDownTms));
    }

    if (humaninputxbox360::mouse_emu)
      humaninputxbox360::mouse_emu->updateDevices();
    return false;
  }

  xStPktId = st.dwPacketNumber;

  state.buttonsPrev = state.buttons;
  state.buttons.reset();
  state.buttons.orWord0(st.Gamepad.wButtons);
  if (st.Gamepad.bLeftTrigger >= 36 && st.Gamepad.bLeftTrigger > axis_dead_thres[deviceType][2])
    state.buttons.orWord0(JOY_XINPUT_REAL_MASK_L_TRIGGER);
  if (st.Gamepad.bRightTrigger >= 36 && st.Gamepad.bRightTrigger > axis_dead_thres[deviceType][2])
    state.buttons.orWord0(JOY_XINPUT_REAL_MASK_R_TRIGGER);


  auto scaleAxis = [](float value, float max_value) -> int {
    return (int)::roundf(
      ::cvt(value, -max_value, max_value, (float)HumanInput::JOY_XINPUT_MIN_AXIS_VAL, (float)HumanInput::JOY_XINPUT_MAX_AXIS_VAL));
  };

  auto applyDeadzone = [](int value, float threshold, float max_value) -> float {
    return ::sign((float)value) * ::cvt((float)std::abs(value), threshold, max_value, 0.f, max_value);
  };

  auto applyDeadzoneAndScale = [&scaleAxis, &applyDeadzone](int value, int threshold,
                                 float max_value = HumanInput::JOY_XINPUT_MAX_AXIS_VAL) -> int {
    int clampedValue = applyDeadzone(value, clamp(threshold, 0, JOY_XINPUT_MAX_AXIS_VAL - 1000), max_value);
    return scaleAxis(clampedValue, max_value);
  };
  auto applyStickDeadzoneAndScale = [&scaleAxis, &applyDeadzone](int &dest_x, int &dest_y, int vx, int vy, int deadzone) -> void {
    deadzone = clamp(deadzone, 0, JOY_XINPUT_MAX_AXIS_VAL - 1000);
    dest_x = vx;
    dest_y = vy;
    if (deadzone)
    {
      unsigned int d2 = sqr(unsigned(abs(vx))) + sqr(unsigned(abs(vy)));
      if (d2 < unsigned(deadzone * deadzone))
        dest_x = dest_y = 0;
      else
      {
        float d = sqrtf(d2), s = float(JOY_XINPUT_MAX_AXIS_VAL) / (JOY_XINPUT_MAX_AXIS_VAL - deadzone);
        dest_x = (int)::roundf((vx - deadzone / d * vx) * s);
        dest_y = (int)::roundf((vy - deadzone / d * vy) * s);
      }
    }

    unsigned int d2 = sqr(unsigned(abs(dest_x))) + sqr(unsigned(abs(dest_y)));
    if (d2 > JOY_XINPUT_MAX_AXIS_VAL * JOY_XINPUT_MAX_AXIS_VAL)
    {
      float d = sqrtf(d2);
      dest_x = (int)::roundf(dest_x * JOY_XINPUT_MAX_AXIS_VAL / d);
      dest_y = (int)::roundf(dest_y * JOY_XINPUT_MAX_AXIS_VAL / d);
    }
  };
  applyStickDeadzoneAndScale(state.x, state.y, st.Gamepad.sThumbLX, st.Gamepad.sThumbLY,
    axis_dead_thres[deviceType][0] * stickDeadZoneScale[0]);
  applyStickDeadzoneAndScale(state.rx, state.ry, st.Gamepad.sThumbRX, st.Gamepad.sThumbRY,
    axis_dead_thres[deviceType][1] * stickDeadZoneScale[1]);
  state.slider[0] = applyDeadzoneAndScale(st.Gamepad.bLeftTrigger, axis_dead_thres[deviceType][2], 255);
  state.slider[1] = applyDeadzoneAndScale(st.Gamepad.bRightTrigger, axis_dead_thres[deviceType][2], 255);

  if (!stg_joy.disableVirtualControls)
  {
    state.z = state.slider[1] - state.slider[0];

    // Virtual keys

    if (st.Gamepad.sThumbLX > MAXSHORT * VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
      state.buttons.orWord0(JOY_XINPUT_REAL_MASK_L_THUMB_RIGHT);
    else if (st.Gamepad.sThumbLX < MAXSHORT * -VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
      state.buttons.orWord0(JOY_XINPUT_REAL_MASK_L_THUMB_LEFT);

    if (st.Gamepad.sThumbLY > MAXSHORT * VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
      state.buttons.orWord0(JOY_XINPUT_REAL_MASK_L_THUMB_UP);
    else if (st.Gamepad.sThumbLY < MAXSHORT * -VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
      state.buttons.orWord0(JOY_XINPUT_REAL_MASK_L_THUMB_DOWN);

    if (st.Gamepad.sThumbRX > MAXSHORT * VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
      state.buttons.orWord0(JOY_XINPUT_REAL_MASK_R_THUMB_RIGHT);
    else if (st.Gamepad.sThumbRX < MAXSHORT * -VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
      state.buttons.orWord0(JOY_XINPUT_REAL_MASK_R_THUMB_LEFT);

    if (st.Gamepad.sThumbRY > MAXSHORT * VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
      state.buttons.orWord0(JOY_XINPUT_REAL_MASK_R_THUMB_UP);
    else if (st.Gamepad.sThumbRY < MAXSHORT * -VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
      state.buttons.orWord0(JOY_XINPUT_REAL_MASK_R_THUMB_DOWN);
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

  if (humaninputxbox360::mouse_emu)
    humaninputxbox360::mouse_emu->updateDevices();
  if (humaninputxbox360::kbd_emu)
    humaninputxbox360::kbd_emu->updateDevices();

  if (def)
    raw_state_joy = state;

  if (client)
    client->stateChanged(this, ordId);

  // debug ( "x=%6d y=%6d  rx=%6d ry=%6d  s0=%6d s1=%6d", state.x, state.y, state.rx, state.ry, state.slider[0], state.slider[1] );
  if (def)
    dagor_demo_reset_idle_timer();
  else if (
    has_def && ((state.getKeysPressed().getWord0() | state.getKeysReleased().getWord0()) & (JOY_XINPUT_REAL_MASK_L_THUMB_RIGHT - 1)))
    dagor_demo_reset_idle_timer();

#if _TARGET_PC
  return !state.buttons.cmpEq(state.buttonsPrev);
#else
  return state.buttons.getWord0() & (JOY_XINPUT_REAL_MASK_L_THUMB_RIGHT - 1);
#endif
}

void HumanInput::Xbox360GamepadDevice::doRumble(float lowFreq, float highFreq)
{
  unsigned short newRumbleLeftMotor = XBOX360_GAMEPAD_RUMBLE_MAX_SPEED * lowFreq;
  unsigned short newRumbleRightMotor = XBOX360_GAMEPAD_RUMBLE_MAX_SPEED * highFreq;
  if (rumbleLeftMotor != 0 || rumbleRightMotor != 0 || newRumbleLeftMotor != 0 || newRumbleRightMotor != 0) // The rumble must be
                                                                                                            // applied continuously or
                                                                                                            // it will fade out.
  {
    rumbleLeftMotor = newRumbleLeftMotor;
    rumbleRightMotor = newRumbleRightMotor;

    XINPUT_VIBRATION xiv;
    xiv.wLeftMotorSpeed = rumbleLeftMotor;
    xiv.wRightMotorSpeed = rumbleRightMotor;
    int res = XInputSetState(userId, &xiv);
    if (res == ERROR_BUSY)
      rumbleLeftMotor = rumbleRightMotor = (unsigned short)-1;
  }
}

bool HumanInput::Xbox360GamepadDevice::isConnected()
{
  if (isVirtual)
    return true;

  XINPUT_STATE st;
  return (XInputGetState(userId, &st) == ERROR_SUCCESS);
}

HumanInput::IGenHidClassDrv *humaninputxbox360::mouse_emu = NULL;
HumanInput::IGenHidClassDrv *humaninputxbox360::kbd_emu = NULL;
