// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)

#pragma once

#import <Cocoa/Cocoa.h>
#import <Appkit/AppKit.h>
#import <objc/runtime.h>
#include <3d/dag_drv3dCmd.h>
#include <debug/dag_hwExcept.h>
#include <debug/dag_except.h>
#include <memory/dag_mem.h>
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_wndProcCompMsg.h>
#include <osApiWrappers/dag_dbgStr.h>
#include <osApiWrappers/dag_messageBox.h>
#include <workCycle/dag_workCycle.h>
#include <util/dag_string.h>
#include <util/dag_globDef.h>
#include <debug/dag_logSys.h>
#include <debug/dag_debug.h>
#include <unistd.h>
#include "dag_addBasePathDef.h"
#include "dag_loadSettings.h"
#include <mach-o/dyld.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <3d/dag_drv3d.h>
#include <osApiWrappers/dag_atomic.h>

NSWindow* dagMainWnd = nullptr;

extern void mouse_api_SetFullscreenMode(int width, int hight);
extern void win32_set_main_wnd(void* wnd);

@interface DagorView : NSView<NSWindowDelegate>
{
}

-(void) viewDidMoveToWindow;
-(BOOL) acceptsFirstResponder;
-(BOOL) becomeFirstResponder;
-(BOOL) resignFirstResponder;

-(BOOL) acceptsFirstMouse: (NSEvent*) theEvent;
-(void) mouseDown: (NSEvent*) theEvent;
-(void) mouseDragged: (NSEvent*) theEvent;
-(void) mouseUp: (NSEvent*) theEvent;
-(void) rightMouseDown: (NSEvent*) theEvent;
-(void) rightMouseDragged: (NSEvent*) theEvent;
-(void) rightMouseUp: (NSEvent*) theEvent;
-(void) otherMouseDown: (NSEvent*) theEvent;
-(void) otherMouseDragged: (NSEvent*) theEvent;
-(void) otherMouseUp: (NSEvent*) theEvent;
-(void) mouseMoved: (NSEvent*) theEvent;
-(void) scrollWheel: (NSEvent*) theEvent;
@end

namespace workcycle_internal
{
  intptr_t main_window_proc(void*,unsigned,uintptr_t,intptr_t);
}

@implementation DagorView

-(void) windowWillClose: (NSNotification*) event
{
  debug("main window close signal received!");
  win32_set_main_wnd(NULL);
  d3d::window_destroyed(dagMainWnd);
  quit_game(0);
}
-(void) viewDidMoveToWindow
{
  [dagMainWnd setAcceptsMouseMovedEvents:YES];
  [dagMainWnd makeFirstResponder:self];

  NSTrackingArea *trackingArea = [[self trackingAreas] firstObject];
  if (trackingArea != NULL)
  {
    [self removeTrackingArea: trackingArea];
  }
  [self addTrackingRect:[self frame] owner:self userData:NULL assumeInside:NO];
}
-(BOOL)acceptsFirstResponder { return YES; }
-(BOOL)becomeFirstResponder  { return YES; }
-(BOOL)resignFirstResponder  { return YES; }

-(BOOL) acceptsFirstMouse: (NSEvent*) theEvent { return YES; }

-(void) mouseDown: (NSEvent*) theEvent
{
  workcycle_internal::main_window_proc(dagMainWnd, GPCM_MouseBtnPress, [theEvent buttonNumber], 0);
}
-(void) mouseDragged: (NSEvent*) theEvent
{
  workcycle_internal::main_window_proc(dagMainWnd, GPCM_MouseMove, [theEvent buttonNumber], 0);
}
-(void) mouseUp: (NSEvent*) theEvent
{
  workcycle_internal::main_window_proc(dagMainWnd, GPCM_MouseBtnRelease, [theEvent buttonNumber], 0);
}
-(void) rightMouseDown: (NSEvent*) theEvent
{
  workcycle_internal::main_window_proc(dagMainWnd, GPCM_MouseBtnPress, [theEvent buttonNumber], 0);
}
-(void) rightMouseDragged: (NSEvent*) theEvent
{
  workcycle_internal::main_window_proc(dagMainWnd, GPCM_MouseMove, [theEvent buttonNumber], 0);
}
-(void) rightMouseUp: (NSEvent*) theEvent
{
  workcycle_internal::main_window_proc(dagMainWnd, GPCM_MouseBtnRelease, [theEvent buttonNumber], 0);
}
-(void) otherMouseDown: (NSEvent*) theEvent
{
  workcycle_internal::main_window_proc(dagMainWnd, GPCM_MouseBtnPress, [theEvent buttonNumber], 0);
}
-(void) otherMouseDragged: (NSEvent*) theEvent
{
  workcycle_internal::main_window_proc(dagMainWnd, GPCM_MouseMove, [theEvent buttonNumber], 0);
}
-(void) otherMouseUp: (NSEvent*) theEvent
{
  workcycle_internal::main_window_proc(dagMainWnd, GPCM_MouseBtnRelease, [theEvent buttonNumber], 0);
}
-(void) mouseMoved: (NSEvent*) theEvent
{
  workcycle_internal::main_window_proc(dagMainWnd, GPCM_MouseMove, [theEvent buttonNumber], 0);
}
-(void) scrollWheel: (NSEvent*) theEvent
{
  //debug("View: scrollWheel (%.2f, %.2f, %.2f)", [theEvent deltaX], [theEvent deltaY], [theEvent deltaZ]);
  if ([theEvent deltaY])
    workcycle_internal::main_window_proc(dagMainWnd, GPCM_MouseWheel, [theEvent deltaY] * -100, 0);
  else
    workcycle_internal::main_window_proc(dagMainWnd, GPCM_MouseWheel, [theEvent deltaX] * -100, 0);
}
@end

