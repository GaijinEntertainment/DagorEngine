// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_globDef.h>
#include <drv/hid/dag_hiJoystick.h>
#include <drv/hid/dag_hiDeclDInput.h>
#include <drv/hid/dag_hiGlobals.h>
#include <ioSys/dag_dataBlock.h>
#include <string.h>

namespace HumanInput
{
struct JoyCookedCreateParams;

enum
{
  MIN_AXIS_VAL = -32000,
  MAX_AXIS_VAL = 32000,
};


class SteamGamepadDevice : public IGenJoystick
{
  enum
  {
    BUTTONS_NUM = 28,
    AXES_NUM = 6,
  };

public:
  SteamGamepadDevice(int gamepad_no, const char *_name, int ord_id);
  ~SteamGamepadDevice();

  bool updateState(int dt_msec, bool def, bool has_def);

  // IGenJoystick interface implementation
  virtual const char *getName() const { return name; }
  virtual const char *getDeviceID() const { return "steam"; }
  virtual bool getDeviceDesc(DataBlock &out_desc) const
  {
    out_desc.clearData();
    out_desc.setBool("xinputDev", true);
    out_desc.setBool("steamDev", true);
    return true;
  }
  virtual unsigned short getUserId() const { return userId; }
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

  virtual int getButtonCount() const { return stg_joy.disableVirtualControls ? 18 : BUTTONS_NUM; }
  virtual const char *getButtonName(int idx) const
  {
    if (idx < 0 || idx >= HumanInput::SteamGamepadDevice::BUTTONS_NUM)
      return NULL;
    return buttonName[idx];
  }

  virtual int getAxisCount() const { return AXES_NUM; }
  virtual const char *getAxisName(int idx) const { return axisName[idx]; }

  virtual int getPovHatCount() const { return 0; }
  virtual const char *getPovHatName(int idx) const { return NULL; }

  virtual int getButtonId(const char *name) const
  {
    for (int i = 0; i < BUTTONS_NUM; i++)
      if (strcmp(name, buttonName[i]) == 0)
        return i;
    return -1;
  }

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
    if (axis_id < 4)
    {
      // thumbs
      axis[axis_id].kMul = (max_val - min_val) / float(MAX_AXIS_VAL - MIN_AXIS_VAL);
      axis[axis_id].kAdd = min_val - MIN_AXIS_VAL * axis[axis_id].kMul;
    }
    else
    {
      // triggers
      axis[axis_id].kMul = (max_val - min_val) / float(MAX_AXIS_VAL);
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

protected:
  unsigned short ordId, userId;
  unsigned xStPktId;
  IGenJoystickClient *client;
  JoystickRawState state;
  int deviceType;

  const char *name;
  struct LogicalAxis
  {
    int *val;
    float kMul, kAdd;
  };
  LogicalAxis axis[AXES_NUM];

  unsigned short rumbleLeftMotor, rumbleRightMotor;
  bool rumblePassed;

  static const char *buttonName[BUTTONS_NUM];
  static const char *axisName[AXES_NUM];
};
} // namespace HumanInput
