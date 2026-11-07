// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "joy_android.h"

#include <util/dag_globDef.h>
#include <drv/hid/dag_hiJoystick.h>
#include <drv/hid/dag_hiDeclDInput.h>
#include <EASTL/bitset.h>
#include <android/input.h>
#include <string.h>

namespace HumanInput
{
// the keys and axes of one gamepad, which the class driver records on the input thread
struct AndroidGamepadInput
{
  enum
  {
    AXIS_X,
    AXIS_Y,
    AXIS_Z,
    AXIS_RZ,
    AXIS_HAT_X,
    AXIS_HAT_Y,
    AXIS_GAS,
    AXIS_BRAKE,
    AXIS_COUNT
  };
  // the android axis of each AXIS_* above, in the same order
  static constexpr int32_t ANDROID_AXES[AXIS_COUNT] = {AMOTION_EVENT_AXIS_X, AMOTION_EVENT_AXIS_Y, AMOTION_EVENT_AXIS_Z,
    AMOTION_EVENT_AXIS_RZ, AMOTION_EVENT_AXIS_HAT_X, AMOTION_EVENT_AXIS_HAT_Y, AMOTION_EVENT_AXIS_GAS, AMOTION_EVENT_AXIS_BRAKE};
  using KeyCodes = eastl::bitset<256>;

  KeyCodes keyCodes; // android key codes held, or pressed since the last read
  float axes[AXIS_COUNT] = {};
};

// reads the input of the gamepad that getGamepadDeviceId_android() names; a zero input when that gamepad has sent no
// input yet or there is no such gamepad
void read_android_gamepad_input(AndroidGamepadInput &out_input);

class JoyAccelGyroInpDevice : public IAndroidJoystick
{
  enum
  {
    BTN_NUM = 28,
  };
  enum
  {
    AXES_NUM = 11 + 6,
  };

public:
  // sensor_source must expose GyroX..GravityZ at axes 9-14 and outlive the device;
  // the class driver updates it before every updateState() of this device
  explicit JoyAccelGyroInpDevice(const IGenJoystick *sensor_source);
  ~JoyAccelGyroInpDevice() override;

  bool updateState() override;

  // IGenJoystick interface implementation
  virtual const char *getName() const { return "android-input"; }
  virtual const char *getDeviceID() const { return "and:gamepad-acc-gyro"; }
  virtual bool getDeviceDesc(DataBlock &out_desc) const;
  virtual unsigned short getUserId() const { return 0; }
  virtual const JoystickRawState &getRawState() const { return state; }
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

  virtual int getButtonCount() const { return BTN_NUM; }
  virtual const char *getButtonName(int idx) const { return btnName[idx]; }

  virtual int getAxisCount() const { return AXES_NUM; }
  virtual const char *getAxisName(int idx) const { return axisName[idx]; }

  virtual int getPovHatCount() const { return 0; }
  virtual const char *getPovHatName(int idx) const { return NULL; }

  virtual int getButtonId(const char *name) const { return -1; }

  virtual int getAxisId(const char *name) const
  {
    for (int i = 0; i < AXES_NUM; i++)
      if (strcmp(name, axisName[i]) == 0)
        return i;
    return -1;
  }

  virtual int getPovHatId(const char *name) const { return -1; }

  virtual void setAxisLimits(int axis_id, float min_val, float max_val)
  {
    G_ASSERT(axis_id >= 0 && axis_id < AXES_NUM);
    axis[axis_id].kMul = (max_val - min_val) / 64000.0f;
    axis[axis_id].kAdd = min_val - -32000.0f * axis[axis_id].kMul;
  }

  virtual bool getButtonState(int btn_id) const { return btn_id >= 0 ? state.buttons.get(btn_id) : false; }

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

  virtual bool isConnected() { return true; }

  virtual bool isStickDeadZoneSetupSupported() const { return true; }
  virtual float getStickDeadZoneScale(int stick_idx) const override
  {
    G_ASSERT_RETURN(stick_idx >= 0 && stick_idx < 2, 0);
    return stickDeadZoneScale[stick_idx];
  }
  virtual void setStickDeadZoneScale(int stick_idx, float scale) override;
  virtual float getStickDeadZoneAbs(int stick_idx) const override;

  virtual void setVendorId(int id) { vendorId = id; }

protected:
  IGenJoystickClient *client;
  const IGenJoystick *sensorSource;
  JoystickRawState state, nextState;

  struct LogicalAxis
  {
    int *val;
    float kMul, kAdd;
  };
  LogicalAxis axis[AXES_NUM];
  int accel[3], gyro[3], gravity[5];

  int vendorId = 0;

  void readInput();
  int keyCodeToButton(int key_code) const;

  static const char *btnName[BTN_NUM];
  static const char *axisName[AXES_NUM];

  float stickDeadZoneScale[2] = {1.f, 1.f};
  float stickDeadZoneBase[2] = {0.2f, 0.2f};
};
} // namespace HumanInput
