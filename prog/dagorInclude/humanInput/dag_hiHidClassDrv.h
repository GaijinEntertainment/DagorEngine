//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <humanInput/dag_hiDecl.h>

namespace HumanInput
{

//
// generic joystick class driver interface
//
class IGenHidClassDrv
{
public:
  // enables/disables all devices of class
  virtual void enable(bool en) = 0;


  // acquires all devices of class
  virtual void acquireDevices() = 0;

  // unacquires all devices of class
  virtual void unacquireDevices() = 0;


  // destroys all devices and class driver
  virtual void destroy() = 0;

  // clears and refreshes list of devices
  virtual void refreshDeviceList() = 0;

  // returns device count of class
  virtual int getDeviceCount() const = 0;

  // performs update for all devices (needed for poll-class devices)
  virtual void updateDevices() = 0;
};
} // namespace HumanInput
