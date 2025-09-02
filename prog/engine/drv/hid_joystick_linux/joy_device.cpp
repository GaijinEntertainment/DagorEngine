// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "joy_device.h"
#include <drv/hid/dag_hiXInputMappings.h>
#include <unistd.h>

#include <perfMon/dag_cpuFreq.h>
#include <util/dag_string.h>
#include <math/dag_mathUtils.h>
#include <drv/hid/dag_hiDInput.h>
#include <drv/hid/dag_hiGlobals.h>
#include <stdio.h>
#include <generic/dag_carray.h>
#include "evdev_helpers.h"

using namespace HumanInput;

// int32 is not enough for [-32768; 32768] to [-32000; 32000] conversion
static int64_t cvt_int(int64_t val, int64_t vmin, int64_t vmax, int64_t omin, int64_t omax)
{
  if (val <= vmin)
    return omin;
  if (val >= vmax)
    return omax;
  return omin + (omax - omin) * (val - vmin) / (vmax - vmin);
}

inline int deadzone_apply(input_absinfo &axis, const input_absinfo &target)
{
  int threshold = cvt_int(axis.flat, 0, axis.maximum, 0, target.maximum);
  int64_t value = cvt_int(axis.value, axis.minimum, axis.maximum, target.minimum, target.maximum);

  // use deadzone only for symmetric axes
  if (target.minimum == -target.maximum)
  {
    if (value <= threshold && value >= -threshold)
      value = 0;
    else
    {
      if (value > 0)
        value -= threshold;
      else
        value += threshold;
    }
    return cvt_int(value, eastl::min<int>(target.minimum + threshold, 0), eastl::max<int>(target.maximum - threshold, 0),
      target.minimum, target.maximum);
  }
  return value;
}

static const char *get_abs_axis_name(int axis)
{
  switch (axis)
  {
    case ABS_X: return "X Axis";
    case ABS_Y: return "Y Axis";
    case ABS_Z: return "Z Axis";
    case ABS_RX: return "Rx Axis";
    case ABS_RY: return "Ry Axis";
    case ABS_RZ: return "Rz Axis";
    case ABS_THROTTLE: return "Throttle";
    case ABS_RUDDER: return "Rudder";
    case ABS_WHEEL: return "Wheel";
    case ABS_GAS: return "Gas";
    case ABS_BRAKE: return "Brake";
    case ABS_PRESSURE: return "Pressure";
    case ABS_DISTANCE: return "Distance";
    case ABS_TILT_X: return "X Tilt";
    case ABS_TILT_Y: return "Y Tilt";
    case ABS_MISC: return "Misc";
  }
  return nullptr;
}

static const char *get_button_name(int btn)
{
  switch (btn)
  {
    case BTN_TRIGGER: return "TRIGGER";
    case BTN_THUMB: return "THUMB";
    case BTN_THUMB2: return "THUMB2";
    case BTN_TOP: return "TOP";
    case BTN_TOP2: return "TOP2";
    case BTN_PINKIE: return "PINKIE";
    case BTN_BASE: return "BASE";
    case BTN_BASE2: return "BASE2";
    case BTN_BASE3: return "BASE3";
    case BTN_BASE4: return "BASE4";
    case BTN_BASE5: return "BASE5";
    case BTN_BASE6: return "BASE6";
    case BTN_DEAD: return "DEAD";
    case BTN_A: return "A";
    case BTN_B: return "B";
    case BTN_C: return "C";
    case BTN_X: return "X";
    case BTN_Y: return "Y";
    case BTN_Z: return "Z";
    case BTN_TL: return "TL";
    case BTN_TR: return "TR";
    case BTN_TL2: return "TL2";
    case BTN_TR2: return "TR2";
    case BTN_SELECT: return "SELECT";
    case BTN_START: return "START";
    case BTN_MODE: return "MODE";
    case BTN_THUMBL: return "THUMBL";
    case BTN_THUMBR: return "THUMBR";
    case BTN_GEAR_DOWN: return "GEAR DOWN";
    case BTN_GEAR_UP: return "GEAR UP";
  }
  return nullptr;
}

