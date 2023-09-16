#include "joy_device.h"
#include <humanInput/dag_hiXInputMappings.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <perfMon/dag_cpuFreq.h>
#include <util/dag_string.h>
#include <math/dag_mathUtils.h>
#include <humanInput/dag_hiDInput.h>
#include <humanInput/dag_hiGlobals.h>
#include <stdio.h>
#include <limits.h>
#include <generic/dag_carray.h>

using namespace HumanInput;

static const carray<uint32_t, JOY_XINPUT_REAL_BTN_COUNT> x360KeyMap = {KEY_MAX, KEY_MAX, KEY_MAX, KEY_MAX, BTN_START, BTN_SELECT,
  BTN_THUMBL, BTN_THUMBR, BTN_TL, BTN_TR, KEY_MAX, KEY_MAX, BTN_A, BTN_B, BTN_X, BTN_Y, KEY_MAX, KEY_MAX, KEY_MAX, KEY_MAX, KEY_MAX,
  KEY_MAX, KEY_MAX, KEY_MAX, KEY_MAX, KEY_MAX, KEY_MAX, KEY_MAX};

#define AXIS_KEY_NUM 2

struct AxisKey
{
  int code[AXIS_KEY_NUM];
};

static const AxisKey x360AxisMap[JOY_XINPUT_REAL_AXIS_COUNT] = {
  {ABS_X, ABS_MAX}, {ABS_Y, ABS_MAX}, {ABS_RX, ABS_MAX}, {ABS_RY, ABS_MAX}, {ABS_Z, ABS_BRAKE}, {ABS_RZ, ABS_GAS}};

#define test_bit(nr, addr) (((1UL << ((nr) % (sizeof(long) * 8))) & ((addr)[(nr) / (sizeof(long) * 8)])) != 0)
#define NBITS(x)           ((((x)-1) / (sizeof(long) * 8)) + 1)

static const float VIRTUAL_THUMBKEY_PRESS_THRESHOLD = 0.5f;

static const carray<int, JOY_XINPUT_REAL_AXIS_COUNT> axis_dead_thres = {7849, 7849, 8689, 8689, 3000, 3000};

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
    case ABS_HAT0X: return "Hot0 X";
    case ABS_HAT0Y: return "Hot0 Y";
  }
  return NULL;
}