@interface Dagor2_App: NSApplication
{
}
-(void) sendEvent:(NSEvent *)e;
@end

@interface NSBundle (JJAdditions)
+(BOOL) JJ_loadNibNamed:(NSString *)aNibNamed owner:(id)owner;
@end

@interface Dagor2_AppDelegate : NSObject <NSApplicationDelegate>
{
  bool isActive;
  bool isBkgModeSupported;
}
- (void)applicationDidChangeScreenParameters:(NSNotification *)notification;
+ (Dagor2_AppDelegate *)sharedAppDelegate;
@end 

DagorView* dagMainView = nullptr;
extern volatile int g_debug_is_in_fatal;

NSWindow* macosx_create_dagor_window(const char* title, int scr_w, int scr_h, NSView* drawView, NSRect rect)
{
  bool windowed = dgs_get_window_mode() != WindowMode::FULLSCREEN_EXCLUSIVE;
#if __MAC_OS_X_VERSION_MAX_ALLOWED > 101200
  NSWindowStyleMask mask = windowed
    ? NSWindowStyleMaskMiniaturizable|NSWindowStyleMaskTitled|NSWindowStyleMaskClosable
    : NSWindowStyleMaskBorderless;
#else
  NSWindowStyleMask mask = windowed ? NSMiniaturizableWindowMask|NSTitledWindowMask|NSClosableWindowMask : NSBorderlessWindowMask;
#endif
  dagMainWnd = [[NSWindow alloc] initWithContentRect: rect
                                        styleMask: mask
                                          backing: NSBackingStoreBuffered
                                            defer: YES];

  [dagMainWnd setLevel:(windowed ? NSNormalWindowLevel : NSMainMenuWindowLevel+1)];
  [dagMainWnd setTitle:[[NSString alloc] initWithUTF8String:title]];
  [dagMainWnd setOpaque:YES];

  NSRect viewRect = NSMakeRect(0.0, 0.0, rect.size.width, rect.size.height);
  dagMainView = [[DagorView alloc] initWithFrame:viewRect];

  [dagMainView addSubview: drawView];

  [dagMainWnd setContentView: dagMainView];
  [dagMainWnd setDelegate: dagMainView];
  [dagMainWnd setInitialFirstResponder:dagMainView];

  [dagMainWnd makeKeyAndOrderFront:NSApp];
  [dagMainWnd makeFirstResponder: dagMainView];

  if (!windowed)
  {
    [dagMainWnd setHidesOnDeactivate:YES];
    [dagMainView setNeedsDisplay:YES];
    [NSCursor hide];
    [dagMainView viewDidMoveToWindow];
    mouse_api_SetFullscreenMode(scr_w, scr_h);
  }
  else
  {
    [dagMainWnd makeMainWindow];
  }

  return dagMainWnd;
}

static void macosx_on_app_activated()
{
  [dagMainWnd orderFront:NSApp];
  [NSApp unhide:NSApp];

  if (interlocked_relaxed_load(g_debug_is_in_fatal) == 0)
  {
    [dagMainWnd deminiaturize:NSApp];
    [dagMainView viewDidMoveToWindow];
    [dagMainWnd makeKeyAndOrderFront:NSApp];

    if (dgs_get_window_mode() == WindowMode::FULLSCREEN_EXCLUSIVE)
    {
      [dagMainView setNeedsDisplay:YES];
      [NSCursor hide];
    }
  }
}

