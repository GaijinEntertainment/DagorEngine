// Copyright (C) Gaijin Games KFT.  All rights reserved.

#import <GameController/GameController.h>
#include <util/dag_globDef.h>
#include <drv/hid/dag_hiXInputMappings.h>
#include <drv/hid/dag_hiJoystick.h>
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_iosWindowScene.h>
#include "joy_acc_gyro_device.h"
#include "../hid_commonCode/joystickAxisUtils.h"

static constexpr int GYRO_SENSOR_SCALE_FACTOR_OFFSET = 14;

namespace hid_ios
{
extern double acc_x, acc_y, acc_z;
extern double r_rate_x, r_rate_y;
} // namespace hid_ios

namespace HumanInput
{

extern JoystickSettings stg_joy;

carray<const char *, JOY_XINPUT_REAL_AXIS_COUNT + IOS_SENSOR_ADDITIONAL_AXIS_NUM> iosAxisName = {"L.Thumb.h", "L.Thumb.v", "R.Thumb.h",
  "R.Thumb.v", "L.Trigger", "R.Trigger", "R+L.Trigger", "none", "none", "GyroX", "GyroY", "none", "GravityX", "GravityY", "GravityZ"};
} // namespace HumanInput

#define IOS_GAMEPAD_BUTTONS(X)                                \
  X(dpad.up, JOY_XINPUT_REAL_BTN_D_UP)                        \
  X(dpad.down, JOY_XINPUT_REAL_BTN_D_DOWN)                    \
  X(dpad.left, JOY_XINPUT_REAL_BTN_D_LEFT)                    \
  X(dpad.right, JOY_XINPUT_REAL_BTN_D_RIGHT)                  \
  X(buttonY, JOY_XINPUT_REAL_BTN_Y)                           \
  X(buttonA, JOY_XINPUT_REAL_BTN_A)                           \
  X(buttonB, JOY_XINPUT_REAL_BTN_B)                           \
  X(buttonX, JOY_XINPUT_REAL_BTN_X)                           \
  X(leftTrigger, JOY_XINPUT_REAL_BTN_L_TRIGGER)               \
  X(rightTrigger, JOY_XINPUT_REAL_BTN_R_TRIGGER)              \
  X(leftShoulder, JOY_XINPUT_REAL_BTN_L_SHOULDER)             \
  X(rightShoulder, JOY_XINPUT_REAL_BTN_R_SHOULDER)            \
  X(buttonMenu, JOY_XINPUT_REAL_BTN_START)                    \
  X(buttonOptions, JOY_XINPUT_REAL_BTN_BACK)                  \
  X(leftThumbstickButton, JOY_XINPUT_REAL_BTN_L_THUMB)        \
  X(rightThumbstickButton, JOY_XINPUT_REAL_BTN_R_THUMB)

// gamepad.valueChangedHandler = ^(GCExtendedGamepad *gamepad, GCControllerElement *element)
// is a good idea but for some reason on menu enter this handler sometimes might be skipped
// and therefore gamepad input breaks completely. (The cause is unknown)
// So polling stays the main input path, pressedChangedHandler only latches presses shorter than a frame.
iOSGamepadEx::iOSGamepadEx(GCController *gc, const char *name) :
  fullName(name), controller([gc retain]), gamepad(gc.extendedGamepad), prevUpdateTimeMsec(get_time_msec())
{
  if (gamepad)
    setPressLatch(true);
}

iOSGamepadEx::~iOSGamepadEx() { clearGamepad(); }

void iOSGamepadEx::clearGamepad()
{
  setMotionActive(false);
  if (gamepad)
    setPressLatch(false);
  latchedPressMask = 0;
  gamepad = nil;
  [controller release];
  controller = nil;
}

void iOSGamepadEx::processButtons()
{
#define SET_BUTTON(gpButton, X360_BUTTON) \
  if (gamepad.gpButton.isPressed)         \
    rawState.buttons.set(HumanInput::X360_BUTTON);
  IOS_GAMEPAD_BUTTONS(SET_BUTTON)
#undef SET_BUTTON

  rawState.buttons.orWord0(latchedPressMask);
  latchedPressMask = 0;
}

