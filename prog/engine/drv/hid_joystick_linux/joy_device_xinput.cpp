// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "joy_device_xinput.h"
#include <generic/dag_carray.h>
#include <drv/hid/dag_hiXInputMappings.h>
#include <math/dag_mathUtils.h>

namespace HumanInput
{

namespace XinputMapping
{

// source msdn
// https://learn.microsoft.com/en-us/windows/win32/xinput/getting-started-with-xinput#dead-zone
#define MS_GAMEPAD_LEFT_THUMB_DEADZONE  7849
#define MS_GAMEPAD_RIGHT_THUMB_DEADZONE 8689

#define STEAM_GAMEPAD_LEFT_THUMB_DEADZONE  3000
#define STEAM_GAMEPAD_RIGHT_THUMB_DEADZONE 2000

#define DEFAULT_GAMEPAD_LEFT_THUMB_DEADZONE  2000
#define DEFAULT_GAMEPAD_RIGHT_THUMB_DEADZONE 2000

#define MS_VENDOR_ID    0x045e
#define STEAM_VENDOR_ID 0x28de

/*
Compatible controllers have two analog directional sticks, each with a digital button, two analog triggers, a digital directional pad
with four directions, and eight digital buttons. The states of each of these inputs are returned in the XINPUT_GAMEPAD structure when
the XInputGetState function is called.
*/
const unsigned required_buttons[] = {
  JOY_XINPUT_REAL_BTN_A,          //
  JOY_XINPUT_REAL_BTN_B,          //
  JOY_XINPUT_REAL_BTN_X,          //
  JOY_XINPUT_REAL_BTN_Y,          //
  JOY_XINPUT_REAL_BTN_START,      //
  JOY_XINPUT_REAL_BTN_BACK,       //
  JOY_XINPUT_REAL_BTN_L_SHOULDER, //
  JOY_XINPUT_REAL_BTN_R_SHOULDER, //
  JOY_XINPUT_REAL_BTN_L_THUMB,    //
  JOY_XINPUT_REAL_BTN_R_THUMB,    //
};

static const unsigned required_axes[] = {
  JOY_XINPUT_REAL_AXIS_L_THUMB_H, //
  JOY_XINPUT_REAL_AXIS_L_THUMB_V, //
  JOY_XINPUT_REAL_AXIS_R_THUMB_H, //
  JOY_XINPUT_REAL_AXIS_R_THUMB_V  //
};

static unsigned map_button(unsigned xinput_btn)
{
  switch (xinput_btn)
  {
    case JOY_XINPUT_REAL_BTN_A: return BTN_A;
    case JOY_XINPUT_REAL_BTN_B: return BTN_B;
    case JOY_XINPUT_REAL_BTN_X: return BTN_X;
    case JOY_XINPUT_REAL_BTN_Y: return BTN_Y;
    case JOY_XINPUT_REAL_BTN_START: return BTN_START;
    case JOY_XINPUT_REAL_BTN_BACK: return BTN_SELECT;
    case JOY_XINPUT_REAL_BTN_L_THUMB: return BTN_THUMBL;
    case JOY_XINPUT_REAL_BTN_R_THUMB: return BTN_THUMBR;
    case JOY_XINPUT_REAL_BTN_L_SHOULDER: return BTN_TL;
    case JOY_XINPUT_REAL_BTN_R_SHOULDER: return BTN_TR;
    case JOY_XINPUT_REAL_BTN_L_TRIGGER: return BTN_TL2;
    case JOY_XINPUT_REAL_BTN_R_TRIGGER: return BTN_TR2;
  }
  return KEY_MAX;
}

#define AXIS_KEY_NUM 2
struct AxisKey
{
  int code[AXIS_KEY_NUM];
};

static AxisKey map_axis(unsigned xinput_axis)
{
  switch (xinput_axis)
  {
    case JOY_XINPUT_REAL_AXIS_L_THUMB_H: return {ABS_X, ABS_X};
    case JOY_XINPUT_REAL_AXIS_L_THUMB_V: return {ABS_Y, ABS_Y};
    case JOY_XINPUT_REAL_AXIS_R_THUMB_H: return {ABS_RX, ABS_RX};
    case JOY_XINPUT_REAL_AXIS_R_THUMB_V: return {ABS_RY, ABS_RY};
    case JOY_XINPUT_REAL_AXIS_L_TRIGGER: return {ABS_Z, ABS_BRAKE};
    case JOY_XINPUT_REAL_AXIS_R_TRIGGER: return {ABS_RZ, ABS_GAS};
  }
  return {ABS_MAX, ABS_MAX};
}

static int virtual_button_map(unsigned xinput_button)
{
  switch (xinput_button)
  {
    case JOY_XINPUT_REAL_BTN_L_THUMB_RIGHT: return JOY_XINPUT_REAL_AXIS_L_THUMB_H;
    case JOY_XINPUT_REAL_BTN_L_THUMB_LEFT: return JOY_XINPUT_REAL_AXIS_L_THUMB_H;
    case JOY_XINPUT_REAL_BTN_L_THUMB_UP: return JOY_XINPUT_REAL_AXIS_L_THUMB_V;
    case JOY_XINPUT_REAL_BTN_L_THUMB_DOWN: return JOY_XINPUT_REAL_AXIS_L_THUMB_V;
    case JOY_XINPUT_REAL_BTN_R_THUMB_RIGHT: return JOY_XINPUT_REAL_AXIS_R_THUMB_H;
    case JOY_XINPUT_REAL_BTN_R_THUMB_LEFT: return JOY_XINPUT_REAL_AXIS_R_THUMB_H;
    case JOY_XINPUT_REAL_BTN_R_THUMB_UP: return JOY_XINPUT_REAL_AXIS_R_THUMB_V;
    case JOY_XINPUT_REAL_BTN_R_THUMB_DOWN: return JOY_XINPUT_REAL_AXIS_R_THUMB_V;
    case JOY_XINPUT_REAL_BTN_L_TRIGGER: return JOY_XINPUT_REAL_AXIS_L_TRIGGER;
    case JOY_XINPUT_REAL_BTN_R_TRIGGER: return JOY_XINPUT_REAL_AXIS_R_TRIGGER;
  }
  return -1;
}

const unsigned virtual_buttons[] = {
  JOY_XINPUT_REAL_BTN_L_THUMB_RIGHT, //
  JOY_XINPUT_REAL_BTN_L_THUMB_LEFT,  //
  JOY_XINPUT_REAL_BTN_L_THUMB_UP,    //
  JOY_XINPUT_REAL_BTN_L_THUMB_DOWN,  //
  JOY_XINPUT_REAL_BTN_R_THUMB_RIGHT, //
  JOY_XINPUT_REAL_BTN_R_THUMB_LEFT,  //
  JOY_XINPUT_REAL_BTN_R_THUMB_UP,    //
  JOY_XINPUT_REAL_BTN_R_THUMB_DOWN,  //
  JOY_XINPUT_REAL_BTN_L_TRIGGER,     //
  JOY_XINPUT_REAL_BTN_R_TRIGGER      //
};

static int virtual_button_axis_sign(unsigned xinput_button)
{
  switch (xinput_button)
  {
    case JOY_XINPUT_REAL_BTN_L_THUMB_RIGHT: return 1;
    case JOY_XINPUT_REAL_BTN_L_THUMB_LEFT: return -1;
    case JOY_XINPUT_REAL_BTN_L_THUMB_UP: return 1;
    case JOY_XINPUT_REAL_BTN_L_THUMB_DOWN: return -1;
    case JOY_XINPUT_REAL_BTN_R_THUMB_RIGHT: return 1;
    case JOY_XINPUT_REAL_BTN_R_THUMB_LEFT: return -1;
    case JOY_XINPUT_REAL_BTN_R_THUMB_UP: return 1;
    case JOY_XINPUT_REAL_BTN_R_THUMB_DOWN: return -1;
    case JOY_XINPUT_REAL_BTN_L_TRIGGER: return 1;
    case JOY_XINPUT_REAL_BTN_R_TRIGGER: return 1;
  }
  return 0;
}

}; // namespace XinputMapping

static const float VIRTUAL_THUMBKEY_PRESS_THRESHOLD = 0.5f;

bool HidJoystickDeviceXInput::device_xinput_compatable(HidJoystickDevice *device)
{
  dag::ConstSpan<AxisData> axes = device->getAxesInfo();
  for (unsigned axis : XinputMapping::required_axes)
  {
    bool found = false;
    for (int sysAxisId : XinputMapping::map_axis(axis).code)
    {
      for (int i = 0; i < axes.size(); ++i)
      {
        if (axes[i].sysAxisId == sysAxisId)
        {
          found = true;
          break;
        }
      }
      if (found)
        break;
    }
    if (!found)
    {
      JOY_TRACE("axis %s not found", joyXInputAxisName[axis]);
      return false;
    }
  }

  dag::ConstSpan<ButtonData> buttons = device->getButtonsInfo();
  for (unsigned button : XinputMapping::required_buttons)
  {
    unsigned sysBtnId = XinputMapping::map_button(button);
    bool found = false;
    for (const ButtonData &btn : buttons)
    {
      if (btn.sysBtnId == sysBtnId)
      {
        found = true;
        break;
      }
    }
    if (!found)
    {
      JOY_TRACE("button %d %x not found", button, sysBtnId);
      return false;
    }
  }
  return device->getPovHatCount() > 0;
}


HidJoystickDeviceXInput::HidJoystickDeviceXInput(eastl::unique_ptr<HidJoystickDevice> device) : joydev(eastl::move(device))
{
  JOY_LOG("use device <%s> as xinput device", joydev->getName());
  input_id id = joydev->getInputId();
  if (id.vendor == MS_VENDOR_ID)
  {
    JOY_LOG("device recognized as xbox gamepad");
    lThumbDeadZone = MS_GAMEPAD_LEFT_THUMB_DEADZONE;
    rThumbDeadZone = MS_GAMEPAD_RIGHT_THUMB_DEADZONE;
  }
  else if (id.vendor == STEAM_VENDOR_ID)
  {
    lThumbDeadZone = STEAM_GAMEPAD_LEFT_THUMB_DEADZONE;
    rThumbDeadZone = STEAM_GAMEPAD_RIGHT_THUMB_DEADZONE;
  }
  else
  {
    lThumbDeadZone = DEFAULT_GAMEPAD_LEFT_THUMB_DEADZONE;
    rThumbDeadZone = DEFAULT_GAMEPAD_RIGHT_THUMB_DEADZONE;
  }

  dag::ConstSpan<AxisData> origAxes = joydev->getAxesInfo();
  axes.resize(JOY_XINPUT_REAL_AXIS_COUNT);
  for (int axisNo = 0; axisNo < axes.size(); ++axisNo)
  {
    AxisData &a = axes[axisNo];
    a.prevVal = 0;
    a.minVal = JOY_XINPUT_MIN_AXIS_VAL;
    a.maxVal = JOY_XINPUT_MAX_AXIS_VAL;
    a.name = joyXInputAxisName[axisNo];
    memset(&a.sysAbsInfo, 0, sizeof(a.sysAbsInfo));

    a.remapIdx = -1;
    a.sysAxisId = ABS_MAX;
    a.sysAbsInfo.minimum = JOY_XINPUT_MIN_AXIS_VAL;
    a.sysAbsInfo.maximum = JOY_XINPUT_MAX_AXIS_VAL;
    if (axisNo == JOY_XINPUT_REAL_AXIS_L_TRIGGER || axisNo == JOY_XINPUT_REAL_AXIS_R_TRIGGER)
      a.sysAbsInfo.minimum = 0;

    if (axisNo == JOY_XINPUT_REAL_AXIS_L_THUMB_V || axisNo == JOY_XINPUT_REAL_AXIS_R_THUMB_V)
      a.inverted = true;

    for (int sysAxisId : XinputMapping::map_axis(axisNo).code)
    {
      for (int i = 0; i < origAxes.size(); ++i)
      {
        if (origAxes[i].sysAxisId == sysAxisId)
        {
          a.sysAxisId = sysAxisId;
          a.remapIdx = i;
          break;
        }
      }
      if (a.remapIdx != -1)
        break;
    }
    if (a.remapIdx != -1)
      JOY_LOG("[%s] map axis %s to %s", getName(), origAxes[a.remapIdx].name, a.name);
    else
      JOY_LOG("[%s] axis %s not mapped", getName(), a.name);
  }

  dag::ConstSpan<ButtonData> origButtons = joydev->getButtonsInfo();
  buttons.resize(JOY_XINPUT_REAL_BTN_COUNT);
  for (int btnNo = 0; btnNo < buttons.size(); ++btnNo)
  {
    ButtonData &b = buttons[btnNo];
    int sysBtnId = XinputMapping::map_button(btnNo);
    b.sysBtnId = KEY_MAX;
    b.name = joyXInputButtonName[btnNo];
    for (int i = 0; i < origButtons.size(); ++i)
    {
      if (origButtons[i].sysBtnId == sysBtnId)
      {
        b.sysBtnId = sysBtnId;
        JOY_LOG("[%s] map button %s to %s", getName(), origButtons[i].name, b.name);
        break;
      }
    }
    if (b.sysBtnId == KEY_MAX)
    {
      int baxis = XinputMapping::virtual_button_map(btnNo);
      if (baxis != -1 && axes[baxis].remapIdx != -1)
        JOY_LOG("[%s] map button %s to axis %s", getName(), b.name, axes[baxis].name);
      else
        JOY_LOG("[%s] button %s not mapped", getName(), b.name);
    }
  }

  memset(&state, 0, sizeof(state));

  updateSelfState();
}

bool HidJoystickDeviceXInput::updateState(int dt_msec, bool def)
{
  if (!joydev->updateState(dt_msec, def))
    return false;
  return updateSelfState();
}


void HidJoystickDeviceXInput::applyCircularDeadZone(int axis_x, int axis_y, int deadzone)
{
  float lx = axes[axis_x].sysAbsInfo.value;
  float ly = axes[axis_y].sysAbsInfo.value;

  float mag = fastsqrt(lx * lx + ly * ly);
  if (mag < REAL_EPS)
    return;

  if (mag > deadzone)
  {
    float normLX = safediv(lx, mag);
    float normLY = safediv(ly, mag);
    mag = (mag - deadzone) / (JOY_XINPUT_MAX_AXIS_VAL - deadzone) * JOY_XINPUT_MAX_AXIS_VAL;
    lx = normLX * mag;
    ly = normLY * mag;
  }
  else
  {
    lx = 0;
    ly = 0;
  }

  axes[axis_x].sysAbsInfo.value = clamp(int(lx), JOY_XINPUT_MIN_AXIS_VAL, JOY_XINPUT_MAX_AXIS_VAL);
  axes[axis_y].sysAbsInfo.value = clamp(int(ly), JOY_XINPUT_MIN_AXIS_VAL, JOY_XINPUT_MAX_AXIS_VAL);
}

bool HidJoystickDeviceXInput::updateSelfState()
{
  dag::ConstSpan<AxisData> origAxes = joydev->getAxesInfo();
  for (int i = 0; i < axes.size(); ++i)
  {
    AxisData &axis = axes[i];
    if (axis.remapIdx == -1)
      continue;

    const AxisData &origAxis = origAxes[axis.remapIdx];
    axis.sysAbsInfo.value = cvt(origAxis.sysAbsInfo.value, origAxis.sysAbsInfo.minimum, origAxis.sysAbsInfo.maximum,
      axis.sysAbsInfo.minimum, axis.sysAbsInfo.maximum);

    if (axis.inverted)
      axis.sysAbsInfo.value *= -1;
  }

  applyCircularDeadZone(JOY_XINPUT_REAL_AXIS_L_THUMB_H, JOY_XINPUT_REAL_AXIS_L_THUMB_V, lThumbDeadZone);
  applyCircularDeadZone(JOY_XINPUT_REAL_AXIS_R_THUMB_H, JOY_XINPUT_REAL_AXIS_R_THUMB_V, rThumbDeadZone);

  for (int i = 0; i < axes.size(); ++i)
  {
    AxisData &axis = axes[i];
    if (axis.remapIdx == -1)
      continue;
    if (axis.prevVal != axis.sysAbsInfo.value)
    {
      JOY_TRACE("[%s] axis %s: %d", getName(), axis.name, axis.sysAbsInfo.value);
      axis.prevVal = axis.sysAbsInfo.value;
    }
  }

  int16_t padX0 = 0, padY0 = 0;

  JoystickRawState const &origState = joydev->getRawState();
  dag::ConstSpan<PovHatData> origPovHats = joydev->getPovHatsInfo();
  for (int i = 0; i < origPovHats.size(); ++i)
  {
    int padX = origPovHats[i].padX;
    int padY = origPovHats[i].padY;
    int angle = povhat_pads_to_angle(padX, padY);
    state.povValuesPrev[i] = state.povValues[i];
    state.povValues[i] = angle;
    if (i == 0)
    {
      padX0 = padX;
      padY0 = padY;
    }
  }

  state.buttonsPrev = state.buttons;
  state.buttons.reset();
  for (int i = 0; i < buttons.size(); ++i)
  {
    ButtonData &b = buttons[i];
    if (b.sysBtnId == KEY_MAX)
      continue;

    if (joydev->isBtnPressed(b.sysBtnId))
    {
      if (!state.buttonsPrev.get(i))
      {
        JOY_TRACE("[%s] xbutton pressed %s", getName(), b.name);
      }
      state.buttons.set(i);
    }
  }

  state.x = axes[JOY_XINPUT_REAL_AXIS_L_THUMB_H].sysAbsInfo.value;
  state.y = axes[JOY_XINPUT_REAL_AXIS_L_THUMB_V].sysAbsInfo.value;

  state.rx = axes[JOY_XINPUT_REAL_AXIS_R_THUMB_H].sysAbsInfo.value;
  state.ry = axes[JOY_XINPUT_REAL_AXIS_R_THUMB_V].sysAbsInfo.value;

  state.slider[0] = axes[JOY_XINPUT_REAL_AXIS_L_TRIGGER].sysAbsInfo.value;
  state.slider[1] = axes[JOY_XINPUT_REAL_AXIS_R_TRIGGER].sysAbsInfo.value;


  for (unsigned btn : XinputMapping::virtual_buttons)
  {
    int axisId = XinputMapping::virtual_button_map(btn);
    if (axisId == -1)
      continue;
    const AxisData &a = axes[axisId];
    if (a.remapIdx == -1) // axis not attached
      continue;
    int sign = XinputMapping::virtual_button_axis_sign(btn);
    if (sign > 0)
    {
      if (a.sysAbsInfo.value > VIRTUAL_THUMBKEY_PRESS_THRESHOLD * a.sysAbsInfo.maximum)
      {
        state.buttons.set(btn);
        JOY_TRACE("[%s] virtual button pressed %s", getName(), buttons[btn].name);
      }
    }
    else if (sign < 0)
    {
      if (a.sysAbsInfo.value < VIRTUAL_THUMBKEY_PRESS_THRESHOLD * a.sysAbsInfo.minimum)
      {
        state.buttons.set(btn);
        JOY_TRACE("[%s] virtual button pressed %s", getName(), buttons[btn].name);
      }
    }
  }

  // Virtual keys
  if (padX0 > 0)
    state.buttons.set(JOY_XINPUT_REAL_BTN_D_RIGHT);
  else if (padX0 < 0)
    state.buttons.set(JOY_XINPUT_REAL_BTN_D_LEFT);
  if (padY0 > 0)
    state.buttons.set(JOY_XINPUT_REAL_BTN_D_UP);
  else if (padY0 < 0)
    state.buttons.set(JOY_XINPUT_REAL_BTN_D_DOWN);

  state.repeat = origState.repeat;
  memcpy(state.bDownTms, origState.bDownTms, sizeof(origState.bDownTms));

  if (client)
    client->stateChanged(this, 0);
  return true;
}

void HidJoystickDeviceXInput::setClient(IGenJoystickClient *cli)
{
  if (cli == client)
    return;

  if (client)
    client->detached(this);
  client = cli;
  if (client)
    client->attached(this);
}

int HidJoystickDeviceXInput::getButtonCount() const { return buttons.size(); }

const char *HidJoystickDeviceXInput::getButtonName(int idx) const
{
  if (idx >= 0 && idx < buttons.size())
    return buttons[idx].name;
  return NULL;
}

int HidJoystickDeviceXInput::getAxisCount() const { return axes.size() + 2 * joydev->getPovHatCount(); }

const char *HidJoystickDeviceXInput::getAxisName(int idx) const
{
  const int ac = axes.size();
  if (idx >= 0 && idx < ac)
    return axes[idx].name;
  dag::ConstSpan<PovHatData> povHats = joydev->getPovHatsInfo();
  if (idx >= ac && idx < ac + 2 * povHats.size())
    return povHats[(idx - ac) / 2].axisName[(idx - ac) % 2];
  return NULL;
}

int HidJoystickDeviceXInput::getButtonId(const char *name) const
{
  for (int i = 0; i < buttons.size(); i++)
    if (strcmp(buttons[i].name, name) == 0)
      return i;
  return -1;
}

int HidJoystickDeviceXInput::getAxisId(const char *name) const
{
  for (int i = 0; i < axes.size(); i++)
    if (strcmp(axes[i].name, name) == 0)
      return i;
  dag::ConstSpan<PovHatData> povHats = joydev->getPovHatsInfo();
  for (int i = 0; i < povHats.size(); i++)
    for (int j = 0; j < 2; j++)
      if (strcmp(povHats[i].axisName[j], name) == 0)
        return axes.size() + i * 2 + j;
  return -1;
}

void HidJoystickDeviceXInput::setAxisLimits(int axis_id, float min_val, float max_val)
{
  const int ac = axes.size();
  if (axis_id >= 0 && axis_id < ac)
  {
    AxisData &axis = axes[axis_id];
    axis.minVal = min_val;
    axis.maxVal = max_val;
  }
}

float HidJoystickDeviceXInput::getAxisPos(int axis_id) const
{
  const int ac = axes.size();
  if (axis_id >= 0 && axis_id < ac)
  {
    AxisData const &axis = axes[axis_id];
    return cvt(axis.sysAbsInfo.value, axis.sysAbsInfo.minimum, axis.sysAbsInfo.maximum, axis.minVal, axis.maxVal);
  }
  dag::ConstSpan<PovHatData> povHats = joydev->getPovHatsInfo();
  if (axis_id >= ac && axis_id < ac + 2 * povHats.size())
    return getVirtualPOVAxis((axis_id - ac) / 2, (axis_id - ac) % 2);
  return 0;
}

int HidJoystickDeviceXInput::getAxisPosRaw(int axis_id) const
{
  const int ac = axes.size();
  if (axis_id >= 0 && axis_id < ac)
    return axes[axis_id].sysAbsInfo.value;
  dag::ConstSpan<PovHatData> povHats = joydev->getPovHatsInfo();
  if (axis_id >= ac && axis_id < ac + 2 * povHats.size())
    return getVirtualPOVAxis((axis_id - ac) / 2, (axis_id - ac) % 2);
  return 0;
}

int HidJoystickDeviceXInput::getPovHatAngle(int hat_id) const
{
  if (hat_id >= 0 && hat_id < getPovHatCount())
    return state.povValues[hat_id];
  return -1;
}

int HidJoystickDeviceXInput::getVirtualPOVAxis(int hat_id, int axis_id) const
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

bool HidJoystickDeviceXInput::getButtonState(int btn_id) const { return btn_id >= 0 ? state.buttons.get(btn_id) : false; }

} // namespace HumanInput
