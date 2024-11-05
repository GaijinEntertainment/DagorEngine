// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "internal.h"

#import <UIKit/UIKit.h>

namespace folders
{
namespace internal
{
String get_gamedata_dir()
{
  NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
  String dir;
  dir = [[paths objectAtIndex:0] UTF8String];
  return dir;
}
} // namespace internal
} // namespace folders
