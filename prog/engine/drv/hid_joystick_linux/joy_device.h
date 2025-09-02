// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/hid/dag_hiJoystick.h>
#include <util/dag_simpleString.h>
#include <generic/dag_tab.h>
#include <generic/dag_smallTab.h>

#include "hid_udev.h"
#include "evdev_helpers.h"

#define JOY_LOG(fmt, ...) debug("[INPUT@%s] " fmt, __FUNCTION__ __VA_OPT__(, ) __VA_ARGS__)

#if defined(DAG_JOY_ENABLE_TRACE)
#define JOY_TRACE JOY_LOG
#else
#define JOY_TRACE(...) \
  {}
#endif

namespace HumanInput
{

struct AxisData
{
  int sysAxisId;
  int remapIdx;
  input_absinfo sysAbsInfo;
  float minVal, maxVal;
  bool inverted = false;
  SimpleString name;
  int prevVal;
};

struct PovHatData : AxisData
{
  int padX;
  int padY;
  SimpleString axisName[2];
};

struct ButtonData
{
  int sysBtnId;
  SimpleString name;
};

inline int povhat_pads_to_angle(int pad_x, int pad_y)
{
  int angle = 0;
  if (pad_x == 0)
  {
    if (pad_y == 0)
      angle = -1;
    else if (pad_y > 0)
      angle = 0;
    else if (pad_y < 0)
      angle = 180 * 100;
  }
  else if (pad_x > 0)
  {
    if (pad_y == 0)
      angle = 90 * 100;
    else if (pad_y > 0)
      angle = 45 * 100;
    else if (pad_y < 0)
      angle = 135 * 100;
  }
  else if (pad_x < 0)
  {
    if (pad_y == 0)
      angle = 270 * 100;
    else if (pad_y > 0)
      angle = 315 * 100;
    else if (pad_y < -0)
      angle = 225 * 100;
  }
  return angle;
}

class UDevJoystick : public IGenJoystick
{
public:
  virtual ~UDevJoystick() = default;

  virtual void updateDevice(UDev::Device const &dev) = 0;
  virtual const char *getModel() const = 0;
  virtual const char *getNode() const = 0;

  virtual bool updateState(int dt_msec, bool def) = 0;
  virtual void setConnected(bool con) = 0;
};

class HidJoystickDevice : public UDevJoystick
{
public:
  HidJoystickDevice(UDev::Device const &dev);
  virtual ~HidJoystickDevice();

  void updateDevice(UDev::Device const &dev) override;
  const char *getModel() const override { return device.model; }
  const char *getNode() const override { return device.devnode; }

  bool updateState(int dt_msec, bool def) override;

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

  void setConnected(bool con) override;

  dag::ConstSpan<AxisData> getAxesInfo() const { return axes; }
  dag::ConstSpan<ButtonData> getButtonsInfo() const { return buttons; }
  dag::ConstSpan<PovHatData> getPovHatsInfo() const { return povHats; }

  bool isBtnPressed(int sys_btn_id) const { return sys_btn_id < KEY_MAX && sysKeys.test(sys_btn_id); }

  input_id getInputId() const { return id; }

protected:
  UDev::Device device;
  IGenJoystickClient *client = nullptr;
  JoystickRawState state;
  bool connected;

  KeysBitArray sysKeys;
  dag::Vector<ButtonData> buttons;
  dag::Vector<AxisData> axes;
  dag::Vector<PovHatData> povHats;

  int jfd;
  char devID[10];
  int lastKeyPressedTime = 0;

  input_id id;

private:
  int getVirtualPOVAxis(int hat_id, int axis_id) const;
};

} // namespace HumanInput
