// Copyright (C) Gaijin Games KFT.  All rights reserved.

#import <Foundation/Foundation.h>
#import <GameController/GameController.h>

#include <EASTL/unique_ptr.h>
#include <ioSys/dag_dataBlock.h>
#include <drv/hid/dag_hiXInputMappings.h>
#include <startup/dag_globalSettings.h>
#include "joy_acc_gyro_classdrv.h"

@implementation iOSInputDevices
HumanInput::IGenJoystickClient *joy_client = nullptr;
bool shouldChangeList = false;
bool changed = false;

static bool gamepad_exist_in_block(const DataBlock *blk, const char * productName, const char *blockName)
{
  if (const DataBlock *layoutBlk = blk->getBlockByName(blockName))
  {
    for (int blockNo = 0; blockNo < layoutBlk->blockCount(); blockNo++)
    {
      const DataBlock *configBlk = layoutBlk->getBlock(blockNo);
      const char * pname = configBlk->getStr("name", NULL);
      if (pname && strcasecmp(pname, productName) == 0)
        return true;
    }
  }
  return false;
}

static int get_gamepad_layout(const char * productName)
{
  const DataBlock *modelsBlk = dgs_get_settings()->getBlockByNameEx("gamepad_layout");
  if (modelsBlk->isEmpty())
  {
    logdbg("gamepad vendors list are not found");
    return HumanInput::GAMEPAD_VENDOR_UNKNOWN;
  }
  logdbg("gamepad: searching in blk gamepad layouts for device: %s", productName);

  const int res = gamepad_exist_in_block(modelsBlk, productName, "nintendo") ? HumanInput::GAMEPAD_VENDOR_NINTENDO
                : gamepad_exist_in_block(modelsBlk, productName, "sony") ? HumanInput::GAMEPAD_VENDOR_SONY
                : gamepad_exist_in_block(modelsBlk, productName, "xbox") ? HumanInput::GAMEPAD_VENDOR_MICROSOFT
                : HumanInput::GAMEPAD_VENDOR_UNKNOWN;

  if (res != HumanInput::GAMEPAD_VENDOR_UNKNOWN)
    logdbg("gamepad: found layout vendorid - %d", res);

  return res;
}

- (id)init
{
  if (self = [super init])
  {
    _controllers = [[NSMutableArray alloc] init];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(controllerDidConnect:)
                                                 name:GCControllerDidConnectNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(controllerDidDisconnect:)
                                                 name:GCControllerDidDisconnectNotification
                                               object:nil];
    [self reconnectDevices];
  }
  return self;
}

- (void)controllerDidDisconnect:(NSNotification *)notification
{
  changed = true;
  shouldChangeList = true;

  // Disconnect may happen at any time so Apple's GCExtendedGamepad inside controller
  // may become invalid at any time, so all pointers should be set to null the moment it
  // happens
  for (iOSJoystickController *controller in _controllers)
  {
    [controller clearGamepad];
  }
}

- (void)controllerDidConnect:(NSNotification *)notification
{
  // For connect it is simpler. Just raise a flag to notify that
  // isDeviceConfigChanged is true and refreshControllerList should work
  changed = true;
  shouldChangeList = true;
}

- (bool)shouldUpdateList
{
  return shouldChangeList;
}

- (bool)isChanged
{
  if (changed)
  {
    changed = false;
    return true;
  }
  return false;
}

