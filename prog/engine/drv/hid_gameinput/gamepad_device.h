// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_globDef.h>
#include <drv/hid/dag_hiJoystick.h>
#include <drv/hid/dag_hiDeclDInput.h>
#include <drv/hid/dag_hiXInputMappings.h>
#include <drv/hid/dag_hiGlobals.h>
#include <ioSys/dag_dataBlock.h>
#include <string.h>

struct GamepadReading
{
  unsigned short buttons;
  unsigned char leftTrigger;
  unsigned char rightTrigger;
  short leftThumbX;
  short leftThumbY;
  short rightThumbX;
  short rightThumbY;
};
void refresh_gamepads();
bool is_gamepad_connected(int slot);
bool read_gamepad(int slot, GamepadReading &out);
void set_gamepad_rumble(int slot, float low_freq, float high_freq);

namespace HumanInput
{
struct JoyCookedCreateParams;

extern IGenHidClassDrv *mouse_emu;

//! thresholds to clamp axis deadzone [0=gamepad, 1=flightstick][0=left, 1=right, 2=trigger]
extern const int axis_dead_thres[2][3];

class GameInputGamepadDevice final : public IGenJoystick
{
  static constexpr int BUTTONS_NUM = JOY_XINPUT_REAL_BTN_COUNT;
  static constexpr int AXES_NUM = JOY_XINPUT_REAL_AXIS_COUNT;

public:
  GameInputGamepadDevice(int gamepad_no, const char *_name, int ord_id, bool is_virtual = false);
  ~GameInputGamepadDevice();

  bool updateState(int dt_msec, bool def, bool has_def);

  // IGenJoystick interface implementation
  const char *getName() const override { return name; }
  const char *getDeviceID() const override { return "x360"; }
  bool getDeviceDesc(DataBlock &out_desc) const override
  {
    out_desc.clearData();
    out_desc.setBool("xinputDev", true);
    return true;
  }
  unsigned short getUserId() const override { return userId; }
  const JoystickRawState &getRawState() const override { return state; }
  void setRawState(const JoystickRawState &other) { state = other; }
  void setClient(IGenJoystickClient *cli) override
  {
    if (cli == client)
      return;

    if (client)
      client->detached(this);
    client = cli;
    if (client)
      client->attached(this);
  }

  IGenJoystickClient *getClient() const override { return client; }

  int getButtonCount() const override { return stg_joy.disableVirtualControls ? JOY_XINPUT_REAL_BTN_R_TRIGGER + 1 : BUTTONS_NUM; }
  const char *getButtonName(int idx) const override
  {
    if (idx < 0 || idx >= BUTTONS_NUM)
      return nullptr;
    return joyXInputButtonName[idx];
  }

  int getAxisCount() const override { return stg_joy.disableVirtualControls ? JOY_XINPUT_REAL_AXIS_R_TRIGGER + 1 : AXES_NUM; }
  const char *getAxisName(int idx) const override { return joyXInputAxisName[idx]; }

  int getPovHatCount() const override { return 0; }
  const char *getPovHatName(int idx) const override { return nullptr; }

  int getButtonId(const char *button_name) const override
  {
    for (int i = 0; i < BUTTONS_NUM; i++)
      if (strcmp(button_name, joyXInputButtonName[i]) == 0)
        return i;
    return -1;
  }

  int getAxisId(const char *button_name) const override
  {
    for (int i = 0; i < AXES_NUM; i++)
      if (strcmp(button_name, joyXInputAxisName[i]) == 0)
        return i;
    return -1;
  }

  int getPovHatId(const char *name) const override { return -1; }

  void setAxisLimits(int axis_id, float min_val, float max_val) override
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

  bool getButtonState(int btn_id) const override
  {
    G_ASSERT(btn_id >= 0 && btn_id < BUTTONS_NUM);
    return state.buttons.get(btn_id);
  }

  float getAxisPos(int axis_id) const override
  {
    G_ASSERT(axis_id >= 0 && axis_id < AXES_NUM);
    return axis[axis_id].val[0] * axis[axis_id].kMul + axis[axis_id].kAdd;
  }

  int getAxisPosRaw(int axis_id) const override
  {
    G_ASSERT(axis_id >= 0 && axis_id < AXES_NUM);
    return axis[axis_id].val[0];
  }

  int getPovHatAngle(int axis_id) const override { return -1; }

  void doRumble(float lowFreq, float highFreq) override;

  bool isConnected() override;
  bool isXinputCompatible() const override { return true; }

  bool isStickDeadZoneSetupSupported() const override { return true; }
  float getStickDeadZoneScale(int stick_idx) const override
  {
    G_ASSERT_RETURN(stick_idx >= 0 && stick_idx < 2, 0);
    return stickDeadZoneScale[stick_idx];
  }
  void setStickDeadZoneScale(int stick_idx, float scale) override
  {
    G_ASSERT_RETURN(stick_idx >= 0 && stick_idx < 2, );
    if (stickDeadZoneScale[stick_idx] != scale)
      hasReading = false;
    stickDeadZoneScale[stick_idx] = scale;
  }
  float getStickDeadZoneAbs(int stick_idx) const override
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

  void copyStickDeadZoneScales(GameInputGamepadDevice &dev)
  {
    for (int s = 0; s < 2; s++)
      stickDeadZoneScale[s] = dev.stickDeadZoneScale[s];
  }

protected:
  bool isVirtual = false;
  unsigned short ordId = 0, userId = 0;
  GamepadReading lastReading = {};
  bool hasReading = false;
  IGenJoystickClient *client = nullptr;
  JoystickRawState state;
  int deviceType = -1;
  int lastKeyPressedTime = 0;
  float stickDeadZoneScale[2] = {1, 1};

  const char *name = nullptr;
  struct LogicalAxis
  {
    int *val = nullptr;
    float kMul = 1.0f, kAdd = 0.0f;
  };
  LogicalAxis axis[AXES_NUM];

  float rumbleLeft = -1.0f;
  float rumbleRight = -1.0f;
};
} // namespace HumanInput
