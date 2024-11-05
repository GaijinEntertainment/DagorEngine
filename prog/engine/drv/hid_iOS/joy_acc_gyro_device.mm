// Copyright (C) Gaijin Games KFT.  All rights reserved.

#import <GameController/GameController.h>
#include <util/dag_globDef.h>
#include <drv/hid/dag_hiXInputMappings.h>
#include <drv/hid/dag_hiJoystick.h>
#include <perfMon/dag_cpuFreq.h>
#include "joy_acc_gyro_device.h"

extern void hid_iOS_update_motion();
extern UIInterfaceOrientation getAppOrientation();

static constexpr int GYRO_SENSOR_SCALE_FACTOR_OFFSET = 14;

namespace hid_ios
{
extern double acc_x, acc_y, acc_z;
extern double g_acc_x, g_acc_y, g_acc_z;
extern double m_acc_x, m_acc_y, m_acc_z;
extern double r_rate_x, r_rate_y, r_rate_z;
extern double g_rel_ang_x, g_rel_ang_y;
} // namespace hid_ios

namespace HumanInput
{

constexpr int DZ_STICK_INTERVAL = 0x7f;

extern JoystickSettings stg_joy;

carray<const char *, JOY_XINPUT_REAL_AXIS_COUNT + IOS_SENSOR_ADDITIONAL_AXIS_NUM> iosAxisName = {"L.Thumb.h", "L.Thumb.v", "R.Thumb.h",
  "R.Thumb.v", "L.Trigger", "R.Trigger", "R+L.Trigger", "none", "none", "GravityDelta.X", "GravityDelta.Y", "none", "Gravity.X", "Gravity.Y", "Gravity.Z"};
} // namespace HumanInput

// gamepad.valueChangedHandler = ^(GCExtendedGamepad *gamepad, GCControllerElement *element)
// is a good idea but for some reason on menu enter this handler sometimes might be skipped
// and therefore gamepad input breaks completely. (The cause is unknown)
iOSGamepadEx::iOSGamepadEx(GCExtendedGamepad *gpad, const char *name) : gamepad(gpad), fullName(name){};

void iOSGamepadEx::clearGamepad() { gamepad = nil; };

#define SET_BUTTON(gpButton, X360_BUTTON)            \
  {                                                  \
    if (gamepad.gpButton.isPressed)                  \
      rawState.buttons.set(HumanInput::X360_BUTTON); \
  }

void iOSGamepadEx::processButtons()
{
  SET_BUTTON(buttonY, JOY_XINPUT_REAL_BTN_Y);
  SET_BUTTON(buttonA, JOY_XINPUT_REAL_BTN_A);
  SET_BUTTON(buttonB, JOY_XINPUT_REAL_BTN_B);
  SET_BUTTON(buttonX, JOY_XINPUT_REAL_BTN_X);

  SET_BUTTON(leftTrigger, JOY_XINPUT_REAL_BTN_L_TRIGGER);
  SET_BUTTON(rightTrigger, JOY_XINPUT_REAL_BTN_R_TRIGGER);
  SET_BUTTON(leftShoulder, JOY_XINPUT_REAL_BTN_L_SHOULDER);
  SET_BUTTON(rightShoulder, JOY_XINPUT_REAL_BTN_R_SHOULDER);

  SET_BUTTON(buttonMenu, JOY_XINPUT_REAL_BTN_START);
  SET_BUTTON(buttonOptions, JOY_XINPUT_REAL_BTN_BACK);

  SET_BUTTON(leftThumbstickButton, JOY_XINPUT_REAL_BTN_L_THUMB);
  SET_BUTTON(rightThumbstickButton, JOY_XINPUT_REAL_BTN_R_THUMB);
}

void iOSGamepadEx::processDPad()
{
  SET_BUTTON(dpad.up, JOY_XINPUT_REAL_BTN_D_UP);
  SET_BUTTON(dpad.down, JOY_XINPUT_REAL_BTN_D_DOWN);
  SET_BUTTON(dpad.left, JOY_XINPUT_REAL_BTN_D_LEFT);
  SET_BUTTON(dpad.right, JOY_XINPUT_REAL_BTN_D_RIGHT);
}

static void apply_stick_deadzone(float &inout_x_rel, float &inout_y_rel, float deadzone_rel)
{
  const int dz = HumanInput::DZ_STICK_INTERVAL;
  int deadzone = eastl::clamp<int>(dz * deadzone_rel, 0, dz - 1);
  int x = (inout_x_rel * dz);
  int y = (inout_y_rel * dz);

  if (deadzone)
  {
    int d2 = x * x + y * y;
    if (d2 < deadzone * deadzone)
      x = y = 0;
    else
    {
      float d = sqrtf(d2), s = float(dz) / (dz - deadzone);
      x = (int)::roundf((x - deadzone / d * x) * s);
      y = (int)::roundf((y - deadzone / d * y) * s);
    }
  }

  int d2 = x * x + y * y;
  if (d2 > dz * dz)
  {
    float d = sqrtf(d2);
    x = (int)::roundf(x * dz / d);
    y = (int)::roundf(y * dz / d);
  }

  inout_x_rel = x / static_cast<float>(dz);
  inout_y_rel = y / static_cast<float>(dz);
}

