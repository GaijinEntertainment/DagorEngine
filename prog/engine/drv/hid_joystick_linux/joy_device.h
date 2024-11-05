// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/hid/dag_hiJoystick.h>
#include <util/dag_simpleString.h>
#include <generic/dag_tab.h>
#include <generic/dag_smallTab.h>

#include "hid_udev.h"

namespace HumanInput
{
class HidJoystickDevice : public IGenJoystick
{
public:
  HidJoystickDevice(UDev::Device const &dev, bool remap_360);
  ~HidJoystickDevice();

  void updateDevice(UDev::Device const &dev);
  const char *getModel() const { return device.model; }
  const char *getNode() const { return device.devnode; }

  bool updateState(int dt_msec, bool def);

  // IGenJoystick interface implementation
  virtual const char *getName() const { return device.name; }
  virtual const char *getDeviceID() const { return devID; }
  virtual bool getDeviceDesc(DataBlock &) const { return false; }
  virtual unsigned short getUserId() const { return 0; }
  virtual const JoystickRawState &getRawState() const { return state; }
  virtual void setClient(IGenJoystickClient *cli);
  virtual IGenJoystickClient *getClient() const;

  virtual int getButtonCount() const;
  virtual const char *getButtonName(int idx) const;

  virtual int getAxisCount() const;
  virtual const char *getAxisName(int idx) const;

  virtual int getPovHatCount() const;
  virtual const char *getPovHatName(int idx) const;

  virtual int getButtonId(const char *name) const;
  virtual int getAxisId(const char *name) const;
  virtual int getPovHatId(const char *name) const;
  virtual void setAxisLimits(int axis_id, float min_val, float max_val);
  virtual bool getButtonState(int btn_id) const;
  virtual float getAxisPos(int axis_id) const;
  virtual int getAxisPosRaw(int axis_id) const;
  virtual int getPovHatAngle(int axis_id) const;

  virtual bool isConnected() { return connected; }

  void setConnected(bool con);
  bool isRemappedAsX360() const;

protected:
  UDev::Device device;
  IGenJoystickClient *client;
  JoystickRawState state;
  bool connected;
  bool remapAsX360;

  struct AxisData
  {
    int axis;
    input_absinfo axis_info;
    float min_val, max_val;
    SimpleString name;
    bool attached;
  };

  struct PovHatData : AxisData
  {
    SimpleString axisName[2];
  };

  struct ButtonData
  {
    int btn;
    SimpleString name;
    bool attached;
  };

  Tab<ButtonData> buttons;
  Tab<AxisData> axes;
  Tab<PovHatData> povHats;

  int jfd;
  char devID[10];

private:
  int getVirtualPOVAxis(int hat_id, int axis_id) const;
};
} // namespace HumanInput
