// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_messageBox.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/dag_miscApi.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_delayedAction.h>
#include <debug/dag_debug.h>
#import <Appkit/AppKit.h>

@interface NSAlert (SynchronousSheet)
-(NSInteger) runModalSheetForWindow:(NSWindow *)aWindow;
-(NSInteger) runModalSheet;
@end

@implementation NSAlert (SynchronousSheet)
-(NSInteger) runModalSheetForWindow:(NSWindow *)aWindow
{
  // Bring up the sheet and wait until stopSynchronousSheet is triggered by a button click
  [self performSelectorOnMainThread:@selector(BE_beginSheetModalForWindow:) withObject:aWindow waitUntilDone:YES];
  NSInteger modalCode = [NSApp runModalForWindow:[self window]];

  // This is called only after stopSynchronousSheet is called (that is,
  // one of the buttons is clicked)
  [NSApp performSelectorOnMainThread:@selector(endSheet:) withObject:[self window] waitUntilDone:YES];

  // Remove the sheet from the screen
  [[self window] performSelectorOnMainThread:@selector(orderOut:) withObject:self waitUntilDone:YES];

  return modalCode;
}

-(NSInteger) runModalSheet
{
  return [self runModalSheetForWindow:[NSApp mainWindow]];
}

-(void) BE_stopSynchronousSheet:(NSWindow *)aWindow withReturnCode:(NSModalResponse)returnCode
{
  [NSApp stopModalWithCode:returnCode];
  [self restoreWindow: aWindow];
}

-(void) BE_beginSheetModalForWindow:(NSWindow *)aWindow
{
  [self blockWindow: aWindow];

  NSWindow *wnd = [self window];
  NSScreen *scr = [wnd screen];

  [self layout];
  NSRect wrc = [wnd frame];
  NSRect src = [scr frame];
  wrc.size.width *= 3;
  int dsz = wrc.size.height * wrc.size.width;
  int height = min(wrc.size.height, src.size.height-100);
  int width = dsz / (float)max(height, 1);
  [wnd setFrame: NSMakeRect(wrc.origin.x, wrc.origin.y, min<int>(width, (int)(src.size.width * 0.5f)), height) display:NO];

  [self beginSheetModalForWindow:aWindow completionHandler: ^(NSModalResponse returnCode)
  {
    [self BE_stopSynchronousSheet:aWindow withReturnCode:returnCode];
  }];
}

-(void) blockWindow:(NSWindow *)aWindow
{
  appDelegate = [NSApp delegate];
  [NSApp setDelegate: nil];
  [NSCursor unhide];

  if (!aWindow)
    return;

  wndDelegate = [aWindow delegate];
  level = [aWindow level];
  acceptsMouseMove = [aWindow acceptsMouseMovedEvents];
  mainView = [aWindow contentView];
  if (mainView)
    trackingArea = [[mainView trackingAreas] firstObject];

  [aWindow setDelegate: nil];
  [aWindow setLevel: NSModalPanelWindowLevel-1];
  [aWindow setAcceptsMouseMovedEvents:NO];
  if (trackingArea)
    [mainView removeTrackingArea: trackingArea];
}

-(void) restoreWindow:(NSWindow *)aWindow
{
  [NSCursor hide];
  [NSApp setDelegate: appDelegate];

  if (!aWindow)
    return;

  [aWindow setDelegate: wndDelegate];
  [aWindow setLevel: level];
  [aWindow setAcceptsMouseMovedEvents: acceptsMouseMove];
  mainView = [aWindow contentView];
  if (mainView)
  {
    if (trackingArea)
    {
      trackingArea = [[mainView trackingAreas] firstObject];
      if (trackingArea != NULL)
        [mainView removeTrackingArea: trackingArea];
      [mainView addTrackingRect:[mainView frame] owner:mainView userData:NULL assumeInside:NO];
    }
  }

  trackingArea = NULL;
  mainView = NULL;
  acceptsMouseMove = false;
}