int vendorId = HumanInput::GAMEPAD_VENDOR_UNKNOWN;
- (int)readVendor:(NSString *)category
{
  if ([category caseInsensitiveCompare:GCProductCategoryXboxOne] == NSOrderedSame
    || [category rangeOfString:@"xbox" options:NSCaseInsensitiveSearch].location != NSNotFound)
  {
    return HumanInput::GAMEPAD_VENDOR_MICROSOFT;
  }

  if ([category caseInsensitiveCompare:GCProductCategoryDualSense] == NSOrderedSame
    || [category caseInsensitiveCompare:GCProductCategoryDualShock4] == NSOrderedSame
    || [category rangeOfString:@"dualshock" options:NSCaseInsensitiveSearch].location != NSNotFound
    || [category rangeOfString:@"dualsense" options:NSCaseInsensitiveSearch].location != NSNotFound)
  {
    return HumanInput::GAMEPAD_VENDOR_SONY;
  }
  if ([category rangeOfString:@"switch" options:NSCaseInsensitiveSearch].location != NSNotFound
    || [category rangeOfString:@"pro" options:NSCaseInsensitiveSearch].location != NSNotFound
    || [category rangeOfString:@"joycon" options:NSCaseInsensitiveSearch].location != NSNotFound
    || [category rangeOfString:@"joy-con" options:NSCaseInsensitiveSearch].location != NSNotFound)
  {
    return HumanInput::GAMEPAD_VENDOR_NINTENDO;
  }
  return get_gamepad_layout([category UTF8String]);
}

- (int)getVendorId
{
  return vendorId;
}

- (void)reconnectDevices
{
  // Remove doesn't send release message to removed object
  for (iOSJoystickController *controller in _controllers)
  {
    [controller release];
  }
  [_controllers removeAllObjects];

  bool inserted = false;
  for (GCController *appleController in GCController.controllers)
  {
    if (appleController.extendedGamepad != nil)
    {
      if (!inserted)
        logdbg("gamepad: found device, vendor: %s, category: %s", [appleController.vendorName UTF8String], [appleController.productCategory UTF8String]);
        vendorId = [self readVendor:appleController.productCategory];

      inserted = true;
      const char *cName = [appleController.vendorName UTF8String];
      [_controllers addObject:[[iOSJoystickController alloc] initWith:appleController.extendedGamepad vendorName:cName]];
    }
    else
    {
      debug("iOSInputDevices::reconnectDevices skip unknown device");
    }
  }
  if (!inserted)
  {
    [_controllers addObject:[[iOSJoystickController alloc] initWith:nil vendorName:"???"]];
    vendorId = HumanInput::GAMEPAD_VENDOR_UNKNOWN;
  }
  [self updateClient];
}

- (void)refreshControllerList
{
  if (shouldChangeList)
  {
    [self reconnectDevices];
    shouldChangeList = false;
  }
}

- (void)update
{
  for (iOSJoystickController *controller in _controllers)
  {
    [controller update];
  }
}

- (void)setClient:(HumanInput::IGenJoystickClient *)cli
{
  joy_client = cli;
  [self updateClient];
}

- (void)updateClient
{
  for (iOSJoystickController *controller in _controllers)
  {
    [controller getJoystick]->setClient(joy_client);
  }
}

- (int)count
{
  return _controllers.count;
}

- (iOSJoystickController *)getItem:(int)idx;
{
  return [_controllers objectAtIndex:idx];
}

@end


@implementation iOSJoystickController
{
  eastl::unique_ptr<iOSGamepadEx> controller_impl;
}
- (id)initWith:(GCExtendedGamepad *)extendedGamepad vendorName:(const char *)name
{
  self = [super init];
  controller_impl = eastl::make_unique<iOSGamepadEx>(extendedGamepad, name);
  return self;
}

- (void)clearGamepad
{
  controller_impl->clearGamepad();
}

- (void)dealloc
{
  controller_impl.reset();
  [super dealloc];
}

- (void)update
{
  controller_impl->update();
}

- (HumanInput::IGenJoystick *)getJoystick
{
  return controller_impl.get();
}

@end

extern void hid_iOS_init_motion();
extern void hid_iOS_term_motion();

iOSJoystickClassDriver::iOSJoystickClassDriver()
{
  devices = [[iOSInputDevices alloc] init];
}


HumanInput::IGenJoystick *iOSJoystickClassDriver::getDevice(int idx) const
{
  if (idx >= 0 && idx < [devices count])
    return [[devices getItem:idx] getJoystick];
  else
    return nullptr;
}


