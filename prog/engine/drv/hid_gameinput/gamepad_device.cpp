// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gamepad_device.h"
#include <math/dag_mathUtils.h>
#include <drv/hid/dag_hiGlobals.h>
#include <supp/_platform.h>
#include <startup/dag_demoMode.h>
#include <debug/dag_debug.h>
#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_statDrv.h>

static constexpr float VIRTUAL_THUMBKEY_PRESS_THRESHOLD = 0.5f;

static constexpr int GAMEPAD_LEFT_THUMB_DEADZONE = (int)(0.24f * 32000);
static constexpr int GAMEPAD_RIGHT_THUMB_DEADZONE = GAMEPAD_LEFT_THUMB_DEADZONE;
static constexpr int GAMEPAD_TRIGGER_THRESHOLD = GAMEPAD_LEFT_THUMB_DEADZONE / 512;

const int HumanInput::axis_dead_thres[2][3] = {
  {GAMEPAD_LEFT_THUMB_DEADZONE, GAMEPAD_RIGHT_THUMB_DEADZONE, GAMEPAD_TRIGGER_THRESHOLD},
  {GAMEPAD_LEFT_THUMB_DEADZONE, GAMEPAD_RIGHT_THUMB_DEADZONE, GAMEPAD_TRIGGER_THRESHOLD},
};

HumanInput::GameInputGamepadDevice::GameInputGamepadDevice(int gamepad_no, const char *_name, int ord_id, bool is_virtual) :
  isVirtual(is_virtual), ordId(ord_id), userId(gamepad_no), name(_name)
{
  DEBUG_CTX("inited gamepad %s", _name);
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

  doRumble(0, 0);
}


HumanInput::GameInputGamepadDevice::~GameInputGamepadDevice()
{
  setClient(nullptr);
  name = nullptr;
}


bool HumanInput::GameInputGamepadDevice::updateState(int dt_msec, bool def, bool has_def)
{
  TIME_PROFILE(HID_GINP_updateState);
  if (isVirtual)
    return false;

  int ctime = ::get_time_msec();

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

  if (!is_gamepad_connected(userId))
  {
    deviceType = -1;
    hasReading = false;
    return false;
  }

  if (deviceType == -1)
    deviceType = 0;

  GamepadReading cur;
  if (!read_gamepad(userId, cur) || (hasReading && memcmp(&cur, &lastReading, sizeof(cur)) == 0))
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

    if (mouse_emu)
      mouse_emu->updateDevices();
    return false;
  }

  lastReading = cur;
  hasReading = true;

  state.buttonsPrev = state.buttons;
  state.buttons.reset();
  state.buttons.orWord0(cur.buttons);
  if (cur.leftTrigger >= 36 && cur.leftTrigger > axis_dead_thres[deviceType][2])
    state.buttons.orWord0(JOY_XINPUT_REAL_MASK_L_TRIGGER);
  if (cur.rightTrigger >= 36 && cur.rightTrigger > axis_dead_thres[deviceType][2])
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
  applyStickDeadzoneAndScale(state.x, state.y, cur.leftThumbX, cur.leftThumbY, axis_dead_thres[deviceType][0] * stickDeadZoneScale[0]);
  applyStickDeadzoneAndScale(state.rx, state.ry, cur.rightThumbX, cur.rightThumbY,
    axis_dead_thres[deviceType][1] * stickDeadZoneScale[1]);
  state.slider[0] = applyDeadzoneAndScale(cur.leftTrigger, axis_dead_thres[deviceType][2], 255);
  state.slider[1] = applyDeadzoneAndScale(cur.rightTrigger, axis_dead_thres[deviceType][2], 255);

  if (!stg_joy.disableVirtualControls)
  {
    state.z = state.slider[1] - state.slider[0];

    // Virtual keys

    if (cur.leftThumbX > MAXSHORT * VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
      state.buttons.orWord0(JOY_XINPUT_REAL_MASK_L_THUMB_RIGHT);
    else if (cur.leftThumbX < MAXSHORT * -VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
      state.buttons.orWord0(JOY_XINPUT_REAL_MASK_L_THUMB_LEFT);

    if (cur.leftThumbY > MAXSHORT * VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
      state.buttons.orWord0(JOY_XINPUT_REAL_MASK_L_THUMB_UP);
    else if (cur.leftThumbY < MAXSHORT * -VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
      state.buttons.orWord0(JOY_XINPUT_REAL_MASK_L_THUMB_DOWN);

    if (cur.rightThumbX > MAXSHORT * VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
      state.buttons.orWord0(JOY_XINPUT_REAL_MASK_R_THUMB_RIGHT);
    else if (cur.rightThumbX < MAXSHORT * -VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
      state.buttons.orWord0(JOY_XINPUT_REAL_MASK_R_THUMB_LEFT);

    if (cur.rightThumbY > MAXSHORT * VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
      state.buttons.orWord0(JOY_XINPUT_REAL_MASK_R_THUMB_UP);
    else if (cur.rightThumbY < MAXSHORT * -VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
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

  if (mouse_emu)
    mouse_emu->updateDevices();

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

  return state.buttons.getWord0() & (JOY_XINPUT_REAL_MASK_L_THUMB_RIGHT - 1);
}


void HumanInput::GameInputGamepadDevice::doRumble(float lowFreq, float highFreq)
{
  TIME_PROFILE(HID_GINP_doRumble);
  if (rumbleLeft != 0 || rumbleRight != 0 || lowFreq != 0 || highFreq != 0)
  {
    rumbleLeft = lowFreq;
    rumbleRight = highFreq;
    set_gamepad_rumble(userId, lowFreq, highFreq);
  }
}


bool HumanInput::GameInputGamepadDevice::isConnected()
{
  if (isVirtual)
    return true;

  TIME_PROFILE(HID_GINP_isConnected);
  return is_gamepad_connected(userId);
}


HumanInput::IGenHidClassDrv *HumanInput::mouse_emu = nullptr;