void macosx_hide_windows_on_alert()
{
  dagor_suppress_d3d_update(true);

  if (dagMainWnd)
  {
    if (dgs_get_window_mode() == WindowMode::FULLSCREEN_EXCLUSIVE)
      [dagMainWnd setLevel:NSModalPanelWindowLevel-1];
    else
      [dagMainWnd setLevel:NSModalPanelWindowLevel-2];

    [dagMainWnd setDelegate: nil];
    [NSCursor unhide];
  }

  // release GPU lock and disable future locks from non-main thread
  int refc = d3d::driver_command(DRV3D_COMMAND_RELEASE_OWNERSHIP, 0, (void*)1, (void*)1);
  debug("macosx_hide_windows_on_alert, releaseGPU=%d", refc);
}

void macosx_app_hide()
{
  [NSCursor unhide];

  [NSApp hide:NSApp];
  [dagMainWnd orderOut:NSApp];
}

void macosx_app_miniaturize()
{
  [NSCursor unhide];

  if (dgs_get_window_mode() == WindowMode::FULLSCREEN_EXCLUSIVE)
  {
    [dagMainWnd orderOut:NSApp];
  }

  [dagMainWnd miniaturize:NSApp];
}

void DagorWinMainInit(int nCmdShow, bool debugmode);
int DagorWinMain ( int nCmdShow, bool debugmode );

extern void default_crt_init_kernel_lib ();
extern void default_crt_init_core_lib ();
extern void apply_hinstance ( void *hInstance, void *hPrevInstance );
extern void macosx_hide_windows_on_alert();

extern "C" const char *dagor_exe_build_date;
extern "C" const char *dagor_exe_build_time;

extern void clear_shortcuts_keyboard();

extern void messagebox_report_fatal_error(const char *title, const char *msg, const char *call_stack);

static bool stop_event_processing = false;

static void messagebox_win_report_fatal_error(const char *title, const char *msg, const char *call_stack)
{
  debug("messagebox_win_report_fatal_error, title=%s, msg=%s, call_stack=%s", title, msg, call_stack);
  macosx_hide_windows_on_alert();
  stop_event_processing = true;
  os_message_box(msg, title, GUI_MB_OK|GUI_MB_ICON_ERROR);

  int refc = d3d::driver_command(DRV3D_COMMAND_RELEASE_OWNERSHIP, 0, 0, (void*)1);
  debug("messagebox_win_report_fatal_error, releaseGPU=%d", refc);
}

bool mbox_user_allows_abort(const char *msg, const char *call_stack)
{
  debug("mbox_user_allows_abort, msg=%s, call_stack=%s", msg, call_stack);
#if DAGOR_DBGLEVEL == 0
  return true;
#else
  if (dgs_execute_quiet)
    return true;

  dagor_suppress_d3d_update(true);
  int refc = d3d::driver_command(DRV3D_COMMAND_RELEASE_OWNERSHIP, 0, (void*)1, (void*)1);
  debug("mbox_user_allows_abort, releaseGPU=%d", refc);
  // failed to release owner ship, exit
  if (refc < 0)
  {
    dagor_suppress_d3d_update(false);
    return true;
  }
  stop_event_processing = true;

  char buf[4096];
  snprintf(buf, sizeof(buf), "%s\n%s", msg, call_stack); buf[sizeof(buf)-1] = 0;
  int result = os_message_box(buf, "FATAL ERROR", GUI_MB_ABORT_RETRY_IGNORE);
  if (result == GUI_MB_BUTTON_2)
    G_DEBUG_BREAK;

  dagor_suppress_d3d_update(false);
  stop_event_processing = false;
  refc = d3d::driver_command(DRV3D_COMMAND_RELEASE_OWNERSHIP, 0, 0, (void*)1);
  debug("mbox_user_allows_abort, releaseGPU=%d", refc);

  return result == GUI_MB_BUTTON_1;
#endif
}