HumanInput::IGenJoystick *iOSJoystickClassDriver::getDeviceByUserId(unsigned short userId) const { return getDevice(userId); }


bool iOSJoystickClassDriver::isDeviceConfigChanged() const { return [devices isChanged]; }


void iOSJoystickClassDriver::useDefClient(HumanInput::IGenJoystickClient *cli)
{
  client = cli;
  [devices setClient:cli];
}


//! selects current default device
void iOSJoystickClassDriver::setDefaultJoystick(HumanInput::IGenJoystick *ref)
{
  // setDefaultJoystick is called after refreshDeviceList
  // that means that defJoystick is already detached and deleted
  if (defJoystick && shouldClearClient)
  {
    defJoystick->setClient(nullptr);
  }
  shouldClearClient = true;

  defJoystick = ref;

  if (defJoystick)
    defJoystick->setClient(client);
}


// enables/disables all devices of class
void iOSJoystickClassDriver::enable(bool enable) { isEnabled = enable; }


// destroys all devices and class driver
void iOSJoystickClassDriver::destroy() { }

void iOSJoystickClassDriver::autoSetDefaultJoystick()
{
  G_ASSERT([devices count] != 0);
  auto joy = [[devices getItem:0] getJoystick];
  setDefaultJoystick(joy->isConnected() ? joy : nullptr);
}

// clears and refreshes list of devices
void iOSJoystickClassDriver::refreshDeviceList()
{
  // After succsesful refreshControllerList defjoystick
  // is an invalid pointer so we must clear it in advance
  bool willUpdate = [devices shouldUpdateList];
  if (willUpdate && defJoystick)
  {
    defJoystick->setClient(nullptr);
  }

  shouldClearClient = !willUpdate;
  // Also refreshDeviceList is called twice on connect/disconnect
  // so shouldUpdateList is added to remove needless contoller list
  // refill with extra allocations
  [devices refreshControllerList];

  if (enableAutoDefault)
  {
    autoSetDefaultJoystick();
  }
}


// returns device count of class
int iOSJoystickClassDriver::getDeviceCount() const { return [devices count]; }


// performs update for all devices (needed for poll-class devices)
void iOSJoystickClassDriver::updateDevices()
{
  if (!defJoystick && enableAutoDefault)
  {
    autoSetDefaultJoystick();
  }

  [devices update];
}

void iOSJoystickClassDriver::enableGyroscope(bool enable, bool)
{
  if (enable)
    hid_iOS_init_motion();
  else
    hid_iOS_term_motion();
}

int iOSJoystickClassDriver::getVendorId() const { return [devices getVendorId]; }


static iOSJoystickClassDriver *iosJoystickClassDriver = nullptr;

namespace HumanInput
{
extern JoystickSettings stg_joy;
IGenJoystickClassDrv *createJoystickClassDriver(bool, bool)
{
  if (!iosJoystickClassDriver)
    iosJoystickClassDriver = new iOSJoystickClassDriver();
  stg_joy.present = true;
  return iosJoystickClassDriver;
}
} // namespace HumanInput

namespace hid_ios
{
extern double g_acc_x, g_acc_y, g_acc_z;
extern double g_base_acc_x, g_base_acc_y, g_base_acc_z;
} // namespace hid_ios

namespace HumanInput
{
void setAsNeutralGsensor_iOS()
{
  using namespace hid_ios;

  g_base_acc_x = g_acc_x;
  g_base_acc_y = g_acc_y;
  g_base_acc_z = g_acc_z;

  double l2 = g_acc_x * g_acc_x + g_acc_y * g_acc_y + g_acc_z * g_acc_z;
  if (l2 > 1e-6)
  {
    l2 = sqrt(l2);
    g_base_acc_x /= l2;
    g_base_acc_y /= l2;
    g_base_acc_z /= l2;
  }
}
} // namespace HumanInput