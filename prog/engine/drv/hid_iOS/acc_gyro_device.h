// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#pragma once

#include <util/dag_globDef.h>
#include <humanInput/dag_hiJoystick.h>
#include <humanInput/dag_hiDeclDInput.h>
#include <string.h>

namespace HumanInput
{
class AccelGyroInpDevice : public IGenJoystick
{
  enum
  {
    AXES_NUM = 11,
  };

public:
  AccelGyroInpDevice();
  ~AccelGyroInpDevice();

  bool updateState(bool def);

  // IGenJoystick interface implementation
  virtual const char *getName() const { return "iOS-input"; }
  virtual const char *getDeviceID() const { return "iOS:acc-gyro"; }
  virtual bool getDeviceDesc(DataBlock &out_desc) const { return false; }
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

  virtual int getButtonCount() const { return 0; }
  virtual const char *getButtonName(int idx) const { return "JB?"; }

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

  virtual bool getButtonState(int btn_id) const { return 0; }

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

protected:
  IGenJoystickClient *client;
  JoystickRawState state;

  struct LogicalAxis
  {
    int *val;
    float kMul, kAdd;
  };
  LogicalAxis axis[AXES_NUM];
  int gravity[5];

  static const char *axisName[AXES_NUM];
};
} // namespace HumanInput