HidJoystickDevice::HidJoystickDevice(UDev::Device const &dev, bool remap_360) :
  device(dev), buttons(midmem), axes(midmem), povHats(midmem), remapAsX360(remap_360)
{
  debug("added joystick: <%s> %04LX:%04LX (%s)", dev.name, dev.id.vendor, dev.id.product, dev.devnode);
  SNPRINTF(devID, sizeof(devID), "%04X:%04X", dev.id.vendor, dev.id.product);

  jfd = open(device.devnode, O_RDONLY, 0);
  G_ASSERT(jfd >= 0);

  unsigned long keybit[NBITS(KEY_MAX)] = {0};
  unsigned long absbit[NBITS(ABS_MAX)] = {0};
  unsigned long relbit[NBITS(REL_MAX)] = {0};

  if ((ioctl(jfd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit) >= 0) && (ioctl(jfd, EVIOCGBIT(EV_ABS, sizeof(absbit)), absbit) >= 0))
  {
    for (int axis = ABS_HAT0X; axis <= ABS_HAT3X; axis += 2)
    {
      if (!test_bit(axis, absbit) || !test_bit(axis + 1, absbit))
        continue;

      input_absinfo absinfo;
      if (ioctl(jfd, EVIOCGABS(axis), &absinfo) < 0)
        continue;

      PovHatData &a = povHats.push_back();
      a.axis_info = absinfo;
      a.axis = axis;
      a.min_val = -1.0f;
      a.max_val = 1.0f;
      a.attached = true;
      a.name = String(0, "POV%d", povHats.size());
      a.axisName[0] = String(0, "POV%d_H", povHats.size());
      a.axisName[1] = String(0, "POV%d_V", povHats.size());

      debug("joystick has povHat '%s'. val: %d min: %d max: %d fuzz: %d flat: %d res: %d", a.name, absinfo.value, absinfo.minimum,
        absinfo.maximum, absinfo.fuzz, absinfo.flat, absinfo.resolution);
    }

    if (remapAsX360)
    {
      axes.resize(JOY_XINPUT_REAL_AXIS_COUNT);
      for (int axisNo = 0; axisNo < axes.size(); ++axisNo)
      {
        AxisData &a = axes[axisNo];
        a.min_val = -1.0f;
        a.max_val = 1.0f;
        a.name = joyXInputAxisName[axisNo];
        memset(&a.axis_info, 0, sizeof(a.axis_info));

        for (int keyNo = 0; keyNo < AXIS_KEY_NUM; ++keyNo)
        {
          a.axis = x360AxisMap[axisNo].code[keyNo];
          a.attached = test_bit(a.axis, absbit) && ioctl(jfd, EVIOCGABS(a.axis), &a.axis_info) >= 0;
          if (a.attached)
            break;
        }
      }

      buttons.resize(x360KeyMap.size());
      for (int btnNo = 0; btnNo < buttons.size(); ++btnNo)
      {
        ButtonData &b = buttons[btnNo];
        b.btn = x360KeyMap[btnNo];
        b.name = joyXInputButtonName[btnNo];
        b.attached = test_bit(b.btn, keybit);
      }

      // One left stick is quite enough for x360 gamepad input,
      // it is needed by steam controller in mouse mode
      if (!(axes[JOY_XINPUT_REAL_AXIS_L_TRIGGER].attached && axes[JOY_XINPUT_REAL_AXIS_R_TRIGGER].attached &&
            axes[JOY_XINPUT_REAL_AXIS_L_THUMB_H].attached && axes[JOY_XINPUT_REAL_AXIS_L_THUMB_V].attached))
      {
        remapAsX360 = false;
      }

      if (
        !(buttons[JOY_XINPUT_REAL_BTN_A].attached && buttons[JOY_XINPUT_REAL_BTN_B].attached &&
          buttons[JOY_XINPUT_REAL_BTN_X].attached && buttons[JOY_XINPUT_REAL_BTN_Y].attached &&
          buttons[JOY_XINPUT_REAL_BTN_START].attached && buttons[JOY_XINPUT_REAL_BTN_BACK].attached &&
          buttons[JOY_XINPUT_REAL_BTN_L_SHOULDER].attached && buttons[JOY_XINPUT_REAL_BTN_R_SHOULDER].attached && povHats.size() > 0))
      {
        remapAsX360 = false;
      }

      if (!remapAsX360)
      {
        clear_and_shrink(axes);
        clear_and_shrink(buttons);
      }
    }

    for (int axis = ABS_X; axis < ABS_MAX; ++axis)
    {
      if (axis >= ABS_HAT0X && axis <= ABS_HAT3Y)
        continue;

      if (!test_bit(axis, absbit))
        continue;

      if (remapAsX360)
      {
        bool foundAxis = false;
        for (int axisNo = 0; axisNo < JOY_XINPUT_REAL_AXIS_COUNT && !foundAxis; ++axisNo)
          for (int keyNo = 0; keyNo < AXIS_KEY_NUM; ++keyNo)
            if (axis == x360AxisMap[axisNo].code[keyNo])
              foundAxis = true;
        if (foundAxis)
          continue;
      }

      input_absinfo absinfo;
      if (ioctl(jfd, EVIOCGABS(axis), &absinfo) < 0)
        continue;

      AxisData &a = axes.push_back();
      a.axis_info = absinfo;
      a.axis = axis;
      a.min_val = -1.0f;
      a.max_val = 1.0f;
      a.attached = true;
      const char *name = get_abs_axis_name(axis);
      if (!name)
        a.name = String(0, "Axis%d", axes.size());
      else
        a.name = name;

      debug("joystick has axis '%s'. val: %d min: %d max: %d fuzz: %d flat: %d res: %d", a.name, absinfo.value, absinfo.minimum,
        absinfo.maximum, absinfo.fuzz, absinfo.flat, absinfo.resolution);
    }

    for (int btn = BTN_MISC; btn < KEY_MAX; ++btn)
    {
      if (!test_bit(btn, keybit))
        continue;

      if (remapAsX360)
      {
        bool foundBtn = false;
        for (int btnNo = 0; btnNo < x360KeyMap.size() && !foundBtn; ++btnNo)
          if (btn == x360KeyMap[btnNo])
            foundBtn = true;
        if (foundBtn)
          continue;
      }

      ButtonData &b = buttons.push_back();
      b.btn = btn;
      b.name = String(0, "Button%d", buttons.size());
      b.attached = true;
    }

    debug("joystick has %d axes", axes.size());
    if (povHats.size() > 0)
      debug("joystick has %d pov hats", povHats.size());
    debug("joystick has %d buttons", buttons.size());
  }

  if (ioctl(jfd, EVIOCGBIT(EV_REL, sizeof(relbit)), relbit) >= 0)
  {
    for (int i = REL_X; i < REL_MAX; ++i)
      if (test_bit(REL_X, relbit))
        debug("joystick has relative axis %d", i);
  }

  connected = true;
  client = NULL;
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

bool HidJoystickDevice::isRemappedAsX360() const { return remapAsX360; }

bool HidJoystickDevice::updateState(int dt_msec, bool def)
{
  if (!connected)
    return false;

  bool changed = false;

  static int lastKeyPressedTime = 0;
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

  // record event
  static constexpr int DATA_SZ = 14 * sizeof(int);
  char prev_state[DATA_SZ];
  memcpy(prev_state, &state, DATA_SZ);

#define DEADCLAMP(value, threshold)                        \
  if (((value) < (threshold)) && ((value) > -(threshold))) \
  value = 0

  input_absinfo absinfo;
  for (int i = 0; i < axes.size(); ++i)
  {
    AxisData &axis = axes[i];
    if (axis.axis != ABS_MAX && ioctl(jfd, EVIOCGABS(axis.axis), &absinfo) >= 0)
    {
      int minAxisVal = JOY_XINPUT_MIN_AXIS_VAL;
      int maxAxisVal = JOY_XINPUT_MAX_AXIS_VAL;
      int absMin = absinfo.minimum;
      int absMax = absinfo.maximum;

      if (remapAsX360)
      {
        switch (axis.axis)
        {
          case ABS_Y:
          case ABS_RY: eastl::swap(absMin, absMax); break;
          case ABS_Z:
          case ABS_RZ: minAxisVal = 0; break;
        }
      }

      int cvtval = cvt(absinfo.value, absMin, absMax, minAxisVal, maxAxisVal);
      if (remapAsX360 && i < axis_dead_thres.size())
        DEADCLAMP(cvtval, axis_dead_thres[i]);
      if (cvtval != axis.axis_info.value)
      {
        axis.axis_info.value = cvtval;
        changed = true;
      }
    }
  }

#undef DEADCLAMP

  int16_t padX = 0, padY = 0;

  for (int i = 0; i < povHats.size(); ++i)
  {
    AxisData &axis = povHats[i];
    input_absinfo absinfoY;
    input_absinfo absinfoX;

    if ((ioctl(jfd, EVIOCGABS(axis.axis), &absinfoX) >= 0) && (ioctl(jfd, EVIOCGABS(axis.axis + 1), &absinfoY) >= 0))
    {
      if (remapAsX360)
        absinfoY.value = -absinfoY.value;

      int angle = 0;
      if (absinfoX.value == 0)
      {
        if (absinfoY.value == 0)
          angle = -1;
        else if (absinfoY.value == 1)
          angle = 0;
        else if (absinfoY.value == -1)
          angle = 180 * 100;
      }
      else if (absinfoX.value == 1)
      {
        if (absinfoY.value == 0)
          angle = 90 * 100;
        else if (absinfoY.value == 1)
          angle = 45 * 100;
        else if (absinfoY.value == -1)
          angle = 135 * 100;
      }
      else if (absinfoX.value == -1)
      {
        if (absinfoY.value == 0)
          angle = 270 * 100;
        else if (absinfoY.value == 1)
          angle = 315 * 100;
        else if (absinfoY.value == -1)
          angle = 225 * 100;
      }

      if (i == 0)
      {
        padX = absinfoX.value;
        padY = absinfoY.value;
      }

      state.povValuesPrev[i] = state.povValues[i];
      state.povValues[i] = angle;
      changed |= (state.povValuesPrev[i] != angle);
    }
  }

  state.buttonsPrev = state.buttons;
  state.buttons.reset();
  unsigned long keybit[NBITS(KEY_MAX)] = {0};
  if (ioctl(jfd, EVIOCGKEY(sizeof(keybit)), keybit) >= 0)
  {
    for (int i = 0; i < buttons.size(); ++i)
    {
      ButtonData &b = buttons[i];
      if (b.btn != KEY_MAX && test_bit(b.btn, keybit))
        state.buttons.set(i);
      else
        state.buttons.clr(i);
    }
  }

  if (remapAsX360)
  {
    state.x = axes[JOY_XINPUT_REAL_AXIS_L_THUMB_H].axis_info.value;
    state.y = axes[JOY_XINPUT_REAL_AXIS_L_THUMB_V].axis_info.value;

    state.rx = axes[JOY_XINPUT_REAL_AXIS_R_THUMB_H].axis_info.value;
    state.ry = axes[JOY_XINPUT_REAL_AXIS_R_THUMB_V].axis_info.value;

    state.slider[0] = axes[JOY_XINPUT_REAL_AXIS_L_TRIGGER].axis_info.value;
    state.slider[1] = axes[JOY_XINPUT_REAL_AXIS_R_TRIGGER].axis_info.value;

    // Virtual keys

    bool is_l3_centered = true;
    bool is_r3_centered = true;

    if (state.x > SHRT_MAX * VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
    {
      is_l3_centered = false;
      state.buttons.set(JOY_XINPUT_REAL_BTN_L_THUMB_RIGHT);
    }
    else if (state.x < SHRT_MAX * -VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
    {
      is_l3_centered = false;
      state.buttons.set(JOY_XINPUT_REAL_BTN_L_THUMB_LEFT);
    }

    if (state.y > SHRT_MAX * VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
    {
      is_l3_centered = false;
      state.buttons.set(JOY_XINPUT_REAL_BTN_L_THUMB_UP);
    }
    else if (state.y < SHRT_MAX * -VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
    {
      is_l3_centered = false;
      state.buttons.set(JOY_XINPUT_REAL_BTN_L_THUMB_DOWN);
    }

    if (state.rx > SHRT_MAX * VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
    {
      is_r3_centered = false;
      state.buttons.set(JOY_XINPUT_REAL_BTN_R_THUMB_RIGHT);
    }
    else if (state.rx < SHRT_MAX * -VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
    {
      is_r3_centered = false;
      state.buttons.set(JOY_XINPUT_REAL_BTN_R_THUMB_LEFT);
    }

    if (state.ry > SHRT_MAX * VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
    {
      is_r3_centered = false;
      state.buttons.set(JOY_XINPUT_REAL_BTN_R_THUMB_UP);
    }
    else if (state.ry < SHRT_MAX * -VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
    {
      is_r3_centered = false;
      state.buttons.set(JOY_XINPUT_REAL_BTN_R_THUMB_DOWN);
    }

    if (state.slider[0] > SHRT_MAX * VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
      state.buttons.set(JOY_XINPUT_REAL_BTN_L_TRIGGER);
    if (state.slider[1] > SHRT_MAX * VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
      state.buttons.set(JOY_XINPUT_REAL_BTN_R_TRIGGER);

    if (state.buttons.get(JOY_XINPUT_REAL_BTN_L_THUMB) && ((is_l3_centered && !state.buttonsPrev.get(JOY_XINPUT_REAL_BTN_L_THUMB)) ||
                                                            (state.buttonsPrev.getWord0() & JOY_XINPUT_REAL_BTN_L_THUMB_CENTER)))
    {
      state.buttons.clr(JOY_XINPUT_REAL_BTN_L_THUMB);
      state.buttons.set(JOY_XINPUT_REAL_BTN_L_THUMB_CENTER);
    }

    if (state.buttons.get(JOY_XINPUT_REAL_BTN_R_THUMB) && ((is_r3_centered && !state.buttonsPrev.get(JOY_XINPUT_REAL_BTN_R_THUMB)) ||
                                                            (state.buttonsPrev.getWord0() & JOY_XINPUT_REAL_BTN_R_THUMB_CENTER)))
    {
      state.buttons.clr(JOY_XINPUT_REAL_BTN_R_THUMB);
      state.buttons.set(JOY_XINPUT_REAL_BTN_R_THUMB_CENTER);
    }

    if (padX > 0)
      state.buttons.set(JOY_XINPUT_REAL_BTN_D_RIGHT);
    else if (padX < 0)
      state.buttons.set(JOY_XINPUT_REAL_BTN_D_LEFT);

    if (padY > 0)
      state.buttons.set(JOY_XINPUT_REAL_BTN_D_UP);
    else if (padY < 0)
      state.buttons.set(JOY_XINPUT_REAL_BTN_D_DOWN);
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

  if (!changed)
    changed = memcmp(prev_state, &state, DATA_SZ) != 0;

  if (def)
  {
    if (changed)
      raw_state_joy = state;
    else
      memcpy(raw_state_joy.bDownTms, state.bDownTms, sizeof(raw_state_joy.bDownTms));
  }

  if (client && changed)
    client->stateChanged(this, 0);

  return changed && !state.buttons.cmpEq(state.buttonsPrev);
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
    axis.min_val = min_val;
    axis.max_val = max_val;
  }
}

bool HidJoystickDevice::getButtonState(int btn_id) const { return btn_id >= 0 ? state.buttons.get(btn_id) : false; }

float HidJoystickDevice::getAxisPos(int axis_id) const
{
  const int ac = axes.size();
  if (axis_id >= 0 && axis_id < ac)
  {
    AxisData const &axis = axes[axis_id];
    return cvt(axis.axis_info.value, JOY_XINPUT_MIN_AXIS_VAL, JOY_XINPUT_MAX_AXIS_VAL, axis.min_val, axis.max_val);
  }
  if (axis_id >= ac && axis_id < ac + 2 * povHats.size())
    return getVirtualPOVAxis((axis_id - ac) / 2, (axis_id - ac) % 2);
  return 0;
}

int HidJoystickDevice::getAxisPosRaw(int axis_id) const
{
  const int ac = axes.size();
  if (axis_id >= 0 && axis_id < ac)
    return axes[axis_id].axis_info.value;
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