void iOSGamepadEx::setPressLatch(bool enable)
{
#define LATCH_BUTTON(gpButton, X360_BUTTON)                              \
  static_assert(HumanInput::X360_BUTTON < 32, "latch mask is 32 bit"); \
  latchButton(gamepad.gpButton, HumanInput::X360_BUTTON, enable);
  IOS_GAMEPAD_BUTTONS(LATCH_BUTTON)
#undef LATCH_BUTTON
}

// GameController calls the handler on GCController.handlerQueue (main queue), the thread of update()
void iOSGamepadEx::latchButton(GCControllerButtonInput *input, int btn, bool enable)
{
  if (!enable)
  {
    input.pressedChangedHandler = nil;
    return;
  }
  const unsigned mask = 1u << btn;
  input.pressedChangedHandler = ^(GCControllerButtonInput *, float, BOOL pressed) {
    if (pressed)
      latchedPressMask |= mask;
  };
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

  HumanInput::apply_stick_deadzone(xAx,yAx,getStickDeadZoneAbs(0));

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

  HumanInput::apply_stick_deadzone(xAx,yAx,getStickDeadZoneAbs(1));

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

void iOSGamepadEx::processTriggers()
{
  const int lt = int(gamepad.leftTrigger.value * HumanInput::JOY_XINPUT_MAX_AXIS_VAL);
  const int rt = int(gamepad.rightTrigger.value * HumanInput::JOY_XINPUT_MAX_AXIS_VAL);
  axis[HumanInput::JOY_XINPUT_REAL_AXIS_L_TRIGGER].val = lt;
  axis[HumanInput::JOY_XINPUT_REAL_AXIS_R_TRIGGER].val = rt;
  if (!HumanInput::stg_joy.disableVirtualControls)
    axis[HumanInput::JOY_XINPUT_REAL_AXIS_LR_TRIGGER].val = rt - lt;
}

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
  G_ASSERT_RETURN(axis_id >= 0 && axis_id < AXES_NUM, );

  using namespace HumanInput;
  iOSAxis &a = axis[axis_id];
  if (axis_id == JOY_XINPUT_REAL_AXIS_L_TRIGGER || axis_id == JOY_XINPUT_REAL_AXIS_R_TRIGGER)
  {
    a.kMul = (max_val - min_val) / float(JOY_XINPUT_MAX_AXIS_VAL);
    a.kAdd = min_val;
  }
  else
  {
    a.kMul = (max_val - min_val) / float(JOY_XINPUT_MAX_AXIS_VAL - JOY_XINPUT_MIN_AXIS_VAL);
    a.kAdd = min_val - JOY_XINPUT_MIN_AXIS_VAL * a.kMul;
  }
}


// current state information
bool iOSGamepadEx::getButtonState(int btn_id) const
{
  G_ASSERT_RETURN(btn_id >= 0 && btn_id < BUTTONS_NUM, false);
  return rawState.buttons.get(btn_id);
}


float iOSGamepadEx::getAxisPos(int axis_id) const
{
  G_ASSERT_RETURN(axis_id >= 0 && axis_id < AXES_NUM, 0.f);
  return axis[axis_id].val * axis[axis_id].kMul + axis[axis_id].kAdd;
}

bool iOSGamepadEx::isConnected() { return true; }

int iOSGamepadEx::getAxisPosRaw(int axis_id) const
{
  G_ASSERT_RETURN(axis_id >= 0 && axis_id < AXES_NUM, 0);
  return int(axis[axis_id].val);
}


static inline int clip_dead_zone(int val, int dz) { return (val >= dz || val <= -dz) ? val : 0; }

static int rescale_and_clip_dead_zone(float val, int multiplier, int deadzone, int edge)
{
  const int v = int(val * multiplier);
  return eastl::clamp((v >= deadzone || v <= -deadzone) ? (v >> GYRO_SENSOR_SCALE_FACTOR_OFFSET) : 0, -edge, edge);
}

bool iOSGamepadEx::setMotionActive(bool want)
{
  const bool active = want && gamepad && controller.motion.hasRotationRate;
  if (active == motionActive)
    return active;
  GCMotion *motion = controller.motion;
  if (motion.sensorsRequireManualActivation)
    motion.sensorsActive = active;
  motionActive = active;
  return active;
}

