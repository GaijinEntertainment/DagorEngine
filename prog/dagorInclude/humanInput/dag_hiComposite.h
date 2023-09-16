//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include "generic/dag_carray.h"
#include <humanInput/dag_hiJoystick.h>
#include <util/dag_stdint.h>
#include <util/dag_simpleString.h>
#include <generic/dag_tab.h>

namespace HumanInput
{
class CompositeJoystickDevice;

class ICompositeForceFeedback
{
public:
  virtual void fxReinit() = 0;
  virtual void fxStart() = 0;
  virtual void fxStop() = 0;
  virtual void fxShutdown() = 0;

  virtual void fxSetSpringZero(float x, float y) = 0;
  virtual void fxSetSpringGain(float ffStickPrs) = 0;
  virtual void fxShake(float shakeLevel) = 0;
  virtual void fxPunch(float x, float y, float gain) = 0;
};

class ICompositeLocalization
{
public:
  enum LocalizationType
  {
    LOC_TYPE_XINP,
    LOC_TYPE_DEVICE,
    LOC_TYPE_AXIS,
    LOC_TYPE_POV,
    LOC_TYPE_BUTTON,
  };

  virtual const char *getLocalizedName(LocalizationType t, const char *key = NULL) = 0;
};

struct Device
{
  IGenJoystick *dev = nullptr;
  bool isXInput = false;
  SimpleString name, devID;

  short int numButtons = 0, numPovs = 0, numAxes = 0;
  short int ofsButtons = 0, ofsPovs = 0, ofsAxes = 0;
};

class CompositeJoystickClassDriver : public IGenJoystickClassDrv
{
public:
  static const int MAX_CLASSDRV = 4;
  static CompositeJoystickClassDriver *create();

  CompositeJoystickClassDriver();
  ~CompositeJoystickClassDriver();

  void addClassDrv(IGenJoystickClassDrv *drv, bool is_xinput);
  void setFFClient(ICompositeForceFeedback *cli) { ffClient = cli; }
  void setLocClient(ICompositeLocalization *cli, bool refresh = false);
  ICompositeLocalization *getLocClient() const { return locClient; }

  void setRumbleTargetRealDevIdx(int real_dev_idx);

  bool init();
  void destroyDevices();

  // generic hid class driver interface
  virtual void enable(bool en);
  virtual void acquireDevices();
  virtual void unacquireDevices();
  virtual void destroy();

  virtual void refreshDeviceList() {}

  virtual int getDeviceCount() const { return 1; }
  virtual void updateDevices();

  // generic joystick class driver interface
  virtual IGenJoystick *getDevice(int idx) const;
  virtual IGenJoystick *getDeviceByUserId(unsigned short userId) const;
  virtual bool isDeviceConfigChanged() const;
  virtual void useDefClient(IGenJoystickClient *cli);

  virtual IGenJoystick *getDefaultJoystick();
  virtual void setDefaultJoystick(IGenJoystick * /*ref*/) {}
  virtual void enableAutoDefaultJoystick(bool /*enable*/) {}

  virtual IGenJoystickClassDrv *setSecondaryClassDrv(IGenJoystickClassDrv *drv);

  virtual bool isStickDeadZoneSetupSupported() const override { return true; }
  virtual float getStickDeadZoneScale(int stick_idx, bool main_gamepad) const override;
  virtual void setStickDeadZoneScale(int stick_idx, bool main_gamepad, float scale) override;
  virtual float getStickDeadZoneAbs(int stick_idx, bool main_gamepad, IGenJoystick *for_joy = nullptr) const override;

  bool haveXInputDevices() const { return haveXInput; }

  void resetDevicesList();

  bool isAxisDigital(int axis_id) const;

  //! returns number of real devices registered to composit
  int getRealDeviceCount(bool logical = false) const;
  //! return name of real device by index; indices are valid from 0 through getRealDeviceCount(true)-1
  const char *getRealDeviceName(int real_dev_idx) const;
  //! return ID (usually VID:PID) of real device by index; indices are valid from 0 through getRealDeviceCount(true)-1
  const char *getRealDeviceID(int real_dev_idx) const;
  //! fills and returns description of real device by index in BLK form; indices are valid from 0 through getRealDeviceCount(true)-1
  bool getRealDeviceDesc(int real_dev_idx, DataBlock &out_desc) const;

  //! return native name of axis by idx; returns NULL for disconnected devices
  const char *getNativeAxisName(int axis_idx) const;
  //! return native name of button by idx; returns NULL for disconnected devices
  const char *getNativeButtonName(int btn_idx) const;

  //! remaps absolute button ID and returs relative button ID, real device index is stored to 'out_real_dev_idx'
  int remapAbsButtonId(int abs_btn_id, int &out_real_dev_idx) const;
  //! remaps relative button ID using real device index and returs absolute button ID
  int remapRelButtonId(int rel_btn_id, int real_dev_idx) const;
  //! remaps absolute axis ID and returs relative axis ID, real device index is stored to 'out_real_dev_idx'
  int remapAbsAxisId(int abs_axis_id, int &out_real_dev_idx) const;
  //! remaps relative axis ID using real device index and returs absolute axis ID
  int remapRelAxisId(int rel_axis_id, int real_dev_idx) const;
  //! remaps absolute pov ID and returs relative pov ID, real device index is stored to 'out_real_dev_idx'
  int remapAbsPovHatId(int abs_pov_id, int &out_real_dev_idx) const;
  //! remaps relative pov ID using real device index and returs absolute pov ID
  int remapRelPovHatId(int rel_pov_id, int real_dev_idx) const;

protected:
  struct ClassDrvItem
  {
    IGenJoystickClassDrv *drv;
    bool isXInput;
  };
  carray<ClassDrvItem, MAX_CLASSDRV> classDrvs;
  IGenJoystickClient *defClient;
  CompositeJoystickDevice *compositeDevice;
  int64_t prevUpdateRefTime;
  bool enabled;
  bool deviceConfigChanged;
  bool haveXInput;
  ICompositeForceFeedback *ffClient;
  ICompositeLocalization *locClient;
  float stickDzScale[4] = {1, 1, 0, 0};

  void refreshCompositeDevicesList();
};

void enableJoyFx(bool isEnabled);
} // namespace HumanInput

extern HumanInput::CompositeJoystickClassDriver *global_cls_composite_drv_joy;
