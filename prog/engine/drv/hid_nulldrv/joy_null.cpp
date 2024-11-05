// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/hid/dag_hiJoystick.h>
#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiCreate.h>
#include <string.h>

using namespace HumanInput;

class NullJoystickDrv : public IGenJoystickClassDrv, public IGenJoystick
{
public:
  // IGenJoystickClassDrv interface
  virtual void enable(bool) {}
  virtual void acquireDevices() {}
  virtual void unacquireDevices() {}
  virtual void destroy() {}
  virtual void refreshDeviceList() {}
  virtual int getDeviceCount() const { return 1; }
  virtual void updateDevices() { memset(&raw_state_joy, 0, sizeof(raw_state_joy)); }
  virtual IGenJoystick *getDeviceByUserId(unsigned short) const { return NULL; }
  virtual bool isDeviceConfigChanged() const { return false; }
  virtual IGenJoystick *getDevice(int idx) const { return idx == 0 ? (IGenJoystick *)this : NULL; }
  virtual void useDefClient(IGenJoystickClient *) {}
  virtual void setDefaultJoystick(IGenJoystick *) {}
  virtual IGenJoystick *getDefaultJoystick() { return (IGenJoystick *)this; }
  virtual void enableAutoDefaultJoystick(bool) {}
  virtual IGenJoystickClassDrv *setSecondaryClassDrv(IGenJoystickClassDrv *) { return NULL; }

  // IGenJoystick interface
  virtual const char *getName() const { return "NullJoystick"; }
  virtual const char *getDeviceID() const { return "null"; }
  virtual bool getDeviceDesc(DataBlock &) const { return false; }
  virtual unsigned short getUserId() const { return 0; }
  virtual const JoystickRawState &getRawState() const { return state; }
  virtual void setClient(IGenJoystickClient *) {}
  virtual IGenJoystickClient *getClient() const { return NULL; }
  virtual int getButtonCount() const { return 0; }
  virtual const char *getButtonName(int) const { return NULL; }
  virtual int getAxisCount() const { return 0; }
  virtual const char *getAxisName(int) const { return NULL; }
  virtual int getPovHatCount() const { return 0; }
  virtual const char *getPovHatName(int) const { return NULL; }
  virtual int getButtonId(const char *) const { return -1; }
  virtual int getAxisId(const char *) const { return -1; }
  virtual int getPovHatId(const char *) const { return -1; }
  virtual void setAxisLimits(int, float, float) {}
  virtual bool getButtonState(int) const { return false; }
  virtual float getAxisPos(int) const { return 0; }
  virtual int getAxisPosRaw(int) const { return 0; }
  virtual int getPovHatAngle(int) const { return 0; }
  virtual bool isConnected() { return false; }
  JoystickRawState state;
};

static NullJoystickDrv drv;

IGenJoystickClassDrv *HumanInput::createNullJoystickClassDriver()
{
  memset(&raw_state_joy, 0, sizeof(raw_state_joy));
  memset(&drv.state, 0, sizeof(drv.state));
  return &drv;
}

#if _TARGET_PC
bool HumanInput::isImeAvailable() { return false; }
bool HumanInput::showScreenKeyboard_IME(const DataBlock &, OnFinishIME, void *) { return false; }
namespace HumanInput
{
void reg_cb_show_ime_steam(OnShowIME) {}
} // namespace HumanInput
#endif
