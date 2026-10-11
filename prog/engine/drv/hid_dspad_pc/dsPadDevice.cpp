// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dsPadDevice.h"
#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiXInputMappings.h>
#include <startup/dag_demoMode.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_mathAng.h>
#include <math/dag_mathBase.h>
#include <perfMon/dag_cpuFreq.h>
#include <debug/dag_debug.h>
#include <util/dag_string.h>
#include <string.h>

using namespace HumanInput;

// bit numbers are the same as order of this table; 0..27 equal the PS5 pad driver
const char *SonyPadDevice::buttonName[SonyPadDevice::BUTTONS_NUM] = {"D.Up", "D.Down", "D.Left", "D.Right", "Start", "Select", "L3",
  "R3", "L1", "R1", "0x0400", "0x0800", "CROSS", "CIRCLE", "SQUARE", "TRIANGLE", "L2", "R2", "LS.Right", "LS.Left", "LS.Up", "LS.Down",
  "RS.Right", "RS.Left", "RS.Up", "RS.Down", "", "", "Create", "PS", "Mute", "Fn.L", "Fn.R", "Paddle.L", "Paddle.R"};

const char *SonyPadDevice::axisName[SonyPadDevice::AXES_NUM] = {
  "L.Thumb.h", "L.Thumb.v", "R.Thumb.h", "R.Thumb.v", "L.Trigger", "R.Trigger", "SensorX", "SensorY", "SensorZ", "R+L.Trigger"};

static const struct
{
  uint32_t src;
  int dst;
} button_map[] = {
  {sonypad::BTN_OPTIONS, JOY_XINPUT_REAL_BTN_START},
  {sonypad::BTN_TOUCHPAD, JOY_XINPUT_REAL_BTN_BACK},
  {sonypad::BTN_L3, JOY_XINPUT_REAL_BTN_L_THUMB},
  {sonypad::BTN_R3, JOY_XINPUT_REAL_BTN_R_THUMB},
  {sonypad::BTN_L1, JOY_XINPUT_REAL_BTN_L_SHOULDER},
  {sonypad::BTN_R1, JOY_XINPUT_REAL_BTN_R_SHOULDER},
  {sonypad::BTN_CROSS, JOY_XINPUT_REAL_BTN_A},
  {sonypad::BTN_CIRCLE, JOY_XINPUT_REAL_BTN_B},
  {sonypad::BTN_SQUARE, JOY_XINPUT_REAL_BTN_X},
  {sonypad::BTN_TRIANGLE, JOY_XINPUT_REAL_BTN_Y},
  {sonypad::BTN_L2, JOY_XINPUT_REAL_BTN_L_TRIGGER},
  {sonypad::BTN_R2, JOY_XINPUT_REAL_BTN_R_TRIGGER},
  {sonypad::BTN_CREATE, SonyPadDevice::BTN_CREATE_IDX},
  {sonypad::BTN_PS, SonyPadDevice::BTN_PS_IDX},
  {sonypad::BTN_MUTE, SonyPadDevice::BTN_MUTE_IDX},
  {sonypad::BTN_FN_LEFT, SonyPadDevice::BTN_FN_LEFT_IDX},
  {sonypad::BTN_FN_RIGHT, SonyPadDevice::BTN_FN_RIGHT_IDX},
  {sonypad::BTN_PADDLE_LEFT, SonyPadDevice::BTN_PADDLE_LEFT_IDX},
  {sonypad::BTN_PADDLE_RIGHT, SonyPadDevice::BTN_PADDLE_RIGHT_IDX},
};

static const uint8_t hat_to_dpad[9] = {
  JOY_XINPUT_REAL_MASK_D_UP,
  JOY_XINPUT_REAL_MASK_D_UP | JOY_XINPUT_REAL_MASK_D_RIGHT,
  JOY_XINPUT_REAL_MASK_D_RIGHT,
  JOY_XINPUT_REAL_MASK_D_DOWN | JOY_XINPUT_REAL_MASK_D_RIGHT,
  JOY_XINPUT_REAL_MASK_D_DOWN,
  JOY_XINPUT_REAL_MASK_D_DOWN | JOY_XINPUT_REAL_MASK_D_LEFT,
  JOY_XINPUT_REAL_MASK_D_LEFT,
  JOY_XINPUT_REAL_MASK_D_UP | JOY_XINPUT_REAL_MASK_D_LEFT,
  0,
};

