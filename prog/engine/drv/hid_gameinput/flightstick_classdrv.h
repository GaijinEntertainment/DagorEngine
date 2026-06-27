// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>

#include <drv/hid/dag_hiJoystick.h>
#include <osApiWrappers/dag_wndProcComponent.h>

#include "flightstick_device.h"


namespace HumanInput
{
class FlightStickClassDriver : public IGenJoystickClassDrv
{
public:
  FlightStickClassDriver();
  ~FlightStickClassDriver();

  void enable(bool enable) override;
  void acquireDevices() override;
  void unacquireDevices() override;
  void destroy() override;

  void refreshDeviceList() override;

  int getDeviceCount() const override;
  void updateDevices() override;

  IGenJoystick *getDevice(int idx) const override;
  IGenJoystick *getDeviceByUserId(unsigned short user_id) const override;
  bool isDeviceConfigChanged() const override;
  void useDefClient(IGenJoystickClient *client) override;

  IGenJoystick *getDefaultJoystick() override;
  void setDefaultJoystick(IGenJoystick *joystick) override;
  void enableAutoDefaultJoystick(bool enable) override;

  IGenJoystickClassDrv *setSecondaryClassDrv(IGenJoystickClassDrv *driver) override;

protected:
  using Device = FlightStickDevice;
  using DeviceList = eastl::vector<eastl::unique_ptr<Device>>;

  bool isEnabled = false;
  bool isDevicesNeedsRefresh = true;
  bool isAutoDefaultJoystick = true;

  int64_t prevUpdateRefTime = 0;
  int nextFlightStickUid = 0;

  DeviceList devices;
  size_t devicesChangeCallbackIdx = size_t(-1);

  IGenJoystickClient *defaultClient = nullptr;
  IGenJoystickClassDrv *secondaryDriver = nullptr;
  IGenJoystick *defaultJoystick = nullptr;

  void add_device(DeviceList &old_devices, DeviceList &new_devices, FSDevicePtr flight_stick);
};
} // namespace HumanInput
