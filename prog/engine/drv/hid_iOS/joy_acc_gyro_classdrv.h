// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/hid/dag_hiJoystick.h>
#include "joy_acc_gyro_device.h"

@protocol iOSInputDeviceUpdate

- (void)update;

@end

@interface iOSJoystickController : NSObject <iOSInputDeviceUpdate>

- (id)initWith:(GCExtendedGamepad *)extendedGamepad vendorName:(const char *)name;
- (void)clearGamepad;
- (void)dealloc;
- (void)update;
- (HumanInput::IGenJoystick *)getJoystick;
@end

@interface iOSInputDevices : NSObject <iOSInputDeviceUpdate>

- (id)init;
- (void)controllerDidDisconnect:(NSNotification *)notification;
- (void)controllerDidConnect:(NSNotification *)notification;
- (void)reconnectDevices;
- (void)update;
- (int)count;
- (iOSJoystickController *)getItem:(int)idx;
- (void)setClient:(HumanInput::IGenJoystickClient *)cli;
- (bool)isChanged;
- (void)refreshControllerList;
- (bool)shouldUpdateList;
- (int)getVendorId;
- (int)readVendor:(NSString *)category;
@property(nonatomic, retain) NSMutableArray *controllers;
@end


class iOSJoystickClassDriver : public HumanInput::IGenJoystickClassDrv
{
public:
  iOSJoystickClassDriver();
  HumanInput::IGenJoystick *getDevice(int idx) const override;
  HumanInput::IGenJoystick *getDeviceByUserId(unsigned short userId) const override;
  bool isDeviceConfigChanged() const override;
  void useDefClient(HumanInput::IGenJoystickClient *new_client) override;

  //! returns current default device, or NULL if not selected
  HumanInput::IGenJoystick *getDefaultJoystick() override { return defJoystick; };

  //! selects current default device
  void setDefaultJoystick(HumanInput::IGenJoystick *ref) override;

  //! enable/disable autoselection for default device
  void enableAutoDefaultJoystick(bool enable) override { enableAutoDefault = enable; };

  //! sets secondary class driver and returns previous sec-class-driver
  HumanInput::IGenJoystickClassDrv *setSecondaryClassDrv(HumanInput::IGenJoystickClassDrv *drv) override { return NULL; };

  // enables/disables all devices of class
  void enable(bool enable) override;

  // acquires all devices of class
  void acquireDevices() override{};

  // unacquires all devices of class
  void unacquireDevices() override{};

  // destroys all devices and class driver
  void destroy() override;

  // clears and refreshes list of devices
  void refreshDeviceList() override;

  // returns device count of class
  int getDeviceCount() const override;

  // performs update for all devices (needed for poll-class devices)
  void updateDevices() override;

  int getVendorId() const override;

  void enableGyroscope(bool enable, bool main_dev) override;

  bool isStickDeadZoneSetupSupported() const override { return true; }
  float getStickDeadZoneScale(int stick_idx, bool) const override
  {
    G_ASSERT_RETURN(stick_idx >= 0 && stick_idx < 2, 0);
    return defJoystick ? defJoystick->getStickDeadZoneScale(stick_idx) : 0.f;
  }
  void setStickDeadZoneScale(int stick_idx, bool main_dev, float scale) override
  {
    if (!main_dev)
      return;
    G_ASSERT_RETURN(stick_idx >= 0 && stick_idx < 2, );
    if (defJoystick)
      defJoystick->setStickDeadZoneScale(stick_idx, scale);
  }
  float getStickDeadZoneAbs(int stick_idx, bool main_dev, HumanInput::IGenJoystick *for_joy) const override
  {
    if (!main_dev)
      return 0.f;
    G_ASSERT_RETURN(stick_idx >= 0 && stick_idx < 2, 0);
    return defJoystick ? defJoystick->getStickDeadZoneAbs(stick_idx) : 0.f;
  }

private:
  void autoSetDefaultJoystick();

  int vendorId = HumanInput::GAMEPAD_VENDOR_UNKNOWN;

  bool enableAutoDefault = true;

  iOSInputDevices *devices = nullptr;
  HumanInput::IGenJoystick *defJoystick = nullptr;
  HumanInput::IGenJoystickClient *client = nullptr;
  bool shouldClearClient = true;

  bool isEnabled = false;
};
