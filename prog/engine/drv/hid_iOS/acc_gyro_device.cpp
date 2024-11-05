// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "acc_gyro_device.h"
#include <drv/hid/dag_hiGlobals.h>
#include <supp/_platform.h>
#include <debug/dag_debug.h>
#include <perfMon/dag_cpuFreq.h>

extern void hid_iOS_update_motion();

namespace hid_ios
{
extern double acc_x, acc_y, acc_z;
extern double g_acc_x, g_acc_y, g_acc_z;
extern double m_acc_x, m_acc_y, m_acc_z;
extern double r_rate_x, r_rate_y, r_rate_z;
extern double g_rel_ang_x, g_rel_ang_y;
} // namespace hid_ios

const char *HumanInput::AccelGyroInpDevice::axisName[HumanInput::AccelGyroInpDevice::AXES_NUM] = {
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

HumanInput::AccelGyroInpDevice::AccelGyroInpDevice()
{
  client = NULL;
  memset(&state, 0, sizeof(state));
  memset(gravity, 0, sizeof(gravity));
  for (int i = 0; i < JoystickRawState::MAX_POV_HATS; i++)
    state.povValues[i] = state.povValuesPrev[i] = -1;

  axis[0].val = &state.x;
  axis[1].val = &state.y;
  axis[2].val = &state.z;
  axis[3].val = &state.rx;
  axis[4].val = &state.ry;
  axis[5].val = &state.rz;
  axis[6].val = &gravity[0];
  axis[7].val = &gravity[1];
  axis[8].val = &gravity[2];

  axis[9].val = &gravity[3];
  axis[10].val = &gravity[4];
  for (int i = 0; i < AXES_NUM; i++)
  {
    axis[i].kMul = 1.0f / 32000.0f;
    axis[i].kAdd = 0.0f;
  }

  doRumble(0, 0);
}
HumanInput::AccelGyroInpDevice::~AccelGyroInpDevice() { setClient(NULL); }


bool HumanInput::AccelGyroInpDevice::updateState(bool def)
{
  using namespace hid_ios;

  hid_iOS_update_motion();

  state.sensorX = acc_x;
  state.sensorY = acc_y;
  state.sensorZ = acc_z;

  state.x = clip_dead_zone(int(m_acc_x * 32000), 400);
  state.y = clip_dead_zone(int(m_acc_y * 32000), 400);
  state.z = clip_dead_zone(int(m_acc_z * 32000), 400);

  state.rx = clip_dead_zone(int(r_rate_x * 32000), 5 * 32000);
  state.ry = clip_dead_zone(int(r_rate_y * 32000), 5 * 32000);
  state.rz = clip_dead_zone(int(r_rate_z * 32000), 5 * 32000);

  state.slider[0] = clip_dead_zone(int(g_rel_ang_x * 32000), 200);
  state.slider[1] = clip_dead_zone(int(g_rel_ang_y * 32000), 200);

  gravity[0] = int(g_acc_x * 32000);
  gravity[1] = int(g_acc_y * 32000);
  gravity[2] = int(g_acc_z * 32000);

  gravity[3] = int(g_rel_ang_x * 32000);
  gravity[4] = int(g_rel_ang_y * 32000);

  if (def)
    raw_state_joy = state;

  if (client)
    client->stateChanged(this, 0);

  return false;
}
