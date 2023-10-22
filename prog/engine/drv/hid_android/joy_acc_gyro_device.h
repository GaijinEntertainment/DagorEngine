// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#pragma once

#include "joy_android.h"

#include <util/dag_globDef.h>
#include <humanInput/dag_hiJoystick.h>
#include <humanInput/dag_hiDeclDInput.h>
#include <osApiWrappers/dag_wndProcComponent.h>
#include <string.h>

namespace HumanInput
{
class JoyAccelGyroInpDevice : public IAndroidJoystick, public IWndProcComponent
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
  JoyAccelGyroInpDevice();
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

  virtual RetCode process(void *hwnd, unsigned msg, uintptr_t wParam, intptr_t lParam, intptr_t &result);

  virtual bool isStickDeadZoneSetupSupported() const { return true; }
  virtual float getStickDeadZoneScale(int stick_idx) const override
  {
    G_ASSERT_RETURN(stick_idx >= 0 && stick_idx < 2, 0);
    return stickDeadZoneScale[stick_idx];
  }
  virtual void setStickDeadZoneScale(int stick_idx, float scale) override;
  virtual float getStickDeadZoneAbs(int stick_idx) const override;

protected:
  IGenJoystickClient *client;
  JoystickRawState state, nextState;

  struct LogicalAxis
  {
    int *val;
    float kMul, kAdd;
  };
  LogicalAxis axis[AXES_NUM];
  int accel[3], gyro[3], gravity[5];

  int deviceId;

  static const char *btnName[BTN_NUM];
  static const char *axisName[AXES_NUM];

  float stickDeadZoneScale[2] = {1.f, 1.f};
  float stickDeadZoneBase[2] = {0.2f, 0.2f};
};
} // namespace HumanInput