id<NSWindowDelegate> wndDelegate;
int level;
id<NSApplicationDelegate> appDelegate;
bool acceptsMouseMove;
NSView *mainView = NULL;
NSTrackingArea *trackingArea = NULL;
@end

static int os_message_box_impl(const char *utf8_text, const char *utf8_caption, int flags)
{
  int buttonCount = 0;
  const char *buttonNames[3] = {NULL};
  NSString *buttonKeys[3] = {@"\r", @"d", @"\e"};

  switch ( flags & 0xF )
  {
    case GUI_MB_OK:
      buttonCount     = 1;
      buttonNames [0] = "OK";
      break;

    case GUI_MB_OK_CANCEL:
      buttonCount     = 2;
      buttonNames [0] = "OK";
      buttonNames [1] = "Cancel";
      break;

    case GUI_MB_YES_NO:
      buttonCount     = 2;
      buttonNames [0] = "Yes";
      buttonNames [1] = "No";
      break;

    case GUI_MB_RETRY_CANCEL:
      buttonCount     = 2;
      buttonNames [0] = "Retry";
      buttonNames [1] = "Cancel";
      break;

    case GUI_MB_ABORT_RETRY_IGNORE:
      buttonCount     = 3;
      buttonNames [0] = "Abort";
      buttonNames [1] = "Retry";
      buttonNames [2] = "Ignore";
      break;

    case GUI_MB_YES_NO_CANCEL:
      buttonCount     = 3;
      buttonNames [0] = "Yes";
      buttonNames [1] = "No";
      buttonNames [2] = "Cancel";
      break;

    case GUI_MB_CANCEL_TRY_CONTINUE:
      buttonCount     = 3;
      buttonNames [0] = "Cancel";
      buttonNames [1] = "Try";
      buttonNames [2] = "Continue";
      break;

    default:
      buttonCount     = 1;
      buttonNames [0] = "OK";
      break;
  }

  NSAlert *msgBox = [[NSAlert alloc] init];
  [msgBox setMessageText:[[NSString alloc] initWithUTF8String:utf8_caption]];
  [msgBox setInformativeText: [[NSString alloc] initWithUTF8String:utf8_text]];
#if __MAC_OS_X_VERSION_MAX_ALLOWED > 101200
  [msgBox setAlertStyle: NSAlertStyleCritical];
#else
  [msgBox setAlertStyle: NSCriticalAlertStyle];
#endif
  for (int buttonNo = 0; buttonNo < buttonCount; ++buttonNo)
  {
    NSButton *button = [msgBox addButtonWithTitle:[[NSString alloc] initWithUTF8String:buttonNames[buttonNo]]];
    [button setKeyEquivalent: buttonKeys[buttonNo]];
  }

  NSWindow *mainWnd = (NSWindow*)win32_get_main_wnd();
  int ret = [msgBox runModalSheetForWindow: mainWnd];
  return (ret == NSAlertFirstButtonReturn ? GUI_MB_BUTTON_1 : (ret == NSAlertSecondButtonReturn ?
                                                               GUI_MB_BUTTON_2 : GUI_MB_BUTTON_3));
}
int mac_message_box_internal(const char *utf8_text, const char *utf8_caption, int flags)
{
  debug("%s(%s, %s, %d)", __FUNCTION__, utf8_text, utf8_caption, flags);

  if (utf8_text == NULL || utf8_caption == NULL)
  {
    debug("%s no text or caption", __FUNCTION__);
    return GUI_MB_FAIL;
  }
  if (is_main_thread())
    return os_message_box_impl(utf8_text, utf8_caption, flags);

  debug("trying to show messahe box on main thread");
  __block int res = 0;
  dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
  dispatch_async(dispatch_get_main_queue(), ^
  {
      res = os_message_box_impl(utf8_text, utf8_caption, flags);
      dispatch_semaphore_signal(semaphore);
  });
  dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
  dispatch_release(semaphore);

  return res;
}