// GCMotion frame of a pad: X right, Y away from the player, Z up. The result is in the phone frame of hid_ios, as if the pad
// were a phone that the player holds upright in the landscape orientation of the app, so axes 9..14 keep their meaning.
void iOSGamepadEx::readPadMotion(double &rate_x, double &rate_y, double &acc_x, double &acc_y, double &acc_z) const
{
  GCMotion *motion = controller.motion;
  const GCRotationRate rate = motion.rotationRate;
  const GCAcceleration acc = motion.hasGravityAndUserAcceleration ? motion.gravity : motion.acceleration;
  // landscape left: phone X is down, Y is right; landscape right: X is up, Y is left; Z is to the player in both
  const double s = ios_get_window_scene().interfaceOrientation == UIInterfaceOrientationLandscapeRight ? -1.0 : 1.0;
  const double radToDeg = 180.0 / M_PI;
  rate_x = -s * rate.z * radToDeg;
  rate_y = s * rate.x * radToDeg;
  acc_x = -s * acc.z;
  acc_y = s * acc.x;
  acc_z = -acc.y;
}

void iOSGamepadEx::updateGyroscopeParameters()
{
  double rateX = hid_ios::r_rate_x, rateY = hid_ios::r_rate_y;
  double accX = hid_ios::acc_x, accY = hid_ios::acc_y, accZ = hid_ios::acc_z;
  if (motionActive)
    readPadMotion(rateX, rateY, accX, accY, accZ);

  axis[9].val = -rescale_and_clip_dead_zone(rateX, HumanInput::JOY_XINPUT_MAX_AXIS_VAL, HumanInput::JOY_XINPUT_MAX_AXIS_VAL,
    HumanInput::JOY_XINPUT_MAX_AXIS_VAL);
  axis[10].val = -rescale_and_clip_dead_zone(rateY, HumanInput::JOY_XINPUT_MAX_AXIS_VAL, HumanInput::JOY_XINPUT_MAX_AXIS_VAL,
    HumanInput::JOY_XINPUT_MAX_AXIS_VAL);

  axis[12].val = clip_dead_zone(int(-accX * HumanInput::JOY_XINPUT_MAX_AXIS_VAL), 400);
  axis[13].val = clip_dead_zone(int(-accY * HumanInput::JOY_XINPUT_MAX_AXIS_VAL), 400);
  axis[14].val = clip_dead_zone(int(-accZ * HumanInput::JOY_XINPUT_MAX_AXIS_VAL), 400);
}


void iOSGamepadEx::notifyClient()
{
  if (client)
    client->stateChanged(this, 0);
}


void iOSGamepadEx::updatePressTime()
{
  const int nowMsec = get_time_msec();
  const int dt_msec = nowMsec - prevUpdateTimeMsec;
  prevUpdateTimeMsec = nowMsec;

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
  processButtons();
  processLeftStick();
  processRightStick();
  processTriggers();

  updatePressTime();
}


void iOSGamepadEx::update()
{
  updateGyroscopeParameters();

  rawState.buttonsPrev = rawState.buttons;
  rawState.buttons.reset();
  if (gamepad)
    processGamepadInput();
  else
    for (int i = 0; i < HumanInput::JOY_XINPUT_REAL_AXIS_COUNT; i++)
      axis[i].val = 0;

  rawState.x = int(axis[HumanInput::JOY_XINPUT_REAL_AXIS_L_THUMB_H].val);
  rawState.y = int(axis[HumanInput::JOY_XINPUT_REAL_AXIS_L_THUMB_V].val);
  rawState.rx = int(axis[HumanInput::JOY_XINPUT_REAL_AXIS_R_THUMB_H].val);
  rawState.ry = int(axis[HumanInput::JOY_XINPUT_REAL_AXIS_R_THUMB_V].val);
  rawState.z = int(axis[HumanInput::JOY_XINPUT_REAL_AXIS_LR_TRIGGER].val);
  rawState.slider[0] = int(axis[HumanInput::JOY_XINPUT_REAL_AXIS_L_TRIGGER].val);
  rawState.slider[1] = int(axis[HumanInput::JOY_XINPUT_REAL_AXIS_R_TRIGGER].val);

  notifyClient();
}