float iOSGamepadEx::getStickDeadZoneAbs(int stick_idx) const
{
  G_ASSERT_RETURN(stick_idx >= 0 && stick_idx < 2, 0);
  return stickDeadZoneBase[stick_idx] * stickDeadZoneScale[stick_idx];
}

void iOSGamepadEx::setStickDeadZoneScale(int stick_idx, float scale)
{
  debug("ios: set gamepad DeadZone to:%f for stick: %d", scale, stick_idx);
  G_ASSERT_RETURN(stick_idx >= 0 && stick_idx < 2, );
  stickDeadZoneScale[stick_idx] = scale;
}

void iOSGamepadEx::processLeftStick()
{
  float xAx = gamepad.leftThumbstick.xAxis.value;
  float yAx = gamepad.leftThumbstick.yAxis.value;

  apply_stick_deadzone(xAx,yAx,stickDeadZoneBase[0] * stickDeadZoneScale[0]);

  xAx *= HumanInput::JOY_XINPUT_MAX_AXIS_VAL;
  yAx *= HumanInput::JOY_XINPUT_MAX_AXIS_VAL;

  axis[HumanInput::JOY_XINPUT_REAL_AXIS_L_THUMB_H].val = xAx;
  axis[HumanInput::JOY_XINPUT_REAL_AXIS_L_THUMB_V].val = yAx;

  if (HumanInput::stg_joy.disableVirtualControls)
    return;

  const int axisValThreshold = HumanInput::JOY_XINPUT_MAX_AXIS_VAL / 2;
  if (xAx > axisValThreshold)
    rawState.buttons.set(HumanInput::JOY_XINPUT_REAL_BTN_L_THUMB_RIGHT);
  if (xAx < -axisValThreshold)
    rawState.buttons.set(HumanInput::JOY_XINPUT_REAL_BTN_L_THUMB_LEFT);

  if (yAx > axisValThreshold)
    rawState.buttons.set(HumanInput::JOY_XINPUT_REAL_BTN_L_THUMB_UP);
  if (yAx < -axisValThreshold)
    rawState.buttons.set(HumanInput::JOY_XINPUT_REAL_BTN_L_THUMB_DOWN);
}

void iOSGamepadEx::processRightStick()
{
  float xAx = gamepad.rightThumbstick.xAxis.value;
  float yAx = gamepad.rightThumbstick.yAxis.value;

  apply_stick_deadzone(xAx,yAx,stickDeadZoneBase[1] * stickDeadZoneScale[1]);

  xAx *= HumanInput::JOY_XINPUT_MAX_AXIS_VAL;
  yAx *= HumanInput::JOY_XINPUT_MAX_AXIS_VAL;

  axis[HumanInput::JOY_XINPUT_REAL_AXIS_R_THUMB_H].val = xAx;
  axis[HumanInput::JOY_XINPUT_REAL_AXIS_R_THUMB_V].val = yAx;

  if (HumanInput::stg_joy.disableVirtualControls)
    return;

  const int axisValThreshold = HumanInput::JOY_XINPUT_MAX_AXIS_VAL / 2;
  if (xAx > axisValThreshold)
    rawState.buttons.set(HumanInput::JOY_XINPUT_REAL_BTN_R_THUMB_RIGHT);
  if (xAx < -axisValThreshold)
    rawState.buttons.set(HumanInput::JOY_XINPUT_REAL_BTN_R_THUMB_LEFT);

  if (yAx > axisValThreshold)
    rawState.buttons.set(HumanInput::JOY_XINPUT_REAL_BTN_R_THUMB_UP);
  if (yAx < -axisValThreshold)
    rawState.buttons.set(HumanInput::JOY_XINPUT_REAL_BTN_R_THUMB_DOWN);
}
#undef SET_BUTTON

// general routines
void iOSGamepadEx::setClient(HumanInput::IGenJoystickClient *cli)
{
  if (cli == client)
    return;

  if (client)
    client->detached(this);

  client = cli;

  if (client)
    client->attached(this);
}


// buttons and axes description
const char *iOSGamepadEx::getButtonName(int idx) const
{
  if (idx < 0 || idx >= BUTTONS_NUM)
    return NULL;
  return HumanInput::joyXInputButtonName[idx];
}


const char *iOSGamepadEx::getAxisName(int idx) const
{
  if (idx < 0 || idx >= AXES_NUM)
    return NULL;
  return HumanInput::iosAxisName[idx];
}


// buttons and axes name resolver
int iOSGamepadEx::getButtonId(const char *name) const
{
  for (int i = 0; i < BUTTONS_NUM; i++)
    if (strcmp(name, HumanInput::joyXInputButtonName[i]) == 0)
      return i;
  return -1;
}


int iOSGamepadEx::getAxisId(const char *name) const
{
  for (int i = 0; i < AXES_NUM; i++)
    if (strcmp(name, HumanInput::iosAxisName[i]) == 0)
      return i;
  return -1;
}


