// Copyright (C) Gaijin Games KFT.  All rights reserved.

#import <Foundation/Foundation.h>
#include "tvos_joystick_class_driver.h"

TvosJoystickClassDriver::TvosJoystickClassDriver():defJoystick(0), client(0)
{
    devices = [[TvosInputDevices alloc] init];
    refreshDeviceList();
}

HumanInput::IGenJoystick *TvosJoystickClassDriver::getDevice(int idx) const
{
  if (idx>=0 && idx<[devices count])
    return [[devices getItem:idx ] getJoystick];
    else
    return 0;
}

HumanInput::IGenJoystick *TvosJoystickClassDriver::getDeviceByUserId(unsigned short userId) const
{
  return getDevice(userId);
}

bool TvosJoystickClassDriver::isDeviceConfigChanged() const
{
  return [devices isChanged];
}

void TvosJoystickClassDriver::useDefClient(HumanInput::IGenJoystickClient *cli)
{
  client = cli;
  [devices setClient: cli];
}

//! returns current default device, or NULL if not selected
HumanInput::IGenJoystick *TvosJoystickClassDriver::getDefaultJoystick()
{
  return defJoystick;
}

//! selects current default device
void TvosJoystickClassDriver::setDefaultJoystick(HumanInput::IGenJoystick *ref)
{
  if (defJoystick)
        defJoystick->setClient(0);

  defJoystick = ref;

  if (defJoystick)
        defJoystick->setClient(client);
}

//! enable/disable autoselection for default device
void TvosJoystickClassDriver::enableAutoDefaultJoystick(bool enable)
{
  NSLog(@"TvosJoystickClassDriver::enableAutoDefaultJoystick(%i)", enable);
}

//! sets secondary class driver and returns previous sec-class-driver
HumanInput::IGenJoystickClassDrv *TvosJoystickClassDriver::setSecondaryClassDrv(HumanInput::IGenJoystickClassDrv *drv)
{
    return 0;
}

// enables/disables all devices of class
void TvosJoystickClassDriver::enable ( bool en )
{
    _enable = en;
}

// acquires all devices of class
void TvosJoystickClassDriver::acquireDevices()
{
}

// unacquires all devices of class
void TvosJoystickClassDriver::unacquireDevices()
{
}

// destroys all devices and class driver
void TvosJoystickClassDriver::destroy () {}

// clears and refreshes list of devices
void TvosJoystickClassDriver::refreshDeviceList()
{
  if ([devices count]>0) {
    setDefaultJoystick([[devices getItem:0] getJoystick]);
  }
}

// returns device count of class
 int TvosJoystickClassDriver::getDeviceCount() const {return [devices count];}

// performs update for all devices (needed for poll-class devices)
void TvosJoystickClassDriver::updateDevices()
{
    [devices update];
    //remote_control.update();
}

static TvosJoystickClassDriver* tvosJoystickClassDriver = NULL;

namespace HumanInput {
    HumanInput::IGenJoystickClassDrv *createTvosJoystickClassDriver()
    {
        if ( !tvosJoystickClassDriver)
            tvosJoystickClassDriver = new TvosJoystickClassDriver();
        return tvosJoystickClassDriver;
    }
}
