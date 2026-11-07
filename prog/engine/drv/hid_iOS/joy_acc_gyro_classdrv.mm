// Copyright (C) Gaijin Games KFT.  All rights reserved.

#import <Foundation/Foundation.h>
#import <GameController/GameController.h>

#include <EASTL/unique_ptr.h>
#include <ioSys/dag_dataBlock.h>
#include <drv/hid/dag_hiXInputMappings.h>
#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_miscApi.h>
#include <drv/hid/dag_hiGlobals.h>
#include "joy_acc_gyro_classdrv.h"

@implementation iOSInputDevices
{
  HumanInput::IGenJoystickClient *joyClient;
  bool shouldChangeList;
  bool changed;
  int vendorId;
}

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

static GCController *get_active_gamepad()
{
  GCController *current = GCController.current;
  if (current.extendedGamepad && [GCController.controllers containsObject:current])
    return current;
  for (GCController *gc in GCController.controllers)
    if (gc.extendedGamepad)
      return gc;
  return nil;
}

- (id)init
{
  if (self = [super init])
  {
    vendorId = HumanInput::GAMEPAD_VENDOR_UNKNOWN;
    _controllers = [[NSMutableArray alloc] init];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(controllerDidConnect:)
                                                 name:GCControllerDidConnectNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(controllerDidDisconnect:)
                                                 name:GCControllerDidDisconnectNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(controllerDidBecomeCurrent:)
                                                 name:GCControllerDidBecomeCurrentNotification
                                               object:nil];
    [self reconnectDevices];
  }
  return self;
}

- (void)controllerDidDisconnect:(NSNotification *)notification
{
  changed = true;
  shouldChangeList = true;

  GCController *gc = notification.object;
  for (iOSJoystickController *controller in _controllers)
    [controller clearGamepad:gc];
}

- (void)controllerDidConnect:(NSNotification *)notification
{
  // For connect it is simpler. Just raise a flag to notify that
  // isDeviceConfigChanged is true and refreshControllerList should work
  changed = true;
  shouldChangeList = true;
}

- (void)controllerDidBecomeCurrent:(NSNotification *)notification
{
  GCController *gc = notification.object;
  if (!gc.extendedGamepad || [[_controllers firstObject] isController:gc])
    return;
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

- (void)addController:(GCController *)gc name:(const char *)name
{
  iOSJoystickController *controller = [[iOSJoystickController alloc] initWith:gc vendorName:name];
  [_controllers addObject:controller];
  [controller release];
}

- (void)reconnectDevices
{
  [_controllers removeAllObjects];

  // the active pad goes first: it is the default joystick and gives the vendor id
  GCController *active = get_active_gamepad();
  if (active)
  {
    NSString *name = active.vendorName ?: active.productCategory;
    logdbg("gamepad: found device, vendor: %s, category: %s", [name UTF8String], [active.productCategory UTF8String]);
    vendorId = [self readVendor:active.productCategory];
    [self addController:active name:[name UTF8String]];
  }
  else
    vendorId = HumanInput::GAMEPAD_VENDOR_UNKNOWN;

  for (GCController *gc in GCController.controllers)
  {
    if (gc == active)
      continue;
    if (gc.extendedGamepad)
      [self addController:gc name:[(gc.vendorName ?: gc.productCategory) UTF8String]];
    else
      debug("iOSInputDevices::reconnectDevices skip unknown device");
  }

  if (![_controllers count])
    [self addController:nil name:"???"];
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
  joyClient = cli;
  [self updateClient];
}

- (void)updateClient
{
  for (iOSJoystickController *controller in _controllers)
  {
    [controller getJoystick]->setClient(joyClient);
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
- (id)initWith:(GCController *)gc vendorName:(const char *)name
{
  if (self = [super init])
    controller_impl = eastl::make_unique<iOSGamepadEx>(gc, name);
  return self;
}

- (void)clearGamepad:(GCController *)gc
{
  if (!gc || controller_impl->isController(gc))
    controller_impl->clearGamepad();
}

- (bool)isController:(GCController *)gc
{
  return controller_impl->isController(gc);
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

- (iOSGamepadEx *)getGamepad
{
  return controller_impl.get();
}

@end

extern void hid_iOS_init_motion();
extern void hid_iOS_term_motion();
extern void hid_iOS_update_motion();

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
  if (defJoystick)
    defJoystick->setClient(nullptr);

  defJoystick = ref;

  if (defJoystick)
    defJoystick->setClient(client);
}


// enables/disables all devices of class
void iOSJoystickClassDriver::enable(bool en) { HumanInput::stg_joy.enabled = en; }


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
  // refreshControllerList deletes all devices, defJoystick among them
  if ([devices shouldUpdateList])
    setDefaultJoystick(nullptr);

  // refreshDeviceList is called twice on connect/disconnect, shouldUpdateList skips the second rebuild
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
  // device list rebuilds and GameController notifications run on the main thread
  if (!is_main_thread())
    return;

  if (!defJoystick && enableAutoDefault)
  {
    autoSetDefaultJoystick();
  }

  updateGyroSource();
  hid_iOS_update_motion();
  [devices update];
}

void iOSJoystickClassDriver::enableGyroscope(bool enable, bool main_dev)
{
  (main_dev ? deviceGyroEnabled : padGyroEnabled) = enable;
  updateGyroSource();
}

// one gyro works at a time: the gyro of the active pad (device 0) when it is enabled and the pad has one, else the phone gyro
void iOSJoystickClassDriver::updateGyroSource()
{
  iOSGamepadEx *pad = [[devices getItem:0] getGamepad];
  const bool padGyro = pad->setMotionActive(padGyroEnabled);

  const bool deviceGyro = !padGyro && (deviceGyroEnabled || padGyroEnabled);
  if (deviceGyro == deviceMotionRunning)
    return;
  if (deviceGyro)
    hid_iOS_init_motion();
  else
    hid_iOS_term_motion();
  deviceMotionRunning = deviceGyro;
}

int iOSJoystickClassDriver::getVendorId() const { return [devices getVendorId]; }

GCController *iOSJoystickClassDriver::getActiveController() const { return [[devices getItem:0] getGamepad]->getController(); }


static iOSJoystickClassDriver *iosJoystickClassDriver = nullptr;

namespace HumanInput
{
IGenJoystickClassDrv *createJoystickClassDriver(bool, bool, bool)
{
  if (!iosJoystickClassDriver)
    iosJoystickClassDriver = new iOSJoystickClassDriver();
  stg_joy.present = true;
  return iosJoystickClassDriver;
}

void *getGamepadController_iOS() { return iosJoystickClassDriver ? (void *)iosJoystickClassDriver->getActiveController() : nullptr; }
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