// player slot colours of SDL and hid-playstation
static const uint8_t default_lightbar[4][3] = {{0, 0, 0x40}, {0x40, 0, 0}, {0, 0x40, 0}, {0x20, 0, 0x20}};
static constexpr float PLAYER_SPACE_YAW_RELAX = 1.41f; // the yawRelaxFactor of GyroWiki

SonyPadDevice::SonyPadDevice(const InitInfo &info, int ord_id) :
  firmware(info.firmware), model(info.model), bluetooth(info.bluetooth), rumbleV2(info.rumbleV2), ordId(ord_id)
{
  memcpy(mac, info.mac, sizeof(mac));
  memset(&state, 0, sizeof(state));
  for (int i = 0; i < JoystickRawState::MAX_POV_HATS; i++)
    state.povValues[i] = state.povValuesPrev[i] = -1;

  axis[0].val = &state.x;
  axis[1].val = &state.y;
  axis[2].val = &state.rx;
  axis[3].val = &state.ry;
  axis[4].val = &state.slider[0];
  axis[5].val = &state.slider[1];
  axis[6].val = &intSensors[0];
  axis[7].val = &intSensors[1];
  axis[8].val = &intSensors[2];
  axis[9].val = &state.z;
  devAxisCount = stg_joy.disableVirtualControls ? 9 : 10;
  switch (model)
  {
    case sonypad::Model::DUALSENSE_EDGE: devButtonCount = BTN_PADDLE_RIGHT_IDX + 1; break;
    case sonypad::Model::DUALSHOCK4: devButtonCount = BTN_PS_IDX + 1; break;
    default: devButtonCount = BTN_MUTE_IDX + 1; break;
  }
  // the one-shot lightbar setup stops the pad's own connect animation, the rest of the block is neutral
  shared.out.resetLightbar = true;
  setLight(false, 0, 0, 0);
}

void SonyPadDevice::setClient(IGenJoystickClient *cli)
{
  if (cli == client)
    return;
  if (client)
    client->detached(this);
  client = cli;
  if (client)
    client->attached(this);
}

const char *SonyPadDevice::getButtonName(int idx) const { return (idx >= 0 && idx < devButtonCount) ? buttonName[idx] : nullptr; }
const char *SonyPadDevice::getAxisName(int idx) const { return (idx >= 0 && idx < devAxisCount) ? axisName[idx] : nullptr; }

int SonyPadDevice::getButtonId(const char *btn_name) const
{
  for (int i = 0; i < devButtonCount; i++)
    if (strcmp(btn_name, buttonName[i]) == 0)
      return i;
  return -1;
}

int SonyPadDevice::getAxisId(const char *axis_name) const
{
  for (int i = 0; i < devAxisCount; i++)
    if (strcmp(axis_name, axisName[i]) == 0)
      return i;
  return -1;
}

void SonyPadDevice::setAxisLimits(int axis_id, float min_val, float max_val)
{
  G_ASSERT_RETURN(axis_id >= 0 && axis_id < devAxisCount, );
  if (axis_id < 4 || axis_id > 5)
  {
    axis[axis_id].kMul = (max_val - min_val) / float(MAX_AXIS_VAL - MIN_AXIS_VAL);
    axis[axis_id].kAdd = min_val - MIN_AXIS_VAL * axis[axis_id].kMul;
  }
  else
  {
    axis[axis_id].kMul = (max_val - min_val) / float(MAX_AXIS_VAL);
    axis[axis_id].kAdd = min_val;
  }
}

bool SonyPadDevice::getButtonState(int btn_id) const
{
  G_ASSERT_RETURN(btn_id >= 0 && btn_id < devButtonCount, false);
  return state.buttons.get(btn_id);
}

float SonyPadDevice::getAxisPos(int axis_id) const
{
  G_ASSERT_RETURN(axis_id >= 0 && axis_id < devAxisCount, 0.f);
  return axis[axis_id].val[0] * axis[axis_id].kMul + axis[axis_id].kAdd;
}

int SonyPadDevice::getAxisPosRaw(int axis_id) const
{
  G_ASSERT_RETURN(axis_id >= 0 && axis_id < devAxisCount, 0);
  return axis[axis_id].val[0];
}