HidJoystickDevice::HidJoystickDevice(UDev::Device const &dev) : device(dev)
{
  JOY_LOG("added device: <%s> %04LX:%04LX (%s)", dev.name, dev.id.vendor, dev.id.product, dev.devnode);
  SNPRINTF(devID, sizeof(devID), "%04X:%04X", dev.id.vendor, dev.id.product);
  id = dev.id;

  jfd = open(device.devnode, O_RDONLY, 0);
  G_ASSERT(jfd >= 0);

  InputBitArray<KEY_MAX> keybit;
  InputBitArray<ABS_MAX> absbit;
  InputBitArray<REL_MAX> relbit;

  if (keybit.evdevRead(jfd, EV_KEY) && absbit.evdevRead(jfd, EV_ABS))
  {
    for (int axis = ABS_HAT0X; axis <= ABS_HAT3X; axis += 2)
    {
      if (!absbit.test(axis) || !absbit.test(axis + 1))
        continue;

      input_absinfo absinfo;
      if (!read_axis(jfd, axis, absinfo))
        continue;
      PovHatData &a = povHats.push_back();
      a.sysAbsInfo = absinfo;
      a.sysAxisId = axis;
      a.minVal = absinfo.minimum;
      a.maxVal = absinfo.maximum;
      a.name = String(0, "POV%d", povHats.size());
      a.axisName[0] = String(0, "POV%d_H", povHats.size());
      a.axisName[1] = String(0, "POV%d_V", povHats.size());
      a.padX = 0;
      a.padY = 0;

      JOY_LOG("device has povHat '%s'. val: %d min: %d max: %d fuzz: %d flat: %d res: %d", a.name, absinfo.value, absinfo.minimum,
        absinfo.maximum, absinfo.fuzz, absinfo.flat, absinfo.resolution);
    }


    for (int axis = ABS_X; axis < ABS_MAX; ++axis)
    {
      if (axis >= ABS_HAT0X && axis <= ABS_HAT3Y)
        continue;

      if (!absbit.test(axis))
        continue;

      input_absinfo absinfo;
      if (!read_axis(jfd, axis, absinfo))
        continue;

      AxisData &a = axes.push_back();
      a.sysAbsInfo = absinfo;
      a.sysAxisId = axis;
      a.sysAbsInfo.minimum = -32000;
      a.sysAbsInfo.maximum = 32000;
      a.remapIdx = -1;
      a.minVal = a.sysAbsInfo.minimum;
      a.maxVal = a.sysAbsInfo.maximum;

      a.sysAbsInfo.value = deadzone_apply(absinfo, a.sysAbsInfo);

      const char *name = get_abs_axis_name(axis);
      if (!name)
        a.name = String(0, "Axis%d", axes.size());
      else
        a.name = name;

      JOY_LOG("device has axis '%s'. val: %d min: %d max: %d fuzz: %d flat: %d res: %d", a.name, absinfo.value, absinfo.minimum,
        absinfo.maximum, absinfo.fuzz, absinfo.flat, absinfo.resolution);
    }

    for (int btn = BTN_MISC; btn < KEY_MAX; ++btn)
    {
      if (!keybit.test(btn))
        continue;

      ButtonData &b = buttons.push_back();
      b.sysBtnId = btn;
      const char *name = get_button_name(btn);
      if (!name)
        b.name = String(0, "Button%d", buttons.size());
      else
        b.name = name;
      JOY_LOG("device has button '%s' with code 0x%x", b.name, b.sysBtnId);
    }

    JOY_LOG("device has %d axes", axes.size());
    if (povHats.size() > 0)
      JOY_LOG("device has %d pov hats", povHats.size());
    JOY_LOG("device has %d buttons", buttons.size());
  }

  if (relbit.evdevRead(jfd, EV_REL))
  {
    bool hasRelAxes = false;
    for (int i = REL_X; i < REL_MAX; ++i)
      if (relbit.test(REL_X))
      {
        hasRelAxes = true;
        JOY_LOG("device has relative axis %x", i);
      }
    if (hasRelAxes)
      JOY_LOG("relative axes not supported");
  }

  connected = true;
  memset(&state, 0, sizeof(state));

  updateState(0, false);
}

HidJoystickDevice::~HidJoystickDevice()
{
  if (jfd >= 0)
    close(jfd);
}