extern "C" int main(int argc, char *argv[])
{
  measure_cpu_freq();
# if DAGOR_DBGLEVEL == 0
  stderr = freopen("/dev/null", "wt", stderr);
#endif

  // replace any relative or non-full path with fully qualified module path
  {
    static char exe_path[MAX_PATH];
    char full_exe_path[MAX_PATH];
    uint32_t size = sizeof(exe_path);
    if (_NSGetExecutablePath(exe_path, &size) == 0)
    {
      strncpy(exe_path, realpath(exe_path, full_exe_path), sizeof(exe_path));
      exe_path[sizeof(exe_path)-1] = 0;
      if (strcmp(argv[0], exe_path) != 0)
      {
        out_debug_str_fmt("module argv[0]=%s -> %s", argv[0], exe_path);
        argv[0] = exe_path;
      }
    }
  }

  dgs_init_argv(argc, argv);
  dgs_report_fatal_error = messagebox_win_report_fatal_error;
  apply_hinstance ( NULL, NULL );

  DagorHwException::setHandler( "main" );

DAGOR_TRY
{

  default_crt_init_kernel_lib ();

  dagor_init_base_path();
  dagor_change_root_directory(::dgs_get_argv("rootdir"));

#if defined(__DEBUG_FILEPATH)
  start_classic_debug_system(__DEBUG_FILEPATH);
#elif defined(__DEBUG_MODERN_PREFIX)
  start_debug_system(argv[0], __DEBUG_MODERN_PREFIX);
#else
  start_debug_system(argv[0]);
#endif

  static DagorSettingsBlkHolder stgBlkHolder;
  ::DagorWinMainInit(0, 0);

  default_crt_init_core_lib ();

  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  Class bundleClass = [NSBundle class];
  Method originalMethod = class_getClassMethod(bundleClass, @selector(loadNibNamed:owner:));
  Method categoryMethod = class_getClassMethod(bundleClass, @selector(JJ_loadNibNamed:owner:));

  method_exchangeImplementations(originalMethod, categoryMethod);
  NSApplicationMain(argc, (const char **) argv);
  [pool release];

} DAGOR_CATCH( DagorException e )
{
#ifdef DAGOR_EXCEPTIONS_ENABLED
  DagorHwException::reportException( e, true );
#endif
} DAGOR_CATCH(...)
{
  flush_debug_file();
  debug_ctx("\nUnhandled exception:\n");
  flush_debug_file();
  _exit(2);
}
  return 0;
}

namespace workcycle_internal { intptr_t main_window_proc(void*,unsigned,uintptr_t,intptr_t); }

@implementation Dagor2_App
-(id) init 
{
  if ((self = [super init]))
  {
    [self setDelegate:[[Dagor2_AppDelegate alloc] init]];
    [self activateIgnoringOtherApps: YES];
  }
  return self;
}

-(void) dealloc 
{
  id delegate = [self delegate];
  if (delegate)
  {
    [self setDelegate:nil];
    [delegate release];
  }

  [super dealloc];
}

-(void) sendEvent:(NSEvent *)e
{
  if (stop_event_processing || [self modalWindow])
    return [super sendEvent:e];
  int type = [e type];
#if __MAC_OS_X_VERSION_MAX_ALLOWED > 101200
  if (type == NSEventTypeKeyDown)
#else
  if (type == NSKeyDown)
#endif
  {
    //debug("View: keyDown %08X <%s> %04X", [e modifierFlags], [[e characters] UTF8String], [e keyCode]);
    NSString *uc = [e characters];
    int char_num = [uc length];
    workcycle_internal::main_window_proc(NULL, GPCM_KeyPress, [e keyCode], char_num>0 ? [uc characterAtIndex:0] : 0);
    for (int i = 1; i < char_num; i ++)
      workcycle_internal::main_window_proc(NULL, GPCM_Char, [uc characterAtIndex:i], 0);

    return; // return here to avoid DING on non-processed key due to windows skip handling keys
  }
#if __MAC_OS_X_VERSION_MAX_ALLOWED > 101200
  else if (type == NSEventTypeKeyUp)
#else
  else if (type == NSKeyUp)
#endif
  {
    //debug("View: keyUp %08X <%s> %04X", [e modifierFlags], [[e characters] UTF8String], [e keyCode]);
    workcycle_internal::main_window_proc(NULL, GPCM_KeyRelease, [e keyCode], 0);
  }
#if __MAC_OS_X_VERSION_MAX_ALLOWED > 101200
  else if (type == NSEventTypeFlagsChanged)
#else
  else if (type == NSFlagsChanged)
#endif
  {
    //debug("View: flagsChanged %08X %04X", [e modifierFlags], [e keyCode]);
    int ev = GPCM_KeyRelease;
    int code = [e keyCode];
    int modif = [e modifierFlags];
    switch (code)
    {
      case 0x38:
        if (modif & 0x02)
          ev = GPCM_KeyPress; // left shift
        break;
      case 0x3C:
        if (modif & 0x04)
          ev = GPCM_KeyPress; // right shift
        break;
      case 0x37:
        if (modif & 0x08)
          ev = GPCM_KeyPress; // left cmd
        break;
      case 0x36:
        if (modif & 0x10)
          ev = GPCM_KeyPress; // right cmd
        break;
      case 0x3A:
        if (modif & 0x20)
          ev = GPCM_KeyPress; // left alt
        break;
      case 0x3D:
        if (modif & 0x40)
          ev = GPCM_KeyPress; // right alt
        break;
      case 0x3B:
        if (modif & 0x01)
          ev = GPCM_KeyPress; // control
        break;

      case 0x3F:
        if (modif & 0x800000)
          ev = GPCM_KeyPress; // fn
        break;

      case 0x39:
        if (modif & 0x010000)
          ev = GPCM_KeyPress; // capslock
        break;

      default:
        return;
    }
    workcycle_internal::main_window_proc(NULL, ev, code, 0);
  }
  [super sendEvent:e];
}
@end

