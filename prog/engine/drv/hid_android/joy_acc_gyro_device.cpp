// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "joy_acc_gyro_device.h"
#include <drv/hid/dag_hiGlobals.h>
#include <ioSys/dag_dataBlock.h>
#include <supp/_platform.h>
#include <supp/dag_android_native_app_glue.h>
#include <dlfcn.h>
#include <debug/dag_debug.h>
#include <perfMon/dag_cpuFreq.h>
#include "joy_helper_android.h"
#include "../hid_commonCode/joystickAxisUtils.h"

namespace HumanInput
{
constexpr int AXIS_SCALE_FACTOR = 32000;
} // namespace HumanInput

const char *HumanInput::JoyAccelGyroInpDevice::btnName[HumanInput::JoyAccelGyroInpDevice::BTN_NUM] = {
  "D.Up",
  "D.Down",
  "D.Left",
  "D.Right",
  "Start",
  "Select",
  "L3",
  "R3",
  "L1",
  "R1",
  "",
  "",
  "A",
  "B",
  "X",
  "Y",
  "L2",
  "R2",
  "LS.Right",
  "LS.Left",
  "LS.Up",
  "LS.Down",
  "RS.Right",
  "RS.Left",
  "RS.Up",
  "RS.Down",
  "L3.Centered",
  "R3.Centered",
};
const char *HumanInput::JoyAccelGyroInpDevice::axisName[HumanInput::JoyAccelGyroInpDevice::AXES_NUM] = {
  "L.Thumb.h",
  "L.Thumb.v",
  "R.Thumb.h",
  "R.Thumb.v",
  "L.Trigger",
  "R.Trigger",
  "AccelX",
  "AccelY",
  "AccelZ",
  "GyroX",
  "GyroY",
  "GyroZ",
  "GravityX",
  "GravityY",
  "GravityZ",
  "G.Bank",
  "G.Pitch",
};

static inline int clip_dead_zone(int val, int dz) { return (val >= dz || val <= -dz) ? val : 0; }

HumanInput::JoyAccelGyroInpDevice::JoyAccelGyroInpDevice(const IGenJoystick *sensor_source) : sensorSource(sensor_source)
{
  client = NULL;
  memset(&state, 0, sizeof(state));
  memset(&nextState, 0, sizeof(nextState));
  memset(accel, 0, sizeof(accel));
  memset(gyro, 0, sizeof(gyro));
  memset(gravity, 0, sizeof(gravity));
  for (int i = 0; i < JoystickRawState::MAX_POV_HATS; i++)
    state.povValues[i] = state.povValuesPrev[i] = -1;

  axis[0].val = &state.x;
  axis[1].val = &state.y;
  axis[2].val = &state.rx;
  axis[3].val = &state.ry;
  axis[4].val = &state.slider[0];
  axis[5].val = &state.slider[1];

  axis[6].val = &accel[0];
  axis[7].val = &accel[1];
  axis[8].val = &accel[2];
  axis[9].val = &gyro[0];
  axis[10].val = &gyro[1];
  axis[11].val = &gyro[2];
  axis[12].val = &gravity[0];
  axis[13].val = &gravity[1];
  axis[14].val = &gravity[2];

  axis[15].val = &gravity[3];
  axis[16].val = &gravity[4];
  for (int i = 0; i < AXES_NUM; i++)
  {
    axis[i].kMul = 1.0f / (float)AXIS_SCALE_FACTOR;
    axis[i].kAdd = 0.0f;
  }

  for (int axis : {0, 1, 11, 14, 15, 16, 22, 23})
    android::input::enable_axis(axis);

  doRumble(0, 0);
}
HumanInput::JoyAccelGyroInpDevice::~JoyAccelGyroInpDevice() { setClient(NULL); }


bool HumanInput::JoyAccelGyroInpDevice::getDeviceDesc(DataBlock &out_desc) const
{
  out_desc.clearData();
  // out_desc.setStr("accelSensor", true);
  return true;
}
bool HumanInput::JoyAccelGyroInpDevice::updateState()
{
  readInput();
  bool changed = memcmp(&state, &nextState, sizeof(state)) != 0;

  static int prev_t = get_time_msec();
  int dt_msec = get_time_msec() - prev_t;
  prev_t = get_time_msec();

  nextState.buttonsPrev = state.buttons;
  state = nextState;

  if (vendorId == HumanInput::GAMEPAD_VENDOR_NINTENDO)
  {
    state.buttons.clr(0);
    state.buttons.clr(1);
    state.buttons.clr(2);
    state.buttons.clr(3);
  }

  const int prevGyro[3] = {gyro[0], gyro[1], gyro[2]};
  const int prevGravity[3] = {gravity[0], gravity[1], gravity[2]};
  for (int i = 9; i <= 14; i++) // GyroX..GravityZ, the only axes the sensor fills
    *axis[i].val = sensorSource->getAxisPosRaw(i);
  changed |= memcmp(prevGyro, gyro, sizeof(prevGyro)) != 0 || memcmp(prevGravity, gravity, sizeof(prevGravity)) != 0;

  for (int i = 0, inc = 0; i < state.MAX_BUTTONS; i += inc)
    if (state.buttons.getIter(i, inc))
      if (!state.buttonsPrev.get(i))
        state.bDownTms[i] = 0;
  if (dt_msec)
    for (int i = 0, inc = 0; i < state.MAX_BUTTONS; i += inc)
      if (state.buttonsPrev.getIter(i, inc))
        state.bDownTms[i] += dt_msec;

  raw_state_joy = state;

  if (client && changed)
    client->stateChanged(this, 0);

  nextState = state;

  return !state.buttons.cmpEq(state.buttonsPrev);
}