float SonyPadDevice::getStickDeadZoneScale(int stick_idx) const
{
  G_ASSERT_RETURN(stick_idx >= 0 && stick_idx < 2, 0);
  return stickDeadZoneScale[stick_idx];
}

void SonyPadDevice::setStickDeadZoneScale(int stick_idx, float scale)
{
  G_ASSERT_RETURN(stick_idx >= 0 && stick_idx < 2, );
  stickDeadZoneScale[stick_idx] = scale;
}

float SonyPadDevice::getStickDeadZoneAbs(int stick_idx) const
{
  G_ASSERT_RETURN(stick_idx >= 0 && stick_idx < 2, 0);
  return stickDeadZoneScale[stick_idx] * DEFAULT_STICK_DEADZONE / STICK_MAX_VAL;
}

int SonyPadDevice::getPairedButtonForAxis(int axis_idx) const
{
  if (axis_idx == 4)
    return JOY_XINPUT_REAL_BTN_L_TRIGGER;
  if (axis_idx == 5)
    return JOY_XINPUT_REAL_BTN_R_TRIGGER;
  return -1;
}

int SonyPadDevice::getPairedAxisForButton(int btn_idx) const
{
  if (btn_idx == JOY_XINPUT_REAL_BTN_L_TRIGGER)
    return 4;
  if (btn_idx == JOY_XINPUT_REAL_BTN_R_TRIGGER)
    return 5;
  return -1;
}

int SonyPadDevice::getJointAxisForAxis(int axis_idx) const
{
  switch (axis_idx)
  {
    case 0: return 1;
    case 1: return 0;
    case 2: return 3;
    case 3: return 2;
  }
  return -1;
}

bool SonyPadDevice::getDeviceDesc(DataBlock &out_desc) const
{
  WinAutoLock lock(cs); // the IO thread rewrites the calibration when a DualShock 4 adapter pairs its pad
  out_desc.clearData();
  out_desc.setBool("touchpad", true);
  out_desc.setBool("orientSensor", true);
  out_desc.setStr("model", sonypad::model_name(model));
  out_desc.setStr("connection", bluetooth ? "bluetooth" : "usb");
  out_desc.setStr("mac", mac);
  out_desc.setStr("firmware", String(0, "hw %08x fw %08x upd %04x", firmware.hwVersion, firmware.fwVersion, firmware.updateVersion));
  out_desc.setBool("rumbleV2", rumbleV2);
  out_desc.setBool("calibrated", calibrated);
  if (shared.sample.hasStatus)
  {
    out_desc.setInt("battery", sonypad::battery_percent(shared.sample));
    out_desc.setBool("charging", shared.sample.charge == sonypad::CHARGE_CHARGING);
    out_desc.setBool("cablePowered", shared.sample.cablePowered);
  }
  return true;
}

bool SonyPadDevice::getMotionState(JoystickMotionState &out) const
{
  WinAutoLock lock(cs);
  out = shared.motion;
  return !interlocked_acquire_load(lost) && shared.sample.hasSensors;
}

bool SonyPadDevice::getTouchpadState(JoystickTouchpadState &out) const
{
  WinAutoLock lock(cs);
  out = JoystickTouchpadState();
  out.width = sonypad::TOUCHPAD_WIDTH;
  out.height = model == sonypad::Model::DUALSHOCK4 ? sonypad::DS4_TOUCHPAD_HEIGHT : sonypad::DS_TOUCHPAD_HEIGHT;
  for (int i = 0; i < 2; i++)
    out.points[i] = shared.sample.touch[i];
  return !interlocked_acquire_load(lost);
}

bool SonyPadDevice::fetchTouchpadGestures(GestureCb cb)
{
  TouchpadGestureQueue fetched;
  {
    WinAutoLock lock(cs);
    fetched = gestureQueue;
    gestureQueue.clear();
  }
  fetched.drain(cb);
  return !interlocked_acquire_load(lost);
}

void SonyPadDevice::enableGyroscope(bool enable)
{
  WinAutoLock lock(cs);
  shared.gyroEnabled = enable;
}

// callers re-apply the mode to every device, so only a change is logged
void SonyPadDevice::enableSensorsTiltCorrection(bool enable)
{
  WinAutoLock lock(cs);
  if (shared.tiltCorrection == enable)
    return;
  debug("[HID][SONY] tilt correction %s", enable ? "on" : "off");
  shared.tiltCorrection = enable;
}