void HidJoystickDevice::updateDevice(UDev::Device const &dev) { device = dev; }

void HidJoystickDevice::setConnected(bool con)
{
  connected = con;
  if (!connected && jfd >= 0)
  {
    close(jfd);
    jfd = -1;
  }

  if (connected && jfd < 0)
  {
    jfd = open(device.devnode, O_RDONLY, 0);
    if (jfd < 0)
    {
      debug("failed to open joystick device %s", device.devnode);
      connected = false;
    }
  }
}

bool HidJoystickDevice::updateState(int dt_msec, bool def)
{
  if (!connected)
    return false;

  bool changed = false;

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

  input_absinfo absinfo;
  for (int i = 0; i < axes.size(); ++i)
  {
    AxisData &axis = axes[i];
    if (axis.sysAxisId != ABS_MAX && read_axis(jfd, axis.sysAxisId, absinfo))
    {
      int val = deadzone_apply(absinfo, axis.sysAbsInfo);
      if (val != axis.sysAbsInfo.value)
      {
        axis.sysAbsInfo.value = val;
        changed = true;
        JOY_TRACE("[%s] %s: %d (%d)", getName(), axis.name, axis.sysAbsInfo.value, absinfo.value);
      }
      switch (axis.sysAxisId)
      {
        case ABS_X: state.x = val; break;
        case ABS_Y: state.y = val; break;
        case ABS_Z: state.z = val; break;
        case ABS_RX: state.rx = val; break;
        case ABS_RY: state.ry = val; break;
        case ABS_RZ: state.rz = val; break;
        case ABS_BRAKE: state.slider[0] = val; break;
        case ABS_GAS: state.slider[1] = val; break;
      }
    }
  }


  for (int i = 0; i < povHats.size(); ++i)
  {
    PovHatData &axis = povHats[i];
    input_absinfo absinfoY;
    input_absinfo absinfoX;

    if (read_axis(jfd, axis.sysAxisId, absinfoX) && read_axis(jfd, axis.sysAxisId + 1, absinfoY))
    {
      axis.padX = absinfoX.value;
      axis.padY = -absinfoY.value;
      int angle = povhat_pads_to_angle(axis.padX, axis.padY);

      state.povValuesPrev[i] = state.povValues[i];
      state.povValues[i] = angle;
      bool povChanged = (state.povValuesPrev[i] != angle);
      changed |= povChanged;

      if (povChanged)
      {
        JOY_TRACE("[%s] hat.x %d hat.y %d, angle %d", getName(), axis.padX, axis.padY, angle);
      }
    }
  }

  state.buttonsPrev = state.buttons;
  state.buttons.reset();
  if (sysKeys.evdevRead(jfd))
  {
    for (int i = 0; i < buttons.size(); ++i)
    {
      ButtonData &b = buttons[i];
      if (b.sysBtnId == KEY_MAX)
        continue;
      if (sysKeys.test(b.sysBtnId))
      {
        if (!state.buttonsPrev.get(i))
          JOY_TRACE("[%s] button pressed %s %d", getName(), b.name, i);
        state.buttons.set(i);
      }
    }
  }

  if (state.getKeysPressed().hasAny()) // Little trick to avoid new static bool isFirstTimePressed
    lastKeyPressedTime = ctime + (stg_joy.keyFirstRepeatDelay - stg_joy.keyRepeatDelay);

  if (!state.buttons.cmpEq(state.buttonsPrev))
  {
    changed = true;
    for (int i = 0, inc = 0; i < state.MAX_BUTTONS; i += inc)
      if (state.buttons.getIter(i, inc))
        if (!state.buttonsPrev.get(i))
          state.bDownTms[i] = 0;
  }

  for (int i = 0, inc = 0; i < state.MAX_BUTTONS; i += inc)
    if (state.buttonsPrev.getIter(i, inc))
      state.bDownTms[i] += dt_msec;


  if (def)
  {
    if (changed)
      raw_state_joy = state;
    else
      memcpy(raw_state_joy.bDownTms, state.bDownTms, sizeof(raw_state_joy.bDownTms));
  }

  if (client && changed)
    client->stateChanged(this, 0);

  return changed;
}

