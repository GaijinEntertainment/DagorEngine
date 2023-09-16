#pragma once

#include <humanInput/dag_hiJoystick.h>
#include <util/dag_simpleString.h>
#include <generic/dag_tab.h>
#include <generic/dag_smallTab.h>
#include "osx_hid_util.h"

namespace HumanInput
{
enum
{
  MIN_AXIS_VAL = -32000,
  MAX_AXIS_VAL = 32000,
};

class HidJoystickDevice : public IGenJoystick
{
public:
  HidJoystickDevice(IOHIDDeviceRef dev);
  ~HidJoystickDevice();

  void setDeviceName(const char *name);
  void setId(int id, int user_id)
  {
    ordId = id;
    userId = user_id;
  }

  IOHIDDeviceRef getDev() { return device; }

  void acquire() {}
  void unacquire() {}

  bool updateState(int dt_msec, bool def);

  // IGenJoystick interface implementation
  virtual const char *getName() const { return name; }
  virtual const char *getDeviceID() const { return devID; }
  virtual bool getDeviceDesc(DataBlock &out_desc) const { return false; }
  virtual unsigned short getUserId() const { return userId; }
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

  void setConnected(bool con) { connected = con; }

protected:
  IOHIDDeviceRef device;
  IGenJoystickClient *client;
  JoystickRawState state;
  bool connected;

  int ordId, userId;

  struct AxisData
  {
    IOHIDElementRef elem;
    int maxLogVal, signedVal;
    float v0, dv;
    int *valStorage;
    SimpleString name;
  };
  struct PovHatData
  {
    IOHIDElementRef elem;
    SimpleString name;
    SimpleString axisName[2];
  };
  struct ButtonData
  {
    IOHIDElementRef elem;
    SimpleString name;
  };

  Tab<ButtonData> buttons;
  Tab<AxisData> axes;
  Tab<PovHatData> povHats;
  SimpleString name;
  char devID[10];

  SmallTab<int, MidmemAlloc> axesValStor;
  static constexpr int MAX_BUTTONS = JoystickRawState::MAX_BUTTONS - 16;

private:
  int getVirtualPOVAxis(int hat_id, int axis_id) const;
};
} // namespace HumanInput