@implementation NSBundle (JJAdditions)
+(BOOL) JJ_loadNibNamed:(NSString *)aNibName owner:(id)owner
{
  if (!aNibName && owner == NSApp)
    return YES; // Hack. No need to load any nib file by default
  else 
    return [self JJ_loadNibNamed:aNibName owner:owner];
}
@end

@implementation Dagor2_AppDelegate

+(Dagor2_AppDelegate *)sharedAppDelegate { return (Dagor2_AppDelegate *)[[NSApplication sharedApplication] delegate]; }

-(id)init
{
  self = [super init];
  debug_cp();
  isActive = FALSE;
  isBkgModeSupported = NO;
  return self;
}

-(void)main
{
  debug_cp();
  int res = DagorWinMain(0,0);
  debug_cp();
  quit_game(res);
  debug_cp();
}

-(void)applicationWillFinishLaunching: (NSNotification*)application
{
  debug_cp();
}

-(void)applicationDidFinishLaunching: (NSNotification*)application
{
  debug_cp();
  [NSTimer scheduledTimerWithTimeInterval:(0) target:self selector:@selector(main) userInfo:nil repeats:NO];
}

-(void)applicationWillTerminate:(NSNotification *)application
{
  debug_cp();
}


-(void)applicationWillResignActive:(NSNotification*)application
{
  debug_cp();
  workcycle_internal::main_window_proc(NULL, GPCM_Activate, GPCMP1_Inactivate, 0);
}

-(void)applicationDidBecomeActive:(NSNotification *)application
{
  debug_cp();
  clear_shortcuts_keyboard();
  macosx_on_app_activated();
  workcycle_internal::main_window_proc(NULL, GPCM_Activate, GPCMP1_Activate, 0);
}

-(void)application: (NSApplication*)application didRegisterForRemoteNotificationsWithDeviceToken: (NSData*)deviceToken
{  
  // internal notification system register
  const int token_len = [deviceToken length];
  const unsigned char *token_bytes = (const unsigned char*)[deviceToken bytes];
  char out_str_buf[100];
  for (int i = 0; i < token_len; ++i)
    snprintf(out_str_buf + i * 2, sizeof(out_str_buf) - i*2, "%.2X", token_bytes[i]);
  
  NSString *preferredLang = [[NSLocale preferredLanguages] objectAtIndex:0];
  const char *langStr = [preferredLang UTF8String];
  NSLog(@"DEVICE TOKEN: %@ locale %@", [NSString stringWithUTF8String: out_str_buf], preferredLang);
}

-(void)application: (NSApplication*)application didFailToRegisterForRemoteNotificationsWithError: (NSError*)error
{
  debug_cp();
}

-(void)application: (NSApplication*)application didReceiveRemoteNotification: (NSDictionary*)userInfo
{
  debug_cp();
}

-(BOOL)application: (NSNotification*)application didFinishLaunchingWithOptions: (NSDictionary*)launchOptions
{
  debug_cp();
  [self applicationDidFinishLaunching:application];
  return YES;
}

-(void)dealloc
{
  debug_cp();
  [super dealloc];
}

-(void)applicationDidChangeScreenParameters:(NSNotification *)notification
{
  NSScreen *screen = [NSScreen mainScreen];
  static bool startup = true;
  if (!startup)
    return;
  startup = false;
  bool enable = screen.maximumExtendedDynamicRangeColorComponentValue > 1.f;
  d3d::driver_command(DRV3D_COMMAND_SET_HDR, &enable, 0, 0);
}
@end
