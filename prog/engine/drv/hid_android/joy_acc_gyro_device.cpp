#include "joy_acc_gyro_device.h"
#include <humanInput/dag_hiGlobals.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_wndProcCompMsg.h>
#include <supp/_platform.h>
#include <supp/dag_android_native_app_glue.h>
#include <dlfcn.h>
#include <debug/dag_debug.h>
#include <perfMon/dag_cpuFreq.h>
#include "joy_helper_android.h"

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

HumanInput::JoyAccelGyroInpDevice::JoyAccelGyroInpDevice()
{
  deviceId = 0;
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
  add_wnd_proc_component(this);
}
HumanInput::JoyAccelGyroInpDevice::~JoyAccelGyroInpDevice()
{
  setClient(NULL);
  del_wnd_proc_component(this);
}


bool HumanInput::JoyAccelGyroInpDevice::getDeviceDesc(DataBlock &out_desc) const
{
  out_desc.clearData();
  // out_desc.setStr("accelSensor", true);
  return true;
}
bool HumanInput::JoyAccelGyroInpDevice::updateState()
{
  using namespace hid_ios;
  bool changed = memcmp(&state, &nextState, sizeof(state)) != 0;

  const int axis_scale = AXIS_SCALE_FACTOR;

  static int prev_t = get_time_msec();
  int dt_msec = get_time_msec() - prev_t;
  prev_t = get_time_msec();

  nextState.buttonsPrev = state.buttons;
  state = nextState;
  state.buttons.clr(26);
  state.buttons.clr(27);
  if (state.buttons.get(6) && (state.buttonsPrev.get(26) || (!(state.buttons.getWord0() & (0xF << 18)) && !state.buttonsPrev.get(6))))
  {
    state.buttons.clr(6);
    state.buttons.set(26);
  }

  if (state.buttons.get(7) && (state.buttonsPrev.get(27) || (!(state.buttons.getWord0() & (0xF << 22)) && !state.buttonsPrev.get(7))))
  {
    state.buttons.clr(7);
    state.buttons.set(27);
  }

  state.sensorX = acc_x;
  state.sensorY = acc_y;
  state.sensorZ = acc_z;

  accel[0] = clip_dead_zone(int(m_acc_x * axis_scale), 400);
  accel[1] = clip_dead_zone(int(m_acc_y * axis_scale), 400);
  accel[2] = clip_dead_zone(int(m_acc_z * axis_scale), 400);

  gyro[0] = clip_dead_zone(int(r_rate_x * axis_scale), 5 * axis_scale);
  gyro[1] = clip_dead_zone(int(r_rate_y * axis_scale), 5 * axis_scale);
  gyro[2] = clip_dead_zone(int(r_rate_z * axis_scale), 5 * axis_scale);

  gravity[0] = int(g_acc_x * axis_scale);
  gravity[1] = int(g_acc_y * axis_scale);
  gravity[2] = int(g_acc_z * axis_scale);

  gravity[3] = clip_dead_zone(int(g_rel_ang_x * axis_scale), 200);
  gravity[4] = clip_dead_zone(int(g_rel_ang_y * axis_scale), 200);

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
  if (!state.buttons.get(6) && state.buttons.get(26))
  {
    nextState.buttons.set(6);
    state.buttons.set(6);
  }
  if (!state.buttons.get(7) && state.buttons.get(27))
  {
    nextState.buttons.set(7);
    state.buttons.set(7);
  }

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

IWndProcComponent::RetCode HumanInput::JoyAccelGyroInpDevice::process(void *hwnd, unsigned msg, uintptr_t wParam, intptr_t lParam,
  intptr_t &result)
{
  if (msg == GPCM_JoystickMovement)
  {
    android::JoystickEvent *event = (android::JoystickEvent *)wParam;
    deviceId = event->deviceId;

    const int axis_scale = AXIS_SCALE_FACTOR;
    const int axis_scale_half = AXIS_SCALE_FACTOR / 2;

    float lx = event->axisValues[0];
    float ly = -event->axisValues[1];

    float rx = event->axisValues[11];
    float ry = -event->axisValues[14];

    apply_stick_deadzone(lx, ly, stickDeadZoneBase[0] * stickDeadZoneScale[0]);
    apply_stick_deadzone(rx, ry, stickDeadZoneBase[1] * stickDeadZoneScale[1]);

    nextState.x = int(lx * axis_scale);
    nextState.y = int(ly * axis_scale);
    nextState.rx = int(rx * axis_scale);
    nextState.ry = int(ry * axis_scale);

    nextState.slider[0] = clip_dead_zone(int(event->axisValues[23] * axis_scale), 400);
    nextState.slider[1] = clip_dead_zone(int(event->axisValues[22] * axis_scale), 400);

    int dpadx = int(event->axisValues[15] * axis_scale);
    int dpady = int(event->axisValues[16] * axis_scale);
    (dpady < -20000) ? nextState.buttons.set(0) : nextState.buttons.clr(0);
    (dpady > 20000) ? nextState.buttons.set(1) : nextState.buttons.clr(1);
    (dpadx < -20000) ? nextState.buttons.set(2) : nextState.buttons.clr(2);
    (dpadx > 20000) ? nextState.buttons.set(3) : nextState.buttons.clr(3);

    (nextState.slider[0] > axis_scale_half) ? nextState.buttons.set(16) : nextState.buttons.clr(16);
    (nextState.slider[1] > axis_scale_half) ? nextState.buttons.set(17) : nextState.buttons.clr(17);

    (nextState.x > axis_scale_half) ? nextState.buttons.set(18) : nextState.buttons.clr(18);
    (nextState.x < -axis_scale_half) ? nextState.buttons.set(19) : nextState.buttons.clr(19);
    (nextState.y > axis_scale_half) ? nextState.buttons.set(20) : nextState.buttons.clr(20);
    (nextState.y < -axis_scale_half) ? nextState.buttons.set(21) : nextState.buttons.clr(21);
    (nextState.rx > axis_scale_half) ? nextState.buttons.set(22) : nextState.buttons.clr(22);
    (nextState.rx < -axis_scale_half) ? nextState.buttons.set(23) : nextState.buttons.clr(23);
    (nextState.ry > axis_scale_half) ? nextState.buttons.set(24) : nextState.buttons.clr(24);
    (nextState.ry < -axis_scale_half) ? nextState.buttons.set(25) : nextState.buttons.clr(25);
  }
  else if (msg == GPCM_KeyPress || msg == GPCM_KeyRelease)
  {
    int btIdx = -1;
    switch (wParam)
    {
      case 96: btIdx = 12; break;
      case 97: btIdx = 13; break;
      case 99: btIdx = 14; break;
      case 100: btIdx = 15; break;
      case 102: btIdx = 8; break;
      case 103: btIdx = 9; break;
      case 106: btIdx = 6; break;
      case 107: btIdx = 7; break;
      case 108: btIdx = 4; break;
      case 109: btIdx = 5; break;
      default: debug("gamepad: keycode (%d) was not remaped", wParam); break;
    }
    if (btIdx >= 0)
      msg == GPCM_KeyPress ? nextState.buttons.set(btIdx) : nextState.buttons.clr(btIdx);
    // debug("%s %d", msg == GPCM_KeyPress ? "dn" : "up", wParam);
  }

  return PROCEED_OTHER_COMPONENTS;
}
