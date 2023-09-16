//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <humanInput/dag_hiDecl.h>
#include <humanInput/dag_hiHidClassDrv.h>
#include <humanInput/dag_hiJoyData.h>
#include <util/dag_globDef.h>

class DataBlock;

namespace HumanInput
{
enum
{
  GAMEPAD_VENDOR_UNKNOWN = -1,
  GAMEPAD_VENDOR_MICROSOFT = 0x45e,
  GAMEPAD_VENDOR_SONY = 0x54c,
  GAMEPAD_VENDOR_NINTENDO = 0x57e,
};

// generic joystick interface
class IGenJoystick
{
public:
  // general routines
  virtual const char *getName() const = 0;
  virtual const char *getDeviceID() const = 0;
  virtual bool getDeviceDesc(DataBlock &out_desc) const = 0;
  virtual unsigned short getUserId() const = 0;
  virtual const JoystickRawState &getRawState() const = 0;
  virtual void setClient(IGenJoystickClient *cli) = 0;
  virtual IGenJoystickClient *getClient() const = 0;
  void enableJoyFx(bool enable) { isFxEnabled = enable; }
  bool isJoyFxEnabled() { return isFxEnabled; }

  // buttons and axes description
  virtual int getButtonCount() const = 0;
  virtual const char *getButtonName(int idx) const = 0;
  virtual int getAxisCount() const = 0;
  virtual const char *getAxisName(int idx) const = 0;
  virtual int getPovHatCount() const = 0;
  virtual const char *getPovHatName(int idx) const = 0;

  // buttons and axes name resolver
  virtual int getButtonId(const char *name) const = 0;
  virtual int getAxisId(const char *name) const = 0;
  virtual int getPovHatId(const char *name) const = 0;

  // axis limits remap setup
  virtual void setAxisLimits(int axis_id, float min_val, float max_val) = 0;

  // sensors
  virtual void enableGyroscope(bool /*enable*/) {}

  // current state information
  virtual bool getButtonState(int btn_id) const = 0;
  virtual float getAxisPos(int axis_id) const = 0;
  virtual int getAxisPosRaw(int axis_id) const = 0;
  // return angle in hundredths of degree [0-36000) or -1 if not pressed
  virtual int getPovHatAngle(int hat_id) const = 0;

  //! returns true when stick dead zone management supported by device driver
  virtual bool isStickDeadZoneSetupSupported() const { return false; }
  //! returns dead zone scale for stick in device
  virtual float getStickDeadZoneScale(int /*stick_idx*/) const { return 0; }
  //! sets dead zone scale for stick in device
  virtual void setStickDeadZoneScale(int /*stick_idx*/, float /*scale*/) {}
  //! returns absolute dead zone for stick in device
  virtual float getStickDeadZoneAbs(int /*stick_idx*/) const { return 0; }

  //! returns paired button index for specified axis (if axis coupled with virtual button), or -1 for no pairing
  virtual int getPairedButtonForAxis(int /*axis_idx*/) const { return -1; }
  //! returns paired axis index for specified button (if axis coupled with virtual button), or -1 for no pairing
  virtual int getPairedAxisForButton(int /*btn_idx*/) const { return -1; }

  //! returns joint axis index for specified axis (for sticks, where 2 axes are related), or -1 for no pairing
  virtual int getJointAxisForAxis(int /*axis_idx*/) const { return -1; }

  // force feedback effects creators
  virtual IJoyFfCondition *createConditionFx(int condfx_type, const JoyFfCreateParams &cp)
  {
    G_UNREFERENCED(condfx_type);
    G_UNREFERENCED(cp);
    return nullptr;
  }

  virtual IJoyFfConstForce *createConstForceFx(const JoyFfCreateParams &cp, float force = 0)
  {
    G_UNREFERENCED(cp);
    G_UNREFERENCED(force);
    return nullptr;
  }

  virtual IJoyFfPeriodic *createPeriodicFx(int periodfx_type, const JoyFfCreateParams &cp)
  {
    G_UNREFERENCED(periodfx_type);
    G_UNREFERENCED(cp);
    return nullptr;
  }

  virtual IJoyFfRampForce *createRampForceFx(const JoyFfCreateParams &cp, float start_f = 0, float end_f = 0)
  {
    G_UNREFERENCED(cp);
    G_UNREFERENCED(start_f);
    G_UNREFERENCED(end_f);
    return nullptr;
  }


  // set rumble motors relative speed, freq = 0..1
  virtual void doRumble(float /*lowFreq*/, float /*highFreq*/) {}
  // set (or reset) light effect on device
  virtual void resetSensorsOrientation()
  { /* VOID */
  }
  virtual void setLight(bool /*set*/, unsigned char /*r*/, unsigned char /*g*/, unsigned char /*b*/) {}
  // returns device connection status
  virtual bool isConnected() = 0;
  virtual bool isRemoteController() const { return false; }

  virtual void enableSensorsTiltCorrection(bool /*enable*/)
  { /* VOID */
  }
  virtual void reportGyroSensorsAngDelta(bool /*report_ang_delta*/)
  { /* VOID */
  }

protected:
  bool isFxEnabled = true;
};


// simple joystick client interface
class IGenJoystickClient
{
public:
  virtual void attached(IGenJoystick *joy) = 0;
  virtual void detached(IGenJoystick *joy) = 0;

  virtual void stateChanged(IGenJoystick *joy, int joy_ord_id) = 0;
};


// generic joystick class driver interface
class IGenJoystickClassDrv : public IGenHidClassDrv
{
public:
  virtual IGenJoystick *getDevice(int idx) const = 0;
  virtual IGenJoystick *getDeviceByUserId(unsigned short userId) const = 0;
  virtual bool isDeviceConfigChanged() const = 0;
  virtual void useDefClient(IGenJoystickClient *cli) = 0;

  //! returns current default device, or NULL if not selected
  virtual IGenJoystick *getDefaultJoystick() = 0;
  //! selects current default device
  virtual void setDefaultJoystick(IGenJoystick *ref) = 0;
  //! enable/disable autoselection for default device
  virtual void enableAutoDefaultJoystick(bool enable) = 0;

  //! sets secondary class driver and returns previous sec-class-driver
  virtual IGenJoystickClassDrv *setSecondaryClassDrv(IGenJoystickClassDrv *drv) = 0;

  //! returns true when stick dead zone management supported by class driver
  virtual bool isStickDeadZoneSetupSupported() const { return false; }
  //! returns dead zone scale for stick in device (main_gamepad means XInput devices for PC/XboxOne and gamepad for PS4)
  virtual float getStickDeadZoneScale(int /*stick_idx*/, bool /*main_gamepad*/) const { return 0; }
  //! sets dead zone scale for stick in device
  virtual void setStickDeadZoneScale(int /*stick_idx*/, bool /*main_gamepad*/, float /*scale*/) {}
  //! returns absolute dead zone for stick in device (optional for_joy may be specified to its private deadzone)
  virtual float getStickDeadZoneAbs(int /*stick_idx*/, bool /*main_gamepad*/, IGenJoystick * /*for_joy*/ = nullptr) const { return 0; }

  //! enable/disable gyroscope in device (main_gamepad means XInput devices for PC/XboxOne, gamepad for PS4)
  virtual void enableGyroscope(bool /*enable*/, bool /*main_dev*/){};

  //! return vendor identifer of the current gamepad (return -1 for default), maybe usefull if a platform can use different gamepad
  //! types
  virtual int getVendorId() const { return GAMEPAD_VENDOR_UNKNOWN; };
};
} // namespace HumanInput