// axis limits remap setup
void iOSGamepadEx::setAxisLimits(int axis_id, float min_val, float max_val)
{
  G_ASSERT(axis_id >= 0 && axis_id < AXES_NUM);

  axis[axis_id].min = min_val;
  axis[axis_id].max = max_val;
}


// current state information
bool iOSGamepadEx::getButtonState(int btn_id) const
{
  G_ASSERT(btn_id >= 0 && btn_id < BUTTONS_NUM);
  return rawState.buttons.get(btn_id);
}


float iOSGamepadEx::getAxisPos(int axis_id) const
{
  G_ASSERT(axis_id >= 0 && axis_id < AXES_NUM);
  return (axis[axis_id].val + 1.0f) / 2.0f * (axis[axis_id].max - axis[axis_id].min) + axis[axis_id].min;
}

bool iOSGamepadEx::isConnected() { return true; }

int iOSGamepadEx::getAxisPosRaw(int axis_id) const
{
  G_ASSERT(axis_id >= 0 && axis_id < AXES_NUM);
  return axis[axis_id].val;
}


static inline int clip_dead_zone(int val, int dz) { return (val >= dz || val <= -dz) ? val : 0; }

static int rescale_and_clip_dead_zone(float val, int multiplier, int deadzone, int edge)
{
  const int v = int(val * multiplier);
  return eastl::clamp((v >= deadzone || v <= -deadzone) ? (v >> GYRO_SENSOR_SCALE_FACTOR_OFFSET) : 0, -edge, edge);
}

void iOSGamepadEx::updateGyroscopeParameters()
{
  using namespace hid_ios;

  hid_iOS_update_motion();

  rawState.sensorX = acc_x;
  rawState.sensorY = acc_y;
  rawState.sensorZ = acc_z;

  rawState.x = clip_dead_zone(int(m_acc_x * HumanInput::JOY_XINPUT_MAX_AXIS_VAL), 400);
  rawState.y = clip_dead_zone(int(m_acc_y * HumanInput::JOY_XINPUT_MAX_AXIS_VAL), 400);
  rawState.z = clip_dead_zone(int(m_acc_z * HumanInput::JOY_XINPUT_MAX_AXIS_VAL), 400);

  rawState.rx = rescale_and_clip_dead_zone(r_rate_x, HumanInput::JOY_XINPUT_MAX_AXIS_VAL, HumanInput::JOY_XINPUT_MAX_AXIS_VAL,
    HumanInput::JOY_XINPUT_MAX_AXIS_VAL);
  rawState.ry = rescale_and_clip_dead_zone(r_rate_y, HumanInput::JOY_XINPUT_MAX_AXIS_VAL, HumanInput::JOY_XINPUT_MAX_AXIS_VAL,
    HumanInput::JOY_XINPUT_MAX_AXIS_VAL);
  rawState.rz = rescale_and_clip_dead_zone(r_rate_z, HumanInput::JOY_XINPUT_MAX_AXIS_VAL, HumanInput::JOY_XINPUT_MAX_AXIS_VAL,
    HumanInput::JOY_XINPUT_MAX_AXIS_VAL);

  rawState.slider[0] = clip_dead_zone(int(g_rel_ang_x * HumanInput::JOY_XINPUT_MAX_AXIS_VAL), 200);
  rawState.slider[1] = clip_dead_zone(int(g_rel_ang_y * HumanInput::JOY_XINPUT_MAX_AXIS_VAL), 200);

  axis[9].val = -rawState.rx;
  axis[10].val = -rawState.ry;

  axis[12].val = clip_dead_zone(int(-acc_x * HumanInput::JOY_XINPUT_MAX_AXIS_VAL), 400);
  axis[13].val = clip_dead_zone(int(-acc_y * HumanInput::JOY_XINPUT_MAX_AXIS_VAL), 400);
  axis[14].val = clip_dead_zone(int(-acc_z * HumanInput::JOY_XINPUT_MAX_AXIS_VAL), 400);
}


void iOSGamepadEx::notifyClient()
{
  if (client)
    client->stateChanged(this, 0);
}


void iOSGamepadEx::updatePressTime()
{
  static int prev_t = get_time_msec();
  int dt_msec = get_time_msec() - prev_t;
  prev_t = get_time_msec();

  for (int i = 0, inc = 0; i < rawState.MAX_BUTTONS; i += inc)
    if (rawState.buttons.getIter(i, inc))
      if (!rawState.buttonsPrev.get(i))
        rawState.bDownTms[i] = 0;

  if (dt_msec)
    for (int i = 0, inc = 0; i < rawState.MAX_BUTTONS; i += inc)
      if (rawState.buttonsPrev.getIter(i, inc))
        rawState.bDownTms[i] += dt_msec;
}


void iOSGamepadEx::processGamepadInput()
{
  rawState.buttonsPrev = rawState.buttons;
  rawState.buttons.reset();
  processDPad();
  processButtons();
  processLeftStick();
  processRightStick();

  updatePressTime();
}


void iOSGamepadEx::update()
{
  updateGyroscopeParameters();

  if (gamepad)
    processGamepadInput();

  notifyClient();
}
