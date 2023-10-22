// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef _GAIJIN_DRV_HID_GAMEPAD_XBOX_GAMEPAD_DEVICE_H
#define _GAIJIN_DRV_HID_GAMEPAD_XBOX_GAMEPAD_DEVICE_H
#pragma once

#include <util/dag_globDef.h>
#include <humanInput/dag_hiJoystick.h>
#include <humanInput/dag_hiDeclDInput.h>
#include <humanInput/dag_hiXInputMappings.h>
#include <humanInput/dag_hiGlobals.h>
#include <ioSys/dag_dataBlock.h>
#include <string.h>

namespace HumanInput
{
struct JoyCookedCreateParams;

//! thresholds to clamp axis deadzone [0=gamepad, 1=flightstick][0=left, 1=right, 2=trigger]
extern const int axis_dead_thres[2][3];

class Xbox360GamepadDevice : public IGenJoystick
{
  enum
  {
    BUTTONS_NUM = JOY_XINPUT_REAL_BTN_COUNT,
    AXES_NUM = JOY_XINPUT_REAL_AXIS_COUNT
  };

public:
  Xbox360GamepadDevice(int gamepad_no, const char *_name, int ord_id, bool is_virtual = false);
  ~Xbox360GamepadDevice();

  bool updateState(int dt_msec, bool def, bool has_def);

  // IGenJoystick interface implementation
  virtual const char *getName() const { return name; }
  virtual const char *getDeviceID() const { return "x360"; }
  virtual bool getDeviceDesc(DataBlock &out_desc) const
  {
    out_desc.clearData();
    out_desc.setBool("xinputDev", true);
    return true;
  }
  virtual unsigned short getUserId() const { return userId; }
  virtual const JoystickRawState &getRawState() const { return state; }
  virtual void setRawState(const JoystickRawState &other) { state = other; }
  virtual void setClient(IGenJoystickClient *cli)
  {
    if (cli == client)
      return;

    if (client)
      client->detached(this);
    client = cli;
    if (client)
      client->attached(this);
  }

  virtual IGenJoystickClient *getClient() const { return client; }

  virtual int getButtonCount() const { return stg_joy.disableVirtualControls ? JOY_XINPUT_REAL_BTN_R_TRIGGER + 1 : BUTTONS_NUM; }
  virtual const char *getButtonName(int idx) const
  {
    if (idx < 0 || idx >= HumanInput::Xbox360GamepadDevice::BUTTONS_NUM)
      return NULL;
    return joyXInputButtonName[idx];
  }

  virtual int getAxisCount() const { return stg_joy.disableVirtualControls ? JOY_XINPUT_REAL_AXIS_R_TRIGGER + 1 : AXES_NUM; }
  virtual const char *getAxisName(int idx) const { return joyXInputAxisName[idx]; }

  virtual int getPovHatCount() const { return 0; }
  virtual const char *getPovHatName(int idx) const { return NULL; }

  virtual int getButtonId(const char *button_name) const
  {
    for (int i = 0; i < BUTTONS_NUM; i++)
      if (strcmp(button_name, joyXInputButtonName[i]) == 0)
        return i;
    return -1;
  }

  virtual int getAxisId(const char *button_name) const
  {
    for (int i = 0; i < AXES_NUM; i++)
      if (strcmp(button_name, joyXInputAxisName[i]) == 0)
        return i;
    return -1;
  }

  virtual int getPovHatId(const char *name) const { return -1; }

  virtual void setAxisLimits(int axis_id, float min_val, float max_val)
  {
    G_ASSERT(axis_id >= 0 && axis_id < AXES_NUM);
    if (axis_id < 4 || axis_id == 6)
    {
      // thumbs
      axis[axis_id].kMul = (max_val - min_val) / float(JOY_XINPUT_MAX_AXIS_VAL - JOY_XINPUT_MIN_AXIS_VAL);
      axis[axis_id].kAdd = min_val - JOY_XINPUT_MIN_AXIS_VAL * axis[axis_id].kMul;
    }
    else
    {
      // triggers
      axis[axis_id].kMul = (max_val - min_val) / float(JOY_XINPUT_MAX_AXIS_VAL);
      axis[axis_id].kAdd = min_val;
    }
  }