void SonyPadDevice::reportGyroSensorsAngDelta(bool report_ang_delta)
{
  WinAutoLock lock(cs);
  if (gyroSensorsAngDelta == report_ang_delta)
    return;
  debug("[HID][SONY] reportGyroSensorsAngDelta: %s", report_ang_delta ? "delta" : "angle");
  gyroSensorsAngDelta = report_ang_delta;
}

void SonyPadDevice::resetSensorsOrientation()
{
  WinAutoLock lock(cs);
  shared.resetOrientation = true;
}

void SonyPadDevice::doRumble(float lowFreq, float highFreq)
{
  const uint8_t lo = uint8_t(clamp(lowFreq, 0.f, 1.f) * 255.f), hi = uint8_t(clamp(highFreq, 0.f, 1.f) * 255.f);
  WinAutoLock lock(cs);
  if (shared.out.rumbleLeft == lo && shared.out.rumbleRight == hi)
    return;
  shared.out.rumbleLeft = lo;
  shared.out.rumbleRight = hi;
  shared.outDirty = true;
}

void SonyPadDevice::setLight(bool set, unsigned char r, unsigned char g, unsigned char b)
{
  const uint8_t *def = default_lightbar[ordId & 3];
  const uint8_t rgb[3] = {set ? r : def[0], set ? g : def[1], set ? b : def[2]};
  WinAutoLock lock(cs);
  if (shared.out.lightbarSet && memcmp(shared.out.lightbar, rgb, 3) == 0)
    return;
  shared.out.lightbarSet = true;
  memcpy(shared.out.lightbar, rgb, 3);
  shared.outDirty = true;
}

void SonyPadDevice::setPlayerLeds(unsigned mask)
{
  WinAutoLock lock(cs);
  if (shared.out.playerLedsSet && shared.out.playerLeds == uint8_t(mask))
    return;
  shared.out.playerLedsSet = true;
  shared.out.playerLeds = uint8_t(mask);
  shared.outDirty = true;
}

void SonyPadDevice::setMicLed(JoystickMicLed mode)
{
  WinAutoLock lock(cs);
  if (shared.out.micLedSet && shared.out.micLed == mode)
    return;
  shared.out.micLedSet = true;
  shared.out.micLed = mode;
  shared.outDirty = true;
}

void SonyPadDevice::setTriggerEffect(const TriggerEffectParam &param)
{
  WinAutoLock lock(cs);
  for (int i = 0; i < TRIGGER_EFFECT_TRIGGER_NUM; i++)
    if ((param.triggerMask & (1 << i)) && memcmp(&shared.out.trigger[i], &param.command[i], sizeof(param.command[i])) != 0)
    {
      shared.out.trigger[i] = param.command[i];
      shared.outDirty = true;
    }
}

void SonyPadDevice::setCalibration(const sonypad::SensorCalibration &calib, bool from_device)
{
  WinAutoLock lock(cs);
  calibration = calib;
  calibrated = from_device;
}

void SonyPadDevice::setLost()
{
  WinAutoLock lock(cs);
  gestures.reset(gestureQueue);
  interlocked_release_store(lost, true);
}

bool SonyPadDevice::takeOutputReport(uint8_t *out, int cap, int &len, bool quiet)
{
  sonypad::OutputState st;
  {
    WinAutoLock lock(cs);
    if (quiet)
    {
      shared.out.rumbleLeft = shared.out.rumbleRight = 0;
      for (TriggerEffectCommand &t : shared.out.trigger)
        t = TriggerEffectCommand();
    }
    else if (!shared.outDirty)
      return false;
    st = shared.out;
    shared.outDirty = st.resetLightbar; // the colour goes again after the light-out
    shared.out.resetLightbar = false;
  }
  len = sonypad::build_output_report(model, bluetooth, rumbleV2, st, btSeq, out, cap);
  return len > 0;
}

static Quat rotation_quat(const Point3 &rot_vec)
{
  const float ang = rot_vec.length();
  return ang > 1e-6f ? Quat(rot_vec, ang) : Quat(0, 0, 0, 1);
}

