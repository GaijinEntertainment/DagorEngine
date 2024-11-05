// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/hid/dag_hiJoystick.h>
#include "remote_control.h"

class TvosJoystickClassDriver : public HumanInput::IGenJoystickClassDrv
{

public:
  TvosJoystickClassDriver();
  virtual HumanInput::IGenJoystick *getDevice(int idx) const;
  virtual HumanInput::IGenJoystick *getDeviceByUserId(unsigned short userId) const;
  virtual bool isDeviceConfigChanged() const;
  virtual void useDefClient(HumanInput::IGenJoystickClient *cli);
  //! returns current default device, or NULL if not selected
  virtual HumanInput::IGenJoystick *getDefaultJoystick();
  //! selects current default device
  virtual void setDefaultJoystick(HumanInput::IGenJoystick *ref);
  //! enable/disable autoselection for default device
  virtual void enableAutoDefaultJoystick(bool enable);

  //! sets secondary class driver and returns previous sec-class-driver
  virtual HumanInput::IGenJoystickClassDrv *setSecondaryClassDrv(HumanInput::IGenJoystickClassDrv *drv);

public:
  // enables/disables all devices of class
  virtual void enable(bool en);


  // acquires all devices of class
  virtual void acquireDevices();

  // unacquires all devices of class
  virtual void unacquireDevices();


  // destroys all devices and class driver
  virtual void destroy();

  // clears and refreshes list of devices
  virtual void refreshDeviceList();

  // returns device count of class
  virtual int getDeviceCount() const;

  // performs update for all devices (needed for poll-class devices)
  virtual void updateDevices();

private:
  // TvosRemoteControl remote_control;
  TvosInputDevices *devices;
  HumanInput::IGenJoystick *defJoystick;
  HumanInput::IGenJoystickClient *client;
  bool _enable;
};
