// Copyright (C) Gaijin Games KFT.  All rights reserved.

#import <Cocoa/Cocoa.h>

#include "../sender.h"

int main(int argc, char **argv)
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  [NSApplication sharedApplication];

  int ret = breakpad::process_report(argc, argv);

  [pool release];
  return ret;
}
