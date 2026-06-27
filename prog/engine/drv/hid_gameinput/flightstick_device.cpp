// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "flightstick_device.h"

#include <debug/dag_assert.h>
#include <util/dag_globDef.h>
#include <util/dag_baseDef.h>
#include <stdio.h>

#include <drv/hid/dag_hiXInputMappings.h>

class DataBlock;
using namespace HumanInput;

bool HumanInput::operator==(const FlightStickDeviceState &lhs, const FlightStickDeviceState &rhs)
{
  return lhs.buttons == rhs.buttons && lhs.axes == rhs.axes;
}

bool HumanInput::operator!=(const FlightStickDeviceState &lhs, const FlightStickDeviceState &rhs) { return !(lhs == rhs); }

FlightStickDevice::~FlightStickDevice() = default;

FSDevicePtr FlightStickDevice::getDevice() const { return device; }

const char *FlightStickDevice::getName() const { return name.c_str(); }

const char *FlightStickDevice::getDeviceID() const { return id; }

bool FlightStickDevice::getDeviceDesc(DataBlock &) const { return false; }

unsigned short FlightStickDevice::getUserId() const { return 0; }

const JoystickRawState &FlightStickDevice::getRawState() const { return rawState; }

void FlightStickDevice::setClient(IGenJoystickClient *other) { client = other; }

IGenJoystickClient *FlightStickDevice::getClient() const { return client; }

int FlightStickDevice::getPovHatCount() const { return 0; }

const char *FlightStickDevice::getPovHatName(int) const { return nullptr; }

int FlightStickDevice::getButtonId(const char *name) const
{
  for (int i = 0; i < getButtonCount(); ++i)
  {
    const char *buttonName = getButtonName(i);
    if (buttonName && ::strcmp(name, buttonName) == 0)
      return i;
  }
  return -1;
}

int FlightStickDevice::getAxisId(const char *name) const
{
  for (int i = 0; i < getAxisCount(); ++i)
    if (::strcmp(name, getAxisName(i)) == 0)
      return i;
  return -1;
}

int FlightStickDevice::getPovHatId(const char *) const { return -1; }

bool FlightStickDevice::getButtonState(int button_id) const
{
  if (button_id < 0 || button_id >= getButtonCount())
  {
    DEBUG_CTX("Invalid button_id = %d", button_id);
    return false;
  }

  return (state.buttons & (static_cast<ButtonBits>(1) << button_id)) != 0;
}

int FlightStickDevice::getPovHatAngle(int axis_id) const { return 0; }

bool FlightStickDevice::isConnected() { return true; }
