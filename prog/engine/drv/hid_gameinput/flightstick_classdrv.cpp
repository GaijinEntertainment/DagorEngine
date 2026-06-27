// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "flightstick_classdrv.h"

#include "perfMon/dag_cpuFreq.h"


using namespace HumanInput;

void FlightStickClassDriver::enable(bool enable)
{
  isEnabled = enable;
  if (secondaryDriver)
    secondaryDriver->enable(enable);
}

void FlightStickClassDriver::acquireDevices()
{
  if (secondaryDriver)
    secondaryDriver->acquireDevices();
}

void FlightStickClassDriver::unacquireDevices()
{
  if (secondaryDriver)
    secondaryDriver->unacquireDevices();
}

void FlightStickClassDriver::destroy()
{
  if (secondaryDriver)
    secondaryDriver->destroy();
  delete this;
}

int FlightStickClassDriver::getDeviceCount() const
{
  return devices.size() + (secondaryDriver ? secondaryDriver->getDeviceCount() : 0);
}

void FlightStickClassDriver::updateDevices()
{
  int dtMsec = prevUpdateRefTime != 0 ? ::get_time_usec(prevUpdateRefTime) / 1000 : 0;
  prevUpdateRefTime = ::ref_time_ticks();

  if (!isEnabled)
    return;

  for (int i = 0; i < devices.size(); ++i)
  {
    bool isUpdated = devices[i]->updateState(dtMsec);
    if (!defaultJoystick && isAutoDefaultJoystick && isUpdated)
    {
      setDefaultJoystick(&*devices[i]);
      if (defaultJoystick->getClient())
        defaultJoystick->getClient()->stateChanged(defaultJoystick, i);
    }
  }

  if (secondaryDriver)
    secondaryDriver->updateDevices();
}


IGenJoystick *FlightStickClassDriver::getDevice(int idx) const
{
  if (idx >= 0 && idx < devices.size())
    return devices[idx].get();
  return secondaryDriver ? secondaryDriver->getDevice(idx - (int)devices.size()) : nullptr;
}

IGenJoystick *FlightStickClassDriver::getDeviceByUserId(unsigned short user_id) const
{
  for (const eastl::unique_ptr<Device> &device : devices)
    if (device->getUserId() == user_id)
      return device.get();
  return secondaryDriver ? secondaryDriver->getDeviceByUserId(user_id) : nullptr;
}

bool FlightStickClassDriver::isDeviceConfigChanged() const { return isDevicesNeedsRefresh; }

void FlightStickClassDriver::useDefClient(IGenJoystickClient *client)
{
  defaultClient = client;
  for (size_t i = 0; i < devices.size(); ++i)
    devices[i]->setClient(client);
  if (secondaryDriver)
    secondaryDriver->useDefClient(client);
}


IGenJoystick *FlightStickClassDriver::getDefaultJoystick() { return defaultJoystick; }

void FlightStickClassDriver::setDefaultJoystick(IGenJoystick *joystick)
{
  defaultJoystick = joystick;
  if (secondaryDriver)
    secondaryDriver->setDefaultJoystick(joystick);
}

void FlightStickClassDriver::enableAutoDefaultJoystick(bool enable) { isAutoDefaultJoystick = enable; }


IGenJoystickClassDrv *FlightStickClassDriver::setSecondaryClassDrv(IGenJoystickClassDrv *driver)
{
  eastl::swap(secondaryDriver, driver);
  return driver;
}


void FlightStickClassDriver::add_device(DeviceList &old_devices, DeviceList &new_devices, FSDevicePtr flight_stick)
{
  for (auto it = old_devices.begin(); it != old_devices.end(); ++it)
    if (flight_stick == (*it)->getDevice())
    {
      new_devices.emplace_back(eastl::move(*it));
      old_devices.erase(it);
      return;
    }
  eastl::unique_ptr<Device> device = eastl::make_unique<Device>(flight_stick, nextFlightStickUid++);
  device->setClient(defaultClient);
  new_devices.push_back(eastl::move(device));
}

FlightStickClassDriver::FlightStickClassDriver()
{
  devicesChangeCallbackIdx = gdk::gameinput::add_devices_config_change_cb([this]() { this->isDevicesNeedsRefresh = true; });
}

FlightStickClassDriver::~FlightStickClassDriver() { gdk::gameinput::remove_devices_config_change_cb(devicesChangeCallbackIdx); }

void FlightStickClassDriver::refreshDeviceList()
{
  if (isDevicesNeedsRefresh)
  {
    DeviceList oldDevices;
    eastl::swap(devices, oldDevices);
    devices.clear();

    gdk::gameinput::DevicesList controllers;
    gdk::gameinput::get_devices(GameInputKindFlightStick, controllers);

    for (IGameInputDevice *sysDevice : controllers)
      if (sysDevice)
        add_device(oldDevices, devices, sysDevice);

    for (const eastl::unique_ptr<Device> &device : oldDevices)
    {
      if (device.get() == defaultJoystick)
      {
        setDefaultJoystick(nullptr);
        break;
      }
    }
    isDevicesNeedsRefresh = false;
  }

  if (secondaryDriver)
    secondaryDriver->refreshDeviceList();
}
