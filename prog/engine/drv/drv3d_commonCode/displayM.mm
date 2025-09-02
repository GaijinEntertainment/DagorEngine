// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#if _TARGET_PC_MACOSX
#import <AppKit/NSScreen.h>
#import <Cocoa/Cocoa.h>


float d3d::get_display_scale()
{
  float userSpaceScale = -1;

  NSScreen *screen = [NSScreen mainScreen];
  NSDictionary *description = [screen deviceDescription];
  NSSize displayPixelSize = [[description objectForKey:NSDeviceSize] sizeValue];
  CGSize displayPhysicalSize = CGDisplayScreenSize([[description objectForKey:@"NSScreenNumber"] unsignedIntValue]);

  if (displayPhysicalSize.width <= 0 || userSpaceScale <= 0 || displayPixelSize.width <= 0)
    return -1.f;

  float dpi = (displayPixelSize.width / displayPhysicalSize.width) * 25.4f / userSpaceScale;
  float scale = dpi / 96.f;
  return scale > 1.f ? scale : 1.f;
}
#elif _TARGET_IOS
#import <UIKit/UIKit.h>

float d3d::get_display_scale()
{
  return [[UIScreen mainScreen] nativeScale];
}
#else
float d3d::get_display_scale() { return 1.f; }
#endif

#if _TARGET_TVOS || _TARGET_IOS

#include <os/proc.h>

unsigned d3d::get_dedicated_gpu_memory_size_kb()
{
  static const size_t totalMemKb = os_proc_available_memory() >> 10;
  static const int cpuMemSize = ::dgs_get_settings()->getBlockByNameEx("iOS")->getInt("cpuReserveKb", 0);
  if (cpuMemSize > 0)
    return max(totalMemKb - cpuMemSize, (size_t)256000);

  return totalMemKb >> 1;
}
#endif
