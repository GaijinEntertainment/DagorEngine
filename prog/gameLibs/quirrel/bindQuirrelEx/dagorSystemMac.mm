#include <quirrel/sqModules/sqModules.h>
#include <quirrel/bindQuirrelEx/bindQuirrelEx.h>

#import <AppKit/NSScreen.h>
#import <Cocoa/Cocoa.h>

SQInteger macosx_get_primary_screen_info(HSQUIRRELVM vm)
{
  NSScreen *mainScreen = [NSScreen mainScreen];

  if (!mainScreen)
  {
    return sq_throwerror(vm, "MacOSX: Cannot get the main screen");
  }

  CGFloat widthInches  = mainScreen.frame.size.width / mainScreen.backingScaleFactor;
  CGFloat heightInches = mainScreen.frame.size.height / mainScreen.backingScaleFactor;

  NSRect rectInPoints = NSMakeRect(0, 0, widthInches, heightInches);

  NSRect rectInPixels = [mainScreen convertRectToBacking:rectInPoints];

  CGFloat widthPixels = rectInPixels.size.width;
  CGFloat heightPixels = rectInPixels.size.height;

  Sqrat::Table res(vm);
  res.SetValue("horizDpi", (widthPixels * 39.37f) / widthInches);
  res.SetValue("vertDpi", (heightPixels * 39.37f) / heightInches);
  res.SetValue("physWidth", widthInches);
  res.SetValue("physHeight", heightInches);
  res.SetValue("pixelsWidth", widthPixels);
  res.SetValue("pixelsHeight", heightPixels);
  sq_pushobject(vm, res);

  return 1;
}