float HumanInput::JoyAccelGyroInpDevice::getStickDeadZoneAbs(int stick_idx) const
{
  G_ASSERT_RETURN(stick_idx >= 0 && stick_idx < 2, 0);
  return stickDeadZoneBase[stick_idx] * stickDeadZoneScale[stick_idx];
}

void HumanInput::JoyAccelGyroInpDevice::setStickDeadZoneScale(int stick_idx, float scale)
{
  debug("android: set gamepad DeadZone to:%f for stick: %d", scale, stick_idx);
  G_ASSERT_RETURN(stick_idx >= 0 && stick_idx < 2, );
  stickDeadZoneScale[stick_idx] = scale;
}
void HumanInput::JoyAccelGyroInpDevice::readInput()
{
  AndroidGamepadInput input;
  read_android_gamepad_input(input);

  const int axis_scale = AXIS_SCALE_FACTOR;
  const int axis_scale_half = AXIS_SCALE_FACTOR / 2;

  float lx = input.axes[AndroidGamepadInput::AXIS_X];
  float ly = -input.axes[AndroidGamepadInput::AXIS_Y];

  float rx = input.axes[AndroidGamepadInput::AXIS_Z];
  float ry = -input.axes[AndroidGamepadInput::AXIS_RZ];

  apply_stick_deadzone(lx, ly, getStickDeadZoneAbs(0));
  apply_stick_deadzone(rx, ry, getStickDeadZoneAbs(1));

  nextState.x = int(lx * axis_scale);
  nextState.y = int(ly * axis_scale);
  nextState.rx = int(rx * axis_scale);
  nextState.ry = int(ry * axis_scale);

  nextState.slider[0] = clip_dead_zone(int(input.axes[AndroidGamepadInput::AXIS_BRAKE] * axis_scale), 400);
  nextState.slider[1] = clip_dead_zone(int(input.axes[AndroidGamepadInput::AXIS_GAS] * axis_scale), 400);

  // the buttons are rebuilt from the axes and the keys of the current gamepad only
  nextState.buttons.reset();

  const int dpadx = int(input.axes[AndroidGamepadInput::AXIS_HAT_X] * axis_scale);
  const int dpady = int(input.axes[AndroidGamepadInput::AXIS_HAT_Y] * axis_scale);
  if (dpady < -20000)
    nextState.buttons.set(0);
  if (dpady > 20000)
    nextState.buttons.set(1);
  if (dpadx < -20000)
    nextState.buttons.set(2);
  if (dpadx > 20000)
    nextState.buttons.set(3);

  if (nextState.slider[0] > axis_scale_half)
    nextState.buttons.set(16);
  if (nextState.slider[1] > axis_scale_half)
    nextState.buttons.set(17);

  if (nextState.x > axis_scale_half)
    nextState.buttons.set(18);
  if (nextState.x < -axis_scale_half)
    nextState.buttons.set(19);
  if (nextState.y > axis_scale_half)
    nextState.buttons.set(20);
  if (nextState.y < -axis_scale_half)
    nextState.buttons.set(21);
  if (nextState.rx > axis_scale_half)
    nextState.buttons.set(22);
  if (nextState.rx < -axis_scale_half)
    nextState.buttons.set(23);
  if (nextState.ry > axis_scale_half)
    nextState.buttons.set(24);
  if (nextState.ry < -axis_scale_half)
    nextState.buttons.set(25);

  for (size_t keyCode = input.keyCodes.find_first(); keyCode < input.keyCodes.size(); keyCode = input.keyCodes.find_next(keyCode))
    if (const int btIdx = keyCodeToButton(int(keyCode)); btIdx >= 0)
      nextState.buttons.set(btIdx);
}

#define REMAP_FOR_NINTENDO_GAMEPAD(n_id, x_id) (vendorId == HumanInput::GAMEPAD_VENDOR_NINTENDO ? (n_id) : (x_id))
int HumanInput::JoyAccelGyroInpDevice::keyCodeToButton(int key_code) const
{
  int btIdx = -1;
  switch (key_code)
  {
    case 19: btIdx = REMAP_FOR_NINTENDO_GAMEPAD(0, -1); break;
    case 20: btIdx = REMAP_FOR_NINTENDO_GAMEPAD(1, -1); break;
    case 21: btIdx = REMAP_FOR_NINTENDO_GAMEPAD(2, -1); break;
    case 22: btIdx = REMAP_FOR_NINTENDO_GAMEPAD(3, -1); break;
    case 96: btIdx = REMAP_FOR_NINTENDO_GAMEPAD(13, 12); break;
    case 97: btIdx = REMAP_FOR_NINTENDO_GAMEPAD(12, 13); break;
    case 99: btIdx = REMAP_FOR_NINTENDO_GAMEPAD(15, 14); break;
    case 100: btIdx = REMAP_FOR_NINTENDO_GAMEPAD(14, 15); break;
    case 102: btIdx = 8; break;
    case 103: btIdx = 9; break;
    case 104: btIdx = 16; break; // required for nintendo gamepad - triggers are not analog
    case 105: btIdx = 17; break; // required for nintendo gamepad
    case 106: btIdx = 6; break;
    case 107: btIdx = 7; break;
    case 108: btIdx = 4; break;
    case 109: btIdx = 5; break;
  }
  return btIdx;
}
#undef REMAP_FOR_NINTENDO_GAMEPAD