// runs per report on the IO thread, with cs held
void SonyPadDevice::integrateSensors(const sonypad::InputSample &s)
{
  Shared &sh = shared;
  float dtUs = 0;
  if (sh.haveTicks)
  {
    dtUs = float((s.sensorTicks - sh.prevTicks) & s.sensorTicksMask) * s.sensorTickUs;
    // the 16-bit DualShock 4 counter wraps in 0.35 s; past that the host clock tells the gap
    const float hostUs = float(ref_time_delta_to_usec(ref_time_ticks() - sh.prevHostTicks));
    if (hostUs > float(s.sensorTicksMask) * s.sensorTickUs)
      dtUs = hostUs;
  }
  sh.prevTicks = s.sensorTicks;
  sh.prevHostTicks = ref_time_ticks();
  sh.haveTicks = true;
  // a report gap after a stall integrates as one step of at most 100 ms, the timestamp keeps the real gap
  const float dt = clamp(dtUs * 1e-6f, 0.f, 0.1f);

  Point3 gyroDegS, accelG;
  for (int i = 0; i < 3; i++)
  {
    gyroDegS[i] = calibration.gyroDegS(i, s.gyroRaw[i]);
    accelG[i] = calibration.accelG(i, s.accelRaw[i]);
    sh.motion.angVelDegS[i] = gyroDegS[i];
    sh.motion.accelG[i] = accelG[i];
  }
  sh.motion.timestampUs += uint32_t(dtUs + 0.5f);
  sh.motion.sampleCount++;

  if (sh.resetOrientation)
  {
    sh.orient = Quat(0, 0, 0, 1);
    sh.resetOrientation = false;
  }
  if (!sh.gyroEnabled || dt <= 0.f)
    return;

  // gravity direction: a slow low pass of the accelerometer, so shakes do not swing the yaw axis
  if (accelG.lengthSq() > 0.25f)
    sh.gravity = lerp(sh.gravity, accelG, clamp(dt * 5.f, 0.f, 1.f));
  Point3 up = sh.gravity;
  up.normalize();
  if (up.lengthSq() < 0.5f)
    up = Point3(0, 1, 0);

  const Point3 w = gyroDegS * DEG_TO_RAD;
  sh.deltaLocal += w * dt;
  const float worldYaw = w.y * up.y + w.z * up.z;
  const float yawRoll = sqrtf(w.y * w.y + w.z * w.z);
  sh.deltaWorldYaw += clamp(worldYaw * PLAYER_SPACE_YAW_RELAX, -yawRoll, yawRoll) * dt;

  sh.orient = sh.orient * rotation_quat(w * dt);
  if (sh.tiltCorrection)
  {
    // pull the integrated up axis towards the measured gravity, a complementary filter
    const Point3 upBody = inverse(sh.orient) * Point3(0, 1, 0);
    sh.orient = sh.orient * rotation_quat(cross(up, upBody) * clamp(dt * 2.f, 0.f, 0.5f));
  }
  sh.orient = ::normalize(sh.orient);
}

bool SonyPadDevice::onInputReport(const uint8_t *data, int len)
{
  sonypad::InputSample s;
  if (!sonypad::parse_input_report(model, bluetooth, data, len, s))
    return false;
  WinAutoLock lock(cs);
  const bool present = !s.dongleEmpty;
  const bool changed = present != (interlocked_acquire_load(padPresent) != 0);
  if (changed && present)
  {
    // the counters of the motion sample belong to one pad connection
    shared.motion = JoystickMotionState();
    shared.haveTicks = false;
  }
  if (present && s.hasSensors)
    integrateSensors(s);
  if (present && !s.hasTouch)
    memcpy(s.touch, shared.sample.touch, sizeof(s.touch));
  if (present)
    gestures.update(s.touch, (s.buttons & sonypad::BTN_TOUCHPAD) != 0, get_time_msec(), gestureQueue);
  else
    gestures.reset(gestureQueue);
  shared.sample = s;
  interlocked_release_store(padPresent, present ? 1 : 0);
  return changed;
}