  virtual bool getButtonState(int btn_id) const
  {
    G_ASSERT(btn_id >= 0 && btn_id < BUTTONS_NUM);
    return state.buttons.get(btn_id);
  }

  virtual float getAxisPos(int axis_id) const
  {
    G_ASSERT(axis_id >= 0 && axis_id < AXES_NUM);
    return axis[axis_id].val[0] * axis[axis_id].kMul + axis[axis_id].kAdd;
  }

  virtual int getAxisPosRaw(int axis_id) const
  {
    G_ASSERT(axis_id >= 0 && axis_id < AXES_NUM);
    return axis[axis_id].val[0];
  }

  virtual int getPovHatAngle(int axis_id) const { return -1; }

  virtual void doRumble(float lowFreq, float highFreq);

  virtual bool isConnected();

  virtual bool isStickDeadZoneSetupSupported() const { return true; }
  virtual float getStickDeadZoneScale(int stick_idx) const override
  {
    G_ASSERT_RETURN(stick_idx >= 0 && stick_idx < 2, 0);
    return stickDeadZoneScale[stick_idx];
  }
  virtual void setStickDeadZoneScale(int stick_idx, float scale) override
  {
    G_ASSERT_RETURN(stick_idx >= 0 && stick_idx < 2, );
    if (stickDeadZoneScale[stick_idx] != scale)
      xStPktId = 0xFFFFFFFF;
    stickDeadZoneScale[stick_idx] = scale;
  }
  virtual float getStickDeadZoneAbs(int stick_idx) const override
  {
    G_ASSERT_RETURN(stick_idx >= 0 && stick_idx < 2, 0);
    return stickDeadZoneScale[stick_idx] * axis_dead_thres[deviceType >= 0 ? deviceType : 0][stick_idx] / JOY_XINPUT_MAX_AXIS_VAL;
  }

  int getPairedButtonForAxis(int axis_idx) const override
  {
    if (axis_idx == JOY_XINPUT_REAL_AXIS_L_TRIGGER)
      return JOY_XINPUT_REAL_BTN_L_TRIGGER;
    if (axis_idx == JOY_XINPUT_REAL_AXIS_R_TRIGGER)
      return JOY_XINPUT_REAL_BTN_R_TRIGGER;
    return -1;
  }
  int getPairedAxisForButton(int btn_idx) const override
  {
    if (btn_idx == JOY_XINPUT_REAL_BTN_L_TRIGGER)
      return JOY_XINPUT_REAL_AXIS_L_TRIGGER;
    if (btn_idx == JOY_XINPUT_REAL_BTN_R_TRIGGER)
      return JOY_XINPUT_REAL_AXIS_R_TRIGGER;
    return -1;
  }

  int getJointAxisForAxis(int axis_idx) const override
  {
    switch (axis_idx)
    {
      case JOY_XINPUT_REAL_AXIS_L_THUMB_H: return JOY_XINPUT_REAL_AXIS_L_THUMB_V;
      case JOY_XINPUT_REAL_AXIS_L_THUMB_V: return JOY_XINPUT_REAL_AXIS_L_THUMB_H;
      case JOY_XINPUT_REAL_AXIS_R_THUMB_H: return JOY_XINPUT_REAL_AXIS_R_THUMB_V;
      case JOY_XINPUT_REAL_AXIS_R_THUMB_V: return JOY_XINPUT_REAL_AXIS_R_THUMB_H;
    }
    return -1;
  }

  void copyStickDeadZoneScales(Xbox360GamepadDevice &dev)
  {
    for (int s = 0; s < 2; s++)
      stickDeadZoneScale[s] = dev.stickDeadZoneScale[s];
  }

protected:
  bool isVirtual;
  unsigned short ordId, userId;
  unsigned xStPktId;
  IGenJoystickClient *client;
  JoystickRawState state;
  int deviceType;
  float stickDeadZoneScale[2] = {1, 1};

  const char *name;
  struct LogicalAxis
  {
    int *val;
    float kMul, kAdd;
  };
  LogicalAxis axis[AXES_NUM];

  unsigned short rumbleLeftMotor = (unsigned short)-1;
  unsigned short rumbleRightMotor = (unsigned short)-1;
};
} // namespace HumanInput

#endif