// IGenJoystick interface implementation
void HidJoystickDevice::setClient(IGenJoystickClient *cli)
{
  if (cli == client)
    return;

  if (client)
    client->detached(this);
  client = cli;
  if (client)
    client->attached(this);
}

IGenJoystickClient *HidJoystickDevice::getClient() const { return client; }

int HidJoystickDevice::getButtonCount() const { return buttons.size(); }

const char *HidJoystickDevice::getButtonName(int idx) const
{
  if (idx >= 0 && idx < buttons.size())
    return buttons[idx].name;
  return NULL;
}

int HidJoystickDevice::getAxisCount() const { return axes.size() + 2 * povHats.size(); }

const char *HidJoystickDevice::getAxisName(int idx) const
{
  const int ac = axes.size();
  if (idx >= 0 && idx < ac)
    return axes[idx].name;
  if (idx >= ac && idx < ac + 2 * povHats.size())
    return povHats[(idx - ac) / 2].axisName[(idx - ac) % 2];
  return NULL;
}

int HidJoystickDevice::getPovHatCount() const { return povHats.size(); }

const char *HidJoystickDevice::getPovHatName(int idx) const
{
  if (idx >= 0 && idx < povHats.size())
    return povHats[idx].name;
  return NULL;
}

int HidJoystickDevice::getButtonId(const char *name) const
{
  for (int i = 0; i < buttons.size(); i++)
    if (strcmp(buttons[i].name, name) == 0)
      return i;
  return -1;
}

int HidJoystickDevice::getAxisId(const char *name) const
{
  for (int i = 0; i < axes.size(); i++)
    if (strcmp(axes[i].name, name) == 0)
      return i;
  for (int i = 0; i < povHats.size(); i++)
    for (int j = 0; j < 2; j++)
      if (strcmp(povHats[i].axisName[j], name) == 0)
        return axes.size() + i * 2 + j;
  return -1;
}

int HidJoystickDevice::getPovHatId(const char *name) const
{
  for (int i = 0; i < povHats.size(); i++)
    if (strcmp(povHats[i].name, name) == 0)
      return i;
  return -1;
}

void HidJoystickDevice::setAxisLimits(int axis_id, float min_val, float max_val)
{
  const int ac = axes.size();
  if (axis_id >= 0 && axis_id < ac)
  {
    AxisData &axis = axes[axis_id];
    axis.minVal = min_val;
    axis.maxVal = max_val;
  }
}

bool HidJoystickDevice::getButtonState(int btn_id) const { return btn_id >= 0 ? state.buttons.get(btn_id) : false; }

float HidJoystickDevice::getAxisPos(int axis_id) const
{
  const int ac = axes.size();
  if (axis_id >= 0 && axis_id < ac)
  {
    AxisData const &axis = axes[axis_id];
    return cvt(axis.sysAbsInfo.value, axis.sysAbsInfo.minimum, axis.sysAbsInfo.maximum, axis.minVal, axis.maxVal);
  }
  if (axis_id >= ac && axis_id < ac + 2 * povHats.size())
    return getVirtualPOVAxis((axis_id - ac) / 2, (axis_id - ac) % 2);
  return 0;
}

int HidJoystickDevice::getAxisPosRaw(int axis_id) const
{
  const int ac = axes.size();
  if (axis_id >= 0 && axis_id < ac)
    return axes[axis_id].sysAbsInfo.value;
  if (axis_id >= ac && axis_id < ac + 2 * povHats.size())
    return getVirtualPOVAxis((axis_id - ac) / 2, (axis_id - ac) % 2);
  return 0;
}

int HidJoystickDevice::getPovHatAngle(int hat_id) const
{
  if (hat_id >= 0 && hat_id < povHats.size())
    return state.povValues[hat_id];
  return -1;
}

int HidJoystickDevice::getVirtualPOVAxis(int hat_id, int axis_id) const
{
  if ((axis_id != 0) && (axis_id != 1))
    return 0;
  int value = getPovHatAngle(hat_id);
  if (value >= 0)
  {
    float angle = -(float(value) / 36000.f) * TWOPI + HALFPI;
    float axisVal = (axis_id == 1) ? sin(angle) : cos(angle);
    return int(axisVal * 32000);
  }
  return 0;
}