void SonyPadDevice::fillState(const Shared &snap, int dt_msec)
{
  const sonypad::InputSample &s = snap.sample;
  state.buttonsPrev = state.buttons;
  state.buttons.reset();

  uint32_t v = hat_to_dpad[s.hat < 9 ? s.hat : 8];
  for (const auto &m : button_map)
    if ((s.buttons & m.src) && m.dst < devButtonCount)
      state.buttons.set(m.dst);
  state.buttons.orWord0(v);

  uint8_t lx = s.lx, ly = s.ly, rx = s.rx, ry = s.ry;
  apply_stick_deadzone(lx, ly, DEFAULT_STICK_DEADZONE * stickDeadZoneScale[0]);
  apply_stick_deadzone(rx, ry, DEFAULT_STICK_DEADZONE * stickDeadZoneScale[1]);
  state.x = remap_axis(lx);
  state.y = -remap_axis(ly);
  state.rx = remap_axis(rx);
  state.ry = -remap_axis(ry);
  state.slider[0] = remap_trigger(s.l2);
  state.slider[1] = remap_trigger(s.r2);

  quat_to_euler(snap.orient, state.sensorX, state.sensorY, state.sensorZ);
  if (gyroSensorsAngDelta)
  {
    // the same order as the PS5 driver euler deltas: heading (Y), attitude (Z), bank (X)
    const float yaw = snap.tiltCorrection ? snap.deltaWorldYaw : snap.deltaLocal.y;
    intSensors[0] = remap_sensor(yaw / HALFPI);
    intSensors[1] = remap_sensor(snap.deltaLocal.z / HALFPI);
    intSensors[2] = remap_sensor(snap.deltaLocal.x / HALFPI);
  }
  else
  {
    intSensors[0] = remap_sensor(state.sensorX / HALFPI);
    intSensors[1] = remap_sensor(state.sensorY / HALFPI);
    intSensors[2] = remap_sensor(state.sensorZ / HALFPI);
  }
  // the composite detects changes by this state
  state.rawSensX = intSensors[0];
  state.rawSensY = intSensors[1];
  state.rawSensZ = intSensors[2];

  if (!stg_joy.disableVirtualControls)
  {
    state.z = state.slider[1] - state.slider[0];
    uint32_t vb = 0;
    vb |= sel_vbtn(state.x, JOY_XINPUT_REAL_MASK_L_THUMB_LEFT, JOY_XINPUT_REAL_MASK_L_THUMB_RIGHT);
    vb |= sel_vbtn(state.y, JOY_XINPUT_REAL_MASK_L_THUMB_DOWN, JOY_XINPUT_REAL_MASK_L_THUMB_UP);
    vb |= sel_vbtn(state.rx, JOY_XINPUT_REAL_MASK_R_THUMB_LEFT, JOY_XINPUT_REAL_MASK_R_THUMB_RIGHT);
    vb |= sel_vbtn(state.ry, JOY_XINPUT_REAL_MASK_R_THUMB_DOWN, JOY_XINPUT_REAL_MASK_R_THUMB_UP);
    state.buttons.orWord0(vb);
  }

  for (int i = 0, inc = 0; i < state.MAX_BUTTONS; i += inc)
    if (state.buttons.getIter(i, inc))
      if (!state.buttonsPrev.get(i))
        state.bDownTms[i] = 0;
  for (int i = 0, inc = 0; i < state.MAX_BUTTONS; i += inc)
    if (state.buttonsPrev.getIter(i, inc))
      state.bDownTms[i] += dt_msec;
}

bool SonyPadDevice::updateState(int dt_msec, bool def)
{
  const int ctime = ::get_time_msec();
  if (state.buttons.hasAny() && ctime > lastKeyPressedTime + stg_joy.keyRepeatDelay)
  {
    lastKeyPressedTime = ctime;
    state.repeat = true;
  }
  else
    state.repeat = false;
  if (def)
    raw_state_joy.repeat = state.repeat;

  Shared snap;
  {
    WinAutoLock lock(cs);
    snap = shared;
    shared.deltaLocal = Point3(0, 0, 0);
    shared.deltaWorldYaw = 0;
  }
  if (!isConnected())
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
    return false;
  }

  fillState(snap, dt_msec);

  if (state.getKeysPressed().hasAny())
    lastKeyPressedTime = ctime + (stg_joy.keyFirstRepeatDelay - stg_joy.keyRepeatDelay);

  if (def)
  {
    raw_state_joy = state;
    if (!state.buttons.cmpEq(state.buttonsPrev) || state.x || state.y || state.rx || state.ry)
      dagor_demo_reset_idle_timer();
  }
  if (client)
    client->stateChanged(this, ordId);
  return (state.buttons.getWord0() & 0xF0FF) != 0;
}
