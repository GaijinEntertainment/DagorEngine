// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <Foundation/NSProcessInfo.h>
#import <UIKit/UIKit.h>
#include <os/proc.h>
#include <mach/mach.h>
#include "ios/ios_net_reachability.h"

void ios_activate_battery_monitoring()
{
  UIDevice *myDevice = [UIDevice currentDevice];
  [myDevice setBatteryMonitoringEnabled:YES];
}

float ios_get_battery_level()
{
  UIDevice *myDevice = [UIDevice currentDevice];
  float level = (float)[myDevice batteryLevel];
  if (level < 0)
    return -1;
  return level;
}

int ios_get_battery_status()
{
  UIDevice *myDevice = [UIDevice currentDevice];
  return [myDevice batteryState];
}

int ios_get_thermal_state()
{
  NSProcessInfoThermalState state = [[NSProcessInfo processInfo] thermalState];
  return (int)state;
}

bool ios_is_ipad()
{
  return ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad);
}

size_t ios_get_available_memory()
{
  size_t mem = 0;
  if (@available(iOS 13, *))
  {
    mem = os_proc_available_memory();
  }
  return mem;
}

size_t ios_get_phys_footprint()
{
  task_vm_info_data_t taskInfo;
  mach_msg_type_number_t infoCount = TASK_VM_INFO_COUNT;
  if (task_info(mach_task_self(), TASK_VM_INFO, (task_info_t)&taskInfo, &infoCount) != KERN_SUCCESS)
    return 0;
  return taskInfo.phys_footprint;
}

int ios_get_network_connection_type()
{
  return [[NetReachability instance] currentReachabilityStatus];
}

void ios_get_os_version(int &major, int &minor, int &patch)
{
  NSOperatingSystemVersion ver = [[NSProcessInfo processInfo] operatingSystemVersion];
  major = ver.majorVersion;
  minor = ver.minorVersion;
  patch = ver.patchVersion;
}
